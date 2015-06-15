#ifndef NETWORKCTLCONNMAN_H
#define NETWORKCTLCONNMAN_H

#include <QObject>
#include <QList>
#include <manager.h>
#include <service.h>
#include <QMutex>
#include "Networkctl.h"

class EMSSsid;
class EMSEthernet;
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

#endif // NETWORKCTLCONNMAN_H
