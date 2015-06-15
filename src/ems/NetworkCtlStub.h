#ifndef NETWORKCTLSTUB_H
#define NETWORKCTLSTUB_H
#include <QObject>
#include <QList>
#include <QMutex>
#include <qdbusextratypes.h>
#include "Networkctl.h"

class Agent : public QObject
{
    Q_OBJECT
public:
    struct InputRequest
    {
        struct Response
        {
            QString passphrase;
            QString name;
        };

        QDBusObjectPath service;
        Response response;
        bool cancel;

        InputRequest(): cancel(false) { }
    };
    struct ErrorRequest
    {
        QDBusObjectPath service;
        QString error;
        bool retry;

        ErrorRequest(): retry(false) { }
    };
    Agent();
    inline InputRequest *currentInputRequest() const { return m_inputRequest; }
    inline ErrorRequest *currentErrorRequest() const { return m_errorRequest; }

Q_SIGNALS:
    void passphraseRequested();
    void nameRequested();
    void browserRequested();
    void errorRaised();
private:
    InputRequest *m_inputRequest;
    ErrorRequest *m_errorRequest;

};

class Technology : public QObject
{

    Q_OBJECT
public :
    Technology();
    ~Technology();

    QDBusObjectPath path() const;
    QString name() const;
    QString type() const;
    bool isConnected() const;

    bool isPowered() const;
    void setPowered(bool powered);

    bool tetheringAllowed() const;
    void setTetheringAllowed(bool allowed);

    QString tetheringIdentifier() const;
    void setTetheringIdentifier(const QString &identifier);

    QString tetheringPassphrase() const;
    void setTetheringPassphrase(const QString &passphrase);

Q_SIGNALS:
    void poweredChanged(bool powered);
    void connectedChanged();
    void nameChanged();
    void typeChanged();
    void tetheringAllowedChanged();
    void tetheringIdentifierChanged();
    void tetheringPassphraseChanged();

    void scanCompleted();

public Q_SLOTS:
    void scan();
};

class EthernetData
{
public :
    EthernetData();
    QString method() const;
    void setMethod(const QString &method);

    QString interface() const;
    void setInterface(const QString &interface);

    QString address() const;
    void setAddress(const QString &address);

    quint16 mtu() const;
    void setMtu(quint16 mtu);

    quint16 speed() const;
    void setSpeed(quint16 speed);

    QString duplex() const;
    void setDuplex(const QString &duplex);
};

class IPV4Data
{

public:
    IPV4Data();
    ~IPV4Data();

    QString method() const;
    void setMethod(const QString &method);

    QString address() const;
    void setAddress(const QString &address);

    QString netmask() const;
    void setNetmask(const QString &netmask);

    QString gateway() const;
    void setGateway(const QString &gateway);
};

class Service : public QObject
{
    Q_OBJECT
public:
    Service();
    enum ServiceState {
        UndefinedState,
        IdleState,
        FailureState,
        AssociationState,
        ConfigurationState,
        ReadyState,
        DisconnectState,
        OnlineState
    };
    ServiceState state() const;
    QDBusObjectPath objectPath() const;
    QString error() const;
    QString name() const;
    QString type() const;
    QStringList security() const;
    quint8 strength() const;
    bool isFavorite() const;
    bool isImmutable() const;
    bool isRoaming() const;
    EthernetData *ethernet() const;
    IPV4Data *ipv4() const;

public Q_SLOTS:
    void connect();
    void disconnect();


};

class Manager{
    Manager();
public :
    QList<Agent*> agents() const;
    QList<Technology*> technologies() const;
    QList<Service*> services() const;
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

#endif // NETWORKCTLSTUB_H
