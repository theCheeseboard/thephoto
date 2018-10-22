#include "eventmodeshow.h"
#include "ui_eventmodeshow.h"

#include <QDebug>
#include <QLayout>
#include <QPainter>
#include <QWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>

EventModeShow::EventModeShow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeShow)
{
    ui->setupUi(this);

    ui->internetDetails->setVisible(false);
    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless", QIcon(":/icons/network-wireless.svg")).pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on", QIcon(":/icons/password-show-on.svg")).pixmap(16, 16));
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
    ui->bottomFrameStack->setCurrentIndex(1);
}

void EventModeShow::showFullScreen(int monitor) {
    //this->showNormal();

    if (QApplication::desktop()->screenCount() <= monitor) {
        monitor = QApplication::desktop()->screenCount() - 1;
    }

    //this->setGeometry(QRect(QApplication::desktop()->screenGeometry(monitor).center(), QSize(5, 5)));
    this->setGeometry(QApplication::desktop()->screenGeometry(monitor));

    QDialog::showFullScreen();
}

void EventModeShow::showNewImage(QImage image) {
    this->px = QPixmap::fromImage(image);

    updateBlurredImage();
    this->repaint();
}

void EventModeShow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QRect pixmapRect;
    pixmapRect.setSize(px.size().scaled(this->width(), this->height() - ui->bottomFrameStack->height(), Qt::KeepAspectRatio));
    pixmapRect.moveLeft(this->width() / 2 - pixmapRect.width() / 2);
    pixmapRect.moveTop((this->height() - ui->bottomFrameStack->height()) / 2 - pixmapRect.height() / 2);

    painter.setBrush(Qt::black);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, this->width(), this->height());

    QRect blurredRect;
    blurredRect.setSize(blurred.size());
    blurredRect.moveLeft(this->width() / 2 - blurredRect.width() / 2);
    blurredRect.moveTop((this->height() - ui->bottomFrameStack->height()) / 2 - blurredRect.height() / 2);

    painter.drawPixmap(blurredRect, blurred);
    painter.drawPixmap(pixmapRect, px);
}

void EventModeShow::addToProfileLayout(QWidget *widget) {
    ui->profileLayout->addWidget(widget);
}

int EventModeShow::getProfileLayoutHeight() {
    return ui->bottomFrameStack->height();
}

void EventModeShow::showError(QString error) {
    ui->statusLabel->setText(error);
    ui->bottomFrameStack->setCurrentIndex(0);
}

void EventModeShow::updateBlurredImage() {
    int radius = 30;
    QGraphicsBlurEffect* blur = new QGraphicsBlurEffect;
    blur->setBlurRadius(radius);

    QGraphicsScene scene;
    QGraphicsPixmapItem item;
    item.setPixmap(px);
    item.setGraphicsEffect(blur);
    scene.addItem(&item);

    QRect pixmapRect;
    pixmapRect.setSize(px.size().scaled(this->width(), this->height() - ui->bottomFrameStack->height(), Qt::KeepAspectRatioByExpanding));
    pixmapRect.moveLeft(this->width() / 2 - pixmapRect.width() / 2);
    pixmapRect.moveTop((this->height() - ui->bottomFrameStack->height()) / 2 - pixmapRect.height() / 2);

    blurred = QPixmap(pixmapRect.size() + QSize(radius * 2, radius * 2));
    QPainter painter(&blurred);
    blurred.fill(Qt::black);
    scene.render(&painter, QRectF(), QRectF(-radius, -radius, px.width() + radius, px.height() + radius));
}
