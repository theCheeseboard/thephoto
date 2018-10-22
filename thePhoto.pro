#-------------------------------------------------
#
# Project created by QtCreator 2016-04-16T20:31:40
#
#-------------------------------------------------

QT       += core gui network dbus concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thePhoto
TEMPLATE = app
CONFIG += c++14

unix:!macx {
    QT += thelib
    TARGET = thephoto
}

win32 {
    QT += thelib
    INCLUDEPATH += "C:/Program Files/thelibs/include"
    LIBS += -L"C:/Program Files/thelibs/lib" -lthe-libs
    #RC_FILE = icon.rc
}

SOURCES += main.cpp\
        mainwindow.cpp \
    managelibrary.cpp \
    #phonedialog.cpp \
    importdialog.cpp \
    event/eventmodeshow.cpp \
    event/eventmodesettings.cpp \
    event/eventsocket.cpp \
    event/eventserver.cpp \
    event/eventnotification.cpp \
    aboutdialog.cpp

HEADERS  += mainwindow.h \
    managelibrary.h \
    #phonedialog.h \
    importdialog.h \
    event/eventmodeshow.h \
    event/eventmodesettings.h \
    event/eventsocket.h \
    event/eventserver.h \
    event/eventnotification.h \
    aboutdialog.h

FORMS    += mainwindow.ui \
    managelibrary.ui \
    #phonedialog.ui \
    importdialog.ui \
    event/eventmodeshow.ui \
    event/eventmodesettings.ui \
    event/eventnotification.ui \
    aboutdialog.ui

RESOURCES += \
    resources.qrc


TRANSLATIONS += translations/vi_VN.ts \
    translations/da_DK.ts \
    translations/es_ES.ts \
    translations/lt_LT.ts \
    translations/nl_NL.ts \
    translations/pl_PL.ts \
    translations/pt_BR.ts \
    translations/ru_RU.ts \
    translations/sv_SE.ts \
    translations/en_AU.ts \
    translations/en_US.ts \
    translations/en_GB.ts \
    translations/en_NZ.ts \
    translations/de_DE.ts \
    translations/id_ID.ts \
    translations/au_AU.ts \
    translations/it_IT.ts \
    translations/nb_NO.ts \
    translations/no_NO.ts \
    translations/ro_RO.ts \
    translations/cy_GB.ts \
    translations/fr_FR.ts

qtPrepareTool(LUPDATE, lupdate)
genlang.commands = "$$LUPDATE -no-obsolete -source-language en_US $$_PRO_FILE_"

qtPrepareTool(LRELEASE, lrelease)
rellang.commands = "$$LRELEASE -removeidentical $$_PRO_FILE_"
QMAKE_EXTRA_TARGETS = genlang rellang
PRE_TARGETDEPS = genlang rellang

# Turn off stripping as this causes the install to fail :(
QMAKE_STRIP = echo

unix:!macx {
    target.path = /usr/bin

    translations.path = /usr/share/thephoto/translations
    translations.files = translations/*

    desktop.path = /usr/share/applications
    desktop.files = thephoto.desktop

    icon.path = /usr/share/icons/hicolor/scalable/apps/
    icon.files = thephoto.svg

    INSTALLS += target translations desktop icon
}
