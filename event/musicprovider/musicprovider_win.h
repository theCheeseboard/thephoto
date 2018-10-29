#ifndef MUSICPROVIDER_WIN_H
#define MUSICPROVIDER_WIN_H

#include <QObject>

class MusicProvider : public QObject
{
        Q_OBJECT
    public:
        explicit MusicProvider(QObject *parent = nullptr);

        QString getMusicString();

    signals:

    public slots:
};

#endif // MUSICPROVIDER_WIN_H
