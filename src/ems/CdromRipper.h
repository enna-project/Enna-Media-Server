#ifndef CDROMRIPPER_H
#define CDROMRIPPER_H

#include <cdda.h>
#include <paranoia.h>
#include <QThread>
#include <QTimer>
#include <QMutex>

#include "Data.h"
#include "WavEncoder.h"
#include "FlacEncoder.h"

class CdromRipper : public QThread
{
    Q_OBJECT
    void run() Q_DECL_OVERRIDE;

public:
    CdromRipper(QObject *parent);
    ~CdromRipper();

    void setCdrom(const EMSCdrom &cdromProperties);
    void setAudioFormat(const QString &audioFormat);

private:
    bool identifyDrive();
    bool openDrive();
    void closeDrive();
    void initializeParanoia();
    void computeDiskSectorQuantity();
    bool ripOneTrack(unsigned int indexTrack);
    bool buildAudioFilenames(unsigned int indexTrack);
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
    QMutex m_emsRipProgressMutex;
    QString m_audioFormat;
    lsn_t m_diskSectorQuantity;

    WavEncoder m_wavEncoder;
    FlacEncoder m_flacEncoder;

    QTimer *m_ripProgressObserverTimer;

    QString m_wavFilenameCurrentTrack;
    QString m_flacFilenameCurrentTrack;

    // Type from the 'paranoia' library
    cdrom_drive_t *m_drive;
    cdrom_paranoia_t *m_paranoiaStruc;

signals:
    void ripProgressChanged(EMSRipProgress ripProgress);
    void resultReady(const QString &result);

public slots:
    void observeRipProgress();
};


#endif // CDROMRIPPER_H
