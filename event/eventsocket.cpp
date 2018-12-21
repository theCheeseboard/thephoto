#include "eventsocket.h"
#include <QDebug>

EventSocket::EventSocket(QObject *parent) : QSslSocket(parent)
{
    connect(this, SIGNAL(readyRead()), this, SLOT(newData()));

    pingTimer = new QTimer();
    pingTimer->setInterval(5000);
    connect(pingTimer, SIGNAL(timeout()), this, SLOT(ping()));

    lastPingSent = QDateTime::fromMSecsSinceEpoch(0);

    connect(this, SIGNAL(aboutToClose()), pingTimer, SLOT(stop()));
}

void EventSocket::newData() {
    buffer.append(readAll());
    readBuffer();
}

void EventSocket::readBuffer() {
    if (imageDataReading) {
        QBuffer b(&buffer);
        b.open(QBuffer::ReadOnly);
        QByteArray imageData = b.read(imageDataReadingLength);
        currentImageData.append(imageData);
        imageDataReadingLength -= imageData.length();
        buffer.remove(0, imageData.length());

        if (imageDataReadingLength == 0) {
            //End of image
            emit imageDataAvailable(currentImageData);

            currentImageData.clear();
            imageDataReading = false;
        }
    } else {
        while (buffer.contains('\n')) {
            int newLine = buffer.indexOf('\n');
            QByteArray lineBytes = buffer.left(newLine);
            buffer.remove(0, lineBytes.length() + 1);
            QString line = lineBytes;

            if (authenticated) {
                //Commands are accepted
                if (line.startsWith("IMAGE START ")) {
                    imageDataReadingLength = line.mid(12).toULongLong();
                    imageDataReading = true;
                    readBuffer();
                    return;
                } else if (line.startsWith("IDENTIFY ")) {
                    if (name == tr("An unknown person")) {
                        name = line.mid(9);
                        emit newUserConnected(name);
                    }
                } else if (line == "REGISTERPING") {
                    //Regsiter for pings
                    pingTimer->start();
                    lastPingReceived = QDateTime::currentDateTimeUtc();

                    //Send a ping immediately
                    ping();
                } else if (line == "PING") {
                    lastPingReceived = QDateTime::currentDateTimeUtc();
                } else if (line == "CLOSE") {
                    //Close the connection
                    close();
                } else if (line.startsWith("TIMER")) {
                    QStringList args = line.split(" ");
                    if (args.count() == 2) {
                        bool ok;
                        int secondsLeft = args.last().toInt(&ok);
                        if (ok) {
                            emit timer(secondsLeft);
                        }
                    }
                }
            } else if (line.trimmed() == "HELLO") {
                authenticated = true;
                writeString("HANDSHAKE OK");
            } else {
                //Handshake didn't work
                //Close connection
                write("HANDSHAKE FAIL\n");
                close();
            }
        }
    }
}

void EventSocket::writeString(QString string) {
    QByteArray data = string.append("\n").toUtf8();
    write(data);
    flush();
}

QString EventSocket::deviceName() {
    return name;
}

void EventSocket::ping() {
    writeString("PING");
    lastPingSent = QDateTime::currentDateTimeUtc();

    if (lastPingSent.toMSecsSinceEpoch() - lastPingReceived.toMSecsSinceEpoch() > 30000) { //30 seconds
        //Close the connection; it's been too long
        closeFromClient();
    }
}

void EventSocket::closeFromClient() {
    //Tell the other side that we're going away
    write("CLOSE");
    close();
}
