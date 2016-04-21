#include "managelibrary.h"
#include "ui_managelibrary.h"

ManageLibrary::ManageLibrary(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::ManageLibrary)
{
    ui->setupUi(this);

    QSettings settings;
    settings.beginGroup("Library");
    int itemsInLibrary = settings.beginReadArray("library");
    for (int i = 0; i < itemsInLibrary; i++) {
        settings.setArrayIndex(i);
        QListWidgetItem* item = new QListWidgetItem();
        item->setText(settings.value("path").toString());
        item->setIcon(QIcon::fromTheme("folder"));
        ui->listWidget->addItem(item);

        libraryItems.append(item->text());
    }
    settings.endArray();
}

ManageLibrary::~ManageLibrary()
{
    delete ui;
}

void ManageLibrary::on_pushButton_3_clicked()
{
    QFileDialog* folderPicker = new QFileDialog(this);
    folderPicker->setFileMode(QFileDialog::DirectoryOnly);
    folderPicker->setOption(QFileDialog::ShowDirsOnly);
    if (folderPicker->exec() == QDialog::Accepted) {
        libraryItems.append(folderPicker->selectedFiles().at(0));

        QListWidgetItem* item = new QListWidgetItem();
        item->setText(folderPicker->selectedFiles().at(0));
        item->setIcon(QIcon::fromTheme("folder"));
        ui->listWidget->addItem(item);
    }
}

void ManageLibrary::on_pushButton_2_clicked()
{
    this->reject();
}

void ManageLibrary::on_pushButton_clicked()
{
    QSettings settings;
    settings.beginGroup("Library");
    settings.beginWriteArray("library");
    int i = 0;
    for (QString item : libraryItems) {
        settings.setArrayIndex(i);
        settings.setValue("path", item);
        i++;
    }
    settings.endArray();

    this->accept();
}

void ManageLibrary::on_pushButton_4_clicked()
{
    for (QListWidgetItem* item : ui->listWidget->selectedItems()) {
        int index = ui->listWidget->row(item);
        libraryItems.removeAt(index);
        delete item;
    }
}
