#ifndef MUSICPROVIDER_LINUX_H
#define MUSICPROVIDER_LINUX_H

#include <QObject>
#include <QVariantMap>

class MusicProvider : public QObject
{
        Q_OBJECT
    public:
        explicit MusicProvider(QObject *parent = nullptr);

        QString getMusicString();
    signals:

    private slots:
        void changeService(QString sevice);
        void updateData(QVariantMap data);
        void updateMpris(QString interfaceName, QMap<QString, QVariant> properties, QStringList changedProperties);

    private:
        QString currentService;
        QStringList knownServices;
        QString musicString;
};

#endif // MUSICPROVIDER_LINUX_H
