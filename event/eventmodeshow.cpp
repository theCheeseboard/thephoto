#include "eventmodeshow.h"
#include "ui_eventmodeshow.h"

#include <QDebug>
#include <QLayout>
#include <QPainter>
#include <QWindow>
#include <QGraphicsScene>
#include <QGraphicsPixmapItem>
#include <QGraphicsBlurEffect>
#include <QScrollArea>
#include <QScrollBar>

EventModeShow::EventModeShow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::EventModeShow)
{
    ui->setupUi(this);

    ui->internetDetails->setVisible(false);
    ui->wifiIcon->setPixmap(QIcon::fromTheme("network-wireless", QIcon(":/icons/network-wireless.svg")).pixmap(16, 16));
    ui->keyIcon->setPixmap(QIcon::fromTheme("password-show-on", QIcon(":/icons/password-show-on.svg")).pixmap(16, 16));

    animationTimerPx = new tVariantAnimation();
    animationTimerPxOpacity = new tVariantAnimation();
    animationTimerBlur = new tVariantAnimation();

    animationTimerPx->setDuration(500);
    animationTimerPxOpacity->setDuration(500);
    animationTimerBlur->setDuration(500);
    connect(animationTimerPx, &tVariantAnimation::valueChanged, [=] {
        this->repaint();
    });

    connect(ui->profileScroller->horizontalScrollBar(), &QScrollBar::rangeChanged, [=] {
        ui->profileScroller->horizontalScrollBar()->setValue(ui->profileScroller->horizontalScrollBar()->maximum());
    });

    scrollRedactor = new QWidget(this);
    scrollRedactor->setParent(ui->profileScroller);
    scrollRedactor->resize(ui->bottomFrameStack->sizeHint().height(), 30 * theLibsGlobal::getDPIScaling());
    scrollRedactor->move(0, 0);
    scrollRedactor->installEventFilter(this);
    scrollRedactor->show();
    scrollRedactor->raise();

    ui->bottomFrameStack->setCurrentIndex(0);
}

EventModeShow::~EventModeShow()
{
    delete ui;
}

void EventModeShow::resizeEvent(QResizeEvent *event) {
    updateRedactor();
}

void EventModeShow::updateRedactor() {
    scrollRedactor->resize(50 * theLibsGlobal::getDPIScaling(), ui->bottomFrameStack->height());
}

void EventModeShow::updateInternetDetails(QString ssid, QString password, bool show) {
    ui->internetDetails->setVisible(show);
    ui->ssid->setText(ssid);
    ui->key->setText(password);
    updateRedactor();
}

void EventModeShow::setCode(QString code) {
    ui->code->setText(code);
    ui->bottomFrameStack->setCurrentIndex(1);
}

void EventModeShow::showFullScreen(int monitor) {    
    #ifdef Q_OS_LINUX
        this->showNormal();
    #endif

    if (QApplication::desktop()->screenCount() <= monitor) {
        monitor = QApplication::desktop()->screenCount() - 1;
    }

    this->setGeometry(QApplication::desktop()->screenGeometry(monitor));

    QDialog::showFullScreen();
}

void EventModeShow::showNewImage(QImage image) {
    pendingImages.push(QPixmap::fromImage(image));
    tryNewImage();
}

void EventModeShow::tryNewImage() {
    if (!blockingOnNewImage && !pendingImages.isEmpty()) {
        blockingOnNewImage = true;

        animationTimerPx->setStartValue((float) 1);
        animationTimerPx->setEndValue((float) 1.25);
        animationTimerPx->setEasingCurve(QEasingCurve::InCubic);
        animationTimerPx->start();
        animationTimerPxOpacity->setStartValue((float) 1);
        animationTimerPxOpacity->setEndValue((float) 0);
        animationTimerPxOpacity->setEasingCurve(QEasingCurve::InCubic);
        animationTimerPxOpacity->start();

        QMetaObject::Connection* connection = new QMetaObject::Connection;
        *connection = connect(animationTimerPx, &tVariantAnimation::finished, [=] {
            disconnect(*connection);

            this->px = pendingImages.pop();

            updateBlurredImage();

            animationTimerPx->setStartValue((float) 0.75);
            animationTimerPx->setEndValue((float) 1);
            animationTimerPx->setEasingCurve(QEasingCurve::OutCubic);
            animationTimerPx->start();

            animationTimerPxOpacity->stop();
            animationTimerPxOpacity->setStartValue((float) 0);
            animationTimerPxOpacity->setEndValue((float) 1);
            animationTimerPxOpacity->setEasingCurve(QEasingCurve::OutCubic);
            animationTimerPxOpacity->start();

            //Give each image at least 5 seconds on the screen
            QTimer::singleShot(5000, [=] {
                blockingOnNewImage = false;
                tryNewImage();
            });
        });
    }
}

void EventModeShow::paintEvent(QPaintEvent *event) {
    QPainter painter(this);

    QRect pixmapRect;
    pixmapRect.setSize(px.size().scaled(this->width(), this->height() - ui->bottomFrameStack->height(), Qt::KeepAspectRatio) * animationTimerPx->currentValue().toFloat());
    pixmapRect.moveLeft(this->width() / 2 - pixmapRect.width() / 2);
    pixmapRect.moveTop((this->height() - ui->bottomFrameStack->height()) / 2 - pixmapRect.height() / 2);

    painter.setBrush(Qt::black);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, this->width(), this->height());

    QRect blurredRect;
    blurredRect.setSize(blurred.size());
    blurredRect.moveLeft(this->width() / 2 - blurredRect.width() / 2);
    blurredRect.moveTop((this->height() - ui->bottomFrameStack->height()) / 2 - blurredRect.height() / 2);

    painter.setOpacity(animationTimerPxOpacity->currentValue().toFloat());
    painter.drawPixmap(blurredRect, blurred);
    painter.drawPixmap(pixmapRect, px);
}

void EventModeShow::addToProfileLayout(QWidget *widget) {
    //ui->profileLayout->addWidget(widget);
    ui->profileArea->layout()->addWidget(widget);
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

bool EventModeShow::eventFilter(QObject *watched, QEvent *event) {
    if (watched = scrollRedactor) {
        if (event->type() == QEvent::Paint) {
            QPainter painter(scrollRedactor);

            QLinearGradient gradient;
            gradient.setStart(0, 0);
            gradient.setFinalStop(scrollRedactor->width(), 0);
            gradient.setColorAt(0, this->palette().color(QPalette::Window));

            QColor endCol = this->palette().color(QPalette::Window);
            endCol.setAlpha(0);
            gradient.setColorAt(1, endCol);

            painter.setBrush(gradient);
            painter.setPen(Qt::transparent);
            painter.drawRect(0, 0, scrollRedactor->width(), scrollRedactor->height());
        }
    }
    return true;
}
