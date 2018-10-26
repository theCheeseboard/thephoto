#include "mainwindow.h"
#include <QApplication>
#include <QTranslator>

#ifdef Q_OS_MAC
#include <CoreFoundation/CFBundle.h>
#endif

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("thePhoto");

    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Services");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Hide %1");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Hide Others");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Show All");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Preferences...");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "About %1");
    QT_TRANSLATE_NOOP("MAC_APPLICATION_MENU", "Quit %1");

    QTranslator localTranslator;
#ifdef Q_OS_MAC
    a.setAttribute(Qt::AA_DontShowIconsInMenus, true);

    CFURLRef appUrlRef = CFBundleCopyBundleURL(CFBundleGetMainBundle());
    CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
    const char *pathPtr = CFStringGetCStringPtr(macPath, CFStringGetSystemEncoding());

    QString bundlePath = QString::fromLocal8Bit(pathPtr);
    localTranslator.load(QLocale::system().name(), bundlePath + "/Contents/translations/");

    CFRelease(appUrlRef);
    CFRelease(macPath);
#endif

#ifdef Q_OS_LINUX
    localTranslator.load(QLocale::system().name(), "/usr/share/thephoto/translations");
#endif

#ifdef Q_OS_WIN
    localTranslator.load(QLocale::system().name(), a.applicationDirPath() + "\\translations");
#endif

    a.installTranslator(&localTranslator);

    MainWindow w;
    w.show();

    return a.exec();
}
