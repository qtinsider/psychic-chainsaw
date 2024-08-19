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

#ifndef CUSTOMWIZARDPREPROCESSOR_H
#define CUSTOMWIZARDPREPROCESSOR_H

#include <QtCore/QString>

QT_FORWARD_DECLARE_CLASS(QScriptEngine)

namespace ProjectExplorer {
namespace Internal {

bool customWizardPreprocess(const QString &in, QString *out, QString *errorMessage);
/* Helper to evaluate an expression. */
bool evaluateBooleanJavaScriptExpression(QScriptEngine &engine, const QString &expression, bool *result, QString *errorMessage);
} // namespace Internal
} // namespace ProjectExplorer

#endif // CUSTOMWIZARDPREPROCESSOR_H
