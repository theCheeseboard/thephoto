#ifndef IMAGEDESCRIPTOR_H
#define IMAGEDESCRIPTOR_H

#include <QSharedPointer>
#include <QImage>
#include <tpromise.h>

struct ImageDescriptorPrivate;
class ImageDescriptor : public QObject
{
        Q_OBJECT
    public:
        enum LoadStatus {
            NotLoaded,
            Loading,
            Loaded
        };

        explicit ImageDescriptor(QString filename);
        ~ImageDescriptor();

        ImageDescriptor::LoadStatus isLoaded();
        ImageDescriptor::LoadStatus isCompactLoaded();

        tPromise<void>* load(bool compactData);
        void unload(bool compactData);

        QPixmap image();
        QPixmap compactImage();
        QDateTime dateTaken();

    signals:
        void loaded(bool compactData);
        void unloaded(bool compactData);

    private:
        ImageDescriptorPrivate* d;

};
typedef QSharedPointer<ImageDescriptor> ImgDesc;

#endif // IMAGEDESCRIPTOR_H
