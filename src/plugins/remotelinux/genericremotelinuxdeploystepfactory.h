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
#ifndef GENERICREMOTELINUXDEPLOYSTEPFACTORY_H
#define GENERICREMOTELINUXDEPLOYSTEPFACTORY_H

#include <projectexplorer/buildstep.h>

namespace RemoteLinux {
namespace Internal {

class GenericRemoteLinuxDeployStepFactory : public ProjectExplorer::IBuildStepFactory
{
    Q_OBJECT
public:
    GenericRemoteLinuxDeployStepFactory(QObject *parent = 0);

    QStringList availableCreationIds(ProjectExplorer::BuildStepList *parent) const;
    QString displayNameForId(const QString &id) const;
    bool canCreate(ProjectExplorer::BuildStepList *parent, const QString &id) const;
    ProjectExplorer::BuildStep *create(ProjectExplorer::BuildStepList *parent, const QString &id);
    bool canRestore(ProjectExplorer::BuildStepList *parent, const QVariantMap &map) const;
    ProjectExplorer::BuildStep *restore(ProjectExplorer::BuildStepList *parent,
        const QVariantMap &map);
    bool canClone(ProjectExplorer::BuildStepList *parent,
        ProjectExplorer::BuildStep *product) const;
    ProjectExplorer::BuildStep *clone(ProjectExplorer::BuildStepList *parent,
        ProjectExplorer::BuildStep *product);
};

} // namespace Internal
} // namespace RemoteLinux

#endif // GENERICREMOTELINUXDEPLOYSTEPFACTORY_H
