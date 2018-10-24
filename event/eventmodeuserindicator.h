#ifndef EVENTMODEUSERINDICATOR_H
#define EVENTMODEUSERINDICATOR_H

#include <QWidget>
#include <QPaintEvent>
#include <tvariantanimation.h>
#include "eventsocket.h"

namespace Ui {
    class EventModeUserIndicator;
}

class EventModeUserIndicator : public QWidget
{
        Q_OBJECT

    public:
        explicit EventModeUserIndicator(EventSocket* sock, QWidget *parent = nullptr);
        ~EventModeUserIndicator();

    private:
        Ui::EventModeUserIndicator *ui;

        EventSocket* sock;
        QColor backgroundCol;
        QString username;
        int timer = 0;

        tVariantAnimation* usernameOpacity;
        tVariantAnimation* cameraTimerOpacity;
        QRect drawableRect;
        bool deleting = false;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
};

#endif // EVENTMODEUSERINDICATOR_H
