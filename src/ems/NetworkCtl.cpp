
#include <QDebug>
#include <QDBusConnection>
#include <QListIterator>
#include <QMetaProperty>
#include <service.h>
#include <manager.h>

#include "NetworkCtl.h"

NetworkCtl* NetworkCtl::_instance = 0;

NetworkCtl::NetworkCtl(QObject *parent): QObject(parent)
{
    m_manager = new Manager(this);
    connect(m_manager,SIGNAL(servicesChanged()),this, SIGNAL(wifiListUpdated()));
    connect(m_manager,SIGNAL(connectedServiceChanged()),this, SIGNAL(connectedChanged()));
    //connect(this->getTechnology("wifi"),SIGNAL(scanCompleted()),this, SIGNAL(wifiListUpdated()));
    m_agent=new Agent("/com/EMS/Connman", m_manager);


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


void NetworkCtl::listServices()
{
    if(m_manager->services().isEmpty())
    {
        qDebug() << " No service listed ";
    }
    else
    {
        foreach (Service *service, m_manager->services())
        {
            qDebug() << "Service Path: "<< "[ " << service->objectPath().path().toLatin1().constData() << " ]" << endl;
            qDebug() << "Name : "<< service->name();
            qDebug() << "Type : "<< service->type();
            qDebug() << "State : "<< getStateString(service->state());
            if(service->type()!="ethernet")
            {
                qDebug() << "Strength : "<< service->strength();
            }

            if(service->state()==Service::ServiceState::OnlineState || service->state()==Service::ServiceState::ReadyState )
            {
                qDebug() << "Interface : "<< service->ethernet()->interface();
                qDebug() << "IP config : "<< service->ipv4()->method();
                if(service->type()!="ethernet")
                {
                    qDebug() << "Security : "<< service->security().join(", ");
                }
                qDebug() << "Address : "<< service->ipv4()->address();
                qDebug() << "Netmask : "<< service->ipv4()->netmask();
                if(service->state()==Service::ServiceState::OnlineState)
                {
                    qDebug() << "Gateway : "<< service->ipv4()->gateway();

                }
                qDebug() << "MAC adress : "<< service->ethernet()->address();
            }
            qDebug() << endl;
        }
    }
}

void NetworkCtl::scanWifi()
{
    if(isWifiPresent())
    {
        this->getTechnology("wifi")->scan();
        qDebug()<< "scanning wifi" <<endl;
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

void NetworkCtl::listTechnologies()
{
    QList<Technology*> listTechno = m_manager->technologies();
    QListIterator<Technology*> iter( listTechno );
    while( iter.hasNext() )
    {
        Technology* tempTech = iter.next();
        qDebug() << "Name: "<< tempTech->name();
        qDebug() << "Type: "<<tempTech->type();
        qDebug() << "Connected: "<<tempTech->isConnected();
        qDebug() << "Powered: "<<tempTech->isPowered();
        qDebug() << "Tethering: "<<tempTech->tetheringAllowed()<<endl;
    }

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

/*
QList<Service*> Connman::getEthService(Connman* m_manager)
{
    QList<Service*> ethServices;
    if(!(m_manager->services().isEmpty()))
    {
        foreach (Service *service, m_manager->services())
        {
            if(service->type()==QString("ethernet"))
            {
                ethServices.append(service);
            }
        }
    }
    return ethServices;
}
*/

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

ConnexionRequest::ConnexionRequest(QString path, QString name, QString passphrase, QString state, int timeout) :
    m_path(path),
    m_name(name),
    m_passphrase(passphrase),
    m_state(state),
    m_timeout(timeout)
{

}

ConnexionRequest::ConnexionRequest()
{

}

QString ConnexionRequest::getName() const
{
    return m_name;
}
QString ConnexionRequest::getPath() const
{
    return m_path;
}
int ConnexionRequest::getTimeout() const
{
    return m_timeout;
}
QString ConnexionRequest::getPassphrase() const
{
    return m_passphrase;
}

void ConnexionRequest::setName(QString name)
{
    m_name=name;
}

void ConnexionRequest::setPath(QString path)
{
    m_path=path;
}

void ConnexionRequest::setTimeout(int timeout)
{
    m_timeout=timeout;
}

void ConnexionRequest::setPassphrase(QString passphrase)
{
    m_passphrase=passphrase;
}

ConnexionRequest::~ConnexionRequest()
{

}


NetworkCtl::~NetworkCtl()
{
}


EMSSsid::~EMSSsid()
{
}
