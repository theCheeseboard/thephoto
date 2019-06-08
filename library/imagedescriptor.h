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
        bool isCompactLoaded();
        tPromise<void>* load(bool compactData);

        QPixmap image();
        QPixmap compactImage();
        QDateTime dateTaken();

    signals:
        void loaded(bool compactData);

    private:
        ImageDescriptorPrivate* d;

};
typedef QSharedPointer<ImageDescriptor> ImgDesc;

#endif // IMAGEDESCRIPTOR_H
