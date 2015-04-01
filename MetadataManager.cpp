#include <QStringList>

#include "MetadataManager.h"
#include "FlacPlugin.h"

#ifdef EMS_PLUGIN_GRACENOTE
#include "GracenotePlugin.h"
#endif

MetadataManager* MetadataManager::_instance = 0;

void MetadataManager::registerAllPlugins()
{
    mutex.lock();

    /* File format plugins */
    plugins.append(new FlacPlugin);

    /* Optional plugins. Activate them with qmake options (see .pro) */
#ifdef EMS_PLUGIN_GRACENOTE
    plugins.append(new GracenotePlugin);
#endif

    mutex.unlock();
}

QVector<MetadataPlugin*> MetadataManager::getAvailablePlugins(QString capability)
{
    QVector<MetadataPlugin*> out;

    mutex.lock();
    foreach (MetadataPlugin* plugin, plugins)
    {
        foreach (QString capPlug, plugin->getCapabilities())
        {
            if (capability == capPlug)
            {
                out.append(plugin);
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
