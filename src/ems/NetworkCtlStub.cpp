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

/*Service* NetworkCtl::getWifiByName(QString wifiName)
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
}*/

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

bool NetworkCtl::isTechnologyPresent(QString techName)
{
    techName = "stub";
    return false;
}

bool NetworkCtl::isTechnologyConnected(QString techName)
{
    techName = "stub";
    return false;
}

bool NetworkCtl::isTechnologyEnabled(QString techName)
{
    techName = "stub";
    return false;
}

Technology* NetworkCtl::getTechnology(QString technologyType)
{
    Technology* result= new Technology();
    technologyType = " ";
    return result;
}

void NetworkCtl::enableTechnology(bool enable,QString techName)
{
    if(enable)
    {
        techName = "stub";
    }
}

void NetworkCtl::enableFavAutoConnect(bool enable)
{
    if(enable)
    {

    }
}

Service* NetworkCtl::getNetworkService( QString techName,QString searchType,QString idNetwork)
{
    Service* serviceRequested = new Service();
    techName = "stub";
    searchType = "stub";
    idNetwork = "stub";
    return serviceRequested;
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

void Service::remove()
{

}

QString Service::name() const
{
    return "stub";
}

Service::~Service()
{

}

