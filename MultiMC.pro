#-------------------------------------------------
#
# Project created by QtCreator 2013-01-08T14:14:58
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MultiMC
TEMPLATE = app


SOURCES += main.cpp\
        gui/mainwindow.cpp \
    data/instancebase.cpp \
    util/pathutils.cpp

HEADERS  += gui/mainwindow.h \
    data/instancebase.h \
    util/pathutils.h \

FORMS    += gui/mainwindow.ui

RESOURCES += \
    multimc.qrc
