/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2009 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
**
**
** GNU Lesser General Public License Usage
**
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this file.
** Please review the following information to ensure the GNU Lesser General
** Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** Other Usage
**
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**************************************************************************/

#include "qmlengine.h"
#include "qmladapter.h"

#include "debuggerstartparameters.h"
#include "debuggeractions.h"
#include "debuggerconstants.h"
#include "debuggercore.h"
#include "debuggerdialogs.h"
#include "debuggermainwindow.h"
#include "debuggerrunner.h"
#include "debuggerstringutils.h"
#include "debuggertooltipmanager.h"

#include "breakhandler.h"
#include "moduleshandler.h"
#include "registerhandler.h"
#include "stackhandler.h"
#include "watchhandler.h"
#include "watchutils.h"

#include <extensionsystem/pluginmanager.h>
#include <projectexplorer/applicationlauncher.h>

#include <utils/environment.h>
#include <utils/qtcassert.h>
#include <utils/fileinprojectfinder.h>

#include <coreplugin/icore.h>
#include <coreplugin/helpmanager.h>

#include <QtCore/QDateTime>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QTimer>

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QMainWindow>
#include <QtGui/QMessageBox>
#include <QtGui/QToolTip>
#include <QtGui/QTextDocument>

#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QHostAddress>

#define DEBUG_QML 1
#if DEBUG_QML
#   define SDEBUG(s) qDebug() << s
#else
#   define SDEBUG(s)
#endif
# define XSDEBUG(s) qDebug() << s

using namespace ProjectExplorer;

namespace Debugger {
namespace Internal {

class QmlEnginePrivate
{
public:
    explicit QmlEnginePrivate(QmlEngine *q);

private:
    friend class QmlEngine;
    QmlAdapter m_adapter;
    ApplicationLauncher m_applicationLauncher;
    Utils::FileInProjectFinder fileFinder;
    QTimer m_noDebugOutputTimer;
};

QmlEnginePrivate::QmlEnginePrivate(QmlEngine *q)
    : m_adapter(q)
{}


///////////////////////////////////////////////////////////////////////
//
// QmlEngine
//
///////////////////////////////////////////////////////////////////////

QmlEngine::QmlEngine(const DebuggerStartParameters &startParameters,
        DebuggerEngine *masterEngine)
  : DebuggerEngine(startParameters, QmlLanguage, masterEngine),
    d(new QmlEnginePrivate(this))
{
    setObjectName(QLatin1String("QmlEngine"));

    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();
    pluginManager->addObject(this);

    connect(&d->m_adapter, SIGNAL(connectionError(QAbstractSocket::SocketError)),
        SLOT(connectionError(QAbstractSocket::SocketError)));
    connect(&d->m_adapter, SIGNAL(serviceConnectionError(QString)),
        SLOT(serviceConnectionError(QString)));
    connect(&d->m_adapter, SIGNAL(connected()),
        SLOT(connectionEstablished()));
    connect(&d->m_adapter, SIGNAL(connectionStartupFailed()),
        SLOT(connectionStartupFailed()));

    connect(&d->m_applicationLauncher,
        SIGNAL(processExited(int)),
        SLOT(disconnected()));
    connect(&d->m_applicationLauncher,
        SIGNAL(appendMessage(QString, Utils::OutputFormat)),
        SLOT(appendMessage(QString, Utils::OutputFormat)));
    connect(&d->m_applicationLauncher,
            SIGNAL(processStarted()),
            &d->m_noDebugOutputTimer,
            SLOT(start()));

    // Only wait 8 seconds for the 'Waiting for connection' on application ouput, then just try to connect
    // (application output might be redirected / blocked)
    d->m_noDebugOutputTimer.setSingleShot(true);
    d->m_noDebugOutputTimer.setInterval(8000);
    connect(&d->m_noDebugOutputTimer, SIGNAL(timeout()), this, SLOT(beginConnection()));
}

QmlEngine::~QmlEngine()
{
    ExtensionSystem::PluginManager *pluginManager =
        ExtensionSystem::PluginManager::instance();

    if (pluginManager->allObjects().contains(this)) {
        pluginManager->removeObject(this);
    }

    delete d;
}

void QmlEngine::setupInferior()
{
    QTC_ASSERT(state() == InferiorSetupRequested, qDebug() << state());

    if (startParameters().startMode == AttachToRemoteServer) {
        emit requestRemoteSetup();
        if (startParameters().qmlServerPort != quint16(-1))
            notifyInferiorSetupOk();
    } if (startParameters().startMode == AttachToQmlPort) {
            notifyInferiorSetupOk();

    } else {
        d->m_applicationLauncher.setEnvironment(startParameters().environment);
        d->m_applicationLauncher.setWorkingDirectory(startParameters().workingDirectory);

        notifyInferiorSetupOk();
    }
}

void QmlEngine::appendMessage(const QString &msg, Utils::OutputFormat /* format */)
{
    showMessage(msg, AppOutput); // FIXME: Redirect to RunControl
}

void QmlEngine::connectionEstablished()
{
    attemptBreakpointSynchronization();

    showMessage(tr("QML Debugger connected."), StatusBar);

    if (!watchHandler()->watcherNames().isEmpty()) {
        synchronizeWatchers();
    }
    connect(watchersModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));

