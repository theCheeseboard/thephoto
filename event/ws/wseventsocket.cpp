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
#include "wseventsocket.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QWebSocket>
#include <QRandomGenerator64>
#include <QTimer>
#include <QBuffer>
#include <QtCrypto>
#include <tlogger.h>

struct WsEventSocketPrivate {
    struct SocketTransfer {
        SocketTransfer() {
            buf = new QBuffer();
        }

        ~SocketTransfer() {
            buf->deleteLater();
        }

        QBuffer* buf;
        int length;
        int current;
        quint64 userId;
    };

    int seq = 0;
    QWebSocket* socket;

    QMap<quint64, QString> users;
    QMap<quint64, QCA::SymmetricKey> symks;
    QMap<int, ReplyFunction> replies;

    QCA::RSAPrivateKey privateKey;

    static void receiveIntoSocketTransfer(SocketTransfer* transfer, QJsonObject object, WsEventSocket* eventSocket) {
        int seq = object.value("seq").toInt();

        QString data = object.value("data").toString();
        transfer->buf->write(data.toUtf8());
        transfer->current += data.length();

        if (transfer->current >= transfer->length) {
            //We've received the entire image
            transfer->buf->close();
            QByteArray pictureData = QByteArray::fromBase64(transfer->buf->buffer());

            eventSocket->sendMessage(transfer->userId, {
                {"done", true},
                {"replyTo", seq}
            });
            emit eventSocket->pictureReceived(transfer->userId, pictureData);
            delete transfer;
        } else {
            //Send the OK to continue receiving
            eventSocket->sendMessage(transfer->userId, {
                {"continue", true},
                {"replyTo", seq}
            }, [ = ](QJsonObject object) {
                WsEventSocketPrivate::receiveIntoSocketTransfer(transfer, object, eventSocket);
            });
        }
    }
};

