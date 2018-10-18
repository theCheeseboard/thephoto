#-------------------------------------------------
#
# Project created by QtCreator 2016-04-16T20:31:40
#
#-------------------------------------------------

QT       += core gui network dbus concurrent thelib

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thephoto
TEMPLATE = app
CONFIG += c++14

SOURCES += main.cpp\
        mainwindow.cpp \
    managelibrary.cpp \
    #phonedialog.cpp \
    importdialog.cpp \
    event/eventmodeshow.cpp \
    event/eventmodesettings.cpp \
    event/eventsocket.cpp \
    event/eventserver.cpp

HEADERS  += mainwindow.h \
    managelibrary.h \
    #phonedialog.h \
    importdialog.h \
    event/eventmodeshow.h \
    event/eventmodesettings.h \
    event/eventsocket.h \
    event/eventserver.h

FORMS    += mainwindow.ui \
    managelibrary.ui \
    #phonedialog.ui \
    importdialog.ui \
    event/eventmodeshow.ui \
    event/eventmodesettings.ui

RESOURCES += \
    resources.qrc
