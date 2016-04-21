#include "phonedialog.h"
#include "ui_phonedialog.h"

PhoneDialog::PhoneDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::PhoneDialog)
{
    ui->setupUi(this);

    ui->ipLabel->setText(QNetworkInterface::allAddresses().at(2).toString());
    ui->scrollArea->setVisible(false);

    server = new QTcpServer(this);
    server->listen(QHostAddress::Any, 26157);
    connect(server, SIGNAL(newConnection()), this, SLOT(newConnection()));
}

PhoneDialog::~PhoneDialog()
{
    delete ui;
}

bool PhoneDialog::eventFilter(QObject *, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = (QKeyEvent*) event;
        if (keyEvent->key() == Qt::Key_Escape) {
            this->close();
        }
    }
    return false;
}

void PhoneDialog::newConnection() {
    socket = server->nextPendingConnection();
    server->close();

    connect(socket, &QTcpSocket::readyRead, [=]() {
        buffer.append(socket->readAll());
        if (buffer.endsWith("\r\n")) {
            ui->scrollArea->setVisible(true);
            ui->connectionFrame->setVisible(false);
            //buffer.remove(buffer.length() - 3, 2);
            QDir::home().mkdir("thePhoto");
            QFile image(QDir::homePath() + "/thePhoto/Image_" + QDateTime::currentDateTime().toString("ddMMyy-hhmmss") + ".jpg");
            image.open(QFile::ReadWrite);
            image.write(buffer);
            image.flush();
            image.close();

            loadImage(image.fileName());

            buffer.clear();
        }
    });

    ui->ipLabel->setVisible(false);
    ui->connectionStatus->setVisible(false);
    ui->trademarkLabel->setVisible(false);
    ui->statusLabel->setText("Your phone is now connected to this PC. You can start taking images now.");
}

void PhoneDialog::loadImage(QString path) {
    QImageReader reader(path);
    reader.setAutoTransform(true);
    QImage image = reader.read();

    if (image.isNull()) {
        ui->imageLabel->setText("Unfortunately, that image didn't send correctly.");
    } else {
        ui->imageLabel->setPixmap(QPixmap::fromImage(image));
        setScaleFactor(calculateScaling(ui->scrollArea->size(), image.size()));
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
