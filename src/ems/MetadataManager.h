#ifndef METADATAMANAGER_H
#define METADATAMANAGER_H

#include <QVector>
#include <QMutex>
#include <QObject>
#include <QString>
#include "Data.h"
#include "MetadataPlugin.h"

/* This class is a plugin manager allowing other modules to get a working
 * plugin if there is one available plugin.
 * Enabling a plugin is made with qmake options.
 */
class MetadataManager : public QObject
{
    Q_OBJECT

    /* -----------------------------
     *          ASYNCHRONOUS API
     * -----------------------------
     */
public slots:
    void update(EMSTrack track, QStringList capabilities);

signals:
    void updated(EMSTrack track, bool complete);

public:
    /* -----------------------------
     *          PUBLIC API
     * -----------------------------
     */
    void registerAllPlugins();
    QStringList getAvailableCapabilities();

    /* -----------------------------
     *          SINGLETON
     * -----------------------------
     */
    static MetadataManager* instance()
    {
        static QMutex mutexinst;
        if (!_instance)
        {
            mutexinst.lock();

            if (!_instance)
                _instance = new MetadataManager;

            mutexinst.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutexinst;
        mutexinst.lock();
        delete _instance;
        _instance = 0;
        mutexinst.unlock();
    }

private:
    QVector<MetadataPlugin*> plugins;
    QMutex mutex;

    /* Internal methods */
    QVector<MetadataPlugin*> getAvailablePlugins(QString capability);

    /* Singleton pattern */
    static MetadataManager* _instance;
    MetadataManager(QObject *parent = 0);
    MetadataManager(const MetadataManager &);
    MetadataManager& operator=(const MetadataManager &);
    ~MetadataManager();
};

#endif // METADATAMANAGER_H
