#include "importdialog.h"
#include "ui_importdialog.h"

importDialog::importDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::importDialog)
{
    ui->setupUi(this);

    QDir::home().mkdir(".thephoto");
    QDir(QDir::homePath() + "/.thephoto/").mkdir("mnt");
    workDir = QDir::homePath() + "/.thephoto/mnt/";

    ui->flag->setPixmap(QIcon::fromTheme("flag").pixmap(24, 24));
    ui->backButton->setVisible(false);

    changeStage(Device);
}

importDialog::~importDialog()
{
    delete ui;
}

void importDialog::changeStage(stages stage) {
    currentStage = stage;

    /*switch (stage) {
    case Device:
        ui->deviceFrame->setVisible(true);
        break;
    case Photo:
        ui->photoFrame->setVisible(true);
    }*/
    ui->stackedWidget->setCurrentIndex(stage);

    setupStage(stage);

    if (stage == 0) {
        ui->backButton->setVisible(false);
    } else {
        ui->backButton->setVisible(true);
    }
    ui->progressThroughInstaller->setValue(stage);
}

void importDialog::setupStage(stages stage) {
    if (stage == Device) {
        ui->deviceList->clear();
        ui->forwardButton->setEnabled(false);
        if (QFile("/usr/bin/ifuse").exists() && QFile("/usr/bin/idevicepair").exists() && QFile("/usr/bin/idevice_id").exists()) {
            //Detect iOS Devices

            QProcess* iosDev = new QProcess();
            iosDev->start("idevice_id -l");
            iosDev->waitForStarted();
            iosDev->waitForFinished();

            QString output(iosDev->readAll());
            for (QString line : output.split("\n")) {
                if (line != "") {
                    if (!line.startsWith("ERROR:")) {
                        QListWidgetItem* item = new QListWidgetItem();
                        QProcess* iosName = new QProcess();
                        iosName->start("idevice_id " + line);
                        iosName->waitForFinished();

                        QString name(iosName->readAll());
                        name = name.trimmed();

                        if (name == "") {
                            item->setText("iOS Device");
                        } else {
                            item->setText(name + " (iOS)");
                        }

                        item->setIcon(QIcon::fromTheme("smartphone"));
                        item->setData(Qt::UserRole, "ios");
                        item->setData(Qt::UserRole + 1, line);
                        ui->deviceList->addItem(item);
                    }
                }
            }
        }


        if (QFile("/usr/bin/jmtpfs").exists()) {
            //Detect MTP devices
            QProcess* mtpDev = new QProcess(this);
            mtpDev->start("jmtpfs -l");
            mtpDev->waitForStarted();


            while (mtpDev->state() == QProcess::Running) {
                QApplication::processEvents();
            }
            QString output(mtpDev->readAll());
            bool startReading = false;
            for (QString line : output.split("\n")) {
                if (line != "") {
                    if (startReading) {
                        QStringList parse = line.split(", "); //busLocation, devNum, productId, vendorId, product, vendor
                        QListWidgetItem* item = new QListWidgetItem();
                        QString text = parse.at(5) + " " + parse.at(4);
                        if (!text.contains("(MTP)")) {
                            text += " (MTP)";
                        }
                        item->setText(parse.at(5) + " " + parse.at(4));
                        item->setIcon(QIcon::fromTheme("smartphone"));
                        item->setData(Qt::UserRole, "mtp");
                        item->setData(Qt::UserRole + 1, parse.at(0));
                        item->setData(Qt::UserRole + 2, parse.at(1));
                        ui->deviceList->addItem(item);
                    } else {
                        if (line.startsWith("Available devices")) {
                            startReading = true;
                        }
                    }
                }
            }

        }
    } else if (stage == Photo) {
        workDir = QDir::homePath() + "/.thephoto/mnt/";
        foundImagePaths.clear();
        ui->detectedPhotoList->clear();

        QProcess::execute("fusermount -u " + workDir);
        ui->photoProgressFrame->setVisible(true);
        ui->photoProgressFrame->raise();
        ui->photoErrorFrame->setVisible(false);
        QListWidgetItem* item = ui->deviceList->selectedItems().first();
        QString dev = item->data(Qt::UserRole).toString();
        if (dev == "mtp") {
            qDebug() << "Mounting MTP device " + item->data(Qt::UserRole + 1).toString() + ", " + item->data(Qt::UserRole + 2).toString();

            QProcess *mountProcess = new QProcess(this);

            mountProcess->start("jmtpfs " + workDir + " -device=" + item->data(Qt::UserRole + 1).toString() + "," + item->data(Qt::UserRole + 2).toString());
            mountProcess->waitForStarted();

            while (mountProcess->state() == QProcess::Running) {
                QApplication::processEvents();
            }

            if (mountProcess->exitCode() != 0) {
                photoError("An error occurred.");
                return;
            }
        } else if (dev == "ios") { //iOS Device
            QString id = item->data(Qt::UserRole + 1).toString();
            qDebug() << "Mounting iOS Device " + id;

            QProcess checkProcess;
            checkProcess.start("idevicepair -u " + id + " validate");
            checkProcess.waitForFinished();


            if (checkProcess.readAll().startsWith("ERROR")) {
                QProcess pairProcess;
                pairProcess.start("idevicepair -u " + id + " pair");
                pairProcess.waitForStarted();

                while (pairProcess.state() == QProcess::Running) {
                    QApplication::processEvents();
                }

                QString pairOutput(pairProcess.readAll());
                if (pairOutput.startsWith("ERROR:")) {
                    if (pairOutput.contains("Please enter the passcode")) { // Ask user to unlock device
                        photoError("To continue, unlock the device.");
                    } else if (pairOutput.contains("Please accept the trust dialog")) { //Ask user to trust PC
                        photoError("To continue, trust this PC on the device.");
                    } else if (pairOutput.contains("user denied the trust dialog")) { //User did not trust PC
                        photoError("You clicked the \"Don't Trust\" button on your device, so we can't access it.");
                    } else { //Generic Error
                        photoError("An error occurred.");
                    }
                    return;
                }
            }

            QString iosDirName = "ios" + id;
            QDir::home().mkdir(".thefile");
            QDir(QDir::homePath() + "/.thefile").mkdir(iosDirName);

            QProcess mountProcess;

            bool mounted = false;
            for (QString file : QDir(QDir::homePath() + "/.thefile/" + iosDirName).entryList()) {
                if (file != "." && file != "..") {
                    mounted = true;
                }
            }
            if (!mounted) {
                mountProcess.start("ifuse -o ro " + workDir + " -u " + id);
                mountProcess.waitForStarted();

                while (mountProcess.state() == QProcess::Running) {
                    QApplication::processEvents();
                }

                if (mountProcess.exitCode() != 0) {
                    photoError("An error occurred.");
                    return;
                }
            }

            checkDcimFolder:
            QDir work(workDir);
            if (work.exists()) {
                if (!work.cd("DCIM")) {
                    QInputDialog* dialog = new QInputDialog(this);
                    dialog->setComboBoxEditable(false);
                    dialog->setLabelText("What medium do you want to access the photos from?");
                    QStringList comboItems;
                    if (QDir(work).cd("Card")) {
                        comboItems.append("Card");
                    }
                    if (QDir(work).cd("Phone")) {
                        comboItems.append("Phone");
                    }
                    if (QDir(work).cd("Internal Storage")) {
                        comboItems.append("Internal Storage");
                    }

                    if (comboItems.count() == 0) {
                        photoError("Can't find any images on the device.");
                    } else {
                        dialog->setComboBoxItems(comboItems);
                        dialog->exec();
                        if (dialog->result() == QDialog::Accepted) {
                            int value = dialog->exec();
                            work.cd(dialog->comboBoxItems().at(value));
                            workDir = work.dirName();

                            goto checkDcimFolder; //Retry the loop
                        } else {
                            photoError("We don't know where to get the photos.");
                            return;
                        }
                    }
                }
            } else {
                photoError("Can't access the device. You may need to allow MTP access on the device.");
                return;
            }

            QDirIterator iterator(work, QDirIterator::Subdirectories);
            while (iterator.hasNext()) {
                QString file = iterator.next();
                if (iterator.fileName() != "." && iterator.fileName() != "..") {
                    if (QFile(file).exists()) {
                        foundImagePaths.append(file);

                        QListWidgetItem* item = new QListWidgetItem();
                        item->setText(QFileInfo(file).fileName());
                        item->setIcon(QIcon::fromTheme("image"));
                        ui->detectedPhotoList->addItem(item);
                    }
                }
            }

            ui->photoProgressFrame->setVisible(false);
            ui->photoSelectFrame->setVisible(true);
            ui->photoSelectFrame->raise();
        }
    } else if (stage == Folder) {

    } else if (stage == Confirm) {
        if (ui->photoLocationName->isChecked()) {
            copyDir = QDir::homePath() + "/thePhoto/" + ui->folderName->text();
        } else {
            copyDir = ui->folderPath->text();
        }
    } else if (stage == Import) {
        //Begin import
        ui->forwardButton->setVisible(false);
        ui->backButton->setVisible(false);
        ui->quitButton->setVisible(false);


    }
}

