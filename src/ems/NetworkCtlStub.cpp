#include "NetworkCtlStub.h"

#include <QDebug>
#include <QDBusConnection>
#include <QListIterator>
#include <QMetaProperty>


/*
 * Stub implementation for NetworkCtl clas.
 * Enable conditional compile for qconnman library
 * Compiled when EMS_LIB_QCONNMAN=no
 *
 * qconnman library enables the control of connman dbus API
 * and is required to use the network features in the app
*/

NetworkCtl* NetworkCtl::_instance = 0;

NetworkCtl::NetworkCtl(QObject *parent): QObject(parent)
{

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

EMSEthernet* NetworkCtl::getPluggedEthernet()
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

/* Technology class from qconnman implementation for dependencies */
void Technology::scan()
{

}

Technology::Technology()
{

}

bool Technology::isPowered() const
{
    return false;
}

Technology::~Technology()
{
}

/* Service class from qconnman implementation for dependencies */
Service::Service()
{

}

void Service::connect()
{

}

void Service::disconnect()
{

}

Service::~Service()
{

}

