#include "imagedescriptor.h"
#include <QEventLoop>
#include <QImageReader>

struct ImageDescriptorPrivate {
    QString filename;
    QPixmap data;
    QPixmap compact;
    ImageDescriptor::LoadStatus loaded = ImageDescriptor::NotLoaded;
    bool compactLoaded = false;
};

ImageDescriptor::ImageDescriptor(QString filename) : QObject(nullptr) {
    d = new ImageDescriptorPrivate();
    d->filename = filename;
}

ImageDescriptor::~ImageDescriptor() {
    delete d;
}

tPromise<void>* ImageDescriptor::load(bool compactData) {
    if (compactData) {
        d->compactLoaded = true;
    } else {
        d->loaded = ImageDescriptor::Loading;
    }

    (new tPromise<QImage>([=](QString& error) {
        QImageReader reader(d->filename);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (compactData) {
            return image.scaledToWidth(500);
        } else {
            return image;
        }
    }))->then([=](QImage image) {
        if (compactData) {
            d->compact = QPixmap::fromImage(image);
        } else {
            d->data = QPixmap::fromImage(image);
            d->loaded = ImageDescriptor::Loaded;
        }
        emit loaded(compactData);
    });

    return new tPromise<void>([=](QString& error) {
        QEventLoop loop;
        connect(this, &ImageDescriptor::loaded, &loop, &QEventLoop::quit);
        loop.exec();
    });
}

void ImageDescriptor::unload(bool compactData) {
    //Always unload the full data
    d->data = QPixmap();
    d->loaded = ImageDescriptor::NotLoaded;
    emit unloaded(false);

    if (compactData) {
        //Unload the compact data
        d->compact = QPixmap();
        d->compactLoaded = false;
        emit unloaded(true);
    }
}

ImageDescriptor::LoadStatus ImageDescriptor::isLoaded() {
    return d->loaded;
}

bool ImageDescriptor::isCompactLoaded() {
    return d->compactLoaded;
}

QDateTime ImageDescriptor::dateTaken() {
    QFileInfo info(d->filename);
    return info.created();
}

QPixmap ImageDescriptor::image() {
    if (isLoaded() == ImageDescriptor::Loaded) {
        return d->data;
    } else {
        return d->compact;
    }
}

QPixmap ImageDescriptor::compactImage() {
    return d->compact;
}