WsEventSocket::WsEventSocket(QWebSocket* socket, QCA::RSAPrivateKey privateKey, QObject* parent) : QObject(parent) {
    d = new WsEventSocketPrivate();
    d->socket = socket;
    d->privateKey = privateKey;

    connect(socket, &QWebSocket::textMessageReceived, this, [ = ](QString message) {
        socket->binaryMessageReceived(message.toUtf8());
    });
    connect(socket, &QWebSocket::binaryMessageReceived, this, [ = ](QByteArray message) {
        //TODO: decrypt message
        QCA::SecureArray decryptOutput;
        bool decryptSuccess = d->privateKey.decrypt(message, &decryptOutput, QCA::EME_PKCS1_OAEP);
        if (decryptSuccess) {
            message = decryptOutput.toByteArray();
        } else if (message.at(0) == 'E') {
            //Try decrypting using known ciphers
            char ivLen = message.at(1);
            QCA::InitializationVector iv(message.mid(2, ivLen));
            QByteArray messageData = message.mid(2 + ivLen);

            for (quint64 userid : d->symks.keys()) {
                QCA::SymmetricKey symk = d->symks.value(userid);
                QCA::Cipher cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Decode, symk, iv);
                QByteArray decrypted = cipher.process(messageData).toByteArray();

                if (!decrypted.isEmpty() && (decrypted.at(0) == '{' || decrypted.at(0) == '[')) {
                    message = decrypted;
                    break;
                }
            }
        }

        QJsonParseError error;
        QJsonDocument doc = QJsonDocument::fromJson(message, &error);
        if (error.error != QJsonParseError::NoError) {
            tWarn("WsEventSocket") << "Invalid JSON payload received";
            return;
        }

        if (!doc.isObject()) {
            tWarn("WsEventSocket") << "Expected JSON Object in payload, received JSON Array";
            return;
        }

        QJsonObject rootObj = doc.object();
        int seq = rootObj.value("seq").toInt();
        if (rootObj.contains("replyTo")) {
            int seq = rootObj.value("replyTo").toInt();
            if (!d->replies.contains(seq)) {
                tWarn("WsEventSocket") << "Invalid reply sequence number";
                return;
            }

            d->replies.value(seq)(rootObj);
            d->replies.remove(seq);
            return;
        }

        QString type = rootObj.value("type").toString();
        if (type == QStringLiteral("serverKeepalive")) {
            //Noop
            return;
        } else if (type == QStringLiteral("rendezvousClientConnect")) {
            emit rendezvousClientConnect();
        } else if (type == QStringLiteral("connect")) {
            QString username = rootObj.value("username").toString();
            quint64 userId = QRandomGenerator64::system()->generate();
            d->users.insert(userId, username);

            QString keyString = rootObj.value("key").toString();
            QByteArray keyBytes = QByteArray::fromBase64(keyString.toUtf8());
            QCA::SymmetricKey symk(keyBytes);

            d->symks.insert(userId, symk);

            emit userLogin(userId, username);
            sendMessage(userId, {
                {"replyTo", seq},
            });
        } else if (type == QStringLiteral("disconnect")) {
            quint64 userId = rootObj.value("userId").toString().toULong();
            if (!d->users.contains(userId)) {
                sendMessage(userId, {
                    {"error", "user-unavailable"},
                    {"replyTo", seq},
                });
                return;
            }

            emit userGone(userId);
            QTimer::singleShot(0, this, [ = ] {
                d->users.remove(userId);
                d->symks.remove(userId);
            });
            sendMessage(userId, {
                {"replyTo", seq},
            });
        } else if (type == QStringLiteral("picture")) {
            QString data = rootObj.value("data").toString();
            quint64 userId = rootObj.value("userId").toString().toULong();
            int length = rootObj.value("length").toInt();

            //Prepare to transfer a picture
            WsEventSocketPrivate::SocketTransfer* transfer = new WsEventSocketPrivate::SocketTransfer();
            transfer->buf->open(QBuffer::WriteOnly);
            transfer->buf->write(data.toUtf8());

            transfer->userId = userId;
            transfer->current = data.length();
            transfer->length = length;


            if (transfer->current >= transfer->length) {
                //We've received the entire image
                transfer->buf->close();
                QByteArray pictureData = QByteArray::fromBase64(transfer->buf->buffer());

                sendMessage(transfer->userId, {
                    {"done", true},
                    {"replyTo", seq}
                });
                emit pictureReceived(transfer->userId, pictureData);
                delete transfer;
            } else {
                sendMessage(userId, {
                    {"continue", true},
                    {"replyTo", seq}
                }, [ = ](QJsonObject object) {
                    WsEventSocketPrivate::receiveIntoSocketTransfer(transfer, object, this);
                });
            }
        }
    });
    connect(socket, &QWebSocket::disconnected, this, [ = ] {
        for (quint64 userId : d->users.keys()) {
            emit userGone(userId);
        }

        QTimer::singleShot(0, [ = ] {
            d->users.clear();
        });

        emit socketDisconnected();
    });
}

WsEventSocket::~WsEventSocket() {
    d->socket->deleteLater();
    delete d;
}

QString WsEventSocket::username(quint64 userId) {
    return d->users.value(userId);
}

void WsEventSocket::sendMessage(quint64 userId, QJsonObject message) {
    sendMessage(userId, message, [ = ](QJsonObject) {});
}

void WsEventSocket::sendMessage(quint64 userId, QJsonObject message, std::function<void (QJsonObject)> reply) {
    message.insert("user", QString::number(userId));
    message.insert("seq", d->seq++);

    QByteArray payload = QJsonDocument(message).toJson();

    //TODO: encrypt payload

    QCA::InitializationVector iv(16);
    QCA::SymmetricKey symk = d->symks.value(userId);
    QCA::Cipher cipher("aes256", QCA::Cipher::CBC, QCA::Cipher::DefaultPadding, QCA::Encode, symk, iv);
    QByteArray encryptedPayload = cipher.process(payload).toByteArray();

    encryptedPayload.prepend(iv.toByteArray());
    encryptedPayload.prepend(iv.size());
    encryptedPayload.prepend('E');

    d->replies.insert(message.value("seq").toInt(), reply);

    d->socket->sendBinaryMessage(encryptedPayload);
}