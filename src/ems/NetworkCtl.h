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
    //QList<SecurityType> toSecurityTypeList(const QStringList &listType);

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

class EMSSsid;
class NetworkCtl : public QObject
{
    Q_OBJECT

private:
    Manager *m_manager;
    Agent *m_agent;
    bool m_enablUpdate;

public:
    Agent* getAgent() const
    {
        return m_agent;
    }

    bool getEnableUpdate()
    {
        return m_enablUpdate;
    }

    void setEnableUpdate(bool enableUpdate)
    {
        m_enablUpdate = enableUpdate;
    }

    static QString getStateString(Service::ServiceState state);
    QList<EMSSsid::SecurityType> toSecurityTypeList(const QStringList &listType);
    QStringList getSecurityTypeString(QList<EMSSsid::SecurityType> securityTypeList);
    QList<EMSSsid> getWifiList();
    Service* getWifiByName(QString wifiName);
    Service* getEthByPath(QString ethPath);
    EMSSsid* getConnectedWifi();
    EMSEthernet* getConnectedEthernet();
    bool isWifiPresent();
    bool isEthernetPresent();

    bool isWifiConnected();
    bool isEthernetConnected();

    bool isWifiEnabled();
    bool isEthernetEnabled();

    void enableWifi(bool enable);
    void enableEthernet(bool enable);

    Technology* getTechnology(QString technologyType);

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
    void wifiConnectedChanged();
    void ethConnectedChanged();

public slots:
    void scanWifi();

};

#endif // NETWORKCTL_H
