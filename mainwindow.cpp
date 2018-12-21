#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "aboutdialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    //ui->graphicsView->installEventFilter(this);
    ui->scrollArea->installEventFilter(this);

    ui->criticalIcon->setPixmap(QIcon::fromTheme("dialog-error", QApplication::style()->standardIcon(QStyle::SP_MessageBoxCritical)).pixmap(32, 32));
    ui->mainToolBar->addWidget(ui->imageNameLabel);

    QStringList arguments = QApplication::arguments();
    arguments.removeFirst();
    if (arguments.count() == 0) {
        reloadLibrary();
        loadImage(0);
    } else {
        ui->noItemsInLibFrame->setVisible(false);
        ui->scrollArea->setVisible(true);

        for (QString argument : arguments) {
            QDir argDir(QFileInfo(argument).absoluteDir());
            for (QString file : argDir.entryList()){
                if (file != "." && file != "..") {
                    FoundImages.append(argDir.path() + "/" + file);
                }
            }

            loadImage(FoundImages.indexOf(argument));
        }
    }

#ifdef Q_OS_MAC
    //Set up Mac toolbar
    ui->mainToolBar->setVisible(false);
    ui->mainToolBar->setParent(nullptr);

    QList<QMacToolBarItem*> allowedItems;
    macToolbar = new QMacToolBar();

    QMacToolBarItem* zoomIn = new QMacToolBarItem();
    zoomIn->setText(tr("Zoom In"));
    zoomIn->setIcon(QIcon(":/icons/zoom-in.svg"));
    connect(zoomIn, SIGNAL(activated()), this, SLOT(on_actionZoom_In_triggered()));
    allowedItems.append(zoomIn);

    QMacToolBarItem* zoomOut = new QMacToolBarItem();
    zoomOut->setText(tr("Zoom Out"));
    zoomOut->setIcon(QIcon(":/icons/zoom-out.svg"));
    connect(zoomOut, SIGNAL(activated()), this, SLOT(on_actionZoom_Out_triggered()));
    allowedItems.append(zoomOut);

    QMacToolBarItem* zoom100 = new QMacToolBarItem();
    zoom100->setText(tr("Zoom to 100%"));
    zoom100->setIcon(QIcon(":/icons/zoom-out.svg")); //TODO: Create icon for zoom to 100
    connect(zoom100, SIGNAL(activated()), this, SLOT(on_actionZoom_to_100_triggered()));
    allowedItems.append(zoom100);

    QMacToolBarItem* zoomFit = new QMacToolBarItem();
    zoomFit->setText(tr("Fit to Window"));
    zoomFit->setIcon(QIcon(":/icons/zoom-out.svg")); //TODO: Create icon for zoom to fit
    connect(zoomFit, SIGNAL(activated()), this, SLOT(on_actionFit_to_Window_triggered()));
    allowedItems.append(zoomFit);

    QMacToolBarItem* del = new QMacToolBarItem();
    del->setText(tr("Delete"));
    del->setIcon(QIcon(":/icons/edit-delete.svg"));
    connect(del, SIGNAL(activated()), this, SLOT(on_actionDelete_triggered()));
    allowedItems.append(del);

    QMacToolBarItem* slideshow = new QMacToolBarItem();
    slideshow->setText(tr("Start Slideshow"));
    slideshow->setIcon(QIcon(":/icons/media-playback-start.svg"));
    connect(slideshow, SIGNAL(activated()), this, SLOT(on_actionStart_Slideshow_triggered()));
    allowedItems.append(slideshow);

    macToolbar->setItems(allowedItems);
    macToolbar->setAllowedItems(allowedItems);
    macToolbar->attachToWindow(this->windowHandle());

