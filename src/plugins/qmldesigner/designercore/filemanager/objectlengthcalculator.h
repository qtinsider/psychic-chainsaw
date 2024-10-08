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

#ifndef OBJECTLENGTHCALCULATOR_H
#define OBJECTLENGTHCALCULATOR_H

#include <qmljs/parser/qmljsastvisitor_p.h>
#include <qmljs/qmljsdocument.h>

namespace QmlDesigner {

class ObjectLengthCalculator: protected QmlJS::AST::Visitor
{
public:
    ObjectLengthCalculator();

    bool operator()(const QString &text, quint32 offset, quint32 &length);

protected:
    using QmlJS::AST::Visitor::visit;

    virtual bool visit(QmlJS::AST::UiObjectBinding *ast);
    virtual bool visit(QmlJS::AST::UiObjectDefinition *ast);

private:
    QmlJS::Document::Ptr m_doc;
    quint32 m_offset;
    quint32 m_length;
};

} // namespace QmlDesigner

#endif // OBJECTLENGTHCALCULATOR_H
