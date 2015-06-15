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

public:

    enum SecurityType {NONE, WEP, PSK, IEEE8021X, WPS, UNKNOWN};
    EMSSsid(QString path, QString name, QString type, QString state, int strength, QStringList securityList);
    EMSSsid();

    QString getName() const;
    QString getType() const;
    QString getState() const;
    int getStrength() const;
    QString getPath() const;
    QStringList getSecurity() const;

    void setName(QString name);
    void setType(QString type);
    void setState(QString state);
    void setStrength(int strength);
    void setPath(QString path);
    void setSecurity(QStringList securityList);
    ~EMSSsid();
};

class EMSEthernet
{
    QString m_path;
    QString m_interface;
    QString m_type;
    QString m_state;
    QString m_ipAdress;

public:


    EMSEthernet(QString path, QString interface, QString type, QString state, QString ipAdress);
    EMSEthernet();

    QString getPath() const;
    QString getInterface() const;
    QString getType() const;
    QString getState() const;
    QString getIpAddress() const;

    void setPath(QString path);
    void setInterface(QString interface);
    void setType(QString type);
    void setState(QString state);
    void setIpAddress(QString ipAdress);

    ~EMSEthernet();
};

#ifdef EMS_LIB_QCONNMAN
#include "NetworkCtlConnman.h"
#else
#include "NetworkCtlStub.h"
#endif

#endif // NETWORKCTL_H