#endif
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::reloadLibrary(bool rebuild) {
    QProgressDialog* dialog = new QProgressDialog(this);
    dialog->setWindowModality(Qt::WindowModal);
    dialog->setWindowTitle(tr("Reading Library..."));
    dialog->setLabelText(tr("<p>We're building your library. Please wait while we do this.</p>"
                         "<p>You can shorten this process by narrowing your library.</p>"));
    dialog->setMaximum(0);

    dialog->show();

    FoundImages.clear();

    QSettings settings;
    settings.beginGroup("Library");
    int itemsInLibrary = settings.beginReadArray("library");
    if (itemsInLibrary == 0) {
        settings.endArray();
        ui->noItemsInLibFrame->setVisible(true);
        //ui->graphicsView->setVisible(false);
        ui->scrollArea->setVisible(false);
    } else {
        ui->noItemsInLibFrame->setVisible(false);
        //ui->graphicsView->setVisible(true);
        ui->scrollArea->setVisible(true);
        QMimeDatabase mimedb;
        for (int i = 0; i < itemsInLibrary; i++) {
            settings.setArrayIndex(i);
            QDir libraryDir(settings.value("path").toString());
            QDirIterator iterator(libraryDir, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                QFile file(iterator.next());
                QFileInfo fileinfo(file);
                if (mimedb.mimeTypeForFile(fileinfo).name().startsWith("image")) { //This is an image file
                    FoundImages.append(file.fileName());
                }
                QApplication::processEvents();
            }
        }
        settings.endArray();
    }

    if (FoundImages.count() == 0) {
        ui->noItemsInLibFrame->setVisible(true);
        //ui->graphicsView->setVisible(false);
        ui->scrollArea->setVisible(false);
    }

    dialog->close();
}

void MainWindow::on_pushButton_clicked()
{
    on_actionManage_Library_triggered();
}

void MainWindow::on_actionManage_Library_triggered()
{
    QEventLoop loop;
    ManageLibrary* libWindow = new ManageLibrary(this);
    libWindow->setWindowModality(Qt::WindowModal);
    connect(libWindow, &ManageLibrary::finished, &loop, &QEventLoop::quit);

    libWindow->show();
    loop.exec();

    if (libWindow->result() == QDialog::Accepted) {
        reloadLibrary(true);
    }
}

void MainWindow::loadImage(int imageIndex) {
    if (FoundImages.count() > imageIndex) {
        currentImage = imageIndex;

        ui->imageNameLabel->setText(QFileInfo(FoundImages.at(imageIndex)).fileName());
        QImageReader reader(FoundImages.at(imageIndex));
        reader.setAutoTransform(true);
        const QImage image = reader.read();
        if (image.isNull()) {
            //Error Error!
        } else {
            ui->imageLabel->setPixmap(QPixmap::fromImage(image));
            setScaleFactor(calculateScaling(ui->scrollArea->size(), image.size()));
        }
    }
}

bool MainWindow::eventFilter(QObject *, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = (QKeyEvent*) event;
        if (keyEvent->key() == Qt::Key_Right) {
            nextImage();
        } else if (keyEvent->key() == Qt::Key_Left) {
            if (currentImage == 0) {
                loadImage(FoundImages.count() - 1);
            } else {
                loadImage(currentImage - 1);
            }
        } else if (keyEvent->key() == Qt::Key_Escape) {
            if (this->isFullScreen()) ui->actionStart_Slideshow->trigger();
        }
    }
    return false;
}

void MainWindow::nextImage() {
    if (currentImage == FoundImages.count() - 1) {
        loadImage(0);
    } else {
        loadImage(currentImage + 1);
    }
}

void MainWindow::resizeEvent(QResizeEvent *event) {
    loadImage(currentImage);
}

float MainWindow::calculateScaling(QSize container, QSize image, bool allowLarger) {
    return calculateScaling(container.width(), container.height(), image.width(), image.height(), allowLarger);
}

float MainWindow::calculateScaling(int containerWidth, int containerHeight, int imageWidth, int imageHeight, bool allowLarger) {
    float heightFactor = 0, widthFactor = 0;
    if (imageHeight > containerHeight || allowLarger) {
        heightFactor = (float) containerHeight / (float) imageHeight;
    }

    if (imageWidth > containerWidth || allowLarger) {
        widthFactor = (float) containerWidth / (float) containerHeight;
    }

    if (heightFactor == 0 && widthFactor == 0) {
        return 1;
    } else if (heightFactor == 0 && widthFactor != 0) {
        return widthFactor;
    } else if (heightFactor != 0 && widthFactor == 0) {
        return heightFactor;
    } else if (heightFactor < widthFactor) {
        return heightFactor;
    } else {
        return widthFactor;
    }
}

