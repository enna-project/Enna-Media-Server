#include <QDebug>
#include "CdromRipper.h"

CdromRipper::CdromRipper(QObject *parent)
    : QThread(parent)
{}

void CdromRipper::setCdrom(const EMSCdrom &cdromProperties)
{
    m_cdromProperties = cdromProperties;
}

/* ---------------------------------------------------------
 *                  THREAD MAIN FUNCTION
 * --------------------------------------------------------- */
/* Main thread function
 * This function is called once and must handle every blocking action.
 * All shared class members (= the one used by the API) MUST be protected by the global mutex.
 */
void CdromRipper::run()
{
    QString result("Rip: OK");

    // EMSCdrom stub for tests
    unsigned int nbTracks = 7, currentTrack = 0, nbSectors, currentSector = 0;
    unsigned int trackBegins[] = {0,3,5,9,12,14,16};
    unsigned int trackEnds[] = {2,4,8,11,13,15,19};

    nbSectors = trackEnds[nbTracks - 1];

    qDebug() << "CdromRipper: start the rip process";

    while (currentTrack < nbTracks)
    {
        for (unsigned int i=trackBegins[currentTrack];
             i<=trackEnds[currentTrack]; ++i)
        {
            currentSector++;
            // Build the rip progress message
            EMSRipProgress ripProgress;
            ripProgress.track_in_progress = currentTrack;
            ripProgress.overall_progress = 100 * currentSector / nbSectors;
            ripProgress.track_progress = 100 * i / (trackEnds[currentTrack] - trackBegins[currentTrack] + 1);
            emit ripProgressChanged(ripProgress);

            // Rip the current sector
            sleep(1);
        }
        currentTrack++;
    }

    qDebug() << "CdromRipper: end of the rip process";
    emit resultReady(result);
}
