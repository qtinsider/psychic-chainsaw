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

#include "command_p.h"

#include "icore.h"
#include "icontext.h"
#include "id.h"

#include <QtCore/QDebug>
#include <QtCore/QTextStream>

#include <QtGui/QAction>
#include <QtGui/QShortcut>
#include <QtGui/QMainWindow>

/*!
    \class Core::Command
    \mainclass

    \brief The class Command represents an action like a menu item, tool button, or shortcut.
    You don't create Command objects directly, instead use \l{ActionManager::registerAction()}
    to register an action and retrieve a Command. The Command object represents the user visible
    action and its properties. If multiple actions are registered with the same ID (but
    different contexts) the returned Command is the shared one between these actions.

    A Command has two basic properties: A default shortcut and a default text. The default
    shortcut is a key sequence that the user can use to trigger the active action that
    the Command represents. The default text is e.g. used for representing the Command
    in the keyboard shortcut preference pane. If the default text is empty, the text
    of the visible action is used.

    The user visible action is updated to represent the state of the active action (if any).
    For performance reasons only the enabled and visible state are considered by default though.
    You can tell a Command to also update the actions icon and text by setting the
    corresponding \l{Command::CommandAttribute}{attribute}.

    If there is no active action, the default behavior of the visible action is to be disabled.
    You can change that behavior to make the visible action hide instead via the Command's
    \l{Command::CommandAttribute}{attributes}.
*/

/*!
    \enum Command::CommandAttribute
    Defines how the user visible action is updated when the active action changes.
    The default is to update the enabled and visible state, and to disable the
    user visible action when there is no active action.
    \omitvalue CA_Mask
    \value CA_UpdateText
        Also update the actions text.
    \value CA_UpdateIcon
        Also update the actions icon.
    \value CA_Hide
        When there is no active action, hide the user "visible" action, instead of just
        disabling it.
    \value CA_NonConfigurable
        Flag to indicate that the keyboard shortcut of this Command should not be
        configurable by the user.
*/

/*!
    \fn void Command::setDefaultKeySequence(const QKeySequence &key)
    Set the default keyboard shortcut that can be used to activate this command to \a key.
    This is used if the user didn't customize the shortcut, or resets the shortcut
    to the default one.
*/

/*!
    \fn void Command::defaultKeySequence() const
    Returns the default keyboard shortcut that can be used to activate this command.
    \sa setDefaultKeySequence()
*/

/*!
    \fn void Command::keySequenceChanged()
    Sent when the keyboard shortcut assigned to this Command changes, e.g.
    when the user sets it in the keyboard shortcut settings dialog.
*/

/*!
    \fn QKeySequence Command::keySequence() const
    Returns the current keyboard shortcut assigned to this Command.
    \sa defaultKeySequence()
*/

/*!
    \fn void Command::setKeySequence(const QKeySequence &key)
    \internal
*/

/*!
    \fn void Command::setDefaultText(const QString &text)
    Set the \a text that is used to represent the Command in the
    keyboard shortcut settings dialog. If you don't set this,
    the current text from the user visible action is taken (which
    is ok in many cases).
*/

/*!
    \fn QString Command::defaultText() const
    Returns the text that is used to present this Command to the user.
    \sa setDefaultText()
*/

/*!
    \fn int Command::id() const
    \internal
*/

/*!
    \fn QString Command::stringWithAppendedShortcut(const QString &string) const
    Returns the \a string with an appended representation of the keyboard shortcut
    that is currently assigned to this Command.
*/

/*!
    \fn QAction *Command::action() const
    Returns the user visible action for this Command.
    If the Command represents a shortcut, it returns null.
    Use this action to put it on e.g. tool buttons. The action
    automatically forwards trigger and toggle signals to the
    action that is currently active for this Command.
    It also shows the current keyboard shortcut in its
    tool tip (in addition to the tool tip of the active action)
    and gets disabled/hidden when there is
    no active action for the current context.
*/

/*!
    \fn QShortcut *Command::shortcut() const
    Returns the shortcut for this Command.
    If the Command represents an action, it returns null.
*/

/*!
    \fn void Command::setAttribute(CommandAttribute attribute)
    Add the \a attribute to the attributes of this Command.
    \sa CommandAttribute
    \sa removeAttribute()
    \sa hasAttribute()
*/

/*!
    \fn void Command::removeAttribute(CommandAttribute attribute)
    Remove the \a attribute from the attributes of this Command.
    \sa CommandAttribute
    \sa setAttribute()
*/

