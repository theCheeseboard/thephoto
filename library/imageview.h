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
        QWidget* sidebar();

        void editTransform(QTransform transform, QSize newSize);


    public slots:
        void animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image);
        void close();
        bool nextImage();
        bool previousImage();

        void deleteCurrentImage();
        void editCurrentImage();
        void endEdit(bool save);

    signals:
        void closed();

        void editStarted(QSize imageSize);
        void editEnded();

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
