#include "librarywindow.h"
#include "ui_librarywindow.h"

#include <QMimeDatabase>
#include <QDirIterator>
#include <QDesktopServices>

#include <tcsdtools.h>
#include <tpopover.h>
#include <tpromise.h>
#include <taboutdialog.h>
#include "managelibrary.h"
#include "library/imageview.h"

#include "event/eventmodesettings.h"

struct LibraryWindowPrivate {
    tCsdTools csd;
    QWidget* csdBox;
    QPointer<ImageView> overlayView;

    QStringList imageLibrary;
    Qt::WindowStates stateBeforeFullScreen;
};

LibraryWindow::LibraryWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::LibraryWindow)
{
    ui->setupUi(this);

    ui->stackedWidget->setCurrentAnimation(tStackedWidget::Fade);
    ui->headerBar->setCurrentAnimation(tStackedWidget::Fade);
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
        ui->headerBar->installEventFilter(this);

        setupMacOS();
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

    QMenu* deleteMenu = new QMenu();
    deleteMenu->addSection(tr("Delete Image?"));
    deleteMenu->addAction(QIcon::fromTheme("edit-delete", QIcon(":/icons/edit-delete.svg")), tr("Delete"), [=] {
        d->overlayView->deleteCurrentImage();
    });
    ui->deleteButton->setMenu(deleteMenu);

    QMenu* saveMenu = new QMenu();
    saveMenu->addSection(tr("Save Edits"));
    saveMenu->addAction(tr("Save In Place"), [=] {
        d->overlayView->endEdit(":INPLACE");
    });
    saveMenu->addAction(tr("Save As New"), [=] {

    })->setEnabled(false); //TODO: Implement this
    saveMenu->addSeparator();
    saveMenu->addAction(QIcon::fromTheme("document-save-as", QIcon(":/icons/document-save-as.svg")), tr("Save As..."), [=] {
        QFileDialog* fileDialog = new QFileDialog(this);
        fileDialog->setAcceptMode(QFileDialog::AcceptSave);
        fileDialog->setNameFilters({
            tr("PNG image (*.png)"),
            tr("JPEG image (*.jpg)")
        });
        fileDialog->setWindowFlag(Qt::Sheet);
        connect(fileDialog, &QFileDialog::fileSelected, this, [=](QString file) {
            d->overlayView->endEdit(file);
        });
        connect(fileDialog, &QFileDialog::finished, fileDialog, &QFileDialog::deleteLater);
        fileDialog->open();
    });
    ui->editSaveButton->setMenu(saveMenu);

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
    QWidget* overlayWidget = new QWidget();
    QBoxLayout* overlayLayout = new QBoxLayout(QBoxLayout::LeftToRight);
    overlayLayout->setContentsMargins(0, 0, 0, 0);
    overlayLayout->setSpacing(0);
    overlayWidget->setLayout(overlayLayout);

    d->overlayView = new ImageView();
    d->overlayView->setImageGrid(ui->libraryPage);
    connect(d->overlayView.data(), &ImageView::destroyed, overlayWidget, &QWidget::deleteLater);
    overlayLayout->addWidget(d->overlayView);
    overlayLayout->addWidget(d->overlayView->sidebar());

    ui->libraryPage->setOverlayWidget(overlayWidget);

    ui->headerBar->setCurrentWidget(ui->imageHeader);

    connect(d->overlayView.data(), &ImageView::closed, this, [=] {
        ui->headerBar->setCurrentWidget(ui->mainHeader);
    });
    connect(d->overlayView.data(), &ImageView::editStarted, this, [=] {
        ui->headerBar->setCurrentWidget(ui->editImageHeader);
    });
    connect(d->overlayView.data(), &ImageView::editEnded, this, [=] {
        ui->headerBar->setCurrentWidget(ui->imageHeader);
    });
    connect(d->overlayView.data(), &ImageView::slideshowModeChanged, this, [=](bool inSlideshow) {
        this->setSlideshowMode(inSlideshow);

        //Change the window state
        if (inSlideshow) {
            d->stateBeforeFullScreen = this->windowState();
            this->showFullScreen();
        } else {
            this->setWindowState(d->stateBeforeFullScreen);
        }
    });

    QTimer::singleShot(0, [=] {
        d->overlayView->animateImageIn(location, sourceRect, image);
    });
}

void LibraryWindow::on_backButton_clicked()
{
    if (!d->overlayView.isNull()) {
        d->overlayView->close();
    }
}

bool LibraryWindow::eventFilter(QObject *watched, QEvent *event) {
#ifdef Q_OS_MAC
    if (watched == ui->headerBar) {
        if (event->type() == QEvent::MouseButtonPress) {
            macOSDrag(static_cast<QMouseEvent*>(event));
        }
    }
#endif
    return false;
}

void LibraryWindow::on_editButton_clicked()
{
    d->overlayView->editCurrentImage();
}

void LibraryWindow::on_editBackButton_clicked()
{
    d->overlayView->endEdit("");
}

void LibraryWindow::on_startSlideshowFromImage_clicked()
{
    d->overlayView->beginSlideshow();
}

void LibraryWindow::setSlideshowMode(bool slideshow) {
    tVariantAnimation* anim = new tVariantAnimation();
    anim->setStartValue(ui->headerWidget->height());
    if (slideshow) {
        anim->setEndValue(0);
    } else {
        anim->setEndValue(ui->headerWidget->sizeHint().height());
    }
    anim->setDuration(500);
    anim->setEasingCurve(QEasingCurve::OutCubic);
    connect(anim, &tVariantAnimation::valueChanged, this, [=](QVariant value) {
        ui->headerWidget->setFixedHeight(value.toInt());
    });
    connect(anim, &tVariantAnimation::finished, anim, &tVariantAnimation::deleteLater);
    anim->start();
}

void LibraryWindow::changeEvent(QEvent *event) {
    if (event->type() == QEvent::WindowStateChange) {
        if ((this->windowState() & Qt::WindowFullScreen) == 0 && !d->overlayView.isNull() && d->overlayView->inSlideshow()) {
            //We're leaving full screen mode so end the slideshow
            //Don't re-enter full screen after we end the slideshow
            d->stateBeforeFullScreen &= ~Qt::WindowFullScreen;
            d->overlayView->endSlideshow();
        }
    }
}

void LibraryWindow::on_actionFile_Bug_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/vicr123/thePhoto/issues"));
}

void LibraryWindow::on_actionSources_triggered()
{
    QDesktopServices::openUrl(QUrl("https://github.com/vicr123/thePhoto"));
}

void LibraryWindow::on_actionAbout_triggered()
{
    tAboutDialog aboutDialog(this);
    aboutDialog.exec();
}
