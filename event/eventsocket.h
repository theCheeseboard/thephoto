#ifndef EVENTSOCKET_H
#define EVENTSOCKET_H

#include <QTcpSocket>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QDateTime>
#include <QSslSocket>
#include <QTimer>

class EventSocket : public QSslSocket
{
        Q_OBJECT
    public:
        explicit EventSocket(QObject *parent = nullptr);
        QString deviceName();

    signals:
        void newImageAvailable(QImage image);
        void newUserConnected(QString name);

    public slots:
        void newData();
        void writeString(QString string);
        void readBuffer();
        void ping();
        void closeFromClient();

    private:
        unsigned char pendingRead;
        bool authenticated = false;
        bool imageDataReading = false;
        qulonglong imageDataReadingLength = 0;

        QByteArray buffer;
        QByteArray encrypted;
        QByteArray currentImageData;
        QString name = tr("An unknown person");

        QTimer* pingTimer;
        QDateTime lastPingSent, lastPingReceived;
};

#endif // EVENTSOCKET_H
