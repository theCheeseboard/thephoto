#include "imagegrid.h"
#include <QPainter>
#include <QScrollBar>
#include <QApplication>
#include <QStyle>
#include <QWheelEvent>
#include <QScroller>
#include <QBoxLayout>
#include <tvariantanimation.h>
#include "imagedescriptormanager.h"

struct ImageLocation {
    enum LocationType {
        Image,
        DateMarker
    };

    ImageGrid* parent;
    QDate dateShown;
    LocationType type = Image;

    QObject* assuranceObject;
    ImgDesc image;
    QRectF rect;
    qreal opacity = 0;

    ImageLocation(ImageGrid* parent) {
        this->parent = parent;
        assuranceObject = new QObject();
    }

    ~ImageLocation() {
        assuranceObject->deleteLater();
    }

    bool isInView(QRect viewport) {
        return viewport.intersects(rect.toRect());
    }

    void draw(QPainter* painter, QRect viewport) {
        QRectF realRect = this->realRect(viewport);
        painter->setPen(Qt::transparent);
        painter->setBrush(QColor(0, 0, 0, 127));
        painter->drawRect(realRect);

        if (type == Image) {
            if (image->isLoaded() == ImageDescriptor::NotLoaded && image->isCompactLoaded() == ImageDescriptor::NotLoaded) load();

            painter->setOpacity(opacity);
            painter->drawPixmap(realRect, image->image(), sourceRect());
        } else if (type == DateMarker) {
            QFont font = parent->font();
            font.setPixelSize(realRect.height() / 3);

            QPointF center = realRect.center();
            center.ry() -= realRect.height() / 12;

            QRectF monthMarker;
            monthMarker.setWidth(QFontMetrics(font).width(dateShown.toString("MMM")) + 1);
            monthMarker.setHeight(QFontMetrics(font).height());
            monthMarker.moveCenter(center);
            painter->setFont(font);
            painter->setPen(parent->palette().color(QPalette::WindowText));
            painter->drawText(monthMarker, dateShown.toString("MMM"));


            font.setPixelSize(realRect.height() / 6);

            QRectF yearMarker;
            yearMarker.setWidth(QFontMetrics(font).width(dateShown.toString("yyyy")) + 1);
            yearMarker.setHeight(QFontMetrics(font).height());
            yearMarker.moveTop(monthMarker.bottom());
            yearMarker.moveRight(monthMarker.right());
            painter->setFont(font);
            painter->setPen(parent->palette().color(QPalette::WindowText));
            painter->drawText(yearMarker, dateShown.toString("yyyy"));
        }
    }

    void load() {
        //Load the compact image
        image->load(true)->then([=] {
            tVariantAnimation* anim = new tVariantAnimation();
            anim->setStartValue(0.0);
            anim->setEndValue(1.0);
            anim->setDuration(500);
            anim->setEasingCurve(QEasingCurve::OutCubic);
            QObject::connect(anim, &tVariantAnimation::valueChanged, assuranceObject, [=](QVariant value) {
                opacity = value.toReal();
                parent->update();
            });
            QObject::connect(anim, &tVariantAnimation::finished, anim, &tVariantAnimation::deleteLater);
            anim->start();
        });
    }

    QRectF realRect(QRectF viewport) {
        QRectF realRect = rect;
        realRect.moveTop(realRect.top() - viewport.top());
        return realRect;
    }

    QRectF sourceRect() {
        //Find the portion of the image to draw
        QSizeF rectSize(16, 9);
        rectSize.scale(image->image().size(), Qt::KeepAspectRatio);
        QRectF sourceRect;
        sourceRect.setSize(rectSize);
        sourceRect.moveCenter(QPoint(image->image().size().width() / 2, image->image().size().height() / 2));
        return sourceRect;
    }
};
typedef QSharedPointer<ImageLocation> ImgLoc;

struct ImageGridPrivate {
    QScrollBar* scrollbar;
    ImageDescriptorManager* mgr;
    QList<ImgLoc> images;
    int xzoom = 4;

    ImgLoc mousePressImage;

    QBoxLayout* layout;

    int scrollbarWidth() {
        return QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent, nullptr, scrollbar);
    }

    int viewWidth(int width) {
        return width - scrollbarWidth();
    }

    ImgLoc imageForCursor(QPoint pointer, QRect viewport) {
        for (ImgLoc image : images) {
            QRectF realRect = image->realRect(viewport);
            if (realRect.contains(pointer)) {
                return image;
            }
        }
        return nullptr;
    }
};

ImageGrid::ImageGrid(QWidget *parent) : QWidget(parent)
{
    d = new ImageGridPrivate();
    d->scrollbar = new QScrollBar();
    d->scrollbar->setParent(this);
    d->scrollbar->setValue(0);
    d->scrollbar->setMaximum(0);
    connect(d->scrollbar, &QScrollBar::valueChanged, this, [=] {
        this->update();
    });

    d->mgr = new ImageDescriptorManager();

    d->layout = new QBoxLayout(QBoxLayout::TopToBottom);
    d->layout->setMargin(0);
    this->setLayout(d->layout);

    QScroller::grabGesture(this, QScroller::LeftMouseButtonGesture);
}