    if (state() == EngineRunRequested)
        notifyEngineRunAndInferiorRunOk();
}

void QmlEngine::beginConnection()
{
    d->m_noDebugOutputTimer.stop();
    showMessage(tr("QML Debugger connecting..."), StatusBar);
    d->m_adapter.beginConnection();
}

void QmlEngine::connectionStartupFailed()
{
    if (isSlaveEngine()) {
        if (masterEngine()->state() != InferiorRunOk) {
            // we're right now debugging C++, just try longer ...
            beginConnection();
            return;
        }
    }

    Core::ICore * const core = Core::ICore::instance();
    QMessageBox *infoBox = new QMessageBox(core->mainWindow());
    infoBox->setIcon(QMessageBox::Critical);
    infoBox->setWindowTitle(tr("Qt Creator"));
    infoBox->setText(tr("Could not connect to the in-process QML debugger.\n"
                        "Do you want to retry?"));
    infoBox->setStandardButtons(QMessageBox::Retry | QMessageBox::Cancel | QMessageBox::Help);
    infoBox->setDefaultButton(QMessageBox::Retry);
    infoBox->setModal(true);

    connect(infoBox, SIGNAL(finished(int)),
            this, SLOT(retryMessageBoxFinished(int)));

    infoBox->show();
}

void QmlEngine::retryMessageBoxFinished(int result)
{
    switch (result) {
    case QMessageBox::Retry: {
        beginConnection();
        break;
    }
    case QMessageBox::Help: {
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest("qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html");
        // fall through
    }
    default:
        if (state() == InferiorRunOk) {
            notifyInferiorSpontaneousStop();
            notifyInferiorIll();
        } else {
        notifyEngineRunFailed();
        }
        break;
    }
}

void QmlEngine::connectionError(QAbstractSocket::SocketError socketError)
{
    if (socketError == QAbstractSocket::RemoteHostClosedError)
        showMessage(tr("QML Debugger: Remote host closed connection."), StatusBar);

    if (!isSlaveEngine()) { // normal flow for slave engine when gdb exits
        notifyInferiorSpontaneousStop();
        notifyInferiorIll();
    }
}

void QmlEngine::serviceConnectionError(const QString &serviceName)
{
    showMessage(tr("QML Debugger: Could not connect to service '%1'.")
        .arg(serviceName), StatusBar);
}

bool QmlEngine::canDisplayTooltip() const
{
    return state() == InferiorRunOk || state() == InferiorStopOk;
}

void QmlEngine::filterApplicationMessage(const QString &msg, int /*channel*/)
{
    static const QString qddserver = QLatin1String("QDeclarativeDebugServer: ");
    static const QString cannotRetrieveDebuggingOutput = ApplicationLauncher::msgWinCannotRetrieveDebuggingOutput();

    const int index = msg.indexOf(qddserver);
    if (index != -1) {
        // we're actually getting debug output
        d->m_noDebugOutputTimer.stop();

        QString status = msg;
        status.remove(0, index + qddserver.length()); // chop of 'QDeclarativeDebugServer: '

        static QString waitingForConnection = QLatin1String("Waiting for connection ");
        static QString unableToListen = QLatin1String("Unable to listen ");
        static QString debuggingNotEnabled = QLatin1String("Ignoring \"-qmljsdebugger=");
        static QString debuggingNotEnabled2 = QLatin1String("Ignoring\"-qmljsdebugger="); // There is (was?) a bug in one of the error strings - safest to handle both
        static QString connectionEstablished = QLatin1String("Connection established");

        QString errorMessage;
        if (status.startsWith(waitingForConnection)) {
            beginConnection();
        } else if (status.startsWith(unableToListen)) {
            //: Error message shown after 'Could not connect ... debugger:"
            errorMessage = tr("The port seems to be in use.");
        } else if (status.startsWith(debuggingNotEnabled) || status.startsWith(debuggingNotEnabled2)) {
            //: Error message shown after 'Could not connect ... debugger:"
            errorMessage = tr("The application is not set up for QML/JS debugging.");
        } else if (status.startsWith(connectionEstablished)) {
            // nothing to do
        } else {
            qWarning() << "Unknown QDeclarativeDebugServer status message: " << status;
        }

        if (!errorMessage.isEmpty()) {
            notifyEngineRunFailed();

            Core::ICore * const core = Core::ICore::instance();
            QMessageBox *infoBox = new QMessageBox(core->mainWindow());
            infoBox->setIcon(QMessageBox::Critical);
            infoBox->setWindowTitle(tr("Qt Creator"));
            //: %1 is detailed error message
            infoBox->setText(tr("Could not connect to the in-process QML debugger:\n%1")
                             .arg(errorMessage));
            infoBox->setStandardButtons(QMessageBox::Ok | QMessageBox::Help);
            infoBox->setDefaultButton(QMessageBox::Ok);
            infoBox->setModal(true);

            connect(infoBox, SIGNAL(finished(int)),
                    this, SLOT(wrongSetupMessageBoxFinished(int)));

            infoBox->show();
        }
    } else if (msg.contains(cannotRetrieveDebuggingOutput)) {
        // we won't get debugging output, so just try to connect ...
        beginConnection();
    }
}

void QmlEngine::showMessage(const QString &msg, int channel, int timeout) const
{
    if (channel == AppOutput || channel == AppError) {
        const_cast<QmlEngine*>(this)->filterApplicationMessage(msg, channel);
    }
    DebuggerEngine::showMessage(msg, channel, timeout);
}

void QmlEngine::closeConnection()
{
    disconnect(watchersModel(),SIGNAL(layoutChanged()),this,SLOT(synchronizeWatchers()));
    d->m_adapter.closeConnection();
}

void QmlEngine::runEngine()
{
    QTC_ASSERT(state() == EngineRunRequested, qDebug() << state());

    if (!isSlaveEngine() && startParameters().startMode != AttachToRemoteServer
            && startParameters().startMode != AttachToQmlPort)
        startApplicationLauncher();

    if (startParameters().startMode == AttachToQmlPort)
        beginConnection();
}

void QmlEngine::startApplicationLauncher()
{
    if (!d->m_applicationLauncher.isRunning()) {
        appendMessage(tr("Starting %1 %2").arg(
                          QDir::toNativeSeparators(startParameters().executable),
                          startParameters().processArgs)
                      + QLatin1Char('\n')
                     , Utils::NormalMessageFormat);
        d->m_applicationLauncher.start(ApplicationLauncher::Gui,
                                    startParameters().executable,
                                    startParameters().processArgs);
    }
}

void QmlEngine::stopApplicationLauncher()
{
    if (d->m_applicationLauncher.isRunning()) {
        disconnect(&d->m_applicationLauncher, SIGNAL(processExited(int)), this, SLOT(disconnected()));
        d->m_applicationLauncher.stop();
    }
}

void QmlEngine::handleRemoteSetupDone(int gdbServerPort, int qmlPort)
{
    Q_UNUSED(gdbServerPort);
    if (qmlPort != -1)
        startParameters().qmlServerPort = qmlPort;
    notifyInferiorSetupOk();
}

void QmlEngine::handleRemoteSetupFailed(const QString &message)
{
    QMessageBox::critical(0,tr("Failed to start application"),
        tr("Application startup failed: %1").arg(message));
    notifyInferiorSetupFailed();
}

void QmlEngine::shutdownInferior()
{
    d->m_noDebugOutputTimer.stop();

    if (d->m_adapter.activeDebuggerClient())
        d->m_adapter.activeDebuggerClient()->endSession();

    if (isSlaveEngine()) {
        resetLocation();
    }
    stopApplicationLauncher();

    notifyInferiorShutdownOk();
}

void QmlEngine::shutdownEngine()
{
    closeConnection();

    // double check (ill engine?):
    stopApplicationLauncher();

    notifyEngineShutdownOk();
    if (!isSlaveEngine())
        showMessage(QString(), StatusBar);
}

void QmlEngine::setupEngine()
{
    connect(&d->m_applicationLauncher, SIGNAL(bringToForegroundRequested(qint64)),
            runControl(), SLOT(bringApplicationToForeground(qint64)),
            Qt::UniqueConnection);

    notifyEngineSetupOk();
}

void QmlEngine::continueInferior()
{
    QTC_ASSERT(state() == InferiorStopOk, qDebug() << state());
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "CONTINUE");
        d->m_adapter.activeDebuggerClient()->continueInferior();
    }
    resetLocation();
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::interruptInferior()
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "INTERRUPT");
        d->m_adapter.activeDebuggerClient()->interruptInferior();
    }
    notifyInferiorStopOk();
}

