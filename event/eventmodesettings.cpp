#include "eventmodesettings.h"
#include "ui_eventmodesettings.h"

#include <QRandomGenerator>
#include <QPainter>
#include <QMessageBox>
#include <QMenu>
#include <QMenuBar>
#include <QStackedWidget>
#include <QScroller>
#include <QScrollBar>
#include "eventmodeuserindicator.h"
#include "ttoast.h"

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

    //Set up macOS menu bar
    QMenuBar* menuBar = new QMenuBar(this);

    saveDir.setPath(QDir::homePath() + "/Pictures/thePhoto");

    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless", QIcon(":/icons/network-wireless.svg")).pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on", QIcon(":/icons/password-show-on.svg")).pixmap(16, 16));
    ui->openMissionControl->setIcon(QIcon("/Applications/Mission Control.app/Contents/Resources/Expose.icns"));
    ui->monitorNumber->setMaximum(QApplication::desktop()->screenCount());

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

    connect(ui->showVignette, SIGNAL(toggled(bool)), this, SLOT(configureVignette()));
    connect(ui->showAudio, SIGNAL(toggled(bool)), this, SLOT(configureVignette()));
    connect(ui->showAuthor, SIGNAL(toggled(bool)), this, SLOT(configureVignette()));
    connect(ui->showClock, SIGNAL(toggled(bool)), this, SLOT(configureVignette()));

    imagesReceivedLayout = new FlowLayout(ui->imagesReceivedWidget, -1, 0, 0);
    imagesReceivedLayout->setContentsMargins(0, 0, 0, 0);
    //imagesReceivedLayout->s
    ui->imagesReceivedWidget->setLayout(imagesReceivedLayout);

    QScroller::grabGesture(ui->imagesReceivedScrollArea->viewport(), QScroller::LeftMouseButtonGesture);
}

EventModeSettings::~EventModeSettings()
{
    showDialog->deleteLater();
    delete ui;
}

void EventModeSettings::show() {
    if (QApplication::screens().count() == 1 && QSysInfo::productType() != "osx") {
        ui->displaysSettingStack->setCurrentIndex(2); //Only one display
    } else {
        if (QSysInfo::productType() == "osx") {
            ui->displaysSettingStack->setCurrentIndex(1); //Mission Control
        } else {
            if (QApplication::screens().count() == 2) {
                ui->displaysSettingStack->setCurrentIndex(3); //Swap Displays
            } else {
                ui->displaysSettingStack->setCurrentIndex(0); //Spin Box
            }
        }

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

    connect(sock, &EventSocket::imageDataAvailable, [=](QByteArray imageData) {
        //Write out to file
        QDir endDir = saveDir;
        if (ui->sessionNameLineEdit->text() == "") {
            endDir.setPath(endDir.absoluteFilePath(tr("Uncategorized")));
        } else {
            endDir.setPath(endDir.absoluteFilePath(ui->sessionNameLineEdit->text()));
        }

        if (ui->storeUserSubfolders->isChecked()) {
            endDir.setPath(endDir.absoluteFilePath(sock->deviceName()));
        }

        if (!endDir.exists()) {
            endDir.mkpath(".");
        }

        QString fileName = "Image_" + QDateTime::currentDateTime().toString("yyMMdd-hhmmsszzz") + ".jpg";
        QFile image(endDir.absoluteFilePath(fileName));
        image.open(QFile::ReadWrite);
        image.write(imageData);
        image.flush();
        image.close();

        QImageReader reader(image.fileName());
        reader.setAutoTransform(true);
        QImage i = reader.read();

        if (i.isNull()) {

        } else {
            QString author = sock->deviceName();

            EventModeShow::ImageProperties imageProperties;
            imageProperties.image = QPixmap::fromImage(i);
            imageProperties.author = author;
            showDialog->showNewImage(imageProperties);

            //Add to transferred images list
            QLabel* label = new QLabel();
            label->setPixmap(QPixmap::fromImage(i).scaledToHeight(imageHeight));
            label->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &EventModeSettings::windowSizeChanged, [=] {
                label->setPixmap(QPixmap::fromImage(i).scaledToHeight(imageHeight));
            });
            connect(label, &QLabel::customContextMenuRequested, [=](QPoint pos) {
                QMenu* menu = new QMenu();
                menu->addSection(tr("About this picture"));

                QAction* authorAction = new QAction();
                authorAction->setText(tr("By %1").arg(author));
                authorAction->setIcon(QIcon::fromTheme("user"));
                authorAction->setEnabled(false);
                menu->addAction(authorAction);

                menu->addSection(tr("For this picture"));
                menu->addAction(QIcon::fromTheme("media-playback-start"), tr("Enqueue for show"), [=] {
                    showDialog->showNewImage(imageProperties);
                });
                menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), [=] {
                    label->setVisible(false);

                    tToast* toast = new tToast();
                    toast->setTitle(tr("Image Deleted"));
                    toast->setText(tr("The image was deleted from your computer."));

                    QMap<QString, QString> actions;
                    actions.insert("undo", "Undo");
                    toast->setActions(actions);
                    toast->show(ui->imagesReceivedScrollArea);
                    connect(toast, &tToast::doDefaultOption, [=] {
                        QFile::remove(endDir.absoluteFilePath(fileName));
                        label->deleteLater();
                    });
                    connect(toast, &tToast::dismiss, [=] {
                        toast->deleteLater();
                    });
                    connect(toast, &tToast::actionClicked, [=] {
                        label->setVisible(true);
                    });
                });
                menu->exec(label->mapToGlobal(pos));
            });
            imagesReceivedLayout->addWidget(label);
        }
    });
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

void EventModeSettings::on_openMissionControl_clicked()
{
    QProcess::startDetached("open -a \"Mission Control\"");
}

void EventModeSettings::on_swapDisplayButton_clicked()
{
    //Swap the displays
    if (ui->monitorNumber->value() == 1) {
        ui->monitorNumber->setValue(2);
    } else {
        ui->monitorNumber->setValue(1);
    }
}

void EventModeSettings::configureVignette() {
    showDialog->configureVignette(ui->showVignette->isChecked(), ui->showClock->isChecked(), ui->showAuthor->isChecked(), ui->showAudio->isChecked());

    if (ui->showVignette->isChecked()) {
        ui->showAudio->setEnabled(true);
        ui->showAuthor->setEnabled(true);
        ui->showClock->setEnabled(true);
    } else {
        ui->showAudio->setEnabled(false);
        ui->showAuthor->setEnabled(false);
        ui->showClock->setEnabled(false);
    }
}

void EventModeSettings::resizeEvent(QResizeEvent* event) {
    //Calculate height of transferred image
    //Assuming we want to fit about 10 4:3 images horizontally
    QSize s(20, 3);
    s.scale(this->width() - 1, this->height(), Qt::KeepAspectRatio);
    imageHeight = s.height();

    emit windowSizeChanged();
}
