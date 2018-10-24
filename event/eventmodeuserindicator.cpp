#include "eventmodeuserindicator.h"
#include "ui_eventmodeuserindicator.h"

#include <QRandomGenerator>
#include <QDebug>
#include <QPainter>
#include <tpropertyanimation.h>

EventModeUserIndicator::EventModeUserIndicator(EventSocket* sock, QWidget *parent) :
    QWidget(parent),
    ui(new Ui::EventModeUserIndicator)
{
    ui->setupUi(this);

    this->sock = sock;
    connect(sock, &EventSocket::newUserConnected, [=](QString name) {
        username = name;
        this->repaint();
    });
    connect(sock, &EventSocket::aboutToClose, [=] {
        deleting = true;

        QRect newRect = drawableRect;
        newRect.moveTop(this->height());

        tVariantAnimation* anim = new tVariantAnimation(this);
        anim->setStartValue(drawableRect);
        anim->setEndValue(newRect);
        anim->setEasingCurve(QEasingCurve::InCubic);
        anim->setDuration(500);
        connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
            drawableRect = value.toRect();
            this->repaint();
        });
        connect(anim, &tVariantAnimation::finished, [=] {
            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(0);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            connect(anim, &tVariantAnimation::finished, [=] {
                anim->deleteLater();
                this->deleteLater();
            });
            anim->start();
        });
        connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
        anim->start();
    });
    connect(sock, &EventSocket::timer, [=](int secondsLeft) {
        qDebug() << "TIMER:" << secondsLeft;

        if (timer == 0 && secondsLeft != 0) {
            //Expand

            QFont f;
            f.setFamily(this->font().family());
            f.setPointSize(1);
            while (QFontMetrics(f).height() < (float) this->height() * (float) 0.75) {
                f.setPointSize(f.pointSize() + 1);
            }
            f.setPointSize(f.pointSize() - 1);

            int width = QFontMetrics(f).width("10") + qMax(this->fontMetrics().width(tr("Camera Timer")),
                             this->fontMetrics().width(username)) + 27 * theLibsGlobal::getDPIScaling();

            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(width);
            anim->setEasingCurve(QEasingCurve::InOutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            anim->start();

            usernameOpacity->setStartValue((float) 1);
            usernameOpacity->setEndValue((float) 0);
            usernameOpacity->start();

            QMetaObject::Connection* connection = new QMetaObject::Connection;
            *connection = connect(usernameOpacity, &tVariantAnimation::finished, [=] {
                disconnect(*connection);

                cameraTimerOpacity->setStartValue((float) 0);
                cameraTimerOpacity->setEndValue((float) 1);
                cameraTimerOpacity->start();
            });
        } else if (timer != 0 && secondsLeft == 0) {
            //Contract
            tVariantAnimation* anim = new tVariantAnimation(this);
            anim->setStartValue(this->width());
            anim->setEndValue(this->height());
            anim->setEasingCurve(QEasingCurve::InOutCubic);
            anim->setDuration(500);
            connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
                this->setFixedWidth(value.toInt());
            });
            connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
            anim->start();

            cameraTimerOpacity->setStartValue((float) 1);
            cameraTimerOpacity->setEndValue((float) 0);
            cameraTimerOpacity->start();

            QMetaObject::Connection* connection = new QMetaObject::Connection;
            *connection = connect(cameraTimerOpacity, &tVariantAnimation::finished, [=] {
                disconnect(*connection);
                timer = secondsLeft;


                usernameOpacity->setStartValue((float) 0);
                usernameOpacity->setEndValue((float) 1);
                usernameOpacity->start();
            });

            this->repaint();
            //Return prematurely so that the timer variable doesn't get set
            //otherwise the animation looks strange
            return;
        }

        timer = secondsLeft;
        this->repaint();
    });

    //Set the background to a random colour
    backgroundCol = QColor::fromRgb(QRandomGenerator::global()->generate());

    this->setFixedWidth(0);

    //Animate in
    tVariantAnimation* anim = new tVariantAnimation(this);
    anim->setStartValue(0);
    anim->setEndValue(this->height());
    anim->setEasingCurve(QEasingCurve::OutCubic);
    anim->setDuration(500);
    connect(anim, &tVariantAnimation::valueChanged, [=](QVariant value) {
        anim->setEndValue(this->height());
        this->setFixedWidth(value.toInt());
    });
    connect(anim, SIGNAL(finished()), anim, SLOT(deleteLater()));
    anim->start();

    usernameOpacity = new tVariantAnimation(this);
    usernameOpacity->setStartValue((float) 0);
    usernameOpacity->setEndValue((float) 1);
    usernameOpacity->setEasingCurve(QEasingCurve::OutCubic);
    usernameOpacity->setDuration(500);
    connect(usernameOpacity, &tVariantAnimation::valueChanged, [=] {
        this->repaint();
    });
    connect(usernameOpacity, &tVariantAnimation::finished, [=] {
        this->repaint();
    });
    usernameOpacity->start();

    cameraTimerOpacity = new tVariantAnimation(this);
    cameraTimerOpacity->setEasingCurve(QEasingCurve::OutCubic);
    cameraTimerOpacity->setDuration(500);
    connect(cameraTimerOpacity, &tVariantAnimation::valueChanged, [=] {
        this->repaint();
    });
    connect(cameraTimerOpacity, &tVariantAnimation::finished, [=] {
        this->repaint();
    });

}

