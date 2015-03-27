######################################################################
# Automatically generated by qmake (3.0) lun. janv. 19 23:37:31 2015
######################################################################

QT += testlib
QT += core network websockets sql dbus
QT -= gui
CONFIG += c++11 link_pkgconfig

PKGCONFIG += libmpdclient libcdio
LIBS += -lmpdclient

TEMPLATE = app
TARGET = enna-media-server
INCLUDEPATH +=


# Input
HEADERS += Database.h \
           DirectoryWorker.h \
           DiscoveryServer.h \
           Grabbers.h \
           Scanner.h \
           sha1.h \
           WebSocketServer.h \          
           Application.h \
           Data.h \
           DefaultSettings.h \
           WebSocket.h \
           JsonApi.h \
           Player.h \
           SmartmontoolsNotifier.h \
           CdromManager.h \
           OnlineDBPlugin.h \
           OnlineDBPluginManager.h

SOURCES += Database.cpp \
           DirectoryWorker.cpp \
           DiscoveryServer.cpp \
           Grabbers.cpp \
           main.cpp \
           Scanner.cpp \
           sha1.cpp \
           WebSocketServer.cpp \
           Application.cpp \
           JsonApi.cpp \
           Player.cpp \
           SmartmontoolsNotifier.cpp \
           CdromManager.cpp \
           OnlineDBPluginManager.cpp \
           Application.cpp \
           CdromManager.cpp

DISTFILES +=

equals(EMS_PLUGIN_GRACENOTE, "yes") {
    message("Enable Gracenote plugin")
    DEFINES += EMS_PLUGIN_GRACENOTE
    PKGCONFIG += gnsdk
    HEADERS += GracenotePlugin.h
    SOURCES += GracenotePlugin.cpp
}
