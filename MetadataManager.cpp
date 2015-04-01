#include <QStringList>

#include "MetadataManager.h"

#ifdef EMS_PLUGIN_GRACENOTE
#include "GracenotePlugin.h"
#endif

MetadataManager* MetadataManager::_instance = 0;

void MetadataManager::registerAllPlugins()
{
    mutex.lock();
#ifdef EMS_PLUGIN_GRACENOTE
    plugins.append(new GracenotePlugin);
#endif
    mutex.unlock();
}

QVector<MetadataPlugin*> MetadataManager::getAvailablePlugins(QStringList capabilities)
{
    QVector<MetadataPlugin*> out;

    mutex.lock();
    foreach (QString cap, capabilities)
    {
        foreach (MetadataPlugin* plugin, plugins)
        {
            foreach (QString capPlug, plugin->getCapabilities())
            {
                if (cap == capPlug)
                {
                    out.append(plugin);
                }
            }
        }
    }
    mutex.unlock();

    return out;
}

MetadataManager::MetadataManager(QObject *parent) : QObject(parent)
{

}

MetadataManager::~MetadataManager()
{

}
