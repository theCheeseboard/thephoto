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
        explicit ImageDescriptor(QString filename);
        ~ImageDescriptor();

        bool isLoaded();
        tPromise<void>* load();

        QImage image();
        QDateTime dateTaken();

    signals:
        void loaded();

    private:
        ImageDescriptorPrivate* d;

};
typedef QSharedPointer<ImageDescriptor> ImgDesc;

#endif // IMAGEDESCRIPTOR_H
