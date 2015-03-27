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
    explicit CdromManager(QObject *parent = 0);
    ~CdromManager();

    bool getAvailableCdroms(QVector<EMSCdrom> cdroms);
    EMSCdrom getCdrom(QString device);

signals:
    void cdromInserted(EMSCdrom cdrom);
    void cdromRemoved(EMSCdrom cdrom);
    /* CDrom already inserted but with additional data given by Gracenote */
    void cdromChanged(EMSCdrom cdrom);

public slots:
    void dbusMessageInsert(QString message);
    void dbusMessageRemove(QString message);
};

#endif // CDROMMANAGER_H
