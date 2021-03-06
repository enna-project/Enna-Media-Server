#ifndef NETWORKCTLSTUB_H
#define NETWORKCTLSTUB_H
#include <QObject>
#include <QList>
#include <QMutex>
#include <qdbusextratypes.h>
#include "Networkctl.h"

/*
 * This file contains a STUB IMPLEMENTATION for NetworkCtl class,
 * in order to enable conditional compilation for qconnman library
 * qconnman library enables the control of connman dbus API
 * and is required to use the network features in the app
*/





/*
 * Implementation of class stub from qconnman library
 * Functions are used in NetworkCtl class
 * These functions do nothing but are required for the conditional compilation
*/
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
    ~Service();
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
    void remove();

public Q_SLOTS:
    void connect();
    void disconnect();

};


class Manager{
    Manager();
    ~Manager();
public :
    QList<Agent*> agents() const;
    QList<Technology*> technologies() const;
    QList<Service*> services() const;
};


/*
 * Implementation of NetworkCtl class stub
 * some functions are used in JsonApi
 * these functions do nothing
*/

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
    Service* getNetworkService( QString techName,QString searchType,QString idNetwork);
    EMSSsid* getConnectedWifi();
    EMSEthernet* getPluggedEthernet();

    bool isTechnologyConnected(QString techName);
    bool isTechnologyEnabled(QString techName);
    bool isTechnologyPresent(QString techName);

    void enableTechnology(bool enable, QString techName);

    Technology* getTechnology(QString technologyType);
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

public slots:
    void scanWifi();

};

#endif // NETWORKCTLSTUB_H
