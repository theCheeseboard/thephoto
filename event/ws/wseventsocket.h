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
#ifndef WSEVENTSOCKET_H
#define WSEVENTSOCKET_H

#include <QObject>
#include <QtCrypto>
#include <functional>

typedef std::function<void(QJsonObject)> ReplyFunction;

class QWebSocket;
struct WsEventSocketPrivate;
class WsEventSocket : public QObject {
        Q_OBJECT
    public:
        explicit WsEventSocket(QWebSocket* socket, QCA::RSAPrivateKey privateKey, QObject* parent = nullptr);
        ~WsEventSocket();

        QString username(quint64 userId);
        void sendMessage(quint64 userId, QJsonObject message);
        void sendMessage(quint64 userId, QJsonObject message, ReplyFunction reply);

    signals:
        void userLogin(quint64 userId, QString username);
        void userGone(quint64 userId);
        void timer(quint64 userId, int durationLeft);
        void pictureReceived(quint64 userId, QByteArray pictureData);
        void socketDisconnected();
        void rendezvousClientConnect();

    private:
        WsEventSocketPrivate* d;
};

#endif // WSEVENTSOCKET_H
