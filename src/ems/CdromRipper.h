#ifndef CDROMRIPPER_H
#define CDROMRIPPER_H

#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>
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
    bool identifyDrive();
    bool openDrive();
    void closeDrive();
    void initializeParanoia();
    void computeDiskSectorQuantity();
    bool ripOneTrack(unsigned int indexTrack);
    QString buildWavFilename(unsigned int indexTrack);
    bool writeRawFile(uint8_t *audioTrackBuf,
                      unsigned int bufferSize,
                      const QString &wavFilename);
    void buildRipProgressMessage(unsigned int indexTrack,
                                 lsn_t currentSector,
                                 lsn_t firstSector,
                                 lsn_t lastSector);
    bool writeDiscId(const QString &dirPath);

    EMSCdrom m_cdromProperties;
    EMSRipProgress m_emsRipProgress;
    lsn_t m_diskSectorQuantity;

    // Type from the 'paranoia' library
    cdrom_drive_t *m_drive;
    cdrom_paranoia_t *m_paranoiaStruc;

signals:
    void ripProgressChanged(EMSRipProgress ripProgress);
    void resultReady(const QString &result);
};


#endif // CDROMRIPPER_H
