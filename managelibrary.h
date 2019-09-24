#ifndef MANAGELIBRARY_H
#define MANAGELIBRARY_H

#include <QDialog>
#include <QFileDialog>
#include <QSettings>

namespace Ui {
class ManageLibrary;
}

class ManageLibrary : public QDialog
{
    Q_OBJECT

public:
    explicit ManageLibrary(QWidget *parent = 0);
    ~ManageLibrary();

private slots:
    void on_backButton_clicked();

    void on_addButton_clicked();

    void on_removeButton_clicked();

    private:
    Ui::ManageLibrary *ui;

    QStringList libraryItems;
};

#endif // MANAGELIBRARY_H
