#include "OnlineDBPluginManager.h"

#ifdef EMS_PLUGIN_GRACENOTE
#include "GracenotePlugin.h"
#endif

OnlineDBPluginManager* OnlineDBPluginManager::_instance = 0;

void OnlineDBPluginManager::registerAllPlugins()
{
    mutex.lock();
#ifdef EMS_PLUGIN_GRACENOTE
    plugins.append(new GracenotePlugin);
#endif
    mutex.unlock();
}

QVector<OnlineDBPlugin*> OnlineDBPluginManager::getAvailablesPlugins()
{
    QVector<OnlineDBPlugin*> out;
    mutex.lock();
    out = plugins;
    mutex.unlock();
    return out;
}

/* By default, use the first plugin we find */
OnlineDBPlugin *OnlineDBPluginManager::getDefaultPlugin()
{
    mutex.lock();
    if (plugins.size())
    {
        mutex.unlock();
        return plugins.first();
    }
    mutex.unlock();
    return NULL;
}

OnlineDBPluginManager::OnlineDBPluginManager(QObject *parent) : QObject(parent)
{

}

OnlineDBPluginManager::~OnlineDBPluginManager()
{

}
