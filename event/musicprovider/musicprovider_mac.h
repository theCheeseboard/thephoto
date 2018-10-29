#ifndef MUSICPROVIDER_MAC_H
#define MUSICPROVIDER_MAC_H

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

#endif // MUSICPROVIDER_MAC_H
