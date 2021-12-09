#include "eventmodeuserindicator.h"
#include "ui_eventmodeuserindicator.h"

#include <QRandomGenerator>
#include <QDebug>
#include <QPainter>
#include <tpropertyanimation.h>

struct EventModeUserIndicatorPrivate {
    WsEventSocket* sock;
    quint64 userId;
    QColor backgroundCol;
    QString username;
    int timer = 0;

    tVariantAnimation* usernameOpacity;
    tVariantAnimation* cameraTimerOpacity;
    QRect drawableRect;
    bool deleting = false;
};

EventModeUserIndicator::EventModeUserIndicator(WsEventSocket* sock, quint64 userId, QWidget* parent) :
    QWidget(parent),
    ui(new Ui::EventModeUserIndicator) {
    ui->setupUi(this);

    d = new EventModeUserIndicatorPrivate();

    d->sock = sock;
    d->userId = userId;
    d->username = d->sock->username(userId);
    connect(sock, &WsEventSocket::userGone, [ = ](quint64 userId) {
        if (d->userId != userId) return;
        d->deleting = true;

        QRect newRect = d->drawableRect;
        newRect.moveTop(this->height());

        tVariantAnimation* anim = new tVariantAnimation(this);
        anim->setStartValue(d->drawableRect);
        anim->setEndValue(newRect);
        anim->setEasingCurve(QEasingCurve::InCubic);
        anim->setDuration(500);
        connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
            d->drawableRect = value.toRect();
            this->repaint();
        });
        connect(anim, &tVariantAnimation::finished, [ = ] {
            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            connect(anim, &tVariantAnimation::finished, [ = ] {
                anim->deleteLater();
                this->deleteLater();
            });
            anim->start();
        });
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        anim->start();
    });
    connect(sock, &WsEventSocket::timer, [ = ](quint64 userId, int secondsLeft) {
        if (userId != d->userId) return;
//        qDebug() << "TIMER:" << secondsLeft;

        if (d->timer == 0 && secondsLeft != 0) {
            //Expand

            QFont f;
            f.setFamily(this->font().family());
            f.setPointSize(1);
            while (QFontMetrics(f).height() < (float) this->height() * (float) 0.75) {
                f.setPointSize(f.pointSize() + 1);
            }
            f.setPointSize(f.pointSize() - 1);

            int width = QFontMetrics(f).horizontalAdvance("10") + qMax(this->fontMetrics().horizontalAdvance(tr("Camera Timer")),
                    this->fontMetrics().horizontalAdvance(d->username)) + 27 * theLibsGlobal::getDPIScaling();

            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(width);
            anim->setEasingCurve(QEasingCurve::InOutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            anim->start();

            d->usernameOpacity->setStartValue((float) 1);
            d->usernameOpacity->setEndValue((float) 0);
            d->usernameOpacity->start();

            QMetaObject::Connection* connection = new QMetaObject::Connection;
            *connection = connect(d->usernameOpacity, &tVariantAnimation::finished, [ = ] {
                disconnect(*connection);

                d->cameraTimerOpacity->setStartValue((float) 0);
                d->cameraTimerOpacity->setEndValue((float) 1);
                d->cameraTimerOpacity->start();
            });
        } else if (d->timer != 0 && secondsLeft == 0) {
            //Contract
            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(this->height());
            anim->setEasingCurve(QEasingCurve::InOutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
            anim->start();

            d->cameraTimerOpacity->setStartValue((float) 1);
            d->cameraTimerOpacity->setEndValue((float) 0);
            d->cameraTimerOpacity->start();

            QMetaObject::Connection* connection = new QMetaObject::Connection;
            *connection = connect(d->cameraTimerOpacity, &tVariantAnimation::finished, [ = ] {
                disconnect(*connection);
                d->timer = secondsLeft;


                d->usernameOpacity->setStartValue((float) 0);
                d->usernameOpacity->setEndValue((float) 1);
                d->usernameOpacity->start();
            });

            this->repaint();
            //Return prematurely so that the timer variable doesn't get set
            //otherwise the animation looks strange
            return;
        }

        d->timer = secondsLeft;
        this->repaint();
    });

    //Set the background to a random colour
    d->backgroundCol = QColor::fromRgb(QRandomGenerator::global()->generate());

    this->setFixedWidth(0);

    //Animate in
    tVariantAnimation* anim = new tVariantAnimation(this);
    anim->setStartValue(0);
    anim->setEndValue(this->height());
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    connect(anim, &tVariantAnimation::valueChanged, [ = ](QVariant value) {
        anim->setEndValue(this->height());
        this->setFixedWidth(value.toInt());
    });
    connect(anim, &QAbstractAnimation::finished, anim, &QObject::deleteLater);
    anim->start();

    d->usernameOpacity = new tVariantAnimation(this);
    d->usernameOpacity->setStartValue((float) 0);
    d->usernameOpacity->setEndValue((float) 1);
    d->usernameOpacity->setEasingCurve(QEasingCurve::OutCubic);
    d->usernameOpacity->setDuration(500);
    connect(d->usernameOpacity, &tVariantAnimation::valueChanged, [ = ] {
        this->repaint();
    });
    connect(d->usernameOpacity, &tVariantAnimation::finished, [ = ] {
        this->repaint();
    });
    d->usernameOpacity->start();

    d->cameraTimerOpacity = new tVariantAnimation(this);
    d->cameraTimerOpacity->setEasingCurve(QEasingCurve::OutCubic);
    d->cameraTimerOpacity->setDuration(500);
    connect(d->cameraTimerOpacity, &tVariantAnimation::valueChanged, [ = ] {
        this->repaint();
    });
    connect(d->cameraTimerOpacity, &tVariantAnimation::finished, [ = ] {
        this->repaint();
    });

}

EventModeUserIndicator::~EventModeUserIndicator() {
    delete d;
    delete ui;
}

void EventModeUserIndicator::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)
    if (!d->deleting) {
        d->drawableRect.setSize(this->size());
        d->drawableRect.setTopLeft(QPoint(0, 0));
    }
}