void QmlEngine::executeStep()
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "STEPINTO");
        d->m_adapter.activeDebuggerClient()->executeStep();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepI()
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "STEPINTO");
        d->m_adapter.activeDebuggerClient()->executeStepI();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeStepOut()
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "STEPOUT");
        d->m_adapter.activeDebuggerClient()->executeStepOut();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNext()
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, "STEPOVER");
        d->m_adapter.activeDebuggerClient()->executeNext();
    }
    notifyInferiorRunRequested();
    notifyInferiorRunOk();
}

void QmlEngine::executeNextI()
{
    executeNext();
}

void QmlEngine::executeRunToLine(const ContextData &data)
{
    Q_UNUSED(data)
    SDEBUG("FIXME:  QmlEngine::executeRunToLine()");
}

void QmlEngine::executeRunToFunction(const QString &functionName)
{
    Q_UNUSED(functionName)
    XSDEBUG("FIXME:  QmlEngine::executeRunToFunction()");
}

void QmlEngine::executeJumpToLine(const ContextData &data)
{
    Q_UNUSED(data)
    XSDEBUG("FIXME:  QmlEngine::executeJumpToLine()");
}

void QmlEngine::activateFrame(int index)
{
    if (state() != InferiorStopOk && state() != InferiorUnrunnable)
        return;

    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, QString("%1 %2").arg(QString("ACTIVATE_FRAME"), QString::number(index)));
        d->m_adapter.activeDebuggerClient()->activateFrame(index);
    }
    gotoLocation(stackHandler()->frames().value(index));
}

