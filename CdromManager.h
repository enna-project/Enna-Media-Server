#ifndef CDROMMANAGER_H
#define CDROMMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

#include "Data.h"

class CdromManager : public QObject
{
    Q_OBJECT

    QMutex mutex;
    QVector<EMSCdrom> cdroms;
    QDBusConnection bus;

public:

    /* ---------------------------------
     *    Public API - Thread safe
     * ---------------------------------
     */
    bool startMonitor();
    void stopMonitor();
    bool getAvailableCdroms(QVector<EMSCdrom> cdroms);
    EMSCdrom getCdrom(QString device);

    /* ---------------------------------
     *    Signleton pattern
     * ---------------------------------
     * See: http://www.qtcentre.org/wiki/index.php?title=Singleton_pattern
     */
    static CdromManager* instance()
    {
        static QMutex mutexinst;
        if (!_instance)
        {
            mutexinst.lock();

            if (!_instance)
                _instance = new CdromManager;

            mutexinst.unlock();
        }
        return _instance;
    }

    static void drop()
    {
        static QMutex mutexinst;
        mutexinst.lock();
        delete _instance;
        _instance = 0;
        mutexinst.unlock();
    }

signals:
    void cdromInserted(EMSCdrom cdrom);
    void cdromRemoved(EMSCdrom cdrom);
    /* CDrom already inserted but with additional data given by Gracenote */
    void cdromChanged(EMSCdrom cdrom);

public slots:
    void dbusMessageInsert(QString message);
    void dbusMessageRemove(QString message);

private:
    /* Singleton pattern */
    static CdromManager* _instance;
    CdromManager(QObject *parent = 0);
    CdromManager(const CdromManager &);
    CdromManager& operator=(const CdromManager &);
    ~CdromManager();

};

#endif // CDROMMANAGER_H
