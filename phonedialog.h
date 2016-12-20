#ifndef PHONEDIALOG_H
#define PHONEDIALOG_H

#include <QDialog>
#include <QKeyEvent>
#include <QTcpServer>
#include <QTcpSocket>
#include <QNetworkInterface>
#include <QImageReader>
#include <QImage>
#include <QFile>
#include <QDir>
#include <QDateTime>
#include <QLabel>
#include <QKeyEvent>

namespace Ui {
class PhoneDialog;
}

class PhoneDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PhoneDialog(QWidget *parent = 0);
    ~PhoneDialog();

public slots:
    void close();

private slots:
    void newConnection();

    void recenterImage();

    void setScaleFactor(float factor);

    void broadcastNumber();

private:
    Ui::PhoneDialog *ui;

    QTcpServer* server;
    QList<QTcpSocket*> sockets;
    QList<QByteArray> buffers;
    //QTcpSocket* socket = NULL;
    //QByteArray buffer;
    float scaleFactor;

    void loadImage(QString path);
    float calculateScaling(QSize container, QSize image, bool allowLarger = false);
    float calculateScaling(int containerWidth, int containerHeight, int imageWidth, int imageHeight, bool allowLarger = false);

    void keyPressEvent(QKeyEvent* event);
};

#endif // PHONEDIALOG_H
