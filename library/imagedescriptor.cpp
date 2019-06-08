#include "imagedescriptor.h"
#include <QEventLoop>
#include <QImageReader>

struct ImageDescriptorPrivate {
    QString filename;
    QPixmap data;
    QPixmap compact;
    ImageDescriptor::LoadStatus loaded = ImageDescriptor::NotLoaded;
    ImageDescriptor::LoadStatus compactLoaded = ImageDescriptor::NotLoaded;
};

ImageDescriptor::ImageDescriptor(QString filename) : QObject(nullptr) {
    d = new ImageDescriptorPrivate();
    d->filename = filename;
}

ImageDescriptor::~ImageDescriptor() {
    delete d;
}

tPromise<void>* ImageDescriptor::load(bool compactData) {
    if (d->compactLoaded == ImageDescriptor::NotLoaded) d->compactLoaded = ImageDescriptor::Loading;
    if (!compactData) d->loaded = ImageDescriptor::Loading;

    (new tPromise<QImage>([=](QString& error) {
        QImageReader reader(d->filename);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        return image;
    }))->then([=](QImage image) {
        //Always load the compact data if needed
        if (d->compactLoaded == ImageDescriptor::Loading) {
            d->compact = QPixmap::fromImage(image.scaledToWidth(300));
            d->compactLoaded = ImageDescriptor::Loaded;
            emit loaded(true);
        }

        if (!compactData) {
            d->data = QPixmap::fromImage(image);
            d->loaded = ImageDescriptor::Loaded;
            emit loaded(false);
        }
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
        d->compactLoaded = ImageDescriptor::NotLoaded;
        emit unloaded(true);
    }
}

ImageDescriptor::LoadStatus ImageDescriptor::isLoaded() {
    return d->loaded;
}

ImageDescriptor::LoadStatus ImageDescriptor::isCompactLoaded() {
    return d->compactLoaded;
}

QDateTime ImageDescriptor::dateTaken() {
    QFileInfo info(d->filename);
    return info.created();
}

QPixmap ImageDescriptor::image() {
    if (d->loaded == ImageDescriptor::Loaded) {
        return d->data;
    } else {
        return d->compact;
    }
}

QPixmap ImageDescriptor::compactImage() {
    return d->compact;
}
