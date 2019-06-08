#include "librarywindow.h"
#include "ui_librarywindow.h"

#include <QMimeDatabase>
#include <QDirIterator>

#include <tcsdtools.h>
#include <tpopover.h>
#include <tpromise.h>
#include "managelibrary.h"
#include "library/imageview.h"

#include "event/eventmodesettings.h"

struct LibraryWindowPrivate {
    tCsdTools csd;
    QWidget* csdBox;

    QStringList imageLibrary;
};

LibraryWindow::LibraryWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LibraryWindow)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentAnimation(tStackedWidget::Fade);
    ui->loaderSpinner->setFixedSize(SC_DPI_T(QSize(32, 32), QSize));

    d = new LibraryWindowPrivate();
    d->csdBox = d->csd.csdBoxForWidget(this);
    if (tCsdGlobal::windowControlsEdge() == tCsdGlobal::Right) {
        static_cast<QBoxLayout*>(ui->headerWidget->layout())->addWidget(d->csdBox);
    } else {
        static_cast<QBoxLayout*>(ui->headerWidget->layout())->insertWidget(0, d->csdBox);
    }

    d->csd.installResizeAction(this);
    d->csd.installMoveAction(ui->headerWidget);


    #ifdef Q_OS_MAC
        ui->menuButton->setVisible(false);
    #else
        ui->menubar->setVisible(false);
        ui->menuButton->setIconSize(SC_DPI_T(QSize(24, 24), QSize));

        QMenu* menu = new QMenu();
        menu->addAction(ui->actionManage_Library);
        menu->addSeparator();
        menu->addAction(ui->actionEvent_Mode);
        menu->addSeparator();
        menu->addAction(ui->actionExit);
        ui->menuButton->setMenu(menu);
    #endif

    loadLibrary();
}

LibraryWindow::~LibraryWindow()
{
    delete ui;
    delete d;
}

void LibraryWindow::on_actionExit_triggered()
{
    this->close();
}

void LibraryWindow::resizeEvent(QResizeEvent *event) {

}


void LibraryWindow::on_actionManage_Library_triggered()
{
    QEventLoop loop;
    ManageLibrary* libWindow = new ManageLibrary(this);
    libWindow->setWindowModality(Qt::WindowModal);
    connect(libWindow, &ManageLibrary::finished, &loop, &QEventLoop::quit);

    libWindow->show();
    loop.exec();

    //if (libWindow->result() == QDialog::Accepted) {
        loadLibrary();
    //}
}

void LibraryWindow::on_manageLibraryButton_clicked()
{
    ui->actionManage_Library->trigger();
}

void LibraryWindow::loadLibrary() {
    ui->stackedWidget->setCurrentWidget(ui->loadingPage);

    (new tPromise<QStringList>([=](QString& error) {
        QThread::sleep(1);
        QStringList imageLibrary;

        QSettings settings;
        settings.beginGroup("Library");
        int itemsInLibrary = settings.beginReadArray("library");
        if (itemsInLibrary == 0) {
            settings.endArray();
            error = "noimages";
            return QStringList();
        } else {
            QMimeDatabase mimedb;
            for (int i = 0; i < itemsInLibrary; i++) {
                settings.setArrayIndex(i);
                QDir libraryDir(settings.value("path").toString());
                QDirIterator iterator(libraryDir, QDirIterator::Subdirectories);
                while (iterator.hasNext()) {
                    QFile file(iterator.next());
                    QFileInfo fileinfo(file);
                    if (QImageReader::supportedMimeTypes().contains(mimedb.mimeTypeForFile(fileinfo).name().toUtf8())) { //This is an image file
                        imageLibrary.append(file.fileName());
                    }
                }
            }
            settings.endArray();
        }

        if (imageLibrary.count() == 0) {
            error = "noimages";
            return QStringList();
        } else {
            return imageLibrary;
        }
    }))->then([=](QStringList imageLibrary) {
        d->imageLibrary = imageLibrary;
        ui->libraryPage->loadImages(imageLibrary);
        ui->stackedWidget->setCurrentWidget(ui->libraryPage);
    })->error([=](QString error) {
        if (error == "noimages") {
            ui->stackedWidget->setCurrentWidget(ui->noImagesPage);
        }
    });
}

void LibraryWindow::on_actionEvent_Mode_triggered()
{
    this->hide();

    if (QStyleFactory::keys().contains("Contemporary") || QStyleFactory::keys().contains("contemporary")) {
        QApplication::setStyle("Contemporary");
    }

    QPalette oldPal = QApplication::palette();

    EventModeSettings* dialog = new EventModeSettings();
    QApplication::setPalette(dialog->palette());
    dialog->show();
    dialog->exec();

    connect(dialog, &EventModeSettings::done, [=] {
        dialog->deleteLater();

        QApplication::setPalette(oldPal);
        this->setPalette(oldPal);

        if (QStyleFactory::keys().contains("Contemporary") || QStyleFactory::keys().contains("contemporary")) {
            #ifdef Q_OS_WIN
                QApplication::setStyle("windowsvista");
            #elif defined(Q_OS_MAC)
                QApplication::setStyle("macintosh");
            #endif
        }

        this->show();
    });
}

void LibraryWindow::on_libraryPage_imageClicked(const QRectF& location, const QRectF& sourceRect, const ImgDesc& image)
{
    ImageView* view = new ImageView();
    ui->libraryPage->setOverlayWidget(view);

    QTimer::singleShot(0, [=] {
        view->animateImageIn(location, sourceRect, image);
    });
}
