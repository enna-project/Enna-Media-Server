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
    bool isFinished = false;
    QString result("Rip: OK");
    unsigned int loop = 0;

    qDebug() << "CdromRipper: start the rip process";

    while (!isFinished)
    {
        sleep(1);
        qDebug() << "CdromRipper: rip in progress..." << loop;
        if (++loop > 5)
            isFinished = true;
    }

    qDebug() << "CdromRipper: end of the rip process";
    emit resultReady(result);
}

