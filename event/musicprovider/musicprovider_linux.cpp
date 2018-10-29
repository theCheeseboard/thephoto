#include "musicprovider_linux.h"

#include <QTimer>
#include <QDBusConnection>
#include <QDBusConnectionInterface>
#include <QDBusInterface>

MusicProvider::MusicProvider(QObject *parent) : QObject(parent)
{    
    QTimer* t = new QTimer();
    t->setInterval(1000);
    connect(t, &QTimer::timeout, [=] {
        knownServices.clear();
        for (QString service : QDBusConnection::sessionBus().interface()->registeredServiceNames().value()) {
            if (service.startsWith("org.mpris.MediaPlayer2")) {
                knownServices.append(service);
            }
        }

        if (knownServices.count() == 0) {
            changeService("");
        } else if (!knownServices.contains(currentService) && knownServices.count() > 0) {
            changeService(knownServices.first());
        }
    });
    t->start();
}

QString MusicProvider::getMusicString() {
    return musicString;
}

void MusicProvider::changeService(QString service) {
    if (currentService != "") {
        QDBusConnection::sessionBus().disconnect(currentService, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(updateMpris(QString,QMap<QString, QVariant>,QStringList)));
    }

    currentService = service;

    if (service == "") {
        musicString = "";
        return;
    }

    QDBusConnection::sessionBus().connect(service, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "PropertiesChanged", this, SLOT(updateMpris(QString,QMap<QString, QVariant>,QStringList)));

    //Get Current Song Metadata
    QDBusMessage MetadataRequest = QDBusMessage::createMethodCall(service, "/org/mpris/MediaPlayer2", "org.freedesktop.DBus.Properties", "Get");
    MetadataRequest.setArguments(QList<QVariant>() << "org.mpris.MediaPlayer2.Player" << "Metadata");

    QDBusPendingCallWatcher* metadata = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(MetadataRequest));
    connect(metadata, &QDBusPendingCallWatcher::finished, [=] {
        QVariantMap replyData;
        QDBusArgument arg(metadata->reply().arguments().first().value<QDBusVariant>().variant().value<QDBusArgument>());

        arg >> replyData;

        updateData(replyData);
    });
}

void MusicProvider::updateData(QVariantMap data) {
    QString album = "";
    QString artist = "";
    QString title = "";

    if (data.contains("xesam:title")) {
        title = data.value("xesam:title").toString();
    }

    if (data.contains("xesam:artist")) {
        QStringList artists = data.value("xesam:artist").toStringList();
        for (QString art : artists) {
            artist.append(art + ", ");
        }
        artist.remove(artist.length() - 2, 2);
    }

    if (data.contains("xesam:album")) {
        album = data.value("xesam:album").toString();
    }

    if (title != "" && artist != "") {
        musicString = title + " · " + artist;
    } else if (title != "" && album != "") {
        musicString = title + " · " + album;
    } else if (title != "") {
        musicString = title;
    } else if (artist != "") {
        musicString = artist;
    } else if (album != "") {
        musicString = album;
    } else {
        musicString = "";
    }
}

void MusicProvider::updateMpris(QString interfaceName, QMap<QString, QVariant> properties, QStringList changedProperties) {
    if (interfaceName == "org.mpris.MediaPlayer2.Player") {
        if (properties.keys().contains("Metadata")) {
            QVariantMap replyData;
            properties.value("Metadata").value<QDBusArgument>() >> replyData;

            updateData(replyData);
        }
    }
}
