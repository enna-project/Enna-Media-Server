#include "NetworkCtlStub.h"


#include <QDebug>
#include <QDBusConnection>
#include <QListIterator>
#include <QMetaProperty>


NetworkCtl* NetworkCtl::_instance = 0;

NetworkCtl::NetworkCtl(QObject *parent): QObject(parent)
{

}

Technology::Technology()
{

}

Service::Service()
{

}

bool Technology::isPowered() const
{
    return false;
}

static bool wifiSortByStrength(EMSSsid a, EMSSsid b)
{
    if(a.getName()!=NULL)
    {

    }
    if(b.getName()!=NULL)
    {

    }
    return false;
}

QString NetworkCtl::getStateString(Service::ServiceState state)
{
    if(state)
    {

    }
    return "";
}

void NetworkCtl::scanWifi()
{

}

QList<EMSSsid> NetworkCtl::getWifiList()
{
    QList<EMSSsid> ssidList;
    return ssidList;
}

Service* NetworkCtl::getWifiByName(QString wifiName)
{
    Service* serviceRequested = new Service();
    wifiName = " ";
    return serviceRequested;
}

Service* NetworkCtl::getEthByPath(QString ethPath)
{
    Service* serviceRequested = new Service();
    ethPath = " ";
    return serviceRequested;
}

EMSSsid* NetworkCtl::getConnectedWifi()
{
    EMSSsid* ssidWifi = new EMSSsid();
    return ssidWifi;
}

EMSEthernet* NetworkCtl::getConnectedEthernet()
{
    EMSEthernet* ethParam = new EMSEthernet();
    return ethParam;
}

bool NetworkCtl::isWifiPresent()
{

    return false;
}


bool NetworkCtl::isEthernetPresent()
{
    return false;
}

bool NetworkCtl::isWifiConnected()
{
    return false;
}

bool NetworkCtl::isEthernetConnected()
{
    return false;
}

bool NetworkCtl::isWifiEnabled()
{
    return false;
}

bool NetworkCtl::isEthernetEnabled()
{
    return false;
}

Technology* NetworkCtl::getTechnology(QString technologyType)
{
    Technology* result= new Technology();
    technologyType = " ";
    return result;
}

void NetworkCtl::enableWifi(bool enable)
{
    enable = false;
}

void NetworkCtl::enableEthernet(bool enable)
{
    enable = false;
}

NetworkCtl::~NetworkCtl()
{
}

