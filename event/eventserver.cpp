#include "eventserver.h"

#include <tpromise.h>
#include "eventsocket.h"

EventServer::EventServer(QObject *parent) : QTcpServer(parent)
{
    (new tPromise<QStringList>([=](QString& error) {
        //Generate an X509 certificate
        QDir::home().mkpath(".thephoto/cert");
        QDir certPath(QDir::homePath() + "/.thephoto/cert");

        QProcess genProc;
        genProc.setProcessChannelMode(QProcess::ForwardedChannels);
        genProc.setWorkingDirectory(certPath.path());
        genProc.start("openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 2 -nodes -subj \"/CN=localhost\"");
        genProc.waitForFinished();

        qDebug() << genProc.readAll();

        QFile certFile(certPath.absoluteFilePath("cert.pem"));
        QFile keyFile(certPath.absoluteFilePath("key.pem"));

        if (!certFile.exists() || !keyFile.exists()) {
            error = tr("Can't create X509 certificate");
            return QStringList();
        }

        QStringList ssl;
        ssl.append(certFile.fileName());
        ssl.append(keyFile.fileName());

        return ssl;
    }))->then([=](QStringList paths) {
        QFile certFile(paths.first());
        QFile keyFile(paths.at(1));

        certFile.open(QFile::ReadOnly);
        keyFile.open(QFile::ReadOnly);

        this->ssl.cert = QSslCertificate(&certFile);
        this->ssl.key = QSslKey(keyFile.readAll(), QSsl::Rsa, QSsl::Pem);
        qDebug() << "SSL Certificate Ready";
    })->error([=](QString error) {
        qDebug() << error;
    });
}

void EventServer::incomingConnection(qintptr handle) {
    //Connection* c = new Connection();
    EventSocket* c = new EventSocket();
    if (c->setSocketDescriptor(handle)) {
        addPendingConnection(c);

        connect(c, static_cast<void(QAbstractSocket::*)(QAbstractSocket::SocketError)>(&QAbstractSocket::error),
            [=](QAbstractSocket::SocketError socketError){
            if (socketError == QSslSocket::SslHandshakeFailedError) {
                qDebug() << "SSL Handshake failure";
            } else if (socketError == QSslSocket::RemoteHostClosedError) {
            } else {
                qDebug() << "Socket Error " + QString::number(socketError);
            }
        });
        connect(c, static_cast<void(QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
            [=](const QList<QSslError> &errors){
            for (QSslError error : errors) {
                qDebug() << "SSL Error " + error.errorString();
            }
        });

        qDebug() << ssl.key.isNull();
        qDebug() << ssl.cert.isNull();

        c->setPrivateKey(ssl.key);
        c->setLocalCertificate(ssl.cert);
        c->setPeerVerifyMode(QSslSocket::VerifyNone);

        c->startServerEncryption();

        emit connectionAvailable(c);
    }
}
