#-------------------------------------------------
#
# Project created by QtCreator 2016-04-16T20:31:40
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thePhoto
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    managelibrary.cpp \
    phonedialog.cpp \
    importdialog.cpp

HEADERS  += mainwindow.h \
    managelibrary.h \
    phonedialog.h \
    importdialog.h

FORMS    += mainwindow.ui \
    managelibrary.ui \
    phonedialog.ui \
    importdialog.ui

RESOURCES += \
    resources.qrc
