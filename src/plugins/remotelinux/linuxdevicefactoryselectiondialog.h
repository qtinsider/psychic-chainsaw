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
#ifndef LINUXDEVICEFACTORYSELECTIONDIALOG_H
#define LINUXDEVICEFACTORYSELECTIONDIALOG_H

#include <QtCore/QList>
#include <QtGui/QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class LinuxDeviceFactorySelectionDialog;
} // namespace Ui
QT_END_NAMESPACE

namespace RemoteLinux {
class ILinuxDeviceConfigurationFactory;

namespace Internal {

class LinuxDeviceFactorySelectionDialog : public QDialog
{
    Q_OBJECT

public:
    explicit LinuxDeviceFactorySelectionDialog(QWidget *parent = 0);
    ~LinuxDeviceFactorySelectionDialog();
    const ILinuxDeviceConfigurationFactory *selectedFactory() const;

private:
    Q_SLOT void handleItemSelectionChanged();

    Ui::LinuxDeviceFactorySelectionDialog *ui;
    QList<const ILinuxDeviceConfigurationFactory *> m_factories;
};

} // namespace Internal
} // namespace RemoteLinux

#endif // LINUXDEVICEFACTORYSELECTIONDIALOG_H
