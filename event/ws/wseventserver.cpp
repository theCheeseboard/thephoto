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
#include "wseventserver.h"

#include <tlogger.h>
#include <QWebSocketServer>
#include <QUrl>
#include "wseventsocket.h"

struct WsEventServerPrivate {
    QWebSocketServer* server;
};

WsEventServer::WsEventServer(QObject* parent) : QObject(parent) {
    d = new WsEventServerPrivate();
    d->server = new QWebSocketServer("thephoto", QWebSocketServer::NonSecureMode, this);
    d->server->listen(QHostAddress::Any, 26158);

    tDebug("WsEventServer") << "Event server URL:" << d->server->serverUrl().toString();

    connect(d->server, &QWebSocketServer::newConnection, this, [ = ] {
        WsEventSocket* socket = new WsEventSocket(d->server->nextPendingConnection());
        emit newSocketAvailable(socket);
    });
}

WsEventServer::~WsEventServer() {
    delete d;
}
