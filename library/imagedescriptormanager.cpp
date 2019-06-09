#include "imagedescriptormanager.h"
#include <QHash>
#include <QDir>

struct ImageDescriptorManagerPrivate {
    QHash<QString, ImgDesc> descriptors;

    QLinkedList<ImgDesc> compactlyLoaded;
    QLinkedList<ImgDesc> loaded;
};

ImageDescriptorManager::ImageDescriptorManager(QObject *parent) : QObject(parent)
{
    d = new ImageDescriptorManagerPrivate();
}

ImgDesc ImageDescriptorManager::descriptorForFile(QString filename) {
    QString cleaned = QDir::cleanPath(filename);
    if (d->descriptors.contains(cleaned)) {
        return d->descriptors.value(cleaned);
    } else {
        ImgDesc descriptor(new ImageDescriptor(cleaned));
        d->descriptors.insert(cleaned, descriptor);
        connect(descriptor.data(), &ImageDescriptor::loaded, this, [=](bool compactData) {
            if (compactData) {
                d->compactlyLoaded.append(descriptor);
                if (d->compactlyLoaded.count() > 100) {
                    d->compactlyLoaded.first()->unload(true);
                }
            } else {
                d->loaded.append(descriptor);
                if (d->loaded.count() > 10) {
                    d->loaded.first()->unload(true);
                }
            }
        });
        connect(descriptor.data(), &ImageDescriptor::unloaded, this, [=](bool compactData) {
            if (compactData) {
                d->compactlyLoaded.removeOne(descriptor);
            } else {
                d->loaded.removeOne(descriptor);
            }
        });
        connect(descriptor.data(), &ImageDescriptor::deleted, this, [=] {
            //Remove this descriptor from the database
            d->descriptors.remove(cleaned);
        });
        return descriptor;
    }
}
