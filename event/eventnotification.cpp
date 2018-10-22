#include "eventnotification.h"
#include "ui_eventnotification.h"

#include <tpropertyanimation.h>
#include <QTimer>
#include <QGraphicsOpacityEffect>

QList<EventNotification*> EventNotification::notifications = QList<EventNotification*>();

EventNotification::EventNotification(QString title, QString text, QWidget *parent) :
    QFrame(parent),
    ui(new Ui::EventNotification)
{
    ui->setupUi(this);

    for (EventNotification* n : notifications) {
        n->moveUp();
    }

    this->setParent(parent);
    notifications.append(this);

    connect(this, &EventNotification::destroyed, [=] {
        notifications.removeOne(this);
    });

    QGraphicsOpacityEffect* opacity = new QGraphicsOpacityEffect();
    opacity->setOpacity(0);
    this->setGraphicsEffect(opacity);

    ui->titleLabel->setText(title);
    ui->messageLabel->setText(text);

    this->setFixedHeight(this->sizeHint().height());
    this->setFixedWidth(300 * theLibsGlobal::getDPIScaling());

    this->move(parent->width() - this->width() - 9 * theLibsGlobal::getDPIScaling(), parent->height());
    this->show();

    tPropertyAnimation* showAnim = new tPropertyAnimation(this, "geometry");
    showAnim->setStartValue(this->geometry());
    showAnim->setEndValue(this->geometry().translated(0, -this->height() - 9 * theLibsGlobal::getDPIScaling()));
    showAnim->setDuration(250);
    showAnim->setEasingCurve(QEasingCurve::OutCubic);
    showAnim->start(QAbstractAnimation::DeleteWhenStopped);
    connect(this, SIGNAL(destroyed(QObject*)), showAnim, SLOT(deleteLater()));

    tPropertyAnimation* opacAnim = new tPropertyAnimation(opacity, "opacity");
    opacAnim->setStartValue((float) 0);
    opacAnim->setEndValue((float) 1);
    opacAnim->setDuration(250);
    opacAnim->setEasingCurve(QEasingCurve::OutCubic);
    opacAnim->start(QAbstractAnimation::DeleteWhenStopped);
    connect(this, SIGNAL(destroyed(QObject*)), opacAnim, SLOT(deleteLater()));

    QTimer::singleShot(10000, [=] {
        tPropertyAnimation* opacAnim = new tPropertyAnimation(opacity, "opacity");
        opacAnim->setStartValue((float) 1);
        opacAnim->setEndValue((float) 0);
        opacAnim->setDuration(250);
        opacAnim->setEasingCurve(QEasingCurve::OutCubic);
        opacAnim->start(QAbstractAnimation::DeleteWhenStopped);
        connect(opacAnim, &tPropertyAnimation::finished, [=] {
            this->deleteLater();
        });
        connect(this, SIGNAL(destroyed(QObject*)), opacAnim, SLOT(deleteLater()));
    });
}

EventNotification::~EventNotification()
{
    delete ui;
}

void EventNotification::moveUp() {
    tPropertyAnimation* showAnim = new tPropertyAnimation(this, "geometry");
    showAnim->setStartValue(this->geometry());
    showAnim->setEndValue(this->geometry().translated(0, -this->height() - 9 * theLibsGlobal::getDPIScaling()));
    showAnim->setDuration(250);
    showAnim->setEasingCurve(QEasingCurve::OutCubic);
    showAnim->start(QAbstractAnimation::DeleteWhenStopped);
    connect(this, SIGNAL(destroyed(QObject*)), showAnim, SLOT(deleteLater()));
}
