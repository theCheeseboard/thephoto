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

        void editImage(QImage image);
        void editTransform(QTransform transform);

    public slots:
        void animateImageIn(QRectF location, QRectF sourceRect, ImgDesc image);
        void close();
        bool nextImage();
        bool previousImage();

        void deleteCurrentImage();
        void editCurrentImage();
        void endEdit(QString filename);

        void beginSlideshow();
        void endSlideshow();
        bool inSlideshow();

    private slots:
        void escPressed();

    signals:
        void closed();
        void slideshowModeChanged(bool inSlideshow);

        void editStarted(QImage originalImage);
        void editEnded();

    private:
        Ui::ImageView *ui;

        ImageViewPrivate* d;

        void paintEvent(QPaintEvent* event);
        void resizeEvent(QResizeEvent* event);
        void mousePressEvent(QMouseEvent* event);

        QRectF calculateEndRect(QSize size);
        void loadImage(ImgDesc image);
};

#endif // IMAGEVIEW_H
