#include "imagedescriptor.h"
#include <QEventLoop>
#include <QImageReader>
#include "easyexif/exif.h"
#include <QFileSystemWatcher>

struct ImageDescriptorPrivate {
    QString filename;
    QPixmap data;
    QPixmap compact;
    ImageDescriptor::LoadStatus loaded = ImageDescriptor::NotLoaded;
    ImageDescriptor::LoadStatus compactLoaded = ImageDescriptor::NotLoaded;
    QMap<ImageDescriptor::MetadataKeys, QVariant> metadata;

    QFileSystemWatcher watcher;
};

ImageDescriptor::ImageDescriptor(QString filename) : QObject(nullptr) {
    d = new ImageDescriptorPrivate();
    d->filename = filename;

    d->watcher.addPath(d->filename);
    connect(&d->watcher, &QFileSystemWatcher::fileChanged, this, [=] {
        if (!QFile::exists(d->filename)) {
            //Unload everything
            unload(false);

            //Tell everyone this image is deleted
            emit deleted();
        }
    });
}

ImageDescriptor::~ImageDescriptor() {
    delete d;
}

tPromise<void>* ImageDescriptor::load(bool compactData) {
    if (d->compactLoaded == ImageDescriptor::NotLoaded) d->compactLoaded = ImageDescriptor::Loading;
    if (!compactData) d->loaded = ImageDescriptor::Loading;

    struct LoadReturn {
        QImage image;
        QMap<MetadataKeys, QVariant> metadata;
    };

    (new tPromise<LoadReturn>([=](QString& error) {
        LoadReturn retVal;

        //Read in file contents
        QFile file(d->filename);
        file.open(QFile::ReadOnly);
        QByteArray imageData = file.readAll();
        file.close();

        //Load in image
        QBuffer buffer(&imageData);
        QImageReader reader(&buffer);
        reader.setAutoTransform(true);
        retVal.image = reader.read();

        //Load image metadata
        easyexif::EXIFInfo exif;
        int failure = exif.parseFrom(reinterpret_cast<const unsigned char*>(imageData.constData()), imageData.length());
        if (!failure) {
            auto insertString = [=, &retVal](MetadataKeys key, std::string string) {
                QString value = QString::fromStdString(string);
                if (value != "") {
                    retVal.metadata.insert(key, value);
                }
            };

            insertString(CameraMake, exif.Make);
            insertString(CameraModel, exif.Model);
            insertString(Software, exif.Software);
            retVal.metadata.insert(BitsPerSample, exif.BitsPerSample);
            insertString(Description, exif.ImageDescription);
            retVal.metadata.insert(Flash, exif.Flash);
            retVal.metadata.insert(ISO, exif.ISOSpeedRatings);
        }

        retVal.metadata.insert(FileName, QFileInfo(d->filename).fileName());
        retVal.metadata.insert(Dimensions, retVal.image.size());

        return retVal;
    }))->then([=](LoadReturn retval) {
        //Update the metadata
        d->metadata = retval.metadata;

        //Always load the compact data if needed
        if (d->compactLoaded == ImageDescriptor::Loading) {
            d->compact = QPixmap::fromImage(retval.image.scaledToWidth(300));
            d->compactLoaded = ImageDescriptor::Loaded;
            emit loaded(true);
        }

        if (!compactData) {
            d->data = QPixmap::fromImage(retval.image);
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

QMap<ImageDescriptor::MetadataKeys, QVariant> ImageDescriptor::metadata() {
    return d->metadata;
}

bool ImageDescriptor::deleteFromDisk() {
    return QFile::remove(d->filename);
}
