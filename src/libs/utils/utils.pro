TEMPLATE = lib
TARGET = Utils
QT += gui \
    network

include(../../qtcreatorlibrary.pri)
include(utils_dependencies.pri)

include(utils-lib.pri)
# Needed for QtCore/private/qwineventnotifier_p.h
win32:include(../../private_headers.pri)

HEADERS += \
    proxyaction.h

SOURCES += \
    proxyaction.cpp

win32: LIBS += -lUser32
