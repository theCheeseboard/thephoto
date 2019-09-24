#-------------------------------------------------
#
# Project created by QtCreator 2016-04-16T20:31:40
#
#-------------------------------------------------

QT       += core gui network concurrent multimedia

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thePhoto
TEMPLATE = app
CONFIG += c++14
SHARE_APP_NAME=thephoto

unix:!macx {
    # Include the-libs build tools
    include(/usr/share/the-libs/pri/buildmaster.pri)

    QT += thelib dbus
    TARGET = thephoto

    target.path = /usr/bin

    desktop.path = /usr/share/applications
    desktop.files = com.vicr123.thephoto.desktop

    icon.path = /usr/share/icons/hicolor/scalable/apps/
    icon.files = icons/thephoto.svg

    INSTALLS += target icon desktop
}

win32 {
    # Include the-libs build tools
    include(C:/Program Files/thelibs/pri/buildmaster.pri)

    QT += thelib
    INCLUDEPATH += "C:/Program Files/thelibs/include"
    LIBS += -L"C:/Program Files/thelibs/lib" -lthe-libs
#    RC_FILE = icon.rc
    TARGET = thePhoto
}

macx {
    # Include the-libs build tools
    include(/usr/local/share/the-libs/pri/buildmaster.pri)

    QT += macextras
    LIBS += -framework CoreFoundation -framework AppKit
    SOURCES += librarywindow-objc.mm

    blueprint {
        TARGET = "thePhoto Blueprint"
#        ICON = icon-bp.icns
        ICON = icon.icns
    } else {
        TARGET = "thePhoto"
        ICON = icon.icns

        sign.commands = codesign -s theSuite $$OUT_PWD/thePhoto.app/
        sign.target = sign
        QMAKE_EXTRA_TARGETS += sign
    }

    INCLUDEPATH += "/usr/local/include/the-libs"
    LIBS += -L/usr/local/lib -lthe-libs

    QMAKE_POST_LINK += $$quote(cp $${PWD}/dmgicon.icns $${PWD}/app-dmg-background.png $${PWD}/node-appdmg-config*.json $${OUT_PWD}/..)
}


SOURCES += main.cpp\
    easyexif/exif.cpp \
    library/imagedescriptor.cpp \
    library/imagedescriptormanager.cpp \
    library/imagegrid.cpp \
    library/imageview.cpp \
    library/imageviewsidebar.cpp \
    librarywindow.cpp \
        mainwindow.cpp \
    managelibrary.cpp \
    #phonedialog.cpp \
    event/eventmodeshow.cpp \
    event/eventmodesettings.cpp \
    event/eventsocket.cpp \
    event/eventserver.cpp \
    event/eventnotification.cpp \
    aboutdialog.cpp \
    event/eventmodeuserindicator.cpp \
    flowlayout.cpp

HEADERS  += mainwindow.h \
    easyexif/exif.h \
    library/imagedescriptor.h \
    library/imagedescriptormanager.h \
    library/imagegrid.h \
    library/imageview.h \
    library/imageviewsidebar.h \
    librarywindow.h \
    managelibrary.h \
    #phonedialog.h \
    event/eventmodeshow.h \
    event/eventmodesettings.h \
    event/eventsocket.h \
    event/eventserver.h \
    event/eventnotification.h \
    aboutdialog.h \
    event/eventmodeuserindicator.h \
    flowlayout.h

macx {
    SOURCES += event/musicprovider/musicprovider_mac.cpp
    HEADERS += event/musicprovider/musicprovider_mac.h
}

unix:!macx {
    SOURCES += event/musicprovider/musicprovider_linux.cpp
    HEADERS += event/musicprovider/musicprovider_linux.h
}

win32 {
    SOURCES += event/musicprovider/musicprovider_win.cpp
    HEADERS += event/musicprovider/musicprovider_win.h
}

FORMS    += mainwindow.ui \
    library/imageview.ui \
    library/imageviewsidebar.ui \
    librarywindow.ui \
    managelibrary.ui \
    #phonedialog.ui \
    event/eventmodeshow.ui \
    event/eventmodesettings.ui \
    event/eventnotification.ui \
    aboutdialog.ui \
    event/eventmodeuserindicator.ui

RESOURCES += \
    resources.qrc \
    icons.qrc

# Turn off stripping as this causes the install to fail :(
QMAKE_STRIP = echo
