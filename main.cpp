#include "mainwindow.h"
#include "librarywindow.h"
#include <tapplication.h>
#include <QTranslator>

#ifdef Q_OS_MAC
#include <CoreFoundation/CFBundle.h>
#endif

int main(int argc, char *argv[])
{
    tApplication a(argc, argv);

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationName("thePhoto");

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
    localTranslator.load(QLocale::system().name(), "../thePhoto/translations");
#endif

#ifdef Q_OS_WIN
    localTranslator.load(QLocale::system().name(), a.applicationDirPath() + "\\translations");
#endif

    a.installTranslator(&localTranslator);

    if (a.arguments().contains("--old-library")) {
        MainWindow* w = new MainWindow;
        w->show();
    } else {
        LibraryWindow* w = new LibraryWindow();
        w->show();
    }

    return a.exec();
}