void MainWindow::scaleImage(float factor) {
    scaleFactor *= factor;
    ui->imageLabel->resize(scaleFactor * ui->imageLabel->pixmap()->size());
    recenterImage();
}

void MainWindow::setScaleFactor(float factor) {
    scaleFactor = factor;
    ui->imageLabel->resize(scaleFactor * ui->imageLabel->pixmap()->size());
    recenterImage();
}

void MainWindow::resetScaleFactor() {
    scaleFactor = 1;
    ui->imageLabel->resize(ui->imageLabel->pixmap()->size());
    recenterImage();
}

void MainWindow::recenterImage() {
    int newx = 0, newy = 0;
    if (ui->imageLabel->width() < ui->scrollArea->width()) {
        newx = ui->scrollArea->width() / 2 - ui->imageLabel->width() / 2;
    }

    if (ui->imageLabel->height() < ui->scrollArea->height()) {
        newy = ui->scrollArea->height() / 2 - ui->imageLabel->height() / 2;
    }

    ui->imageLabel->move(newx, newy);
}

void MainWindow::on_actionZoom_In_triggered()
{
    scaleImage(1.25);
}

void MainWindow::on_actionZoom_Out_triggered()
{
    scaleImage(0.8);
}

void MainWindow::on_actionZoom_to_100_triggered()
{
    setScaleFactor(1);
}

void MainWindow::on_actionFit_to_Window_triggered()
{
    setScaleFactor(calculateScaling(ui->scrollArea->size(), ui->imageLabel->size(), true));
}

void MainWindow::on_actionStart_Slideshow_triggered()
{
    if (!ui->scrollArea->isVisible()) return; //No pictures so no point doing a slideshow

    if (this->isFullScreen()) {
        //Exit slideshow
#ifdef Q_OS_MAC
        macToolbar->attachToWindow(this->windowHandle());
#else
        ui->mainToolBar->setVisible(true);
#endif
        this->showNormal();
        ui->menuBar->setVisible(true);

        if (slideshowTimer != nullptr) {
            slideshowTimer->stop();
            slideshowTimer->deleteLater();
            slideshowTimer = nullptr;
        }
    } else {
        //Enter slideshow

        this->showFullScreen();
#ifdef Q_OS_MAC
        macToolbar->detachFromWindow();
#elif
        ui->mainToolBar->setVisible(false);
#endif
        ui->menuBar->setVisible(false);

        slideshowTimer = new QTimer(this);
        slideshowTimer->setInterval(10000);
        connect(slideshowTimer, SIGNAL(timeout()), this, SLOT(nextImage()));
        slideshowTimer->start();
    }
}

void MainWindow::on_actionConnect_to_Phone_triggered()
{
    //PhoneDialog* dialog = new PhoneDialog(this);
    //dialog->showFullScreen();

    /*if (QApplication::desktop()->screenCount() == 1) {
        QMessageBox::warning(this, tr("Connect another screen"), tr("Connect another screen to this PC to use Event Mode."), QMessageBox::Ok, QMessageBox::Ok);
    } else {*/
        this->hide();

        if (QStyleFactory::keys().contains("Contemporary")) {
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

            if (QStyleFactory::keys().contains("Contemporary")) {
                #ifdef Q_OS_WIN
                    QApplication::setStyle("windowsvista");
                #elif defined(Q_OS_MAC)
                    QApplication::setStyle("macintosh");
                #endif
            }

            this->show();
        });
    //}
}

void MainWindow::on_actionDelete_triggered()
{
    if (QMessageBox::warning(this, tr("Delete Image"), tr("You're about to delete this image forever. Are you sure?"), QMessageBox::Yes | QMessageBox::No, QMessageBox::No) == QMessageBox::Yes) {
        QFile(FoundImages.at(currentImage)).remove();
        FoundImages.removeAt(currentImage);
        nextImage();
    }
}

void MainWindow::show() {
    QMainWindow::show();
    loadImage(currentImage);
}

void MainWindow::on_actionAbout_triggered()
{
    AboutDialog a;
    a.exec();
}

void MainWindow::on_actionExit_triggered()
{
    QApplication::exit();
}