void QmlEngine::selectThread(int index)
{
    Q_UNUSED(index)
}

void QmlEngine::insertBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointInsertRequested, qDebug() << id << this << state);
    handler->notifyBreakpointInsertProceeding(id);

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->insertBreakpoint(id);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->insertBreakpoint(id);
        }
    }
}

void QmlEngine::removeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointRemoveRequested, qDebug() << id << this << state);
    handler->notifyBreakpointRemoveProceeding(id);

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->removeBreakpoint(id);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->removeBreakpoint(id);
        }
    }

    if (handler->state(id) == BreakpointRemoveProceeding) {
        handler->notifyBreakpointRemoveOk(id);
    }
}

void QmlEngine::changeBreakpoint(BreakpointModelId id)
{
    BreakHandler *handler = breakHandler();
    BreakpointState state = handler->state(id);
    QTC_ASSERT(state == BreakpointChangeRequested, qDebug() << id << this << state);
    handler->notifyBreakpointChangeProceeding(id);

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->changeBreakpoint(id);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->changeBreakpoint(id);
        }
    }

    if (handler->state(id) == BreakpointChangeProceeding) {
        handler->notifyBreakpointChangeOk(id);
    }
}

void QmlEngine::attemptBreakpointSynchronization()
{
    if (!stateAcceptsBreakpointChanges()) {
        showMessage(_("BREAKPOINT SYNCHRONIZATION NOT POSSIBLE IN CURRENT STATE"));
        return;
    }

    BreakHandler *handler = breakHandler();

    foreach (BreakpointModelId id, handler->unclaimedBreakpointIds()) {
        // Take ownership of the breakpoint. Requests insertion.
        if (acceptsBreakpoint(id))
            handler->setEngine(id, this);
    }

    foreach (BreakpointModelId id, handler->engineBreakpointIds(this)) {
        switch (handler->state(id)) {
        case BreakpointNew:
            // Should not happen once claimed.
            QTC_CHECK(false);
            continue;
        case BreakpointInsertRequested:
            insertBreakpoint(id);
            continue;
        case BreakpointChangeRequested:
            changeBreakpoint(id);
            continue;
        case BreakpointRemoveRequested:
            removeBreakpoint(id);
            continue;
        case BreakpointChangeProceeding:
        case BreakpointInsertProceeding:
        case BreakpointRemoveProceeding:
        case BreakpointInserted:
        case BreakpointDead:
            continue;
        }
        QTC_ASSERT(false, qDebug() << "UNKNOWN STATE"  << id << state());
    }

    DebuggerEngine::attemptBreakpointSynchronization();

    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->updateBreakpoints();
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients()) {
            client->updateBreakpoints();
        }
    }
}

