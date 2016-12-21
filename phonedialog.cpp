#include "phonedialog.h"
#include "ui_phonedialog.h"

PhoneDialog::PhoneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PhoneDialog)
{
    ui->setupUi(this);

    ui->scrollArea->setVisible(false);
    if (QNetworkInterface::allAddresses().count() < 3) {
        ui->ipLabel->setText("Connect to the internet.");
    } else {
        ui->ipLabel->setText(QNetworkInterface::allAddresses().at(2).toString());

        server = new QTcpServer(this);
        server->listen(QHostAddress::Any, 26157);
        connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
    }
}

PhoneDialog::~PhoneDialog()
{
    delete ui;
}

void PhoneDialog::keyPressEvent(QKeyEvent *event) {
    if (event->key() == Qt::Key_Escape) {
        this->close();
    }
}

void PhoneDialog::resizeEvent(QResizeEvent *event) {

}

void PhoneDialog::newConnection() {
    QTcpSocket* socket = server->nextPendingConnection();
    //server->close();

    sockets.append(socket);
    buffers.append(QByteArray());
    connect(socket, &QTcpSocket::readyRead, [=]() {
        int index = sockets.indexOf(socket);
        QByteArray buffer = buffers.at(sockets.indexOf(socket));
        buffer.append(socket->readAll());
        if (buffer.endsWith("\r\n")) {
            if (buffer.endsWith("ENDSTREAM\r\n")) {
                buffers.removeAt(sockets.indexOf(socket));
                sockets.removeOne(socket);
                broadcastNumber();
                if (sockets.count() == 0) {
                    this->close();
                }
                return;
            } else {
                //Reply with OK
                socket->write("OK\r\n");
                socket->flush();

                ui->scrollArea->setVisible(true);
                ui->connectionFrame->setVisible(false);
                QDir::home().mkdir("thePhoto");
                QFile image(QDir::homePath() + "/thePhoto/Image_" + QDateTime::currentDateTime().toString("ddMMyy-hhmmsszzz") + ".jpg");
                image.open(QFile::ReadWrite);
                image.write(buffer);
                image.flush();
                image.close();

                loadImage(image.fileName());

                buffer.clear();
            }
        }
        buffers.replace(index, buffer);
    });

    broadcastNumber();

    ui->ipLabel->setVisible(false);
    ui->connectionStatus->setVisible(false);
    ui->trademarkLabel->setVisible(false);
    ui->statusLabel->setText("Your phone is now connected to this PC. You can start taking images now.");
}

void PhoneDialog::broadcastNumber() {
    for (QTcpSocket* socket : sockets) {
        socket->write(QString("CONNECTIONS " + QString::number(sockets.count()) + "\r\n").toUtf8());
        socket->flush();
    }
}

void PhoneDialog::loadImage(QString path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (image.isNull()) {
        ui->imageLabel->setText("Unfortunately, that image didn't send correctly.");
    } else {
        ui->imageLabel->setPixmap(QPixmap::fromImage(image));
        setScaleFactor(calculateScaling(this->size(), image.size()));
    }
}

float PhoneDialog::calculateScaling(QSize container, QSize image, bool allowLarger) {
    return calculateScaling(container.width(), container.height(), image.width(), image.height(), allowLarger);
}

float PhoneDialog::calculateScaling(int containerWidth, int containerHeight, int imageWidth, int imageHeight, bool allowLarger) {
    float heightFactor = 0, widthFactor = 0;
    if (imageHeight > containerHeight || allowLarger) {
        heightFactor = (float) containerHeight / (float) imageHeight;
    }

    if (imageWidth > containerWidth || allowLarger) {
        widthFactor = (float) containerWidth / (float) containerHeight;
    }

    if (heightFactor == 0 && widthFactor == 0) {
        return 1;
    } else if (heightFactor == 0 && widthFactor != 0) {
        return widthFactor;
    } else if (heightFactor != 0 && widthFactor == 0) {
        return heightFactor;
    } else if (heightFactor < widthFactor) {
        return heightFactor;
    } else {
        return widthFactor;
    }
}

void PhoneDialog::setScaleFactor(float factor) {
    scaleFactor = factor;
    ui->imageLabel->resize(scaleFactor * ui->imageLabel->pixmap()->size());
    recenterImage();
}

void PhoneDialog::recenterImage() {
    int newx = 0, newy = 0;
    if (ui->imageLabel->width() < ui->scrollArea->width()) {
        newx = ui->scrollArea->width() / 2 - ui->imageLabel->width() / 2;
    }

    if (ui->imageLabel->height() < ui->scrollArea->height()) {
        newy = ui->scrollArea->height() / 2 - ui->imageLabel->height() / 2;
    }

    ui->imageLabel->move(newx, newy);
}

void PhoneDialog::close() {
    QDialog::close();

    for (QTcpSocket* socket : sockets) {
        socket->write(QString("ENDSTREAM\r\n").toUtf8());
        socket->flush();
    }

    this->deleteLater();
}
