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

#ifdef Q_OS_LINUX
    #include "musicprovider/musicprovider_linux.h"
#elif defined(Q_OS_MAC)
    #include "musicprovider/musicprovider_mac.h"
#elif defined(Q_OS_WIN)
    #include "musicprovider/musicprovider_win.h"
#endif

namespace Ui {
    class EventModeShow;
}

class EventModeShow : public QDialog
{
        Q_OBJECT

    public:
        explicit EventModeShow(QWidget *parent = nullptr);
        ~EventModeShow();

        struct ImageProperties {
            QPixmap image;
            QString author;
        };

    public slots:
        void setCode(QString code);
        void updateInternetDetails(QString ssid, QString password, bool show);
        void showFullScreen(int monitor);
        void showNewImage(ImageProperties image);
        void addToProfileLayout(QWidget* widget);
        int getProfileLayoutHeight();
        void showError(QString error);
        void updateRedactor();
        void configureVignette(bool showVignette, bool showClock, bool showAuthor, bool showAudio);

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

        ImageProperties px;
        QPixmap blurred;
        QStack<ImageProperties> pendingImages;
        bool blockingOnNewImage = false;

        MusicProvider* musicProvider;
        bool showVignette = true, showClock = true, showAuthor = true, showAudio = true;

        QWidget* scrollRedactor;


        tVariantAnimation *animationTimerPx, *animationTimerPxOpacity, *animationTimerBlur;
};

#endif // EVENTMODESHOW_H