EventModeUserIndicator::~EventModeUserIndicator()
{
    delete ui;
}

void EventModeUserIndicator::resizeEvent(QResizeEvent *event) {
    if (!deleting) {
        drawableRect.setSize(this->size());
        drawableRect.setTopLeft(QPoint(0, 0));
    }
}

void EventModeUserIndicator::paintEvent(QPaintEvent *event) {
    QPen foregroundPen;

    if ((backgroundCol.red() + backgroundCol.green() + backgroundCol.blue()) / 3 > 127) {
        foregroundPen = QPen(Qt::black);
    } else {
        foregroundPen = QPen(Qt::white);
    }

    QPainter painter(this);
    painter.setPen(Qt::transparent);
    painter.setBrush(backgroundCol);
    painter.drawRect(drawableRect);

    painter.save();
    if (username != "") {
        painter.setPen(foregroundPen);
        painter.setOpacity(usernameOpacity->currentValue().toFloat());

        QString nameCharacter(username.at(0).toUpper());

        QFont f;
        f.setFamily(this->font().family());
        f.setPointSize(1);
        while (QFontMetrics(f).height() < (float) this->height() * (float) 0.75) {
            f.setPointSize(f.pointSize() + 1);
        }
        f.setPointSize(f.pointSize() - 1);
        painter.setFont(f);

        QFontMetrics metrics(f);
        int charWidth = metrics.width(nameCharacter);
        int charHeight = metrics.height();

        QRect textRect;
        textRect.setWidth(charWidth);
        textRect.setHeight(charHeight);
        textRect.moveTop(drawableRect.top() + drawableRect.height() / 2 - charHeight / 2);
        textRect.moveLeft(drawableRect.left() + drawableRect.height() / 2 - charWidth / 2); //Use height to position it correctly during animation
        painter.drawText(textRect, nameCharacter);
    }
    painter.restore();

    painter.save();
    if (timer != 0) {
        painter.setPen(foregroundPen);
        painter.setOpacity(cameraTimerOpacity->currentValue().toFloat());

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
        int charWidth = metrics.width("10");
        int charHeight = metrics.height();

        QRect textRect;
        textRect.setWidth(charWidth);
        textRect.setHeight(charHeight);
        textRect.moveTop(this->height() / 2 - charHeight / 2);
        textRect.moveLeft(9 * theLibsGlobal::getDPIScaling()); //Use height to position it correctly during animation

        QRect realTextRect = textRect;
        realTextRect.setHeight(charHeight);
        realTextRect.setWidth(metrics.width(QString::number(timer)));
        realTextRect.moveCenter(textRect.center());

        painter.drawText(realTextRect, QString::number(timer));

        painter.setFont(this->font());
        textRect.moveLeft(textRect.right() + 9);
        textRect.setHeight(this->fontMetrics().height());
        textRect.setWidth(this->fontMetrics().width(text) + 1);
        textRect.moveBottom(this->height() / 2);
        painter.drawText(textRect, text);

        f = this->font();
        f.setBold(true);
        painter.setFont(f);

        textRect.setWidth(QFontMetrics(f).width(username));
        textRect.moveTop(textRect.bottom());
        painter.drawText(textRect, username);
    }
    painter.restore();
}
