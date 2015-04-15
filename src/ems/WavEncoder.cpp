#include "WavEncoder.h"
#include <sndfile.h>
#include <QDebug>

bool WavEncoder::write(const QString &filename,
                       const uint8_t *rawBuffer,
                       const unsigned int bufferSize)
{
    // Set file settings, 16bit Stereo PCM
    SF_INFO info;
    info.format = SF_FORMAT_WAV | SF_FORMAT_PCM_16;
    info.channels = 2;
    info.samplerate = 44100;

    // Open sound file for writing
    SNDFILE *sndFile = sf_open(qPrintable(filename), SFM_WRITE, &info);
    if (sndFile == NULL) {
        qCritical() << "WavEncoder: error opening sound file: "
                    << sf_strerror(sndFile);
        return false;
    }

    // Write
    unsigned int writtenBytes = sf_write_raw(sndFile, rawBuffer, bufferSize);

    // Check correct number of frames saved
    if (writtenBytes != bufferSize) {
        qCritical() << "WavEncoder: did not write enough frames for source";
        sf_close(sndFile);
        return false;
    }

    // Tidy up
    sf_write_sync(sndFile);
    sf_close(sndFile);

    return true;
}
