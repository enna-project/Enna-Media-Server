#include "Networkctl.h"


/*
 * Main NetworkCtl file used to implement shared class (EMSSID and EMSEthernet)
 * used in NetworkCtlStub and NetworkCtlConnman
*/


EMSSsid::EMSSsid(QString path, QString name, QString type, QString state, int strength, QStringList securityList, bool favorite) :
    m_path(path),
    m_name(name),
    m_type(type),
    m_state(state),
    m_strength(strength),
    m_securityList(securityList),
    m_favorite(favorite)
{

}

EMSSsid::EMSSsid()
{
    m_path = "";
    m_name = "";
    m_type = "";
    m_state = "";
    m_strength = 0;
    m_securityList = QStringList();
    m_favorite = "";
}

// Get methods of EMSSsid class
QString EMSSsid::getName() const
{
    return m_name;
}

QString EMSSsid::getType() const
{
    return m_type;
}

QString EMSSsid::getState() const
{
    return m_state;
}

int EMSSsid::getStrength() const
{
    return m_strength;
}

QString EMSSsid::getPath() const
{
    return m_path;
}

QStringList EMSSsid::getSecurity() const
{
    return m_securityList;
}

bool EMSSsid::getFavorite() const
{
    return m_favorite;
}

// Set methods of EMSSsid class
void EMSSsid::setName(QString name)
{
     m_name = name;
}

void EMSSsid::setType(QString type)
{
    m_type = type;
}

void EMSSsid::setState(QString state)
{
    m_state = state;
}

void EMSSsid::setStrength(int strength)
{
    m_strength = strength;
}

void EMSSsid::setPath(QString path)
{
    m_path = path;
}

void EMSSsid::setSecurity(QStringList securityList)
{
    m_securityList = securityList;
}

void EMSSsid::setFavorite(bool favorite)
{
    m_favorite = favorite;
}

EMSSsid::~EMSSsid()
{
}

EMSEthernet::EMSEthernet(QString path, QString type, QString state):
    m_path(path),
    m_type(type),
    m_state(state)
{

}

EMSEthernet::EMSEthernet()
{

}

// Get methods of EMSEthernet class
QString EMSEthernet::getPath() const
{
    return m_path;
}

QString EMSEthernet::getType() const
 {
    return m_type;
}

QString EMSEthernet::getState() const
{
    return m_state;
}

// Set methods of EMSEthernet class
void EMSEthernet::setPath(QString path)
{
    m_path = path;
}

void EMSEthernet::setType(QString type)
{
    m_type = type;
}

void EMSEthernet::setState(QString state)
{
    m_state = state;
}

EMSEthernet::~EMSEthernet()
{

}


EMSNetworkConfig::EMSNetworkConfig(QString interface, QString macAddress, QString addressAllocation, QString ipAddress, QString netmask, QString gateway):
    m_interface(interface),
    m_macAdress(macAddress),
    m_addressAllocation(addressAllocation),
    m_ipAddress(ipAddress),
    m_netmask(netmask),
    m_gateway(gateway)
{

}

EMSNetworkConfig::EMSNetworkConfig()
{

}

// Get methods of EMSNetworkConfig class
QString EMSNetworkConfig::getInterface() const
{
    return m_interface;
}

QString EMSNetworkConfig::getMacAddress() const
{
    return m_macAdress;
}

QString EMSNetworkConfig::getAddressAllocation() const
{
    return m_addressAllocation;
}

QString EMSNetworkConfig::getIpAddress() const
{
    return m_ipAddress;
}

QString EMSNetworkConfig::getNetmask() const
{
    return m_netmask;
}

QString EMSNetworkConfig::getGateway() const
{
    return m_gateway;
}

// Set methods of EMSNetworkConfig class
void EMSNetworkConfig::setInterface(QString interface)
{
    m_interface = interface;
}

void EMSNetworkConfig::setMacAddress(QString macAddress)
{
    m_macAdress = macAddress;
}

void EMSNetworkConfig::setAddressAllocation(QString addressAllocation)
{
    m_addressAllocation = addressAllocation;
}

void EMSNetworkConfig::setIpAddress(QString ipAddress)
{
    m_ipAddress = ipAddress;
}

void EMSNetworkConfig::setNetmask(QString netmask)
{
    m_netmask = netmask;
}

void EMSNetworkConfig::setGateway(QString gateway)
{
    m_gateway = gateway;
}

EMSNetworkConfig::~EMSNetworkConfig()
{

}
