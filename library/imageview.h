#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include "imagedescriptor.h"

namespace Ui {
    class ImageView;
}

struct ImageViewPrivate;
class ImageGrid;
class ImageView : public QWidget
{
        Q_OBJECT

    public:
        explicit ImageView(QWidget *parent = nullptr);
        ~ImageView();

        void setImageGrid(ImageGrid* grid);

    public slots:
        void animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image);
        void close();
        void nextImage();
        void previousImage();

    signals:
        void closed();

    private:
        Ui::ImageView *ui;

        ImageViewPrivate* d;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
        void mousePressEvent(QMouseEvent* event);

        QRectF calculateEndRect();
        void loadImage(ImgDesc image);
};

#endif // IMAGEVIEW_H
