#-------------------------------------------------
#
# Project created by QtCreator 2014-06-06T01:07:16
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MagicHand
TEMPLATE = app

CONFIG += c++11

SOURCES += main.cpp\
        mainwindow.cpp \
    mhwebserver.cpp \
    mhserver.cpp \
    mhwebsocket.cpp

HEADERS  += mainwindow.h \
    mhwebserver.h \
    mhserver.h \
    mhplatform.h \
    mhwebsocket.h

FORMS    += mainwindow.ui

RESOURCES += \
    resources.qrc


macx{
    LIBS+= -framework ApplicationServices -framework AppKit
    OBJECTIVE_SOURCES += \
        mhplatform_osx.mm
}

