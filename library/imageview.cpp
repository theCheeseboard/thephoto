#include "imageview.h"
#include "ui_imageview.h"

#include <QPainter>
#include <tpropertyanimation.h>
#include <QGraphicsOpacityEffect>
#include <QShortcut>
#include <ttoast.h>
#include "imagegrid.h"
#include "imageviewsidebar.h"

struct ImageViewPrivate {
    ImageGrid* grid = nullptr;
    ImageViewSidebar* sidebar;

    tVariantAnimation* opacityAnimation = nullptr;
    tVariantAnimation* locationAnimation = nullptr;
    tVariantAnimation* sourceRectAnimation = nullptr;
    ImgDesc image;

    bool closing = false;
    bool editing = false;
    bool inSlideshow = false;

    QTransform editTransformationMatrix;
    QImage editingImage, editingImageWithTransform;
    QMetaObject::Connection imageDeleteConnection;

    QTimer* slideshowTimer;
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

    d->sidebar = new ImageViewSidebar(this);
    connect(this, &ImageView::editStarted, d->sidebar, &ImageViewSidebar::startEdit);
    connect(this, &ImageView::editEnded, d->sidebar, &ImageViewSidebar::endEdit);

    QShortcut* escShortcut = new QShortcut(QKeySequence(Qt::Key_Escape), this, SLOT(escPressed()));
    escShortcut->setAutoRepeat(false);

    QShortcut* nextShortcut = new QShortcut(QKeySequence(Qt::Key_Right), this, SLOT(nextImage()));
    QShortcut* prevShortcut = new QShortcut(QKeySequence(Qt::Key_Left), this, SLOT(previousImage()));

    d->slideshowTimer = new QTimer();
    d->slideshowTimer->setInterval(5000);
    connect(d->slideshowTimer, &QTimer::timeout, this, &ImageView::nextImage);
}

ImageView::~ImageView()
{
    if (d->opacityAnimation != nullptr) d->opacityAnimation->deleteLater();
    if (d->locationAnimation != nullptr) d->locationAnimation->deleteLater();
    if (d->sourceRectAnimation != nullptr) d->sourceRectAnimation->deleteLater();
    delete ui;
    delete d;
}

QWidget* ImageView::sidebar() {
    return d->sidebar;
}

void ImageView::editImage(QImage image)
{
    d->editingImage = image;
    editTransform(d->editTransformationMatrix);
}

void ImageView::editTransform(QTransform transform)
{
    d->editTransformationMatrix = transform;
    d->editingImageWithTransform = d->editingImage.transformed(transform);
    d->locationAnimation->setEndValue(calculateEndRect(d->editingImageWithTransform.size()));
    d->sourceRectAnimation->setEndValue(QRectF(0, 0, d->editingImageWithTransform.size().width(), d->editingImageWithTransform.size().height()));
}

void ImageView::setImageGrid(ImageGrid *grid) {
    d->grid = grid;
}

void ImageView::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    painter.setOpacity(d->opacityAnimation->currentValue().toReal());
    painter.setBrush(Qt::black);
    painter.setPen(Qt::transparent);
    painter.drawRect(0, 0, this->width(), this->height());
    painter.setOpacity(1);

    if (!d->image.isNull()) {
        QImage drawImage;
        if (d->editing) {
            drawImage = d->editingImageWithTransform;
        } else {
            drawImage = d->image->image().toImage();
        }
        painter.drawImage(d->locationAnimation->currentValue().toRectF(), drawImage, d->sourceRectAnimation->currentValue().toRectF());
    }
    painter.end();
    this->update();
}

void ImageView::animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image) {
    d->locationAnimation = new tVariantAnimation();
    d->locationAnimation->setDuration(250);
    d->locationAnimation->setEasingCurve(QEasingCurve::OutCubic);

    d->sourceRectAnimation = new tVariantAnimation();
    d->sourceRectAnimation->setDuration(250);
    d->sourceRectAnimation->setEasingCurve(QEasingCurve::OutCubic);

    loadImage(image);

    d->opacityAnimation->start();

    d->locationAnimation->setStartValue(location);
    d->locationAnimation->setEndValue(calculateEndRect(image->image().size()));
    connect(d->locationAnimation, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->update();
    });
    d->locationAnimation->start();

    d->sourceRectAnimation->setStartValue(sourceRect);
    d->sourceRectAnimation->setEndValue(QRectF(0, 0, image->image().width(), image->image().height()));
    connect(d->sourceRectAnimation, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        this->update();
    });
    d->sourceRectAnimation->start();

    d->sidebar->show();
}

void ImageView::close() {
    if (d->inSlideshow) {
        //Exit slideshow
        this->endSlideshow();
    } else {
        emit closed();
        d->closing = true;

        QRectF endRect = calculateEndRect(d->image->image().size());
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

        d->sidebar->hide();
    }
}

void ImageView::resizeEvent(QResizeEvent *event) {
    if (!d->image.isNull() && !d->closing) {
        if (d->editing) {
            d->locationAnimation->setEndValue(calculateEndRect(d->editingImageWithTransform.size()));
        } else {
            d->locationAnimation->setEndValue(calculateEndRect(d->image->image().size()));
        }
    }
}

void ImageView::mousePressEvent(QMouseEvent *event) {

}

QRectF ImageView::calculateEndRect(QSize size) {
    QSizeF endSize = size.scaled(this->width(), this->height(), Qt::KeepAspectRatio);
    QRectF endRect;
    endRect.setSize(endSize);
    endRect.moveCenter(QPointF(this->width() / 2, this->height() / 2));
    return endRect;
}

