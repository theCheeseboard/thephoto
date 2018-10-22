#ifndef EVENTMODESHOW_H
#define EVENTMODESHOW_H

#include <QDialog>
#include <QFrame>
#include <QLabel>
#include <QIcon>
#include <QApplication>
#include <QDesktopWidget>
#include <QTimer>
#include <QEventLoop>

namespace Ui {
    class EventModeShow;
}

class EventModeShow : public QDialog
{
        Q_OBJECT

    public:
        explicit EventModeShow(QWidget *parent = 0);
        ~EventModeShow();

    public slots:
        void setCode(QString code);
        void updateInternetDetails(QString ssid, QString password, bool show);
        void showFullScreen(int monitor);
        void showNewImage(QImage image);
        void addToProfileLayout(QWidget* widget);
        int getProfileLayoutHeight();
        void showError(QString error);

    private:
        Ui::EventModeShow *ui;

        void paintEvent(QPaintEvent* event);
        QPixmap px;
};

#endif // EVENTMODESHOW_H
