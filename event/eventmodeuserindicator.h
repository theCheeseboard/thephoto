#ifndef EVENTMODEUSERINDICATOR_H
#define EVENTMODEUSERINDICATOR_H

#include <QWidget>
#include "ws/wseventsocket.h"

namespace Ui {
    class EventModeUserIndicator;
}

struct EventModeUserIndicatorPrivate;
class EventModeUserIndicator : public QWidget {
        Q_OBJECT

    public:
        explicit EventModeUserIndicator(WsEventSocket* sock, quint64 userId, QWidget* parent = nullptr);
        ~EventModeUserIndicator();

    private:
        Ui::EventModeUserIndicator* ui;
        EventModeUserIndicatorPrivate* d;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
};

#endif // EVENTMODEUSERINDICATOR_H
