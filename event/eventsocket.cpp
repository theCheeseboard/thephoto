#include "eventsocket.h"
#include <QDebug>

EventSocket::EventSocket(QObject *parent) : QSslSocket(parent)
{
    connect(this, SIGNAL(readyRead()), this, SLOT(newData()));
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
            //Write out to file
            QDir::home().mkdir("thePhoto");
            QFile image(QDir::homePath() + "/thePhoto/Image_" + QDateTime::currentDateTime().toString("ddMMyy-hhmmsszzz") + ".jpg");
            image.open(QFile::ReadWrite);
            image.write(currentImageData);
            image.flush();
            image.close();


            QImageReader reader(image.fileName());
            reader.setAutoTransform(true);
            QImage i = reader.read();

            if (i.isNull()) {
                //ui->imageLabel->setText("Unfortunately, that image didn't send correctly.");
            } else {
                //ui->imageLabel->setPixmap(QPixmap::fromImage(image));
                //setScaleFactor(calculateScaling(this->size(), image.size()));
                emit newImageAvailable(i);
            }

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
    //QByteArray encrypted = cipher->encrypted(data);

    //sock->write(encrypted);
    write(data);
}
