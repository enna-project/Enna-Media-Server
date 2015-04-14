#ifndef CDROMRIPPER_H
#define CDROMRIPPER_H

#include <QThread>

#include "Data.h"

class CdromRipper : public QThread
{
    Q_OBJECT
    void run() Q_DECL_OVERRIDE;

public:
    CdromRipper(QObject *parent);

    void setCdrom(const EMSCdrom &cdromProperties);

protected:

private:
    EMSCdrom m_cdromProperties;

signals:
    void ripProgressChanged(EMSRipProgress ripProgress);
    void resultReady(const QString &result);
};


#endif // CDROMRIPPER_H
