#ifndef ONLINEDBPLUGINMANAGER_H
#define ONLINEDBPLUGINMANAGER_H

#include <QVector>
#include <QMutex>
#include <QObject>
#include "Data.h"
#include "OnlineDBPlugin.h"

/* This class is a plugin manager allowing other modules to get a working
 * plugin if there is one available plugin.
 * Enabling a plugin is made with qmake options.
 */
class OnlineDBPluginManager : public QObject
{
    Q_OBJECT

public:
    /* -----------------------------
     *          PUBLIC API
     * -----------------------------
     */
    void registerAllPlugins();
    QVector<OnlineDBPlugin*> getAvailablesPlugins();
    OnlineDBPlugin *getDefaultPlugin();


    /* -----------------------------
     *          SINGLETON
     * -----------------------------
     */
    static OnlineDBPluginManager* instance()
    {
        static QMutex mutexinst;
        if (!_instance)
        {
            mutexinst.lock();

            if (!_instance)
                _instance = new OnlineDBPluginManager;

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
    QMutex mutex;
    QVector<OnlineDBPlugin*> plugins;

    /* Singleton pattern */
    static OnlineDBPluginManager* _instance;
    OnlineDBPluginManager(QObject *parent = 0);
    OnlineDBPluginManager(const OnlineDBPluginManager &);
    OnlineDBPluginManager& operator=(const OnlineDBPluginManager &);
    ~OnlineDBPluginManager();
};

#endif // ONLINEDBPLUGINMANAGER_H
