#ifndef EVENTSOCKET_H
#define EVENTSOCKET_H

#include <QTcpSocket>
#include <QBuffer>
#include <QDir>
#include <QFile>
#include <QImageReader>
#include <QDateTime>
#include <QSslSocket>

class EventSocket : public QSslSocket
{
        Q_OBJECT
    public:
        explicit EventSocket(QObject *parent = nullptr);

    signals:
        void newImageAvailable(QImage image);

    public slots:
        void newData();
        void writeString(QString string);
        void readBuffer();

    private:
        unsigned char pendingRead;
        bool authenticated = false;
        bool imageDataReading = false;
        qulonglong imageDataReadingLength = 0;

        QByteArray buffer;
        QByteArray encrypted;
        QByteArray currentImageData;
};

#endif // EVENTSOCKET_H
