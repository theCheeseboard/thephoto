#ifndef EVENTMODESETTINGS_H
#define EVENTMODESETTINGS_H

#include <QDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <QHostAddress>
#include <QRandomGenerator>
#include <QNetworkInterface>
#include <QSpinBox>
#include "eventserver.h"
#include "eventsocket.h"
#include "eventmodeshow.h"
#include "eventnotification.h"

namespace Ui {
    class EventModeSettings;
}

class EventModeSettings : public QDialog
{
        Q_OBJECT

    public:
        explicit EventModeSettings(QWidget *parent = 0);
        ~EventModeSettings();

    public slots:
        void show();

        void newConnection(EventSocket* socket);

        void reject();

    private slots:
        void on_closeEventModeButton_clicked();

        void on_showWifiDetails_toggled(bool checked);

        void on_ssid_textChanged(const QString &arg1);

        void on_key_textChanged(const QString &arg1);

        void on_monitorNumber_valueChanged(int arg1);

        void on_exchangedImagesButton_toggled(bool checked);

        void on_connectedUsersButton_toggled(bool checked);

        void on_sessionSettingsButton_toggled(bool checked);

    private:
        Ui::EventModeSettings *ui;

        EventModeShow* showDialog;
        int encryptionCode;

        EventServer* server;
        QList<EventSocket*> sockets;
};

#endif // EVENTMODESETTINGS_H
