#include "eventserver.h"

#include <tpromise.h>
#include "eventsocket.h"

EventServer::EventServer(QObject *parent) : QTcpServer(parent)
{
    (new tPromise<QStringList>([=](QString& error) {
        //Generate an X509 certificate
        QProcess genProc;

        #ifdef Q_OS_WIN
            QString opensslConfigFile = QApplication::applicationDirPath() + "/openssl.cfg";

            QStringList env = genProc.environment();
            env.append("OPENSSL_CONF=" + opensslConfigFile);
            genProc.setEnvironment(env);
        #endif

        genProc.setWorkingDirectory(certificateDirectory.path());
        genProc.start("openssl req -x509 -newkey rsa:4096 -keyout key.pem -out cert.pem -days 2 -nodes -subj \"/CN=localhost\"");
        genProc.waitForFinished(-1);

        qDebug() << genProc.readAll();

        QFile certFile(certificateDirectory.filePath("cert.pem"));
        QFile keyFile(certificateDirectory.filePath("key.pem"));

        #ifdef Q_OS_MAC
            //Convert to a PKCS1 key that Secure Transport likes
            genProc.start("openssl rsa -in key.pem -out pkcs1key.key");
            genProc.waitForFinished(-1);

            keyFile.setFileName(certificateDirectory.filePath("pkcs1key.key"));
        #endif


        if (!certFile.exists() || !keyFile.exists() || genProc.exitCode() != 0) {
            error = tr("Can't create X509 certificate. Event Mode cannot start.");
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

        emit ready();
    })->error([=](QString error) {
        qDebug() << error;

        emit this->error(error);
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
