
#include <QDebug>
#include <QDBusConnection>
#include <QListIterator>
#include <QMetaProperty>
#include <service.h>
#include <manager.h>

#include "NetworkCtlConnman.h"


/*
 * Implementation for NetworkCtl class, using qconnman
 * Compiled when EMS_LIB_QCONNMAN=yes
 *
 * qconnman library enables the control of connman dbus API
 * and is required to use the network features in the app
*/

NetworkCtl* NetworkCtl::_instance = 0;

NetworkCtl::NetworkCtl(QObject *parent): QObject(parent)
{
    m_manager = new Manager(this);
    connect(m_manager,SIGNAL(servicesChanged()),this, SIGNAL(wifiListUpdated()));
    connect(this->getTechnology("wifi"),SIGNAL(connectedChanged()),this,SIGNAL(wifiConnectedChanged()));
    connect(this->getTechnology("ethernet"),SIGNAL(connectedChanged()),this,SIGNAL(ethConnectedChanged()));
    m_agent=new Agent("/com/EMS/Connman", m_manager);
    m_enablUpdate = false;
}

static bool wifiSortByStrength(EMSSsid a, EMSSsid b)
{
    return a.getStrength() > b.getStrength();
}

QString NetworkCtl::getStateString(Service::ServiceState state)
{
    switch (state) {
    case Service::ServiceState::UndefinedState:
        return  "undefined";
        break;
    case Service::ServiceState::IdleState:
        return  "idle";
        break;
    case Service::ServiceState::FailureState :
        return  "failure";
        break;
    case Service::ServiceState::AssociationState :
        return  "association";
        break;
    case Service::ServiceState::ConfigurationState :
        return  "configuration";
        break;
    case Service::ServiceState::ReadyState :
        return  "ready";
        break;
    case Service::ServiceState::DisconnectState :
        return  "disconnect";
        break;
    case Service::ServiceState::OnlineState :
        return  "online";
        break;
    default:
        return "unknown state";
    }
}

void NetworkCtl::scanWifi()
{
    if(isWifiPresent())
    {
        this->getTechnology("wifi")->scan();
        qDebug() << "scanning wifi" << endl;
    }

}

QList<EMSSsid> NetworkCtl::getWifiList()
{
    QList<EMSSsid> ssidList;

    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isWifiPresent())
        {
            qDebug()<< "Acquiring wifi list" ;
            foreach (Service *service, m_manager->services())
            {
                if(service->type() == "wifi")
                {
                    QString name;
                    if(!service->name().isEmpty())
                    {
                        name=service->name();
                    }
                    else // manage Hidden SSID
                    {
                        name="hidden SSID";
                    }
                    EMSSsid ssidWifi(service->objectPath().path(),
                                     name,
                                     service->type(),
                                     getStateString(service->state()),
                                     service->strength(),
                                     getSecurityTypeString(toSecurityTypeList(service->security())));
                    ssidList.append(ssidWifi);
                }
            }
            qSort(ssidList.begin(), ssidList.end(), wifiSortByStrength);
        }
    }
    return ssidList;
}

Service* NetworkCtl::getWifiByName(QString wifiName)
{
    Service* serviceRequested = NULL;
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isWifiPresent())
        {
            qDebug() << "Acquiring wifi :"<< wifiName;
            QListIterator<Service*> iter(m_manager->services());
            Service* service;
            bool found = false;
            while(iter.hasNext() && !found)
            {
                service = iter.next();
                if(service->type() == "wifi" && service->name() == wifiName)
                {
                   serviceRequested = service;
                   found = true;
                }
            }
        }
        else
        {
            qDebug() << " No wifi detected ";
        }
    }
    return serviceRequested;
}

Service* NetworkCtl::getEthByPath(QString ethPath)
{
    Service* serviceRequested = NULL;
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isEthernetPresent())
        {
            //qDebug() << "Acquiring wifi :"<< wifiName;
            QListIterator<Service*> iter(m_manager->services());
            Service* service;
            bool found = false;
            while(iter.hasNext() && !found)
            {
                service = iter.next();
                if(service->type() == "ethernet" && service->objectPath().path() == ethPath)
                {
                   serviceRequested = service;
                   found = true;
                }
            }
        }
        else
        {
            qDebug() << " No ethernet interface detected ";
        }
    }
    return serviceRequested;
}
EMSSsid* NetworkCtl::getConnectedWifi()
{
    EMSSsid* ssidWifi = new EMSSsid();
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isWifiPresent())
        {
            if(this->isWifiConnected())
            {
                QString state;
                foreach (Service *service, m_manager->services())
                {
                    state = getStateString(service->state());
                    if(service->type() == "wifi" && (state == "ready" || state=="online"))
                    {

                        ssidWifi->setName(service->name());
                        ssidWifi->setPath(service->objectPath().path());
                        ssidWifi->setState(state);
                        ssidWifi->setStrength(service->strength());
                        ssidWifi->setType(service->type());
                        ssidWifi->setSecurity(NetworkCtl::instance()->getSecurityTypeString(toSecurityTypeList(service->security())));
                    }
                }
            }
            else
            {
                qDebug() << " Wifi offline ";
            }

        }
    }
    return ssidWifi;
}

