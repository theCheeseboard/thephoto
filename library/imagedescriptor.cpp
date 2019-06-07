#include "imagedescriptor.h"
#include <QEventLoop>
#include <QImageReader>

struct ImageDescriptorPrivate {
    QString filename;
    QImage data;
    bool loaded = false;
};

ImageDescriptor::ImageDescriptor(QString filename) : QObject(nullptr) {
    d = new ImageDescriptorPrivate();
    d->filename = filename;
}

ImageDescriptor::~ImageDescriptor() {
    delete d;
}

tPromise<void>* ImageDescriptor::load() {
    d->loaded = true;

    (new tPromise<QImage>([=](QString& error) {
        QImageReader reader(d->filename);
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        return image;
    }))->then([=](QImage image) {
        d->data = image;
        emit loaded();
    });

    return new tPromise<void>([=](QString& error) {
        QEventLoop loop;
        connect(this, &ImageDescriptor::loaded, &loop, &QEventLoop::quit);
        loop.exec();
    });
}

bool ImageDescriptor::isLoaded() {
    return d->loaded;
}

QDateTime ImageDescriptor::dateTaken() {
    QFileInfo info(d->filename);
    return info.created();
}

QImage ImageDescriptor::image() {
    return d->data;
}
