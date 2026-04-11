# -------------------------------------------------
# HttpServer - Built-in HTTP file server for DG-LAN Core.
# Serves shared files over HTTP with Range support,
# peer redirect, and JSON API.
# -------------------------------------------------
QT += network
QT -= gui
TARGET = HttpServer

TEMPLATE = lib

include(../../Common/common.pri)
include(../../Libs/protobuf.pri)

CONFIG += staticlib \
   link_prl \
   create_prl
INCLUDEPATH += . \
   ../..

DEFINES += HTTPSERVER_LIBRARY
SOURCES += priv/HttpServer.cpp \
    priv/HttpConnection.cpp \
    priv/Builder.cpp \
    ../../Protos/common.pb.cc \
    priv/Log.cpp
HEADERS += IHttpServer.h \
    priv/HttpServer.h \
    priv/HttpConnection.h \
    Builder.h \
    priv/Log.h \
    ../../Protos/common.pb.h
