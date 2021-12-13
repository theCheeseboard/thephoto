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
#ifndef WSRENDEZVOUSSERVER_H
#define WSRENDEZVOUSSERVER_H

#include <QObject>

class WsEventSocket;
struct WsRendezvousServerPrivate;
class WsRendezvousServer : public QObject {
        Q_OBJECT
    public:
        explicit WsRendezvousServer(QObject* parent = nullptr);
        ~WsRendezvousServer();

        enum ConnectionState {
            SettingUp,
            Connected,
            Error
        };

        void reconnect();
        void issueKeys();
        void connectNewClient();

        ConnectionState currentConnectionState();


    signals:
        void newSocketAvailable(WsEventSocket* socket);
        void newServerIdAvailable(int serverId, QString hmac);
        void connectionStateChanged(ConnectionState state);

    private:
        WsRendezvousServerPrivate* d;
};

#endif // WSRENDEZVOUSSERVER_H
