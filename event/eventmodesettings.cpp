#include "eventmodesettings.h"
#include "ui_eventmodesettings.h"

EventModeSettings::EventModeSettings(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeSettings)
{
    ui->setupUi(this);

    showDialog = new EventModeShow();
    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless").pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on").pixmap(16, 16));
    ui->monitorNumber->setMaximum(QApplication::desktop()->screenCount());

    if (QNetworkInterface::allAddresses().count() < 3) {
        showDialog->setCode(tr("Error"));
    } else {
        QHostAddress addr = QNetworkInterface::allAddresses().at(2);

        QString code = QString("%1").arg(addr.toIPv4Address(), 10, 10, QChar('0'));
        code.insert(5, " ");

        showDialog->setCode(code);
        //ui->ipLabel->setText(addr.toString());

        server = new EventServer(this);
        server->listen(QHostAddress::Any, 26157);
        connect(server, SIGNAL(connectionAvailable(EventSocket*)), this, SLOT(newConnection(EventSocket*)));
    }
}

EventModeSettings::~EventModeSettings()
{
    showDialog->deleteLater();
    delete ui;
}

void EventModeSettings::show() {
    showDialog->showFullScreen(1);
    QDialog::show();

    this->move(QApplication::desktop()->screenGeometry(0).center());
}

void EventModeSettings::on_closeEventModeButton_clicked()
{
    showDialog->close();
    this->close();
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
    QDialog::show();

    if (QApplication::desktop()->screenNumber(this->geometry().topLeft()) == monitor) {
        QRect rect;
        if (monitor == 0) {
            rect = QRect(QApplication::desktop()->screenGeometry(1).center(), this->size());
        } else {
            rect = QRect(QApplication::desktop()->screenGeometry(0).center(), this->size());
        }
        this->setGeometry(rect);
    }
}

void EventModeSettings::newConnection(EventSocket* sock) {
    connect(sock, SIGNAL(newImageAvailable(QImage)), showDialog, SLOT(showNewImage(QImage)));
    sockets.append(sock);
}
