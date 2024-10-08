/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
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

#ifndef EXTERNALTOOL_H
#define EXTERNALTOOL_H

#include <QtCore/QObject>
#include <QtCore/QStringList>
#include <QtCore/QProcess>
#include <QtCore/QSharedPointer>
#include <QtCore/QTextCodec>
#include <QtCore/QMetaType>

namespace Utils {
class QtcProcess;
}
namespace Core {
class ActionContainer;
namespace Internal {

class ExternalTool : public QObject
{
    Q_OBJECT

public:
    enum OutputHandling {
        Ignore,
        ShowInPane,
        ReplaceSelection
    };

    ExternalTool();
    explicit ExternalTool(const ExternalTool *other);
    ~ExternalTool();

    QString id() const;
    QString description() const;
    QString displayName() const;
    QString displayCategory() const;
    int order() const;
    OutputHandling outputHandling() const;
    OutputHandling errorHandling() const;
    bool modifiesCurrentDocument() const;

    QStringList executables() const;
    QString arguments() const;
    QString input() const;
    QString workingDirectory() const;

    void setFileName(const QString &fileName);
    void setPreset(QSharedPointer<ExternalTool> preset);
    QString fileName() const;
    // all tools that are preset (changed or unchanged) have the original value here:
    QSharedPointer<ExternalTool> preset() const;

    static ExternalTool *createFromXml(const QByteArray &xml, QString *errorMessage = 0, const QString &locale = QString());
    static ExternalTool *createFromFile(const QString &fileName, QString *errorMessage = 0,
                                        const QString &locale = QString());

    bool save(QString *errorMessage = 0) const;

    bool operator==(const ExternalTool &other) const;
    bool operator!=(const ExternalTool &other) const { return !((*this) == other); }
    ExternalTool &operator=(const ExternalTool &other);

    void setId(const QString &id);
    void setDisplayCategory(const QString &category);
    void setDisplayName(const QString &name);
    void setDescription(const QString &description);
    void setOutputHandling(OutputHandling handling);
    void setErrorHandling(OutputHandling handling);
    void setModifiesCurrentDocument(bool modifies);
    void setExecutables(const QStringList &executables);
    void setArguments(const QString &arguments);
    void setInput(const QString &input);
    void setWorkingDirectory(const QString &workingDirectory);

private:
    QString m_id;
    QString m_description;
    QString m_displayName;
    QString m_displayCategory;
    int m_order;
    QStringList m_executables;
    QString m_arguments;
    QString m_input;
    QString m_workingDirectory;
    OutputHandling m_outputHandling;
    OutputHandling m_errorHandling;
    bool m_modifiesCurrentDocument;

    QString m_fileName;
    QString m_presetFileName;
    QSharedPointer<ExternalTool> m_presetTool;
};

class ExternalToolRunner : public QObject
{
    Q_OBJECT
public:
    ExternalToolRunner(const ExternalTool *tool);
    ~ExternalToolRunner();

    bool hasError() const;
    QString errorString() const;

private slots:
    void started();
    void finished(int exitCode, QProcess::ExitStatus status);
    void error(QProcess::ProcessError error);
    void readStandardOutput();
    void readStandardError();

private:
    void run();
    bool resolve();

    const ExternalTool *m_tool; // is a copy of the tool that was passed in
    QString m_resolvedExecutable;
    QString m_resolvedArguments;
    QString m_resolvedInput;
    QString m_resolvedWorkingDirectory;
    Utils::QtcProcess *m_process;
    QTextCodec *m_outputCodec;
    QTextCodec::ConverterState m_outputCodecState;
    QTextCodec::ConverterState m_errorCodecState;
    QString m_processOutput;
    QString m_expectedFileName;
    bool m_hasError;
    QString m_errorString;
};

} // Internal
} // Core

Q_DECLARE_METATYPE(Core::Internal::ExternalTool *)

#endif // EXTERNALTOOL_H