void EventModeUserIndicator::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPen foregroundPen;

    if ((d->backgroundCol.red() + d->backgroundCol.green() + d->backgroundCol.blue()) / 3 > 127) {
        foregroundPen = QPen(Qt::black);
    } else {
        foregroundPen = QPen(Qt::white);
    }

    QPainter painter(this);
    painter.setPen(Qt::transparent);
    painter.setBrush(d->backgroundCol);
    painter.drawRect(d->drawableRect);

    painter.save();
    if (d->username != "") {
        painter.setPen(foregroundPen);
        painter.setOpacity(d->usernameOpacity->currentValue().toFloat());

        QString nameCharacter(d->username.at(0).toUpper());

        QFont f;
        f.setFamily(this->font().family());
        f.setPointSize(1);
        while (QFontMetrics(f).height() < (float) this->height() * (float) 0.75) {
            f.setPointSize(f.pointSize() + 1);
        }
        f.setPointSize(f.pointSize() - 1);
        painter.setFont(f);

        QFontMetrics metrics(f);
        int charWidth = metrics.horizontalAdvance(nameCharacter);
        int charHeight = metrics.height();

        QRect textRect;
        textRect.setWidth(charWidth);
        textRect.setHeight(charHeight);
        textRect.moveTop(d->drawableRect.top() + d->drawableRect.height() / 2 - charHeight / 2);
        textRect.moveLeft(d->drawableRect.left() + d->drawableRect.height() / 2 - charWidth / 2); //Use height to position it correctly during animation
        painter.drawText(textRect, nameCharacter);
    }
    painter.restore();

    painter.save();
    if (d->timer != 0) {
        painter.setPen(foregroundPen);
        painter.setOpacity(d->cameraTimerOpacity->currentValue().toFloat());

        QString text = tr("Camera Timer");

        QFont f;
        f.setFamily(this->font().family());
        f.setPointSize(1);
        while (QFontMetrics(f).height() < (float) this->height() * (float) 0.75) {
            f.setPointSize(f.pointSize() + 1);
        }
        f.setPointSize(f.pointSize() - 1);
        painter.setFont(f);

        QFontMetrics metrics(f);
        int charWidth = metrics.horizontalAdvance("10");
        int charHeight = metrics.height();

        QRect textRect;
        textRect.setWidth(charWidth);
        textRect.setHeight(charHeight);
        textRect.moveTop(this->height() / 2 - charHeight / 2);
        textRect.moveLeft(9 * theLibsGlobal::getDPIScaling()); //Use height to position it correctly during animation

        QRect realTextRect = textRect;
        realTextRect.setHeight(charHeight);
        realTextRect.setWidth(metrics.horizontalAdvance(QString::number(d->timer)));
        realTextRect.moveCenter(textRect.center());

        painter.drawText(realTextRect, QString::number(d->timer));

        painter.setFont(this->font());
        textRect.moveLeft(textRect.right() + 9);
        textRect.setHeight(this->fontMetrics().height());
        textRect.setWidth(this->fontMetrics().horizontalAdvance(text) + 1);
        textRect.moveBottom(this->height() / 2);
        painter.drawText(textRect, text);

        f = this->font();
        f.setBold(true);
        painter.setFont(f);

        textRect.setWidth(QFontMetrics(f).horizontalAdvance(d->username));
        textRect.moveTop(textRect.bottom());
        painter.drawText(textRect, d->username);
    }
    painter.restore();
}
