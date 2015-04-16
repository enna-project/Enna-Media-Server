#ifndef NETWORKCTL_H
#define NETWORKCTL_H

#include <QObject>
#include <QList>
#include <manager.h>
#include <service.h>
#include <QMutex>

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
    //QList<SecurityType> toSecurityTypeList(const QStringList &listType);


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

class EMSSsid;
class NetworkCtl : public QObject
{
    Q_OBJECT

private:
    Manager *m_manager;
    Agent *m_agent;

public:
    Agent* getAgent() const
    {
        return m_agent;
    }

    static QString getStateString(Service::ServiceState state);
    QList<EMSSsid::SecurityType> toSecurityTypeList(const QStringList &listType);
    QStringList getSecurityTypeString(QList<EMSSsid::SecurityType> securityTypeList);
    QList<EMSSsid> getWifiList();
    Service* getWifiByName(QString wifiName);
    EMSSsid* getConnectedWifi();
    bool isWifiPresent();
    bool isEthernetPresent();

    bool isWifiConnected();
    bool isEthernetConnected();

    bool isWifiEnabled();
    bool isEthernetEnabled();

    void enableWifi(bool enable);
    void enableEthernet(bool enable);

    Technology* getTechnology(QString technologyType);

    void listServices();
    void listTechnologies();
    //QString technologyName(Technology *technology) ;
    //QList<Service*> getEthService(Connman* m_manager);
    //void enableTechnology(Technology* technology,bool enable);

    /* Signleton pattern
     * See: http://www.qtcentre.org/wiki/index.php?title=Singleton_pattern
     */
    static NetworkCtl* instance()
    {
        static QMutex mutex;
        if (!_instance)
        {
            mutex.lock();

            if (!_instance)
                _instance = new NetworkCtl;

            mutex.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutex;
        mutex.lock();
        delete _instance;
        _instance = 0;
        mutex.unlock();
    }

private:
    /* Hide all other access to this class */
    static NetworkCtl* _instance;
    NetworkCtl(QObject *parent = 0);
    NetworkCtl(const NetworkCtl &);
    NetworkCtl& operator=(const NetworkCtl &);
    ~NetworkCtl();

signals:
    void requestScan();
    void wifiListUpdated();
    void connectedChanged();

public slots:
    //QVariantMap getProperties();
    void scanWifi();

};

/*
class EMSSsid
{
    QString m_path;
    QString m_name;
    QString m_type;
    QString m_state;
    int m_strength;
    QString m_security;

public:
    enum SecurityType {NONE, WEP, PSK, IEEE8021X, WPS, UNKNOWN};

    EMSSsid(QString path,QString name, QString type, QString state, int strength,EMSSsid::SecurityType security);
    EMSSsid();
    //QList<SecurityType> toSecurityTypeList(const QStringList &listType);

    QString getSecurityTypeString(SecurityType type);
    QString getName() const;
    QString getType() const;
    QString getState() const;
    int getStrength() const;
    QString getPath() const;
    QString getSecurity() const;

    void setName(QString name);
    void setType(QString type);
    void setState(QString state);
    void setStrength(int strength);
    void setPath(QString path);
    void setSecurity(QString security);
    ~EMSSsid();
};
*/
class ConnexionRequest
{
    QString m_path;
    QString m_name;
    QString m_passphrase;
    QString m_state;
    int m_timeout;
public:

    ConnexionRequest(QString path, QString name, QString m_passphrase,QString state, int timeout);
    ConnexionRequest();
    QString getName() const;
    QString getPath() const;
    int getTimeout() const;
    QString getPassphrase() const;

    void setName(QString name);
    void setPath(QString path);
    void setTimeout(int timeout);
    void setPassphrase(QString passphrase);

    ~ConnexionRequest();
};
#endif // NETWORKCTL_H