ImageGrid::~ImageGrid() {
    delete d;
}

void ImageGrid::loadImages(QStringList images) {
    d->images.clear();
    for (QString image : images) {
        ImgLoc loc(new ImageLocation(this));
        loc->image = d->mgr->descriptorForFile(image);
        connect(loc->image.data(), &ImageDescriptor::deleted, loc->assuranceObject, [=] {
            QTimer::singleShot(0, [=] {
                //Remove the image from the grid
                d->images.removeAll(loc);
                relocate();
            });
        });
        d->images.append(loc);
    }

    //Sort images by date
    std::sort(d->images.begin(), d->images.end(), [=](const ImgLoc& first, const ImgLoc& second) {
        return first->image->dateTaken() > second->image->dateTaken();
    });

    //Go through and add date markers
    QDate beforeDate;
    for (int i = 0; i < d->images.count(); i++) {
        ImgLoc loc = d->images.at(i);
        if (loc->type == ImageLocation::Image) {
            if (beforeDate.month() != loc->image->dateTaken().date().month() || beforeDate.year() != loc->image->dateTaken().date().year()) {
                //Add a date marker
                beforeDate = loc->image->dateTaken().date();
                ImgLoc loc(new ImageLocation(this));
                loc->type = ImageLocation::DateMarker;
                loc->dateShown = beforeDate;
                d->images.insert(i, loc);
            }
        }
    }

    relocate();
}

void ImageGrid::paintEvent(QPaintEvent *event) {
    QPainter painter(this);
    QRect viewport(0, 0, this->width(), this->height());
    viewport.moveTop(d->scrollbar->value());

    for (ImgLoc loc : d->images) {
        if (loc->isInView(viewport)) {
            painter.save();
            loc->draw(&painter, viewport);
            painter.restore();
        }
    }
}

void ImageGrid::wheelEvent(QWheelEvent *event) {
    d->scrollbar->setValue(d->scrollbar->value() - event->pixelDelta().y());
    this->update();
}

void ImageGrid::resizeEvent(QResizeEvent *event) {
    d->scrollbar->setGeometry(d->viewWidth(this->width()), 0, d->scrollbarWidth(), this->height());
    relocate();
}

void ImageGrid::relocate() {
    QSizeF imgSize;
    imgSize.setWidth(static_cast<qreal>(d->viewWidth(this->width())) / d->xzoom);
    imgSize.setHeight(imgSize.width() * 9 / 16);

    int x = 0, y = 0;

    for (ImgLoc loc : d->images) {
        QRectF rect;
        rect.setX(imgSize.width() * x);
        rect.setY(imgSize.height() * y);
        rect.setSize(imgSize);
        loc->rect = rect;

        x++;
        if (x == d->xzoom) {
            x = 0;
            y++;
        }
    }

    //Set scrollbar limits
    qreal maxHeight = imgSize.height() * (y + (x == 0 ? 0 : 1));
    int max = maxHeight - this->height();
    if (max < 0) max = 0;
    d->scrollbar->setMaximum(max);
    d->scrollbar->setPageStep(this->height());

    this->update();
}

void ImageGrid::mousePressEvent(QMouseEvent *event) {
    //Find the image that was pressed
    QRect viewport(0, 0, this->width(), this->height());
    viewport.moveTop(d->scrollbar->value());
    d->mousePressImage = d->imageForCursor(event->pos(), viewport);
}

void ImageGrid::mouseReleaseEvent(QMouseEvent *event) {
    if (!d->mousePressImage.isNull()) {
        //Find the image that was pressed
        QRect viewport(0, 0, this->width(), this->height());
        viewport.moveTop(d->scrollbar->value());
        ImgLoc image = d->imageForCursor(event->pos(), viewport);

        if (d->mousePressImage == image) {
            if (image->type == ImageLocation::Image) {
                emit imageClicked(image->realRect(viewport), image->sourceRect(), image->image);
            }
        }
        d->mousePressImage = nullptr;
    }
}

void ImageGrid::setOverlayWidget(QWidget* widget) {
    d->layout->addWidget(widget);
    connect(widget, &QWidget::destroyed, this, [=] {
        d->layout->removeWidget(widget);
    });
}

ImgDesc ImageGrid::nextImage(ImgDesc image) {
    for (auto i = d->images.begin(); i != d->images.end(); i++) {
        if ((*i)->image == image) {
            //Return the next one
            do {
                i++;
            } while (i != d->images.end() && (*i)->type != ImageLocation::Image);

            if (i == d->images.end()) {
                return nullptr;
            } else {
                return (*i)->image;
            }
        }
    }
    return nullptr;
}

ImgDesc ImageGrid::prevImage(ImgDesc image) {
    for (auto i = d->images.rbegin(); i != d->images.rend(); i++) {
        if ((*i)->image == image) {
            //Return the previous one
            do {
                i++; //Using a reverse iterator here
            } while (i != d->images.rend() && (*i)->type != ImageLocation::Image);

            if (i == d->images.rend()) {
                return nullptr;
            } else {
                return (*i)->image;
            }
        }
    }
    return nullptr;
}
