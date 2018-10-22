#ifndef EVENTSERVER_H
#define EVENTSERVER_H

#include <QObject>
#include <QTcpServer>
#include <QProcess>
#include <QSslCertificate>
#include <QSslKey>
#include "eventsocket.h"

struct SslInformation {
    QSslCertificate cert;
    QSslKey key;
};

class EventServer : public QTcpServer
{
        Q_OBJECT
    public:
        explicit EventServer(QObject *parent = nullptr);

    signals:
        void connectionAvailable(EventSocket* socket);
        void ready();
        void error(QString err);

    public slots:

    private:
        SslInformation ssl;

        void incomingConnection(qintptr handle);
};

#endif // EVENTSERVER_H
