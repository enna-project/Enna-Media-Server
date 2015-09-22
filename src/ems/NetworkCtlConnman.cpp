
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
    m_agent=new Agent("/com/EMS/Connman", m_manager);
    connect(m_manager,SIGNAL(servicesChanged()),this, SIGNAL(wifiListUpdated()));
    connect(this->getTechnology("wifi"),SIGNAL(connectedChanged()),this,SIGNAL(wifiConnectedChanged()));
    connect(this->getTechnology("ethernet"),SIGNAL(connectedChanged()),this,SIGNAL(ethConnectedChanged()));
    connect(getAgent(),SIGNAL(errorRaised()),this,SIGNAL(agentErrorRaised()));
    enableTechnology(true,"wifi");
    enableTechnology(true, "ethernet");
    m_enableUpdate = false;
    //Allow autoconnect to favorite networks on start
    enableFavAutoConnect(true);
}

static bool wifiSortByStrength(EMSSsid a, EMSSsid b)
{
    //connected and favorite wifi have to be in the top of the list
    QString stateA = a.getState();
    QString stateB = b.getState();
    if(stateA == "online" || stateA == "ready" || stateA == "association" || stateA == "configuration")
    {
        return true;
    }
    if(stateB == "online" || stateB == "ready" || stateB == "association" || stateB == "configuration")
    {
        return false;
    }
    if(a.getFavorite())
    {
        return true;
    }
    if(b.getFavorite())
    {
        return false;
    }
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
    if(isTechnologyPresent("wifi"))
    {
        getTechnology("wifi")->scan();
        qDebug() << "scanning wifi" << endl;
    }
    else
    {
        qDebug() << "No wifi detected" << endl;
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
        if(isTechnologyPresent("wifi"))
        {
            qDebug()<< "Acquiring wifi list" ;
            foreach (Service *service, m_manager->services())
            {
                if(service->type() == "wifi")
                {
                    //don't list hidden ssid
                    if(!service->name().isEmpty())
                    {
                        EMSSsid ssidWifi(service->objectPath().path(),
                                         service->name(),
                                         service->type(),
                                         getStateString(service->state()),
                                         service->strength(),
                                         getSecurityTypeString(toSecurityTypeList(service->security())),
                                         service->isFavorite());
                        ssidList.append(ssidWifi);
                    }
                }
            }
            qSort(ssidList.begin(), ssidList.end(), wifiSortByStrength);
        }
        else
        {
            qDebug()<< "No wifi detected" ;
        }
    }
    return ssidList;
}

void NetworkCtl::enableFavAutoConnect(bool enable)
{
    QListIterator<Service*> iter(m_manager->services());
    Service* service;

    while(iter.hasNext())
    {
        service = iter.next();
        if(service->isFavorite())
        {
            service->setAutoConnect(enable);
        }
    }
}

Service* NetworkCtl::getNetworkService( QString techName,QString searchType,QString idNetwork)
{
    Service* serviceRequested = NULL;
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(isTechnologyPresent(techName))
        {
            qDebug() << "Acquiring "<< techName << " : "<< idNetwork;
            QListIterator<Service*> iter(m_manager->services());
            Service* service;
            bool found = false;
            //search network by path
            if(searchType == "path")
            {
                while(iter.hasNext() && !found)
                {
                    service = iter.next();
                    if(service->type() == techName && service->objectPath().path() == idNetwork)
                    {
                        serviceRequested = service;
                        found = true;
                    }
                }
            }
            //search network by name
            else if(searchType == "name")
            {
                while(iter.hasNext() && !found)
                {
                    service = iter.next();
                    if(service->type() == techName && service->name() == idNetwork)
                    {
                        serviceRequested = service;
                        found = true;
                    }
                }
            }
        }
        else
        {
            qDebug() << " No " << techName <<" detected ";
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
        if(this->isTechnologyPresent("wifi"))
        {
            if(this->isTechnologyConnected("wifi"))
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

EMSEthernet* NetworkCtl::getPluggedEthernet()
{
    EMSEthernet* ethParam = new EMSEthernet();
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isTechnologyPresent("ethernet"))
        {
            QString state;
            foreach (Service *service, m_manager->services())
            {
                state = getStateString(service->state());
                if(service->type() == "ethernet")
                {
                    ethParam->setPath(service->objectPath().path());
                    ethParam->setState(state);
                    ethParam->setType(service->type());
                }
            }
            if(ethParam->getPath().isEmpty())
            {
                qDebug() << " No ethernet plugged ";
            }
        }
    }
    return ethParam;
}

EMSNetworkConfig* NetworkCtl::getNetworkConfig(QString techName, QString techPath)
{
    EMSNetworkConfig* networkConfig = new EMSNetworkConfig();
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        if(this->isTechnologyPresent(techName))
        {
            if(this->isTechnologyConnected(techName))
            {
                QString state;
                QString path;
                foreach (Service *service, m_manager->services())
                {
                    path = service->objectPath().path();
                    if(service->type() == techName && path == techPath)
                    {
                        networkConfig->setInterface(service->ethernet()->interface());
                        networkConfig->setMacAddress(service->ethernet()->address());
                        networkConfig->setNetmask(service->ipv4()->netmask());
                        networkConfig->setAddressAllocation(service->ipv4()->method());
                        networkConfig->setGateway(service->ipv4()->gateway());
                        networkConfig->setIpAddress(service->ipv4()->address());
                        qDebug() << "got informations for" << networkConfig->getIpAddress();
                    }
                }
            }
            else
            {
                qDebug() << techName << "not connected";
            }

        }
    }
    return networkConfig;
}

bool NetworkCtl::isTechnologyPresent(QString techName)
{
    bool result = false;
    if(getTechnology(techName))
    {
        result = true;
    }
    return result;
}

bool NetworkCtl::isTechnologyEnabled(QString techName)
{
    Technology* technology = getTechnology(techName);
    if(technology)
    {
        return technology->isPowered();
    }
    return false;
}

bool NetworkCtl::isTechnologyConnected(QString techName)
{
    Technology* technology = getTechnology(techName);
    if(technology)
    {
        return technology->isConnected();
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

void NetworkCtl::enableTechnology(bool enable, QString techName)
{
    if(techName == "wifi" || techName == "ethernet")
    {
        if(isTechnologyPresent(techName))
        {
            Technology* technology = getTechnology(techName);

            technology->setPowered(enable);
            if(enable)
                qDebug() << "Enable " << technology->name();
            else
                qDebug() << "Disable " << technology->name();
        }
        else
        {
            qDebug() << techName << "is not available";
        }
    }
    else
    {
        qDebug() << "No technology " << techName ;
    }
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


