#include "imageview.h"
#include "ui_imageview.h"

#include <QPainter>
#include <tpropertyanimation.h>
#include <QGraphicsOpacityEffect>

struct ImageViewPrivate {
    tVariantAnimation* opacityAnimation = nullptr;
    tVariantAnimation* locationAnimation = nullptr;
    tVariantAnimation* sourceRectAnimation = nullptr;
    ImgDesc image;
};

ImageView::ImageView(QWidget *parent) :
    QWidget(parent),
    ui(new Ui::ImageView)
{
    ui->setupUi(this);

    d = new ImageViewPrivate();

    d->opacityAnimation = new tVariantAnimation();
    d->opacityAnimation->setStartValue(0.0);
    d->opacityAnimation->setEndValue(1.0);
    d->opacityAnimation->setDuration(250);
    d->opacityAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->opacityAnimation, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->update();
    });
}

ImageView::~ImageView()
{
    if (d->opacityAnimation != nullptr) d->opacityAnimation->deleteLater();
    if (d->locationAnimation != nullptr) d->locationAnimation->deleteLater();
    if (d->sourceRectAnimation != nullptr) d->sourceRectAnimation->deleteLater();
    delete ui;
    delete d;
}

void ImageView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setOpacity(d->opacityAnimation->currentValue().toReal());
    painter.setBrush(Qt::black);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, this->width(), this->height());
    painter.setOpacity(1);

    if (!d->image.isNull()) {
        painter.drawImage(d->locationAnimation->currentValue().toRectF(), d->image->image(), d->sourceRectAnimation->currentValue().toRectF());
    }
}

void ImageView::animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image) {
    d->image = image;

    d->opacityAnimation->start();

    d->locationAnimation = new tVariantAnimation();
    d->locationAnimation->setStartValue(location);
    d->locationAnimation->setEndValue(calculateEndRect());
    d->locationAnimation->setDuration(250);
    d->locationAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->locationAnimation, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->update();
    });
    d->locationAnimation->start();

    d->sourceRectAnimation = new tVariantAnimation();
    d->sourceRectAnimation->setStartValue(sourceRect);
    d->sourceRectAnimation->setEndValue(QRectF(0, 0, image->image().width(), image->image().height()));
    d->sourceRectAnimation->setDuration(250);
    d->sourceRectAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->sourceRectAnimation, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->update();
    });
    d->sourceRectAnimation->start();
}

void ImageView::close() {
    QRectF endRect = calculateEndRect();
    endRect.moveTop(endRect.top() + this->height() / 8);
    d->locationAnimation->setStartValue(d->locationAnimation->currentValue());
    d->locationAnimation->setEndValue(endRect);
    d->locationAnimation->setEasingCurve(QEasingCurve::InCubic);
    d->locationAnimation->start();

    QGraphicsOpacityEffect* effect = new QGraphicsOpacityEffect();
    effect->setOpacity(1);
    this->setGraphicsEffect(effect);

    tPropertyAnimation* opacityAnimation = new tPropertyAnimation(effect, "opacity");
    opacityAnimation->setStartValue(1.0);
    opacityAnimation->setEndValue(0.0);
    opacityAnimation->setDuration(250);
    opacityAnimation->setEasingCurve(QEasingCurve::InCubic);
    connect(opacityAnimation, &tPropertyAnimation::finished, opacityAnimation, &tPropertyAnimation::deleteLater);
    connect(opacityAnimation, &tPropertyAnimation::finished, this, &ImageView::deleteLater);
    opacityAnimation->start();
}

void ImageView::resizeEvent(QResizeEvent *event) {
    if (!d->image.isNull()) {
        d->locationAnimation->setEndValue(calculateEndRect());
    }
}

void ImageView::mousePressEvent(QMouseEvent *event) {
    close();
}

QRectF ImageView::calculateEndRect() {
    QSizeF endSize = d->image->image().size().scaled(this->width(), this->height(), Qt::KeepAspectRatio);
    QRectF endRect;
    endRect.setSize(endSize);
    endRect.moveCenter(QPointF(this->width() / 2, this->height() / 2));
    return endRect;
}
