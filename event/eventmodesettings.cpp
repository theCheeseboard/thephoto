#include "eventmodesettings.h"
#include "ui_eventmodesettings.h"

#include <QRandomGenerator>
#include <QPainter>
#include <QMessageBox>

EventModeSettings::EventModeSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeSettings)
{
    ui->setupUi(this);

    showDialog = new EventModeShow();
    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless", QIcon(":/icons/network-wireless.svg")).pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on", QIcon(":/icons/password-show-on.svg")).pixmap(16, 16));
    ui->monitorNumber->setMaximum(QApplication::desktop()->screenCount());

    if (QNetworkInterface::allAddresses().count() < 3) {
        showDialog->setCode(tr("Error"));
    } else {
        QHostAddress addr = QNetworkInterface::allAddresses().at(2);

        for (QHostAddress addr : QNetworkInterface::allAddresses()) {
            if (addr.isLoopback()) continue;
            if (addr.isLinkLocal()) continue;

            QString code = QString("%1").arg(addr.toIPv4Address(), 10, 10, QChar('0'));
            code.insert(5, " ");

            server = new EventServer(this);
            server->listen(QHostAddress::Any, 26157);
            connect(server, SIGNAL(connectionAvailable(EventSocket*)), this, SLOT(newConnection(EventSocket*)));
            connect(server, &EventServer::ready, [=] {
                showDialog->setCode(code);
            });
            connect(server, &EventServer::error, [=](QString error) {
                showDialog->showError(error);
            });
        }
    }
}

EventModeSettings::~EventModeSettings()
{
    showDialog->deleteLater();
    delete ui;
}

void EventModeSettings::show() {
    showDialog->showFullScreen(1);

    QDialog::showFullScreen();
    this->setGeometry(QApplication::desktop()->screenGeometry(0));

    //this->move(QApplication::desktop()->screenGeometry(0).center());
}

void EventModeSettings::on_closeEventModeButton_clicked()
{
    if (QMessageBox::question(this, tr("End Event Mode?"), tr("Close connections to all connected devices and end Event Mode?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        this->reject();
    }
}

void EventModeSettings::on_showWifiDetails_toggled(bool checked)
{
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_ssid_textChanged(const QString &arg1)
{
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_key_textChanged(const QString &arg1)
{
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_monitorNumber_valueChanged(int arg1)
{
    int monitor = arg1 - 1;
    showDialog->showFullScreen(monitor);

    QDialog::showFullScreen();

    if (QApplication::desktop()->screenNumber(this->geometry().center()) == monitor) {
        if (monitor == 0) {
            this->setGeometry(QApplication::desktop()->screenGeometry(1));
        } else {
            this->setGeometry(QApplication::desktop()->screenGeometry(0));
        }
    }
}

void EventModeSettings::newConnection(EventSocket* sock) {
    QLabel* userLayout = new QLabel;

    connect(sock, SIGNAL(newImageAvailable(QImage)), showDialog, SLOT(showNewImage(QImage)));
    connect(sock, &EventSocket::newUserConnected, [=](QString name) {
        new EventNotification(tr("User Connected"), name, showDialog);

        //Generate a new square based on the name
        QPixmap px(showDialog->getProfileLayoutHeight(), showDialog->getProfileLayoutHeight());
        QPainter painter(&px);

        //Set the background to a random colour
        QColor backgroundCol = QColor::fromRgb(QRandomGenerator::global()->generate());
        painter.setBrush(backgroundCol);
        painter.setPen(Qt::transparent);
        painter.drawRect(0, 0, px.width(), px.height());

        if ((backgroundCol.red() + backgroundCol.green() + backgroundCol.blue()) / 3 > 127) {
            painter.setPen(Qt::black);
        } else {
            painter.setPen(Qt::white);
        }

        QString nameCharacter(name.at(0).toUpper());

        QFont f;
        f.setFamily(this->font().family());
        f.setPointSize(1);
        while (QFontMetrics(f).height() < (float) px.height() * (float) 0.75) {
            f.setPointSize(f.pointSize() + 1);
        }
        f.setPointSize(f.pointSize() - 1);
        painter.setFont(f);

        QFontMetrics metrics(f);
        int charWidth = metrics.width(nameCharacter);
        int charHeight = metrics.height();

        QRect textRect;
        textRect.setWidth(charWidth);
        textRect.setHeight(charHeight);
        textRect.moveTop(px.height() / 2 - charHeight / 2);
        textRect.moveLeft(px.width() / 2 - charWidth / 2);
        painter.drawText(textRect, nameCharacter);
        painter.end();

        userLayout->setPixmap(px);
        showDialog->addToProfileLayout(userLayout);
    });
    connect(sock, &EventSocket::aboutToClose, [=] {
        new EventNotification(tr("User Disconnected"), sock->deviceName(), showDialog);
        sockets.removeOne(sock);
        userLayout->deleteLater();
    });
    sockets.append(sock);
}

void EventModeSettings::reject() {
    showDialog->close();
    for (EventSocket* sock : sockets) {
        //Close all sockets
        sock->closeFromClient();
    }

    QDialog::reject();
}

void EventModeSettings::on_exchangedImagesButton_toggled(bool checked)
{
    if (checked) {
        ui->mainStack->setCurrentIndex(2);
    }
}

void EventModeSettings::on_connectedUsersButton_toggled(bool checked)
{
    if (checked) {
        ui->mainStack->setCurrentIndex(1);
    }
}

void EventModeSettings::on_sessionSettingsButton_toggled(bool checked)
{
    if (checked) {
        ui->mainStack->setCurrentIndex(0);
    }
}
