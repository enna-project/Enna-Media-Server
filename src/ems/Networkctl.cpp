#include "Networkctl.h"


EMSSsid::EMSSsid(QString path, QString name, QString type, QString state, int strength, QStringList securityList) :
    m_path(path),
    m_name(name),
    m_type(type),
    m_state(state),
    m_strength(strength),
    m_securityList(securityList)
{

}

EMSSsid::EMSSsid()
{
    m_strength = 0;
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

EMSSsid::~EMSSsid()
{
}

EMSEthernet::EMSEthernet(QString path, QString interface, QString type, QString state, QString ipAdress):
    m_path(path),
    m_interface(interface),
    m_type(type),
    m_state(state),
    m_ipAdress(ipAdress)
{

}

EMSEthernet::EMSEthernet()
{

}


QString EMSEthernet::getPath() const
{
    return m_path;
}
QString EMSEthernet::getInterface() const
{
    return m_interface;
}
QString EMSEthernet::getType() const
 {
    return m_type;
}
QString EMSEthernet::getState() const
{
    return m_state;
}
QString EMSEthernet::getIpAddress() const
{
    return m_ipAdress;
}

void EMSEthernet::setPath(QString path)
{
    m_path = path;
}

void EMSEthernet::setInterface(QString interface)
{
    m_interface = interface;
}

void EMSEthernet::setType(QString type)
{
    m_type = type;
}

void EMSEthernet::setState(QString state)
{
    m_state = state;
}

void EMSEthernet::setIpAddress(QString ipAdress)
{
    m_ipAdress = ipAdress;
}

EMSEthernet::~EMSEthernet()
{

}

