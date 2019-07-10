#ifndef LIBRARYWINDOW_H
#define LIBRARYWINDOW_H

#include <QMainWindow>
#include "library/imagedescriptor.h"

namespace Ui {
    class LibraryWindow;
}

struct LibraryWindowPrivate;
class LibraryWindow : public QMainWindow
{
        Q_OBJECT

    public:
        explicit LibraryWindow(QWidget *parent = nullptr);
        ~LibraryWindow();

    private slots:
        void on_actionExit_triggered();

        void on_actionManage_Library_triggered();

        void on_manageLibraryButton_clicked();

        void loadLibrary();

        void on_actionEvent_Mode_triggered();

        void on_libraryPage_imageClicked(const QRectF& location, const QRectF& sourceRect, const ImgDesc& image);

        void on_backButton_clicked();

        void on_startSlideshowFromImage_clicked();

    private:
        Ui::LibraryWindow *ui;

#ifdef Q_OS_MAC
        void setupMacOS();
        void macOSDrag(QMouseEvent* event);
#endif

        LibraryWindowPrivate* d;
        void resizeEvent(QResizeEvent* event);
        void changeEvent(QEvent* event);
        bool eventFilter(QObject* watched, QEvent* event);

        void setSlideshowMode(bool slideshow);
};

#endif // LIBRARYWINDOW_H
