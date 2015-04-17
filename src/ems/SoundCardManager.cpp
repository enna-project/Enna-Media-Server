#include <QDebug>
#include <QtDBus>

#include "DefaultSettings.h"
#include "Player.h"
#include "SoundCardManager.h"

#define INTERFACE "com.EnnaMediaServer.SoundCard"
#define SIGNAL_NAME_PLUGGED "plugged"
#define SIGNAL_NAME_UNPLUGGED "unplugged"

/* This file contain the logic for enabling/disabling output soundc card.
 *
 * -----------------------    UDEV AND MPD      ------------------------
 * The udev rules send the device path of the sound card which is plugged
 * using a dbugs signal. This path is formated like [*]/sound/card[ID]
 * The ID means that this card correspond to /sys/class/sound/card[ID]
 * The associated alsa device should be "hw:[ID]".
 *
 * In the MPD configuration are defined "sound cards" handled by MPD.
 * You have to set all the sound card you want to use and all other
 * "output" like streaming, etc. Note the number of each card definition
 * in the MPD configuration file (starting from 1). You also can get
 * this number typing "mpc outputs". In this file, this number is named
 * ID_MPD and correspond to EMSSndCard.id.
 *
 * -----------------------  EMS CONFIGURATION   ------------------------
 * In the Qt configuration file used by EMS (see DefaultSettings.h), you
 * have to set all sound card which should be handled by EMS.
 *
 * All other MPD output will not be changed at all. If the configuration
 * set enable=1, it will always stay enabled.
 *
 * In the EMS configuration, the syntax of each "card" entrie is :
 *
 * [soundcard[X]]
 * id_mpd=[ID_MPD]
 * id_system=[ID]
 * name="[name]"
 * type="[type]"
 * priority=[priority]
 *
 * Where :
 *
 * - X is a number starting from 0. This number is only used to set
 *   multiple values in the configuration file, it means nothing more.
 *   When you add a sound card, use X+1.
 *
 * - ID_MPD is the n'th output definition in the MPD configuration file.
 *
 * - ID is the system sound card number
 *
 * - name is a name (set you want)
 *
 * - type is the kind of output. You can set what you want but only one
 *   output of this type will be enabled at the same time
 *
 * - priority is a number used to know which card should be enabled when
 *   several sound card of the same type is present at the same time.
 */

/* ---------------------------------------------------------
 *               INTERNAL FUNCTIONS - UTILS
 * --------------------------------------------------------- */
void SoundCardManager::applyPolicy()
{
    /* Check if the policy is respected
     * The policy is quite simple :
     * Only one "type" is enabled at the same time
     * and it is the one with the highest priority
     */
    foreach (QString type, getTypeList())
    {
        QVector<EMSSndCard> candidates = getMpdIDbyType(type, true);
        if (candidates.size() <= 0)
        {
            continue;
        }
        /* Find the highest priority */
        int elected = 0;
        for (int i=0; i<candidates.size(); i++)
        {
            if (candidates.at(i).priority > candidates.at(elected).priority)
            {
                elected = i;
            }
        }
        /* MPD will stop the music if no output is enabled
         * so send "enable" before disabling the other */
        /* Send order to enable if needed */
        for (int i=0; i<candidates.size(); i++)
        {
            if (i == elected && !candidates.at(i).enabled)
            {
                qDebug() << "Enabling output " << candidates.at(i).name;
                Player::instance()->enableOutput(candidates.at(i).id_mpd);
            }
        }
        /* Send order to disable if needed */
        for (int i=0; i<candidates.size(); i++)
        {
            if (i != elected && candidates.at(i).enabled)
            {
                qDebug() << "Disabling output " << candidates.at(i).name;
                Player::instance()->disableOutput(candidates.at(i).id_mpd);
            }
        }
    }

    /* Make sure all non-present card are disabled */
    foreach (EMSSndCard card, sndCards)
    {
        if (!card.present && card.enabled)
        {
            qDebug() << "Disabling not present output " << card.name;
            Player::instance()->disableOutput(card.id_mpd);
        }
    }

    configured = true;
}

QStringList SoundCardManager::getTypeList()
{
    QStringList types;

    foreach (EMSSndCard card, sndCards)
    {
        if (!types.contains(card.type))
        {
            types.append(card.type);
        }
    }

    return types;
}

QVector<EMSSndCard> SoundCardManager::getMpdIDbyType(QString type, bool onlyPresent)
{
    QVector<EMSSndCard> out;

    foreach (EMSSndCard card, sndCards)
    {
        if (card.type == type && (!onlyPresent || card.present))
        {
            out.append(card);
        }
    }

    return out;
}

void SoundCardManager::getInitialPresence()
{
    for(int i=0; i<sndCards.size(); i++)
    {
#ifdef Q_OS_LINUX
        // Check the presence of a sound card using sysfs
        if (QFileInfo(QString("/sys/class/sound/card%1").arg(sndCards.at(i).id_system)).exists())
        {
            EMSSndCard card = sndCards.at(i);
            card.present = true;
            qDebug() << "Sound card " << sndCards.at(i).name << " is present";
            sndCards.replace(i, card);
        }
        else
        {
            EMSSndCard card = sndCards.at(i);
            card.present = false;
            qDebug() << "Sound card " << sndCards.at(i).name << " is not present";
            sndCards.replace(i, card);
        }
#else
        // By default, if we are not able to check if this output is present,
        // enable it.
        EMSSndCard card = sndCards.at(i);
        card.present = true;
        sndCards.replace(i, card);
#endif
    }
}

