#ifndef IMAGEDESCRIPTORMANAGER_H
#define IMAGEDESCRIPTORMANAGER_H

#include <QObject>
#include "imagedescriptor.h"

struct ImageDescriptorManagerPrivate;
class ImageDescriptorManager : public QObject
{
        Q_OBJECT
    public:
        explicit ImageDescriptorManager(QObject *parent = nullptr);

        ImgDesc descriptorForFile(QString filename);

    signals:

    private:
        ImageDescriptorManagerPrivate* d;
};

#endif // IMAGEDESCRIPTORMANAGER_H