bool QmlEngine::acceptsBreakpoint(BreakpointModelId id) const
{
    if (!DebuggerEngine::isCppBreakpoint(breakHandler()->breakpointData(id)))
            return true;

    //If it is a Cpp Breakpoint query if the type can be also handled by the debugger client
    //TODO: enable setting of breakpoints before start of debug session
    //For now, the event breakpoint can be set after the activeDebuggerClient is known
    //This is because the older client does not support BreakpointOnQmlSignalHandler
    bool acceptBreakpoint = false;
    if (d->m_adapter.activeDebuggerClient()) {
        acceptBreakpoint = d->m_adapter.activeDebuggerClient()->acceptsBreakpoint(id);
    }
    return acceptBreakpoint;
}

void QmlEngine::loadSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

void QmlEngine::loadAllSymbols()
{
}

void QmlEngine::reloadModules()
{
}

void QmlEngine::requestModuleSymbols(const QString &moduleName)
{
    Q_UNUSED(moduleName)
}

//////////////////////////////////////////////////////////////////////
//
// Tooltip specific stuff
//
//////////////////////////////////////////////////////////////////////

bool QmlEngine::setToolTipExpression(const QPoint &mousePos,
    TextEditor::ITextEditor *editor, const DebuggerToolTipContext &ctx)
{
    // This is processed by QML inspector, which has dependencies to 
    // the qml js editor. Makes life easier.
    emit tooltipRequested(mousePos, editor, ctx.position);
    return true;
}

//////////////////////////////////////////////////////////////////////
//
// Watch specific stuff
//
//////////////////////////////////////////////////////////////////////

void QmlEngine::assignValueInDebugger(const WatchData *data,
    const QString &expression, const QVariant &valueV)
{
    quint64 objectId =  data->id;
    if (objectId > 0 && !expression.isEmpty() && d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, QString("%1 %2 %3 %4 %5").arg(
                       QString("SET_PROPERTY"), QString::number(objectId), QString(expression),
                       valueV.toString()));
        d->m_adapter.activeDebuggerClient()->assignValueInDebugger(expression.toUtf8(), objectId, expression, valueV.toString());
    }
}

