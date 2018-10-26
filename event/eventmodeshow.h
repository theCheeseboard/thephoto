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
#include <QStack>
#include <QKeyEvent>
#include <tvariantanimation.h>

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
        void updateRedactor();

    signals:
        void returnToBackstage();

    private:
        Ui::EventModeShow *ui;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
        bool eventFilter(QObject *watched, QEvent *event);
        void keyPressEvent(QKeyEvent* event);

        void updateBlurredImage();
        void tryNewImage();

        QPixmap px;
        QPixmap blurred;
        QStack<QPixmap> pendingImages;
        bool blockingOnNewImage = false;

        QWidget* scrollRedactor;


        tVariantAnimation *animationTimerPx, *animationTimerPxOpacity, *animationTimerBlur;
};

#endif // EVENTMODESHOW_H
