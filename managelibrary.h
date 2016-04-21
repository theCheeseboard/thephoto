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
    void on_pushButton_3_clicked();

    void on_pushButton_2_clicked();

    void on_pushButton_clicked();

    void on_pushButton_4_clicked();

private:
    Ui::ManageLibrary *ui;

    QStringList libraryItems;
};

#endif // MANAGELIBRARY_H
