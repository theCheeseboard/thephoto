#include "eventmodesettings.h"
#include "ui_eventmodesettings.h"

#include <QRandomGenerator>
#include <QPainter>
#include <QMessageBox>
#include <QMenu>
#include "eventmodeuserindicator.h"

EventModeSettings::EventModeSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeSettings)
{
    ui->setupUi(this);

    showDialog = new EventModeShow();
    connect(showDialog, &EventModeShow::returnToBackstage, [=] {
        showDialog->hide();
        this->show();
    });

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
    if (QApplication::screens().count() == 1) {
        ui->useMonitorLabel->setVisible(false);
        ui->monitorNumber->setVisible(false);
    } else {
        showDialog->showFullScreen(1);
        ui->backToEventModeButton->setVisible(false);
    }

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
    if (QApplication::screens().count() != 1) {
        #ifdef Q_OS_LINUX
            this->showNormal();
        #endif

        int monitor = arg1 - 1;
        showDialog->showFullScreen(monitor);

        if (QApplication::desktop()->screenNumber(this->geometry().center()) == monitor) {
            if (monitor == 0) {
                this->setGeometry(QApplication::desktop()->screenGeometry(1));
            } else {
                this->setGeometry(QApplication::desktop()->screenGeometry(0));
            }
        }

        QDialog::showFullScreen();
    }
}

void EventModeSettings::newConnection(EventSocket* sock) {
    if (bans.contains(sock->peerAddress())) {
        //Banned from session
        sock->closeFromClient();
        return;
    }

    QLabel* userLayout = new QLabel;

    QListWidgetItem* userEntry = new QListWidgetItem();
    userEntry->setData(Qt::UserRole, QVariant::fromValue(sock));
    userEntry->setText(tr("Unidentified User"));
    ui->usersList->addItem(userEntry);

    EventModeUserIndicator* userIndicator = new EventModeUserIndicator(sock);
    showDialog->addToProfileLayout(userIndicator);

    connect(sock, SIGNAL(newImageAvailable(QImage)), showDialog, SLOT(showNewImage(QImage)));
    connect(sock, &EventSocket::newUserConnected, [=](QString name) {
        new EventNotification(tr("User Connected"), name, showDialog);

        userEntry->setText(name);
    });
    connect(sock, &EventSocket::aboutToClose, [=] {
        new EventNotification(tr("User Disconnected"), sock->deviceName(), showDialog);
        sockets.removeOne(sock);
        userLayout->deleteLater();

        delete userEntry;
    });
    sockets.append(sock);
}

void EventModeSettings::reject() {
    for (EventSocket* sock : sockets) {
        //Close all sockets
        sock->closeFromClient();
    }
    showDialog->close();

    QDialog::reject();
    emit done();
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

void EventModeSettings::on_usersList_customContextMenuRequested(const QPoint &pos)
{
    QMenu* menu = new QMenu();
    if (ui->usersList->selectedItems().count() == 1) {
        QListWidgetItem* selected = ui->usersList->selectedItems().first();
        menu->addSection(tr("For %1").arg(selected->text()));
        menu->addAction(tr("Kick"), [=] {
            if (QMessageBox::question(this, tr("Kick?"), tr("Kick %1? They'll be able to rejoin the session by entering the session code again.").arg(selected->text()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                EventSocket* sock = selected->data(Qt::UserRole).value<EventSocket*>();
                sock->closeFromClient();
            }
        });
        menu->addAction(tr("Ban"), [=] {
            if (QMessageBox::question(this, tr("Ban?"), tr("Ban %1? They won't be able to rejoin this session.").arg(selected->text()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                EventSocket* sock = selected->data(Qt::UserRole).value<EventSocket*>();
                bans.append(sock->peerAddress());
                sock->closeFromClient();
            }
        });
    } else if (ui->usersList->selectedItems().count() > 1) {

    }
    menu->exec(ui->usersList->mapToGlobal(pos));
}

void EventModeSettings::on_backToEventModeButton_clicked()
{
    this->hide();
    showDialog->showFullScreen(0);

    new EventNotification(tr("Welcome to Event Mode!"), tr("To get back to the Backstage, simply hit the TAB key."), showDialog);
}
