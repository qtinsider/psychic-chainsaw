/**************************************************************************
**
** This file is part of Qt Creator
**
** Copyright (c) 2011 Nokia Corporation and/or its subsidiary(-ies).
**
** Contact: Nokia Corporation (qt-info@nokia.com)
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

#include "deviceorientation.h"
#include <QtDBus>
#include <QDebug>

#define ORIENTATION_SERVICE "com.nokia.SensorService"
#define ORIENTATION_PATH "/org/maemo/contextkit/Screen/TopEdge"
#define CONTEXT_INTERFACE "org.maemo.contextkit.Property"
#define CONTEXT_CHANGED "ValueChanged"
#define CONTEXT_SUBSCRIBE "Subscribe"
#define CONTEXT_UNSUBSCRIBE "Unsubscribe"
#define CONTEXT_GET "Get"


class HarmattanOrientation : public DeviceOrientation
{
    Q_OBJECT
public:
    HarmattanOrientation()
        : o(UnknownOrientation), sensorEnabled(false)
    {
        resumeListening();
        // connect to the orientation change signal
        bool ok = QDBusConnection::systemBus().connect(ORIENTATION_SERVICE, ORIENTATION_PATH,
                CONTEXT_INTERFACE,
                CONTEXT_CHANGED,
                this,
                SLOT(deviceOrientationChanged(QList<QVariant>,quint64)));
//        qDebug() << "connection OK" << ok;
        QDBusMessage reply = QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(ORIENTATION_SERVICE, ORIENTATION_PATH,
                                               CONTEXT_INTERFACE, CONTEXT_GET));
        if (reply.type() != QDBusMessage::ErrorMessage) {
            QList<QVariant> args;
            qvariant_cast<QDBusArgument>(reply.arguments().at(0)) >> args;
            deviceOrientationChanged(args, 0);
        }
    }

    ~HarmattanOrientation()
    {
        // unsubscribe from the orientation sensor
        if (sensorEnabled)
            QDBusConnection::systemBus().call(
                QDBusMessage::createMethodCall(ORIENTATION_SERVICE, ORIENTATION_PATH,
                                               CONTEXT_INTERFACE, CONTEXT_UNSUBSCRIBE));
    }

    inline Orientation orientation() const
    {
        return o;
    }

    void setOrientation(Orientation)
    {
    }

    void pauseListening() {
        if (sensorEnabled) {
            // unsubscribe from the orientation sensor
            QDBusConnection::systemBus().call(
                    QDBusMessage::createMethodCall(ORIENTATION_SERVICE, ORIENTATION_PATH,
                                                   CONTEXT_INTERFACE, CONTEXT_UNSUBSCRIBE));
            sensorEnabled = false;
        }
    }

    void resumeListening() {
        if (!sensorEnabled) {
            // subscribe to the orientation sensor
            QDBusMessage reply = QDBusConnection::systemBus().call(
                    QDBusMessage::createMethodCall(ORIENTATION_SERVICE, ORIENTATION_PATH,
                                                   CONTEXT_INTERFACE, CONTEXT_SUBSCRIBE));

            if (reply.type() == QDBusMessage::ErrorMessage) {
                qWarning("Unable to retrieve device orientation: %s", qPrintable(reply.errorMessage()));
            } else {
                sensorEnabled = true;
            }
        }
    }

private Q_SLOTS:
    void deviceOrientationChanged(QList<QVariant> args,quint64)
    {
        if (args.count() == 0)
            return;
        Orientation newOrientation = toOrientation(args.at(0).toString());
        if (newOrientation != o) {
            o = newOrientation;
            emit orientationChanged();
        }
//        qDebug() << "orientation" << args.at(0).toString();
    }

private:
    static Orientation toOrientation(const QString &nativeOrientation)
    {
        if (nativeOrientation == "top")
            return Landscape;
        else if (nativeOrientation == "left")
            return Portrait;
        else if (nativeOrientation == "bottom")
            return LandscapeInverted;
        else if (nativeOrientation == "right")
            return PortraitInverted;
        return UnknownOrientation;
    }

private:
    Orientation o;
    bool sensorEnabled;
};

DeviceOrientation* DeviceOrientation::instance()
{
    static HarmattanOrientation *o = new HarmattanOrientation;
    return o;
}

#include "deviceorientation_harmattan.moc"
