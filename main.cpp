#include "mainwindow.h"
#include "librarywindow.h"
#include <tapplication.h>
#include <QTranslator>
#include <tsettings.h>
#include <tstylemanager.h>
#include <QtCrypto>

#ifdef Q_OS_MAC
    #include <CoreFoundation/CFBundle.h>
#endif

int main(int argc, char* argv[]) {
    tApplication a(argc, argv);

#ifdef Q_OS_LINUX
    if (QDir("/usr/share/thephoto").exists()) {
        a.setShareDir("/usr/share/thephoto");
    } else if (QDir(QDir::cleanPath(QApplication::applicationDirPath() + "/../share/thephoto/")).exists()) {
        a.setShareDir(QDir::cleanPath(QApplication::applicationDirPath() + "/../share/thephoto/"));
    }
#endif
    a.installTranslators();

    a.setOrganizationName("theSuite");
    a.setOrganizationDomain("");
    a.setApplicationIcon(QIcon::fromTheme("com.vicr123.thephoto", QIcon(":/icons/com.vicr123.thephoto.svg")));
    a.setApplicationVersion("2.0");
    a.setGenericName(QApplication::translate("main", "Photo Manager"));
    a.setAboutDialogSplashGraphic(a.aboutDialogSplashGraphicFromSvg(":/icons/aboutsplash.svg"));
    a.setApplicationLicense(tApplication::Gpl3OrLater);
    a.setCopyrightHolder("Victor Tran");
    a.setCopyrightYear("2021");
    a.addCopyrightLine(QApplication::translate("main", "This product includes software developed by the OpenSSL Project for use in the OpenSSL Toolkit (http://www.openssl.org/)"));
    a.addCopyrightLine(QApplication::translate("main", "This product includes cryptographic software written by Eric Young (eay@cryptsoft.com)"));
    a.addCopyrightLine(QApplication::translate("main", "Android™ is a trademark of Google LLC"));
#ifdef T_BLUEPRINT_BUILD
    a.setApplicationName("thePhoto Blueprint");
#else
    a.setApplicationName("thePhoto");
#endif


#ifdef Q_OS_MAC
    a.setAttribute(Qt::AA_DontShowIconsInMenus, true);
#endif

    tSettings settings;
    QObject::connect(&settings, &tSettings::settingChanged, [ = ](QString key, QVariant value) {
        if (key == "theme/mode") {
            tStyleManager::setOverrideStyleForApplication(value.toString() == "light" ? tStyleManager::ContemporaryLight : tStyleManager::ContemporaryDark);
        }
    });
    tStyleManager::setOverrideStyleForApplication(settings.value("theme/mode").toString() == "light" ? tStyleManager::ContemporaryLight : tStyleManager::ContemporaryDark);

    QCA::Initializer initialiser;

    if (a.arguments().contains("--old-library")) {
        MainWindow* w = new MainWindow;
        w->show();
    } else {
        LibraryWindow* w = new LibraryWindow();
        w->show();
    }

    return a.exec();
}
