/****************************************
 *
 *   INSERT-PROJECT-NAME-HERE - INSERT-GENERIC-NAME-HERE
 *   Copyright (C) 2020 Victor Tran
 *
 *   This program is free software: you can redistribute it and/or modify
 *   it under the terms of the GNU General Public License as published by
 *   the Free Software Foundation, either version 3 of the License, or
 *   (at your option) any later version.
 *
 *   This program is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *   GNU General Public License for more details.
 *
 *   You should have received a copy of the GNU General Public License
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 * *************************************/
#include "wsrendezvousserver.h"

#include <tlogger.h>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QtCrypto>
#include <QRandomGenerator>
#include <QMessageAuthenticationCode>
#include "wseventsocket.h"

struct WsRendezvousServerPrivate {
    QNetworkAccessManager mgr;

    WsRendezvousServer::ConnectionState state = WsRendezvousServer::SettingUp;
    QWebSocket* currentPendingSocket = nullptr;
    QString token;
    int serverNumber;
    QString hmac;

    QCA::RSAPrivateKey privateKey;
    bool keysIssued = false;
};

WsRendezvousServer::WsRendezvousServer(QObject* parent) : QObject(parent) {
    d = new WsRendezvousServerPrivate();
    reconnect();

    QByteArray hmac;
    hmac.resize(2);
    QRandomGenerator::securelySeeded().generate(hmac.begin(), hmac.end());
    d->hmac = hmac.toHex().toUpper();
}

WsRendezvousServer::~WsRendezvousServer() {
    delete d;
}

void WsRendezvousServer::reconnect() {
    //Try to connect to the rendezvous server
    tDebug("WsRendezvousServer") << "Opening Rendezvous server session";

    QUrl rendezvousSetupUrl;
    rendezvousSetupUrl.setScheme(serverIsSecure() ? "https" : "http");
    rendezvousSetupUrl.setHost(server());
    rendezvousSetupUrl.setPort(serverPort());
    rendezvousSetupUrl.setPath("/setup");
    QNetworkReply* reply = d->mgr.get(QNetworkRequest(rendezvousSetupUrl));
    connect(reply, &QNetworkReply::finished, this, [ = ] {
        QByteArray data = reply->readAll();
        QJsonDocument replyDoc = QJsonDocument::fromJson(data);
        QJsonObject replyObj = replyDoc.object();

        d->serverNumber = replyObj.value("serverNumber").toInt();
        d->token = replyObj.value("token").toString();

        if (d->keysIssued) {
            emit newServerIdAvailable(d->serverNumber, d->hmac);
            connectNewClient();
        } else {
            issueKeys();
        }
    });
}

void WsRendezvousServer::issueKeys() {
    QCA::KeyGenerator keygen;
    keygen.setBlockingEnabled(true);
    d->privateKey = keygen.createRSA(4096).toRSA();

    QByteArray payload = d->privateKey.toPublicKey().toPEM().toUtf8();

    QUrl keySetupUrl;
    keySetupUrl.setScheme(serverIsSecure() ? "https" : "http");
    keySetupUrl.setHost(server());
    keySetupUrl.setPort(serverPort());
    keySetupUrl.setPath(QStringLiteral("/keys/%1").arg(d->serverNumber));

    QNetworkRequest keySetupRequest(keySetupUrl);
    keySetupRequest.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(d->token).toUtf8());
    keySetupRequest.setRawHeader("Host", server().toUtf8());
    keySetupRequest.setRawHeader("X-thePhoto-HMAC", QMessageAuthenticationCode::hash(payload, d->hmac.toUtf8(), QCryptographicHash::Sha512).toBase64());
    keySetupRequest.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-pem-file");

    QNetworkReply* reply = d->mgr.post(keySetupRequest, payload);
    connect(reply, &QNetworkReply::finished, this, [ = ] {
        if (reply->error() != QNetworkReply::NoError) {
            d->state = Error;
            emit connectionStateChanged(Error);
            return;
        }

        d->keysIssued = true;
        emit newServerIdAvailable(d->serverNumber, d->hmac);
        connectNewClient();
    });
}

void WsRendezvousServer::connectNewClient() {
    //Only connect a new client if there is no pending client
    if (d->currentPendingSocket) return;

    if (d->state == Error) {
        d->state = SettingUp;
        emit connectionStateChanged(SettingUp);
    }

    //Connect a new WebSockets client
    tDebug("WsRendezvousServer") << "Opening Rendezvous server client";

    QUrl rendezvousConnectUrl;
    rendezvousConnectUrl.setScheme(serverIsSecure() ? "wss" : "ws");
    rendezvousConnectUrl.setHost(server());
    rendezvousConnectUrl.setPort(serverPort());
    rendezvousConnectUrl.setPath(QStringLiteral("/setup/%1").arg(d->serverNumber));

    QNetworkRequest rendezvousRequest(rendezvousConnectUrl);
    rendezvousRequest.setRawHeader("Authorization", QStringLiteral("Bearer %1").arg(d->token).toUtf8());
    rendezvousRequest.setRawHeader("Host", server().toUtf8());

    QWebSocket* socket = new QWebSocket("", QWebSocketProtocol::VersionLatest, this);
    socket->open(rendezvousRequest);
    connect(socket, &QWebSocket::connected, this, [ = ] {
        if (d->currentPendingSocket == socket) {
            d->state = Connected;
            emit connectionStateChanged(Connected);

            d->currentPendingSocket = nullptr;
        }

        WsEventSocket* eventSocket = new WsEventSocket(socket, d->privateKey);
        connect(eventSocket, &WsEventSocket::rendezvousClientConnect, this, [ = ] {
            connectNewClient();
        });
        emit newSocketAvailable(eventSocket);
    });
    connect(socket, &QWebSocket::aboutToClose, this, [ = ] {
        if (d->currentPendingSocket == socket) d->currentPendingSocket = nullptr;
        socket->deleteLater();
        tCritical("WsRendezvousServer") << "WebSocket Closed";
    });
    connect(socket, QOverload<QAbstractSocket::SocketError>::of(&QWebSocket::error), this, [ = ](QAbstractSocket::SocketError error) {
        tCritical("WsRendezvousServer") << "Could not open rendezvous server client";
        tCritical("WsRendezvousServer") << error;
        tCritical("WsRendezvousServer") << socket->closeCode();
        tCritical("WsRendezvousServer") << socket->closeReason();

        if (d->currentPendingSocket == socket) {
            //There was an error getting to the rendezvous server :(
            d->currentPendingSocket = nullptr;

            d->state = Error;
            emit connectionStateChanged(Error);
        }
    });
    d->currentPendingSocket = socket;
}

WsRendezvousServer::ConnectionState WsRendezvousServer::currentConnectionState() {
    return d->state;
}

bool WsRendezvousServer::serverIsSecure() {
    if (qEnvironmentVariableIsSet("THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER_SECURE")) {
        return qEnvironmentVariable("THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER_SECURE") == "true";
    } else {
        return THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER_SECURE;
    }
}

QString WsRendezvousServer::server() {
    return qEnvironmentVariable("THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER", THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER);
}

int WsRendezvousServer::serverPort() {
    bool ok;
    int port = qEnvironmentVariableIntValue("THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER_PORT", &ok);
    if (!ok) port = THEPHOTO_EVENT_MODE_RENDEZVOUS_SERVER_PORT;
    return port;
}
