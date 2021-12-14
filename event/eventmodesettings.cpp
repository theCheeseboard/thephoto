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
#include <QScreen>
#include "ws/wseventserver.h"
#include "ws/wsrendezvousserver.h"
#include "eventmodeuserindicator.h"
#include "ttoast.h"

struct EventModeSettingsPrivate {
    QHash<quint64, QListWidgetItem*> userItems;
};

EventModeSettings::EventModeSettings(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::EventModeSettings) {
    ui->setupUi(this);

    d = new EventModeSettingsPrivate();

    showDialog = new EventModeShow();
    connect(showDialog, &EventModeShow::returnToBackstage, this, [ = ] {
        showDialog->hide();
        this->show();
    });

    //Set up macOS menu bar
    new QMenuBar(this);

#ifdef T_BLUEPRINT_BUILD
    ui->menuButton->setIcon(QIcon(":/icons/com.vicr123.thephoto_blueprint.svg"));
#else
    ui->menuButton->setIcon(QIcon::fromTheme("com.vicr123.thephoto", QIcon(":/icons/com.vicr123.thephoto.svg")));
#endif
    ui->menuButton->setIconSize(SC_DPI_T(QSize(24, 24), QSize));
//    ui->menuButton->setMenu(menu);

    saveDir.setPath(QDir::homePath() + "/Pictures/thePhoto");

    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless", QIcon(":/icons/network-wireless.svg")).pixmap(SC_DPI_T(QSize(16, 16), QSize)));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on", QIcon(":/icons/password-show-on.svg")).pixmap(SC_DPI_T(QSize(16, 16), QSize)));
    ui->openMissionControl->setIcon(QIcon("/Applications/Mission Control.app/Contents/Resources/Expose.icns"));
    ui->monitorNumber->setMaximum(QApplication::screens().count());
    ui->mainStack->setCurrentAnimation(tStackedWidget::SlideHorizontal);
    ui->closeEventModeButton->setProperty("type", "destructive");

    WsEventServer* wsServer = new WsEventServer(this);
    connect(wsServer, &WsEventServer::newSocketAvailable, this, &EventModeSettings::newSocketAvailable);

    WsRendezvousServer* wsRendezvousServer = new WsRendezvousServer(this);
    connect(wsRendezvousServer, &WsRendezvousServer::newSocketAvailable, this, &EventModeSettings::newSocketAvailable);
    connect(wsRendezvousServer, &WsRendezvousServer::connectionStateChanged, this, [ = ](WsRendezvousServer::ConnectionState state) {
        switch (state) {
            case WsRendezvousServer::SettingUp:
                showDialog->connecting();
                break;
            case WsRendezvousServer::Connected:
                showDialog->ready();
                break;
            case WsRendezvousServer::Error:
                showDialog->showError(tr("An error has occurred. View more details in the Backstage."));
                break;
        }
    });
    connect(wsRendezvousServer, &WsRendezvousServer::newServerIdAvailable, this, [ = ](int newServerId, QString hmac) {
        ui->roomInfoLabel->setText(tr("Your room code is %1.").arg(QStringLiteral("<b>%1 %2</b>").arg(newServerId, 4, 10, QLatin1Char('0')).arg(hmac)));
        showDialog->setCode(QStringLiteral("%1 %2").arg(newServerId, 4, 10, QLatin1Char('0')).arg(hmac));
    });
    ui->roomInfoLabel->setText(tr("We're preparing a room for you. Hang tight!"));

    connect(ui->showVignette, &QAbstractButton::toggled, this, &EventModeSettings::configureVignette);
    connect(ui->showAudio, &QAbstractButton::toggled, this, &EventModeSettings::configureVignette);
    connect(ui->showAuthor, &QAbstractButton::toggled, this, &EventModeSettings::configureVignette);
    connect(ui->showClock, &QAbstractButton::toggled, this, &EventModeSettings::configureVignette);

    imagesReceivedLayout = new FlowLayout(ui->imagesReceivedWidget, -1, 0, 0);
    imagesReceivedLayout->setContentsMargins(0, 0, 0, 0);
    ui->imagesReceivedWidget->setLayout(imagesReceivedLayout);

    QScroller::grabGesture(ui->imagesReceivedScrollArea->viewport(), QScroller::LeftMouseButtonGesture);
}

