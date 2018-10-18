#include "eventmodeshow.h"
#include "ui_eventmodeshow.h"

#include <QDebug>
#include <QLayout>
#include <QPainter>

EventModeShow::EventModeShow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeShow)
{
    ui->setupUi(this);

    ui->internetDetails->setVisible(false);
    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless").pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on").pixmap(16, 16));
}

EventModeShow::~EventModeShow()
{
    delete ui;
}

void EventModeShow::updateInternetDetails(QString ssid, QString password, bool show) {
    ui->internetDetails->setVisible(show);
    ui->ssid->setText(ssid);
    ui->key->setText(password);
}

void EventModeShow::setCode(QString code) {
    ui->code->setText(code);
}

void EventModeShow::showFullScreen(int monitor) {
    this->showNormal();

    if (QApplication::desktop()->screenCount() <= monitor) {
        monitor = QApplication::desktop()->screenCount() - 1;
    }

    this->setGeometry(QRect(QApplication::desktop()->screenGeometry(monitor).center(), QSize(5, 5)));

    QDialog::showFullScreen();
}

void EventModeShow::showNewImage(QImage image) {
    this->px = QPixmap::fromImage(image);

    this->repaint();
}

void EventModeShow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QRect pixmapRect;
    pixmapRect.setSize(px.size().scaled(this->width(), this->height() - ui->frame->height(), Qt::KeepAspectRatio));
    pixmapRect.moveLeft(this->width() / 2 - pixmapRect.width() / 2);
    pixmapRect.moveTop(this->height() / 2 - pixmapRect.height() / 2);

    painter.setBrush(Qt::black);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, this->width(), this->height());
    painter.drawPixmap(pixmapRect, px);
}