/*!
    \fn bool Command::hasAttribute(CommandAttribute attribute) const
    Returns if the Command has the \a attribute set.
    \sa CommandAttribute
    \sa removeAttribute()
    \sa setAttribute()
*/

/*!
    \fn bool Command::isActive() const
    Returns if the Command has an active action/shortcut for the current
    context.
*/

/*!
    \fn bool Command::isScriptable() const
    Returns if the Command is scriptable. A scriptable command can be called
    from a script without the need for the user to interact with it.
*/

/*!
    \fn bool Command::isScriptable(const Context &) const
    Returns if the Command is scriptable for the given context.
    A scriptable command can be called from a script without the need for the user to
    interact with it.
*/

/*!
    \fn Command::~Command()
    \internal
*/

using namespace Core::Internal;

/*!
    \class CommandPrivate
    \internal
*/

CommandPrivate::CommandPrivate(int id)
    : m_attributes(0), m_id(id), m_isKeyInitialized(false)
{
}

void CommandPrivate::setDefaultKeySequence(const QKeySequence &key)
{
    if (!m_isKeyInitialized)
        setKeySequence(key);
    m_defaultKey = key;
}

QKeySequence CommandPrivate::defaultKeySequence() const
{
    return m_defaultKey;
}

void CommandPrivate::setKeySequence(const QKeySequence &key)
{
    Q_UNUSED(key)
    m_isKeyInitialized = true;
}

void CommandPrivate::setDefaultText(const QString &text)
{
    m_defaultText = text;
}

QString CommandPrivate::defaultText() const
{
    return m_defaultText;
}

int CommandPrivate::id() const
{
    return m_id;
}

QAction *CommandPrivate::action() const
{
    return 0;
}

QShortcut *CommandPrivate::shortcut() const
{
    return 0;
}

Core::Context CommandPrivate::context() const
{
    return m_context;
}

void CommandPrivate::setAttribute(CommandAttribute attr)
{
    m_attributes |= attr;
}

void CommandPrivate::removeAttribute(CommandAttribute attr)
{
    m_attributes &= ~attr;
}

bool CommandPrivate::hasAttribute(CommandAttribute attr) const
{
    return (m_attributes & attr);
}

QString CommandPrivate::stringWithAppendedShortcut(const QString &str) const
{
    return Utils::ProxyAction::stringWithAppendedShortcut(str, keySequence());
}

// ---------- Shortcut ------------

/*!
    \class Shortcut
    \internal
*/

Shortcut::Shortcut(int id)
    : CommandPrivate(id), m_shortcut(0), m_scriptable(false)
{

}

void Shortcut::setShortcut(QShortcut *shortcut)
{
    m_shortcut = shortcut;
}

QShortcut *Shortcut::shortcut() const
{
    return m_shortcut;
}

void Shortcut::setContext(const Core::Context &context)
{
    m_context = context;
}

Core::Context Shortcut::context() const
{
    return m_context;
}

void Shortcut::setKeySequence(const QKeySequence &key)
{
    CommandPrivate::setKeySequence(key);
    m_shortcut->setKey(key);
    emit keySequenceChanged();
}

QKeySequence Shortcut::keySequence() const
{
    return m_shortcut->key();
}

void Shortcut::setDefaultText(const QString &text)
{
    m_defaultText = text;
}

QString Shortcut::defaultText() const
{
    return m_defaultText;
}

void Shortcut::setCurrentContext(const Core::Context &context)
{
    foreach (int ctxt, m_context) {
        if (context.contains(ctxt)) {
            if (!m_shortcut->isEnabled()) {
                m_shortcut->setEnabled(true);
                emit activeStateChanged();
            }
            return;
        }
    }
    if (m_shortcut->isEnabled()) {
        m_shortcut->setEnabled(false);
        emit activeStateChanged();
    }
    return;
}

bool Shortcut::isActive() const
{
    return m_shortcut->isEnabled();
}

bool Shortcut::isScriptable() const
{
    return m_scriptable;
}

bool Shortcut::isScriptable(const Core::Context &) const
{
    return m_scriptable;
}

void Shortcut::setScriptable(bool value)
{
    m_scriptable = value;
}

// ---------- Action ------------

/*!
  \class Action
  \internal
*/
Action::Action(int id)
    : CommandPrivate(id),
    m_action(new Utils::ProxyAction(this)),
    m_active(false),
    m_contextInitialized(false)
{
    m_action->setShortcutVisibleInToolTip(true);
    connect(m_action, SIGNAL(changed()), this, SLOT(updateActiveState()));
}

