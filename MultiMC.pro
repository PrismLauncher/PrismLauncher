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
    util/pathutils.cpp \
    data/instancelist.cpp \
    data/stdinstance.cpp \
    data/inifile.cpp \
    gui/settingsdialog.cpp \
    gui/modeditwindow.cpp \
    data/appsettings.cpp \
    data/settingsbase.cpp

HEADERS  += gui/mainwindow.h \
    data/instancebase.h \
    util/pathutils.h \
    data/instancelist.h \
    data/stdinstance.h \
    data/inifile.h \
    gui/settingsdialog.h \
    gui/modeditwindow.h \
    data/appsettings.h \
    data/settingsbase.h \
    util/settingsmacros.h \
    util/settingsmacrosundef.h

FORMS    += gui/mainwindow.ui \
    gui/settingsdialog.ui \
    gui/modeditwindow.ui

RESOURCES += \
    multimc.qrc
