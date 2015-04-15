#ifndef WAVENCODER_H
#define WAVENCODER_H

#include <QObject>

class WavEncoder : public QObject
{
    Q_OBJECT

public:
    bool write(const QString &filename,
               const uint8_t *rawBuffer,
               const unsigned int bufferSize);
};

#endif // WAVENCODER_H
