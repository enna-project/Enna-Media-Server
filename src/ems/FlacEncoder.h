#ifndef FLACENCODER_H
#define FLACENCODER_H

#include <QObject>
#include <FLAC++/metadata.h>
#include <FLAC++/encoder.h>

#define READSIZE 1024

class FlacEncoder : public QObject
{
    class InternalFlacEncoder : public FLAC::Encoder::File
    {
        public:
            InternalFlacEncoder();
            unsigned int m_lastEncodingProgress;
        protected:
            virtual void progress_callback(FLAC__uint64 bytes_written,
                                           FLAC__uint64 samples_written,
                                           unsigned frames_written,
                                           unsigned total_frames_estimate);
    };

    Q_OBJECT

public:
    bool encode();

    void setInputFilename(const QString &inputFilename);
    void setOutputFilename(const QString &outputFilename);
    void clearFilenames();

private:
    void initializeEncodingData();
    bool openWavInputFile();
    bool readWavHeader();
    bool configureInternalEncoder();
    bool addMetadata();
    bool initializeInternalEncoder();
    bool processEncoding();
    void closeEncoder();

    QString m_inputFilename;
    QString m_outputFilename;

    // FLAC encoding data
    FLAC__byte m_buffer[READSIZE/*samples*/ * 2/*bytes_per_sample*/ * 2/*channels*/];
    FLAC__int32 m_pcm[READSIZE/*samples*/ * 2/*channels*/];
    FlacEncoder::InternalFlacEncoder m_internalEncoder;
    FLAC__StreamEncoderInitStatus m_initStatus;
    FLAC__StreamMetadata *m_metadata[2];
    FLAC__StreamMetadata_VorbisComment_Entry m_entry;
    FILE *m_fileIn;
    unsigned m_totalSamples;  // we can use a 32-bit number due to WAVE size limitations
    unsigned m_sampleRate;
    unsigned m_channels;
    unsigned m_bps;
};

#endif // FLACENCODER_H