EMSEthernet* NetworkCtl::getConnectedEthernet()
{
    EMSEthernet* ethParam = new EMSEthernet();
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isEthernetPresent())
        {
            if(this->isEthernetConnected())
            {
                QString state;
                foreach (Service *service, m_manager->services())
                {
                    state = getStateString(service->state());
                    if(service->type() == "ethernet" && (state == "ready" || state=="online"))
                    {
                        ethParam->setPath(service->objectPath().path());
                        ethParam->setInterface(service->ethernet()->interface());
                        ethParam->setState(state);
                        ethParam->setType(service->type());
                        ethParam->setIpAddress(service->ipv4()->address());
                    }
                }
            }
            else
            {
                qDebug() << " Ethernet offline ";
            }

        }
    }
    return ethParam;
}

bool NetworkCtl::isWifiPresent()
{
    bool result = false;
    if(getTechnology("wifi"))
    {
        result = true;
    }
    return result;
}


bool NetworkCtl::isEthernetPresent()
{
    bool result = false;
    if(getTechnology("ethernet"))
    {
        result = true;
    }
    return result;
}

bool NetworkCtl::isWifiConnected()
{
    Technology* technology=getTechnology("wifi");
    if(technology)
    {
        return technology->isConnected();
    }
    return false;
}

bool NetworkCtl::isEthernetConnected()
{
    Technology* technology=getTechnology("ethernet");
    if(technology)
    {
        return technology->isConnected();
    }
    return false;
}

bool NetworkCtl::isWifiEnabled()
{
    Technology* technology=getTechnology("wifi");
    if(technology)
    {
        return technology->isPowered();
    }
    return false;
}

bool NetworkCtl::isEthernetEnabled()
{
    Technology* technology=getTechnology("ethernet");
    if(technology)
    {
        return technology->isPowered();
    }
    return false;
}

Technology* NetworkCtl::getTechnology(QString technologyType)
{
    Technology* result = NULL;

    foreach(Technology* technology,m_manager->technologies())
    {
        if(technology->type() == technologyType)
        {
            result = technology;
        }
    }
    return result;
}

void NetworkCtl::enableWifi(bool enable)
{
    Technology* technology = getTechnology("wifi");
    technology->setPowered(enable);
    if(enable)
        qDebug() << "Enable " << technology->name();
    else
        qDebug() << "Disable " << technology->name();
}

void NetworkCtl::enableEthernet(bool enable)
{
    Technology* technology = getTechnology("ethernet");
    technology->setPowered(enable);
    if(enable)
        qDebug() << "Enable " << technology->name();
    else
        qDebug() << "Disable " << technology->name();
}



QList<EMSSsid::SecurityType> NetworkCtl::toSecurityTypeList(const QStringList &listType)
{
    QList<EMSSsid::SecurityType> securityTypeList;
    foreach(QString type,listType)
    {
        if (type == "none")
            securityTypeList.append(EMSSsid::SecurityType::NONE);
        else if (type == "wep")
            securityTypeList.append(EMSSsid::SecurityType::WEP);
        else if (type == "psk")
            securityTypeList.append(EMSSsid::SecurityType::PSK);
        else if (type == "ieee8021x")
            securityTypeList.append(EMSSsid::SecurityType::IEEE8021X);
        else if (type == "wps")
            securityTypeList.append(EMSSsid::SecurityType::WPS);
        else  securityTypeList.append(EMSSsid::SecurityType::UNKNOWN);
    }
    return securityTypeList;
}

QStringList NetworkCtl::getSecurityTypeString(QList<EMSSsid::SecurityType> securityTypeList)
{
    QStringList listType;
    foreach(EMSSsid::SecurityType securityType,securityTypeList)
    {
        switch (securityType) {
        case EMSSsid::SecurityType::NONE :
            listType.append("none");
            break;
        case EMSSsid::SecurityType::WEP :
            listType.append("wep");
                    break;
        case EMSSsid::SecurityType::PSK :
            listType.append("wpa/wpa2 personal");
                    break;
        case EMSSsid::SecurityType::WPS :
            listType.append("wps");
                    break;
        case EMSSsid::SecurityType::IEEE8021X :
            listType.append("wpa/wpa2 enterprise");
                    break;
        default:
            listType.append("unknown");
        }
    }
    return listType;
}

NetworkCtl::~NetworkCtl()
{
}