void QmlEngine::updateWatchData(const WatchData &data,
    const WatchUpdateFlags &)
{
//    qDebug() << "UPDATE WATCH DATA" << data.toString();
    //watchHandler()->rebuildModel();
    showStatusMessage(tr("Stopped."), 5000);

    if (!data.name.isEmpty() && d->m_adapter.activeDebuggerClient()) {
        if (data.isValueNeeded()) {
            logMessage(LogSend, QString("%1 %2 %3").arg(QString("EXEC"), QString(data.iname),
                                                        QString(data.name)));
            d->m_adapter.activeDebuggerClient()->updateWatchData(&data);
        }
        if (data.isChildrenNeeded()
                && watchHandler()->isExpandedIName(data.iname)) {
            d->m_adapter.activeDebuggerClient()->expandObject(data.iname, data.id);
        }
    }

    synchronizeWatchers();

    if (!data.isSomethingNeeded())
        watchHandler()->insertData(data);
}

void QmlEngine::synchronizeWatchers()
{
    QStringList watchedExpressions = watchHandler()->watchedExpressions();
    // send watchers list
    logMessage(LogSend, QString("%1 %2").arg(
                   QString("WATCH_EXPRESSIONS"), watchedExpressions.join(", ")));
    if (d->m_adapter.activeDebuggerClient()) {
        d->m_adapter.activeDebuggerClient()->synchronizeWatchers(watchedExpressions);
    } else {
        foreach (QmlDebuggerClient *client, d->m_adapter.debuggerClients())
            client->synchronizeWatchers(watchedExpressions);
    }
}

unsigned QmlEngine::debuggerCapabilities() const
{
    return AddWatcherCapability|AddWatcherWhileRunningCapability;
    /*ReverseSteppingCapability | SnapshotCapability
        | AutoDerefPointersCapability | DisassemblerCapability
        | RegisterCapability | ShowMemoryCapability
        | JumpToLineCapability | ReloadModuleCapability
        | ReloadModuleSymbolsCapability | BreakOnThrowAndCatchCapability
        | ReturnFromFunctionCapability
        | CreateFullBacktraceCapability
        | WatchpointCapability
        | AddWatcherCapability;*/
}

QString QmlEngine::toFileInProject(const QUrl &fileUrl)
{
    if (startParameters().startMode != AttachToQmlPort) {
        if (d->fileFinder.projectDirectory().isEmpty()) {
            d->fileFinder.setProjectDirectory(startParameters().projectSourceDirectory);
            d->fileFinder.setProjectFiles(startParameters().projectSourceFiles);
        }
    }

    return d->fileFinder.findFile(fileUrl);
}

void QmlEngine::inferiorSpontaneousStop()
{
    if (state() == InferiorRunOk)
        notifyInferiorSpontaneousStop();
}

void QmlEngine::disconnected()
{
    showMessage(tr("QML Debugger disconnected."), StatusBar);
    notifyInferiorExited();
}

void QmlEngine::wrongSetupMessageBoxFinished(int result)
{
    if (result == QMessageBox::Help) {
        Core::HelpManager *helpManager = Core::HelpManager::instance();
        helpManager->handleHelpRequest(
                    QLatin1String("qthelp://com.nokia.qtcreator/doc/creator-debugging-qml.html"));
    }
}

void QmlEngine::executeDebuggerCommand(const QString& command)
{
    if (d->m_adapter.activeDebuggerClient()) {
        logMessage(LogSend, QString("%1 %2 %3").arg(QString("EXEC"), QString("console"),
                                                          QString(command)));
        d->m_adapter.activeDebuggerClient()->executeDebuggerCommand(command);
    }
}


QString QmlEngine::qmlImportPath() const
{
    return startParameters().environment.value("QML_IMPORT_PATH");
}

void QmlEngine::logMessage(LogDirection direction, const QString &message)
{
    QString msg = "QmlDebugger";
    if (direction == LogSend) {
        msg += " sending ";
    } else {
        msg += " receiving ";
    }
    msg += message;
    showMessage(msg, LogDebug);
}

QmlEngine *createQmlEngine(const DebuggerStartParameters &sp,
    DebuggerEngine *masterEngine)
{
    return new QmlEngine(sp, masterEngine);
}

} // namespace Internal
} // namespace Debugger

