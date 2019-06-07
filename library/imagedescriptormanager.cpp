#include "imagedescriptormanager.h"
#include <QHash>
#include <QDir>

struct ImageDescriptorManagerPrivate {
    QHash<QString, ImgDesc> descriptors;
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
        return descriptor;
    }
}
