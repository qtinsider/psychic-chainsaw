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

import QtQuick 1.1

Item {
    id: root
    height: childrenRect.height
    property string header
    property QtObject model
    property Component delegate

    Rectangle {
        id: header
        color: "#f2f2f2"
        width: parent.width
        height: 51

        Text {
            id: titleText
            x: 10
            y: 10
            text: root.header
            anchors.verticalCenter: parent.verticalCenter
            width: parent.width
            font.bold: true
            font.pointSize: 14
            color: "#444"
            elide: Text.ElideRight
        }
    }
    Rectangle {
        height: 1
        color: "#a09c90"
        anchors.top: header.bottom
        width: parent.width
    }

    Column {
        id: dataSection
        anchors.top: header.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.leftMargin: 16
        anchors.topMargin: 16

        Repeater {
            model: root.model
            delegate: root.delegate
        }
    }
}
