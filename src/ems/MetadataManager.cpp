#include <QStringList>

#include "MetadataManager.h"
#include "FlacPlugin.h"

#ifdef EMS_PLUGIN_GRACENOTE
#include "GracenotePlugin.h"
#endif

MetadataManager* MetadataManager::_instance = 0;

void MetadataManager::update(EMSTrack *track, QStringList capabilities)
{
    /* The order of this list is respected. The plugin are used consecutively. */
    foreach(QString capability, capabilities)
    {
        QVector<MetadataPlugin*> availablePlugin = getAvailablePlugins(capability);
        foreach(MetadataPlugin* plugin, availablePlugin)
        {
            /* Synchronous update, this slot lives in the MetadataManager's thread */
            plugin->lock();
            plugin->update(track);
            plugin->unlock();
            emit updated(track);
        }
    }
}

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

QStringList MetadataManager::getAvailableCapabilities()
{
    QStringList out;

    mutex.lock();
    foreach (MetadataPlugin* plugin, plugins)
    {
        foreach (QString capPlug, plugin->getCapabilities())
        {
            out.append(capPlug);
        }
    }
    mutex.unlock();

    out.sort();
    out.removeDuplicates();
    return out;
}

MetadataManager::MetadataManager(QObject *parent) : QObject(parent)
{

}

MetadataManager::~MetadataManager()
{

}