/* ---------------------------------------------------------
 *               SLOTS - DBUS SIGNAL HANDLER
 * --------------------------------------------------------- */
void SoundCardManager::dbusMessagePlugged(QString message)
{
    unsigned int id = 0;
    /* First retrieve system ID from path like :
     * pci0000:00/0000:00:1c.3/0000:05:00.0/usb2/2-1/2-1:1.0/sound/card1
     */
    QStringList parts = message.split('/');
    if (parts.size() <= 0)
    {
        id = message.toUInt();
    }
    else
    {
        QString cardDevName = parts.last();
        if(cardDevName.startsWith("card"))
        {
            cardDevName.remove("card");
        }
        id = cardDevName.toUInt();

    }
    qDebug() << "Sound card plugged : " << message;
    qDebug() << "Sound card system ID is " << QString("%1").arg(id);

    for(int i=0; i<sndCards.size(); i++)
    {
        if (sndCards.at(i).id_system == id)
        {
            EMSSndCard card = sndCards.at(i);
            card.present = true;
            sndCards.replace(i, card);
        }
    }
    applyPolicy();
}

void SoundCardManager::dbusMessageUnplugged(QString message)
{
    unsigned int id = 0;
    /* First retrieve system ID from path like :
     * pci0000:00/0000:00:1c.3/0000:05:00.0/usb2/2-1/2-1:1.0/sound/card1
     */
    QStringList parts = message.split('/');
    if (parts.size() <= 0)
    {
        id = message.toUInt();
    }
    else
    {
        QString cardDevName = parts.last();
        if(cardDevName.startsWith("card"))
        {
            cardDevName.remove("card");
        }
        id = cardDevName.toUInt();

    }
    qDebug() << "Sound card unplugged : " << message;
    qDebug() << "Sound card system ID was " << QString("%1").arg(id);

    for(int i=0; i<sndCards.size(); i++)
    {
        if (sndCards.at(i).id_system == id)
        {
            EMSSndCard card = sndCards.at(i);
            card.present = false;
            sndCards.replace(i, card);
        }
    }
    applyPolicy();
}

void SoundCardManager::mpdOutputsChanged(QVector<EMSSndCard> mpdOutputs)
{
    foreach (EMSSndCard output, mpdOutputs)
    {
        for (int i=0; i<sndCards.size(); i++)
        {
            if (sndCards.at(i).id_mpd == output.id_mpd)
            {
                EMSSndCard card = sndCards.at(i);
                card.enabled = output.enabled;
                sndCards.replace(i, card);
            }
        }
    }

    /* First apply */
    if (!configured)
    {
        applyPolicy();
    }
}

/* ---------------------------------------------------------
 *              CONSTRUCTOR / DESTRUCTOR
 * --------------------------------------------------------- */

SoundCardManager::SoundCardManager(QObject *parent) : QObject(parent), bus(QDBusConnection::systemBus())
{
    QSettings settings;

    qRegisterMetaType<EMSPlayerStatus>("QVector<EMSSndCard>");

    unsigned int i= 0 ;
    bool cont = true;
    while (cont)
    {
        QString group = QString("soundcard%1").arg(i);
        settings.beginGroup(group);
        if (settings.childKeys().size() > 0)
        {
            EMSSndCard sndCard;
            EMS_LOAD_SETTINGS(sndCard.id_mpd, "id_mpd", 1, UInt)
            EMS_LOAD_SETTINGS(sndCard.id_system, "id_system", 0, UInt)
            EMS_LOAD_SETTINGS(sndCard.name, "name", "Default", String)
            EMS_LOAD_SETTINGS(sndCard.type, "type", "main_speaker", String)
            EMS_LOAD_SETTINGS(sndCard.priority, "priority", 0, UInt)
            sndCards.append(sndCard);
        }
        else
        {
            break;
        }
        settings.endGroup();
        i++;
    }

    if (bus.isConnected())
    {
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_PLUGGED, this, SLOT(dbusMessagePlugged(QString)));
        bus.connect(QString(), QString(), INTERFACE , SIGNAL_NAME_UNPLUGGED, this, SLOT(dbusMessageUnplugged(QString)));
    }

    getInitialPresence();

    configured = false;
    connect(Player::instance(), SIGNAL(outputsChanged(QVector<EMSSndCard>)), this, SLOT(mpdOutputsChanged(QVector<EMSSndCard>)));
}

SoundCardManager::~SoundCardManager()
{
    if (bus.isConnected())
    {
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_PLUGGED, this, SLOT(dbusMessagePlugged(QString)));
        bus.disconnect(QString(), QString(), INTERFACE , SIGNAL_NAME_UNPLUGGED, this, SLOT(dbusMessageUnplugged(QString)));
    }
}
