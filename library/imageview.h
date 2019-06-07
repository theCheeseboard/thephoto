#ifndef IMAGEVIEW_H
#define IMAGEVIEW_H

#include <QWidget>
#include "imagedescriptor.h"

namespace Ui {
    class ImageView;
}

struct ImageViewPrivate;
class ImageView : public QWidget
{
        Q_OBJECT

    public:
        explicit ImageView(QWidget *parent = nullptr);
        ~ImageView();

    public slots:
        void animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image);
        void close();

    private:
        Ui::ImageView *ui;

        ImageViewPrivate* d;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
        void mousePressEvent(QMouseEvent* event);

        QRectF calculateEndRect();
};

#endif // IMAGEVIEW_H
