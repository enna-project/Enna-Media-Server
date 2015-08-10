#ifndef NETWORKCTL_H
#define NETWORKCTL_H

#include <QObject>
#include "qstringlist.h"

class EMSSsid
{
    QString m_path;
    QString m_name;
    QString m_type;
    QString m_state;
    int m_strength;
    QStringList m_securityList;
    bool m_favorite;

public:

    enum SecurityType {NONE, WEP, PSK, IEEE8021X, WPS, UNKNOWN};
    EMSSsid(QString path, QString name, QString type, QString state, int strength, QStringList securityList, bool favorite);
    EMSSsid();

    QString getName() const;
    QString getType() const;
    QString getState() const;
    int getStrength() const;
    QString getPath() const;
    QStringList getSecurity() const;
    bool getFavorite() const;

    void setName(QString name);
    void setType(QString type);
    void setState(QString state);
    void setStrength(int strength);
    void setPath(QString path);
    void setSecurity(QStringList securityList);
    void setFavorite(bool favorite);
    ~EMSSsid();
};

class EMSEthernet
{
    QString m_path;
    QString m_type;
    QString m_state;

public:


    EMSEthernet(QString path, QString type, QString state);
    EMSEthernet();

    QString getPath() const;
    QString getType() const;
    QString getState() const;

    void setPath(QString path);
    void setType(QString type);
    void setState(QString state);

    ~EMSEthernet();
};

class EMSNetworkConfig
{
    QString m_interface;
    QString m_macAdress;
    QString m_addressAllocation;
    QString m_ipAddress;
    QString m_netmask;
    QString m_gateway;

public:


    EMSNetworkConfig(QString interface, QString macAddress, QString addressAllocation,
                     QString ipAddress, QString netmask, QString gateway);
    EMSNetworkConfig();

    QString getInterface() const;
    QString getMacAddress() const;
    QString getAddressAllocation() const;
    QString getIpAddress() const;
    QString getNetmask() const;
    QString getGateway() const;

    void setInterface(QString interface);
    void setMacAddress(QString macAddress);
    void setAddressAllocation(QString addressAllocation);
    void setIpAddress(QString ipAddress);
    void setNetmask(QString netmask);
    void setGateway(QString gateway);

    ~EMSNetworkConfig();
};

#ifdef EMS_LIB_QCONNMAN
#include "NetworkCtlConnman.h"
#else
#include "NetworkCtlStub.h"
#endif

#endif // NETWORKCTL_H