EventModeSettings::~EventModeSettings() {
    showDialog->deleteLater();
    delete d;
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
    this->setGeometry(QApplication::screens().first()->geometry());

    //this->move(QApplication::desktop()->screenGeometry(0).center());
}

void EventModeSettings::on_closeEventModeButton_clicked() {
    if (QMessageBox::question(this, tr("End Event Mode?"), tr("Close connections to all connected devices and end Event Mode?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        this->reject();
    }
}

void EventModeSettings::on_showWifiDetails_toggled(bool checked) {
    Q_UNUSED(checked)
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_ssid_textChanged(const QString& arg1) {
    Q_UNUSED(arg1)
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_key_textChanged(const QString& arg1) {
    Q_UNUSED(arg1)
    showDialog->updateInternetDetails(ui->ssid->text(), ui->key->text(), ui->showWifiDetails->isChecked());
}

void EventModeSettings::on_monitorNumber_valueChanged(int arg1) {
    if (QApplication::screens().count() != 1) {
#ifdef Q_OS_LINUX
        this->showNormal();
#endif

        int monitor = arg1 - 1;
        showDialog->showFullScreen(monitor);

        if (QApplication::screens().indexOf(QApplication::screenAt(this->geometry().center())) == monitor) {
            if (monitor == 0) {
                this->setGeometry(QApplication::screens().at(1)->geometry());
            } else {
                this->setGeometry(QApplication::screens().at(0)->geometry());
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

//    QLabel* userLayout = new QLabel;

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

void EventModeSettings::on_exchangedImagesButton_toggled(bool checked) {
    if (checked) {
        ui->mainStack->setCurrentIndex(2);
    }
}

void EventModeSettings::on_connectedUsersButton_toggled(bool checked) {
    if (checked) {
        ui->mainStack->setCurrentIndex(1);
    }
}

void EventModeSettings::on_sessionSettingsButton_toggled(bool checked) {
    if (checked) {
        ui->mainStack->setCurrentIndex(0);
    }
}

void EventModeSettings::on_usersList_customContextMenuRequested(const QPoint& pos) {
    QMenu* menu = new QMenu();
    if (ui->usersList->selectedItems().count() == 1) {
        QListWidgetItem* selected = ui->usersList->selectedItems().first();
        menu->addSection(tr("For %1").arg(selected->text()));
        menu->addAction(tr("Kick"), [ = ] {
            if (QMessageBox::question(this, tr("Kick?"), tr("Kick %1? They'll be able to rejoin the session by entering the session code again.").arg(selected->text()), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
                EventSocket* sock = selected->data(Qt::UserRole).value<EventSocket*>();
                sock->closeFromClient();
            }
        });
        menu->addAction(tr("Ban"), [ = ] {
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

void EventModeSettings::on_backToEventModeButton_clicked() {
    this->hide();
    showDialog->showFullScreen(0);

    new EventNotification(tr("Welcome to Event Mode!"), tr("To get back to the Backstage, simply hit the TAB key."), showDialog);
}

void EventModeSettings::on_openMissionControl_clicked() {
    QProcess::startDetached("open", {"-a", "Mission Control"});
}

void EventModeSettings::on_swapDisplayButton_clicked() {
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

void EventModeSettings::newSocketAvailable(WsEventSocket* socket) {
    connect(socket, &WsEventSocket::userLogin, this, [ = ](quint64 userId, QString username) {
        new EventNotification(tr("User Connected"), username, showDialog);

        QListWidgetItem* userEntry = new QListWidgetItem();
        userEntry->setData(Qt::UserRole, userId);
        userEntry->setText(username);
        ui->usersList->addItem(userEntry);
        d->userItems.insert(userId, userEntry);

        EventModeUserIndicator* userIndicator = new EventModeUserIndicator(socket, userId);
        showDialog->addToProfileLayout(userIndicator);
    });
    connect(socket, &WsEventSocket::userGone, this, [ = ](quint64 userId) {
        new EventNotification(tr("User Disconnected"), socket->username(userId), showDialog);

        QListWidgetItem* item = d->userItems.take(userId);
        delete item;
    });
    connect(socket, &WsEventSocket::pictureReceived, this, [ = ](quint64 userId, QByteArray pictureData) {
        //Write out to file
        QDir endDir = saveDir;
        if (ui->sessionNameLineEdit->text() == "") {
            endDir.setPath(endDir.absoluteFilePath(tr("Uncategorized")));
        } else {
            endDir.setPath(endDir.absoluteFilePath(ui->sessionNameLineEdit->text()));
        }

        if (ui->storeUserSubfolders->isChecked()) {
            endDir.setPath(endDir.absoluteFilePath(socket->username(userId)));
        }

        if (!endDir.exists()) {
            endDir.mkpath(".");
        }

        QString fileName = "Image_" + QDateTime::currentDateTime().toString("yyMMdd-hhmmsszzz") + ".jpg";
        QFile image(endDir.absoluteFilePath(fileName));
        image.open(QFile::ReadWrite);
        image.write(pictureData);
        image.flush();
        image.close();

        QImageReader reader(image.fileName());
        reader.setAutoTransform(true);
        QImage i = reader.read();

        if (i.isNull()) {

        } else {
            QString author = socket->username(userId);

            EventModeShow::ImageProperties imageProperties;
            imageProperties.image = QPixmap::fromImage(i);
            imageProperties.author = author;
            showDialog->showNewImage(imageProperties);

            //Add to transferred images list
            QLabel* label = new QLabel();
            label->setPixmap(QPixmap::fromImage(i).scaledToHeight(imageHeight));
            label->setContextMenuPolicy(Qt::CustomContextMenu);
            connect(this, &EventModeSettings::windowSizeChanged, [ = ] {
                label->setPixmap(QPixmap::fromImage(i).scaledToHeight(imageHeight));
            });
            connect(label, &QLabel::customContextMenuRequested, [ = ](QPoint pos) {
                QMenu* menu = new QMenu();
                menu->addSection(tr("About this picture"));

                QAction* authorAction = new QAction();
                authorAction->setText(tr("By %1").arg(author));
                authorAction->setIcon(QIcon::fromTheme("user"));
                authorAction->setEnabled(false);
                menu->addAction(authorAction);

                menu->addSection(tr("For this picture"));
                menu->addAction(QIcon::fromTheme("media-playback-start"), tr("Enqueue for show"), [ = ] {
                    showDialog->showNewImage(imageProperties);
                });
                menu->addAction(QIcon::fromTheme("edit-delete"), tr("Delete"), [ = ] {
                    label->setVisible(false);

                    tToast* toast = new tToast();
                    toast->setTitle(tr("Image Deleted"));
                    toast->setText(tr("The image was deleted from your computer."));

                    QMap<QString, QString> actions;
                    actions.insert("undo", "Undo");
                    toast->setActions(actions);
                    toast->show(ui->imagesReceivedScrollArea);
                    connect(toast, &tToast::doDefaultOption, [ = ] {
                        QFile::remove(endDir.absoluteFilePath(fileName));
                        label->deleteLater();
                    });
                    connect(toast, &tToast::dismiss, [ = ] {
                        toast->deleteLater();
                    });
                    connect(toast, &tToast::actionClicked, [ = ] {
                        label->setVisible(true);
                    });
                });
                menu->exec(label->mapToGlobal(pos));
            });
            imagesReceivedLayout->addWidget(label);
        }
    });
}

void EventModeSettings::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)

    //Calculate height of transferred image
    //Assuming we want to fit about 10 4:3 images horizontally
    QSize s(20, 3);
    s.scale(this->width() - 1, this->height(), Qt::KeepAspectRatio);
    imageHeight = s.height();

    emit windowSizeChanged();
}
