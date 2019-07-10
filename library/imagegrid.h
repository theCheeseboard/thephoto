#ifndef IMAGEGRID_H
#define IMAGEGRID_H

#include <QWidget>
#include "imagedescriptor.h"

struct ImageGridPrivate;
class ImageGrid : public QWidget
{
        Q_OBJECT
    public:
        explicit ImageGrid(QWidget *parent = nullptr);
        ~ImageGrid();

        ImgDesc nextImage(ImgDesc image);
        ImgDesc prevImage(ImgDesc image);
        ImgDesc firstImage();
        ImgDesc lastImage();

    signals:
        void imageClicked(QRectF location, QRectF sourceRect, ImgDesc image);

    public slots:
        void loadImages(QStringList images);
        void setOverlayWidget(QWidget* overlayWidget);

    private:
        ImageGridPrivate* d;

        void paintEvent(QPaintEvent* event);
        void wheelEvent(QWheelEvent* event);
        void resizeEvent(QResizeEvent* event);
        void mousePressEvent(QMouseEvent* event);
        void mouseReleaseEvent(QMouseEvent* event);

        void relocate();
};

#endif // IMAGEGRID_H