void importDialog::on_forwardButton_clicked()
{
    changeStage(stages(currentStage + 1));
}

void importDialog::on_backButton_clicked()
{
    changeStage(stages(currentStage - 1));
}

void importDialog::on_quitButton_clicked()
{
    this->close();
}

void importDialog::close() {
    if (QDir(workDir).entryList().count() == 2) { //.. and . entries are ignored
        QDir(workDir).removeRecursively();
    } else {
        if (QProcess::execute("fusermount -u " + workDir) == 0) {
            QDir(workDir).removeRecursively();
        } else {
            QMessageBox::critical(this, "Can't unmount", "We couldn't unmount the device.", QMessageBox::Ok, QMessageBox::Ok);
        }
    }
    QDialog::close();
}

void importDialog::on_deviceList_currentItemChanged(QListWidgetItem *current, QListWidgetItem *previous)
{
    if (current == NULL) {
        ui->forwardButton->setEnabled(false);
    } else {
        ui->forwardButton->setEnabled(true);
    }
}

void importDialog::on_pushButton_clicked()
{
    setupStage(Photo);
}

void importDialog::photoError(QString message) {
    ui->photoErrorFrame->setVisible(true);
    ui->photoProgressFrame->setVisible(false);
    ui->photoError->setText(message);
}


void importDialog::on_photoLocationName_toggled(bool checked)
{
    if (checked) {
        ui->photoLocationSelectName->setEnabled(true);
        ui->photoLocationSelectFolder->setEnabled(false);
    }
}

void importDialog::on_photoLocationFolder_toggled(bool checked)
{
    if (checked) {
        ui->photoLocationSelectName->setEnabled(false);
        ui->photoLocationSelectFolder->setEnabled(true);
    }
}

void importDialog::on_browseForFolderButton_clicked()
{
    QFileDialog* dialog = new QFileDialog;
    dialog->setAcceptMode(QFileDialog::AcceptOpen);
    dialog->setFileMode(QFileDialog::DirectoryOnly);
    if (dialog->exec() == QFileDialog::Accepted) {
        ui->folderPath->setText(dialog->selectedFiles().first());
    }
}
