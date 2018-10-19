#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSettings>
#include <QDir>
#include <QDirIterator>
#include <QMimeDatabase>
#include <QMimeData>
#include <QProgressDialog>
#include <QKeyEvent>
#include <QImageReader>
#include <QTimer>
#include "managelibrary.h"
#include "event/eventmodesettings.h"
#include "importdialog.h"

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

public slots:
    void reloadLibrary(bool rebuild = false);

    void loadImage(int imageIndex);

    void nextImage();

    void show();

private slots:
    void on_pushButton_clicked();

    void on_actionManage_Library_triggered();

    void scaleImage(float factor);

    void setScaleFactor(float factor);

    void resetScaleFactor();

    void recenterImage();

    void on_actionZoom_In_triggered();

    void on_actionZoom_Out_triggered();

    void on_actionZoom_to_100_triggered();

    void on_actionFit_to_Window_triggered();

    void on_actionStart_Slideshow_triggered();

    void on_actionConnect_to_Phone_triggered();

    void on_actionImport_from_Phone_triggered();

    void on_actionDelete_triggered();

private:
    Ui::MainWindow *ui;

    QStringList FoundImages;
    int currentImage = 0;
    float scaleFactor;
    QTimer* slideshowTimer = NULL;

    float calculateScaling(QSize container, QSize image, bool allowLarger = false);
    float calculateScaling(int containerWidth, int containerHeight, int imageWidth, int imageHeight, bool allowLarger = false);

    bool eventFilter(QObject *, QEvent * event);
    void resizeEvent(QResizeEvent *event);
};

#endif // MAINWINDOW_H
