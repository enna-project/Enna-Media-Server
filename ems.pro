######################################################################
# Automatically generated by qmake (3.0) lun. janv. 19 23:37:31 2015
######################################################################

QT += testlib
QT += core network websockets sql dbus
QT -= gui
CONFIG += c++11 link_pkgconfig

PKGCONFIG += libmpdclient

TEMPLATE = app
TARGET = enna-media-server
INCLUDEPATH +=


QT_CONFIG -= no-pkg-config

CONFIG += link_pkgconfig
PKGCONFIG += libmpdclient



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
           CdromManager.h

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
           Player.cpp \
           SmartmontoolsNotifier.cpp \
           CdromManager.cpp


DISTFILES +=

