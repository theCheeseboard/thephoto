#ifndef EVENTNOTIFICATION_H
#define EVENTNOTIFICATION_H

#include <QFrame>

namespace Ui {
    class EventNotification;
}

class EventNotification : public QFrame
{
        Q_OBJECT

    public:
        explicit EventNotification(QString title, QString text, QWidget *parent);
        ~EventNotification();

    protected:
        void moveUp();

    private:
        Ui::EventNotification *ui;

        static QList<EventNotification*> notifications;
};

#endif // EVENTNOTIFICATION_H
