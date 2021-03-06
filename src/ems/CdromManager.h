#ifndef CDROMMANAGER_H
#define CDROMMANAGER_H

#include <QObject>
#include <QtDBus/QtDBus>

#include "Data.h"
#ifdef EMS_CDROM_RIPPER
#include "CdromRipper.h"
#endif

class CdromManager : public QObject
{
    Q_OBJECT

    QThread worker;
    QMutex mutex;
    QVector<EMSCdrom> cdroms;
    QDBusConnection bus;
#ifdef EMS_CDROM_RIPPER
    CdromRipper *m_cdromRipper = nullptr;
#endif
    QString m_ripAudioFormat;

public:

    /* ---------------------------------
     *    Public API - Thread safe
     * ---------------------------------
     */
    void getAvailableCdroms(QVector<EMSCdrom> *cdromsOut);
    bool getCdrom(QString device, EMSCdrom *cdromOut);
    bool isRipInProgress();
    void setRipAudioFormat(const QString &ripAudioFormat);

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
    void cdromEjected(EMSCdrom cdrom);
    /* CDrom already inserted but with additional data given by Gracenote */
    void cdromChanged(EMSCdrom cdrom);
    /* Signal binded to MetadataManager */
    void cdromTrackNeedUpdate(EMSTrack track, QStringList capabilities);
    /* Signal binded to WebSocketServer */
    void ripProgressChanged(EMSRipProgress ripProgress);


public slots:
    void cdromTrackUpdated(EMSTrack track, bool complete);
    void dbusMessageInsert(QString message);
    void dbusMessageRemove(QString message);
    bool startMonitor();
    void stopMonitor();
    void startRip();
    void ripProgress(EMSRipProgress ripProgress);
    void handleCdromRipperResults(const QString &message);

private:
    /* Singleton pattern */
    static CdromManager* _instance;
    CdromManager(QObject *parent = 0);
    CdromManager(const CdromManager &);
    CdromManager& operator=(const CdromManager &);
    ~CdromManager();

};


#endif // CDROMMANAGER_H