bool ImageView::nextImage() {
    if (d->editing) return false;
    if (d->grid == nullptr) return false;

    if (d->slideshowTimer->isActive()) {
        d->slideshowTimer->stop();
        d->slideshowTimer->start();
    }

    ImgDesc newImage = d->grid->nextImage(d->image);
    if (newImage.isNull()) newImage = d->grid->firstImage();
    if (!newImage.isNull()) {
        loadImage(newImage);
        return true;
    }
    return false;
}

bool ImageView::previousImage() {
    if (d->editing) return false;
    if (d->grid == nullptr) return false;

    if (d->slideshowTimer->isActive()) {
        d->slideshowTimer->stop();
        d->slideshowTimer->start();
    }

    ImgDesc newImage = d->grid->prevImage(d->image);
    if (newImage.isNull()) newImage = d->grid->lastImage();
    if (!newImage.isNull()) {
        loadImage(newImage);
        return true;
    }
    return false;
}

void ImageView::loadImage(ImgDesc image) {
    if (d->imageDeleteConnection) disconnect(d->imageDeleteConnection);
    d->image = image;
    d->imageDeleteConnection = connect(d->image.data(), &ImageDescriptor::deleted, this, [=] {
        //Try to move to the next image or the previous image
        if (nextImage()) return;
        if (previousImage()) return;

        //No images left; close the image viewer
        close();
    });

    if (image->isLoaded() == ImageDescriptor::NotLoaded) {
        //Load the full image
        image->load(false)->then([=] {
            d->locationAnimation->setEndValue(calculateEndRect(image->image().size()));
            d->sourceRectAnimation->setEndValue(QRectF(0, 0, image->image().width(), image->image().height()));
            this->update();
        });
    }

    d->locationAnimation->setEndValue(calculateEndRect(image->image().size()));
    d->sourceRectAnimation->setEndValue(QRectF(0, 0, image->image().width(), image->image().height()));
    this->update();

    if (d->grid != nullptr) {
        //Preload the next and previous image
        ImgDesc nextImage = d->grid->nextImage(image);
        if (!nextImage.isNull() && nextImage->isLoaded() == ImageDescriptor::NotLoaded) nextImage->load(false);
        ImgDesc prevImage = d->grid->prevImage(image);
        if (!prevImage.isNull() && prevImage->isLoaded() == ImageDescriptor::NotLoaded) prevImage->load(false);
    }

    //Update sidebar metadata
    QLocale locale;
    QList<ImageViewSidebarSection> sections;
    sections.append({tr("File"), {
                         {tr("Filename"), image->metadata().value(ImageDescriptor::FileName).toString()},
                         {tr("Dimensions"), ([=] {
                              QSize size = image->metadata().value(ImageDescriptor::Dimensions).toSize();
                              return QString("%1×%2").arg(size.width()).arg(size.height());
                          })()},
                     }});
    sections.append({tr("Image"), {
                         {tr("Flash"), image->metadata().value(ImageDescriptor::Flash).toBool() ? tr("Yes") : tr("No")},
                         {tr("ISO"), locale.toString(image->metadata().value(ImageDescriptor::ISO).toInt())}
                     }});
    sections.append({tr("Camera"), {
                         {tr("Camera Make"), image->metadata().value(ImageDescriptor::CameraMake).toString()},
                         {tr("Camera Model"), image->metadata().value(ImageDescriptor::CameraModel).toString()}
                     }});
    d->sidebar->setSections(sections);
}

void ImageView::deleteCurrentImage() {
    d->image->deleteFromDisk();
}


void ImageView::editCurrentImage()
{
    if (d->editing) return;
    d->editTransformationMatrix.reset();
    d->editingImage = d->image->image().toImage();
    d->editingImageWithTransform = d->editingImage;

    emit editStarted(d->editingImage);
    d->editing = true;
}

void ImageView::endEdit(QString filename)
{
    if (!d->editing) return;

    if (!filename.isEmpty()) {
        //Save the edits to the new filename
        if (filename == ":INPLACE") {
            filename = d->image->fileName();
        }

        d->editingImageWithTransform.save(filename);
        d->image->reload(false);
    }

    emit editEnded();

    d->locationAnimation->setEndValue(calculateEndRect(d->image->image().size()));
    d->sourceRectAnimation->setEndValue(QRectF(0, 0, d->image->image().size().width(), d->image->image().size().height()));

    d->editing = false;
}

void ImageView::beginSlideshow() {
    if (d->inSlideshow) return;

    this->setCursor(QCursor(Qt::BlankCursor));
    d->sidebar->hide();
    d->slideshowTimer->start();
    d->inSlideshow = true;
    emit slideshowModeChanged(true);

    tToast* toast = new tToast();
    toast->setTitle(tr("Starting Slideshow"));
    toast->setText(tr("Hit ESC to exit slideshow mode."));
    connect(toast, &tToast::dismissed, toast, &tToast::deleteLater);
    toast->show(this->window());
}

void ImageView::endSlideshow() {
    if (!d->inSlideshow) return;

    this->setCursor(QCursor(Qt::ArrowCursor));
    d->sidebar->show();
    d->slideshowTimer->stop();
    d->inSlideshow = false;
    emit slideshowModeChanged(false);
}

bool ImageView::inSlideshow() {
    return d->inSlideshow;
}

void ImageView::escPressed()
{
    if (d->editing) {
        endEdit("");
    } else {
        close();
    }
}