QAction *Action::action() const
{
    return m_action;
}

void Action::setKeySequence(const QKeySequence &key)
{
    CommandPrivate::setKeySequence(key);
    m_action->setShortcut(key);
    emit keySequenceChanged();
}

QKeySequence Action::keySequence() const
{
    return m_action->shortcut();
}

void Action::setCurrentContext(const Core::Context &context)
{
    m_context = context;

    QAction *currentAction = 0;
    for (int i = 0; i < m_context.size(); ++i) {
        if (QAction *a = m_contextActionMap.value(m_context.at(i), 0)) {
            currentAction = a;
            break;
        }
    }

    m_action->setAction(currentAction);
    updateActiveState();
}

void Action::updateActiveState()
{
    setActive(m_action->isEnabled() && m_action->isVisible() && !m_action->isSeparator());
}

static QString msgActionWarning(QAction *newAction, int k, QAction *oldAction)
{
    QString msg;
    QTextStream str(&msg);
    str << "addOverrideAction " << newAction->objectName() << '/' << newAction->text()
         << ": Action ";
    if (oldAction)
        str << oldAction->objectName() << '/' << oldAction->text();
    str << " is already registered for context " << k << ' '
        << Core::Id::fromUniqueIdentifier(k).toString()
        << '.';
    return msg;
}

void Action::addOverrideAction(QAction *action, const Core::Context &context, bool scriptable)
{
#ifdef Q_WS_MAC
    action->setIconVisibleInMenu(false);
#endif
    if (isEmpty())
        m_action->initialize(action);
    if (context.isEmpty()) {
        m_contextActionMap.insert(0, action);
    } else {
        for (int i = 0; i < context.size(); ++i) {
            int k = context.at(i);
            if (m_contextActionMap.contains(k))
                qWarning("%s", qPrintable(msgActionWarning(action, k, m_contextActionMap.value(k, 0))));
            m_contextActionMap.insert(k, action);
        }
    }
    m_scriptableMap[action] = scriptable;
    setCurrentContext(m_context);
}

void Action::removeOverrideAction(QAction *action)
{
    QMutableMapIterator<int, QPointer<QAction> > it(m_contextActionMap);
    while (it.hasNext()) {
        it.next();
        if (it.value() == 0) {
            it.remove();
        } else if (it.value() == action) {
            it.remove();
        }
    }
    setCurrentContext(m_context);
}

bool Action::isActive() const
{
    return m_active;
}

void Action::setActive(bool state)
{
    if (state != m_active) {
        m_active = state;
        emit activeStateChanged();
    }
}

bool Action::isEmpty() const
{
    return m_contextActionMap.isEmpty();
}

bool Action::isScriptable() const
{
    return m_scriptableMap.values().contains(true);
}

bool Action::isScriptable(const Core::Context &context) const
{
    if (context == m_context && m_scriptableMap.contains(m_action->action()))
        return m_scriptableMap.value(m_action->action());

    for (int i = 0; i < context.size(); ++i) {
        if (QAction *a = m_contextActionMap.value(context.at(i), 0)) {
            if (m_scriptableMap.contains(a) && m_scriptableMap.value(a))
                return true;
        }
    }
    return false;
}

void Action::setAttribute(CommandAttribute attr)
{
    CommandPrivate::setAttribute(attr);
    switch (attr) {
    case Core::Command::CA_Hide:
        m_action->setAttribute(Utils::ProxyAction::Hide);
        break;
    case Core::Command::CA_UpdateText:
        m_action->setAttribute(Utils::ProxyAction::UpdateText);
        break;
    case Core::Command::CA_UpdateIcon:
        m_action->setAttribute(Utils::ProxyAction::UpdateIcon);
        break;
    case Core::Command::CA_NonConfigurable:
        break;
    }
}

void Action::removeAttribute(CommandAttribute attr)
{
    CommandPrivate::removeAttribute(attr);
    switch (attr) {
    case Core::Command::CA_Hide:
        m_action->removeAttribute(Utils::ProxyAction::Hide);
        break;
    case Core::Command::CA_UpdateText:
        m_action->removeAttribute(Utils::ProxyAction::UpdateText);
        break;
    case Core::Command::CA_UpdateIcon:
        m_action->removeAttribute(Utils::ProxyAction::UpdateIcon);
        break;
    case Core::Command::CA_NonConfigurable:
        break;
    }
}
