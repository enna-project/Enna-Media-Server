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
    bool m_enableUpdate;

public:
    Agent* getAgent() const
    {
        return m_agent;
    }

    bool getEnableUpdate()
    {
        return m_enableUpdate;
    }

    void setEnableUpdate(bool enableUpdate)
    {
        m_enableUpdate = enableUpdate;
    }

    static QString getStateString(Service::ServiceState state);
    QList<EMSSsid::SecurityType> toSecurityTypeList(const QStringList &listType);
    QStringList getSecurityTypeString(QList<EMSSsid::SecurityType> securityTypeList);
    QList<EMSSsid> getWifiList();

    EMSSsid* getConnectedWifi();
    EMSEthernet* getPluggedEthernet();
    EMSNetworkConfig* getNetworkConfig(QString techName, QString techPath);
    Service* getNetworkService( QString techName,QString searchType,QString idNetwork);

    bool isTechnologyConnected(QString techName);
    bool isTechnologyEnabled(QString techName);
    bool isTechnologyPresent(QString technName);

    void enableTechnology(bool enable, QString technName);
    Technology* getTechnology(QString technologyType);
    //enable or disable autoconnect to favorite networks
    void enableFavAutoConnect(bool enable);

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
    void agentErrorRaised();

public slots:
    void scanWifi();

};

#endif // NETWORKCTLCONNMAN_H
