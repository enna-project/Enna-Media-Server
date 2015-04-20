#include "FlacEncoder.h"
#include <QDebug>
#include <cstring>

void FlacEncoder::setInputFilename(const QString &inputFilename)
{
    m_inputFilename = inputFilename;
}

void FlacEncoder::setOutputFilename(const QString &outputFilename)
{
    m_outputFilename = outputFilename;
}

void FlacEncoder::clearFilenames()
{
    m_outputFilename.clear();
    m_inputFilename.clear();
}

bool FlacEncoder::encode(const EMSTrack *emsTrack)
{
    // preconditions
    if (m_inputFilename.isEmpty() || m_outputFilename.isEmpty())
        return false;

    initializeEncodingData();

    if (!openWavInputFile())
        return false;

    if (!readWavHeader())
    {
        closeEncoder();
        return false;
    }

    if (!configureInternalEncoder())
    {
        qCritical() << "FlacEncoder: Flac encoder configuration failed";
        closeEncoder();
        return false;
    }

    if (!addMetadata(emsTrack))
    {
        qCritical() << "FlacEncoder: add metadata failed";
        closeEncoder();
        return false;
    }

    if (!initializeInternalEncoder())
    {
        qCritical() << "FlacEncoder: internal encoder initialization failed";
        closeEncoder();
        return false;
    }

    if (!processEncoding())
    {
        qCritical() << "FlacEncoder: encoding process failed";
        closeEncoder();
        return false;
    }

    closeEncoder();
    return true;
}

void FlacEncoder::initializeEncodingData()
{
    m_channels = 0;
    m_bps = 0;
    m_fileIn = 0;

    memset(m_buffer, 0, READSIZE * 2 * 2 * sizeof(FLAC__byte));
    memset(m_pcm, 0, READSIZE * 2 * sizeof(FLAC__int32));
    memset(&m_totalSamples, 0, sizeof(unsigned));
    memset(&m_sampleRate, 0, sizeof(unsigned));
}

bool FlacEncoder::openWavInputFile()
{
    m_fileIn = fopen(qPrintable(m_inputFilename), "rb");

    if ((m_fileIn) == NULL)
    {
        qCritical() << "FlacEncoder: ERROR: opening " << m_inputFilename << " for input";
        return false;
    }
    return true;
}

bool FlacEncoder::readWavHeader()
{
    // Read the header
    if (fread(m_buffer, 1, 44, m_fileIn) != 44 ||
        memcmp(m_buffer, "RIFF", 4) ||
        memcmp(m_buffer + 8, "WAVEfmt \020\000\000\000\001\000\002\000", 16) ||
        memcmp(m_buffer + 32, "\004\000\020\000data", 8))
    {
        qCritical() << "FlacEncoder: ERROR: invalid/unsupported WAVE file, "
                    << "only 16bps stereo WAVE in canonical form allowed";
        return false;;
    }

    // Extract parameters from the header
    m_sampleRate = (m_buffer[27] << (8*3)) |
                   (m_buffer[26] << (8*2)) |
                   (m_buffer[25] << 8) |
                   m_buffer[24];
    m_channels = 2;
    m_bps = 16;
    m_totalSamples = ( (m_buffer[43] << 8*3) |
                       (m_buffer[42] << 8*2) |
                       (m_buffer[41] << 8) |
                       m_buffer[40]) / 4;
    return true;
}

bool FlacEncoder::configureInternalEncoder()
{
    bool ok = true;

    ok &= m_internalEncoder.set_verify(true);
    ok &= m_internalEncoder.set_compression_level(5);
    ok &= m_internalEncoder.set_channels(m_channels);
    ok &= m_internalEncoder.set_bits_per_sample(m_bps);
    ok &= m_internalEncoder.set_sample_rate(m_sampleRate);
    ok &= m_internalEncoder.set_total_samples_estimate(m_totalSamples);

    return ok;
}

bool FlacEncoder::addMetadata(const EMSTrack *emsTrack)
{
    bool ok = true;

    m_metadata[0] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_VORBIS_COMMENT);
    m_metadata[1] = FLAC__metadata_object_new(FLAC__METADATA_TYPE_PADDING);

    if ((m_metadata[0] == NULL) ||
        (m_metadata[1] == NULL))
    {
        qCritical() << "FlacEncoder: Metadata: metadata  object creation failed";
        return false;
    }

    // Add the artists
    foreach(EMSArtist artist, emsTrack->artists)
    {
        ok &= FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&m_entry,
                "ARTIST",
                qPrintable(artist.name));
        ok &= FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0],
                m_entry, /*copy=*/false);
    }

    // Add the genres
    foreach(EMSGenre genre, emsTrack->genres)
    {
        ok &= FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&m_entry,
                "GENRE",
                qPrintable(genre.name));
        ok &= FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0],
                m_entry, /*copy=*/false);
    }

    // Add the album
    ok &= FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&m_entry,
            "ALBUM",
            qPrintable(emsTrack->album.name));
    ok &= FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0],
            m_entry, /*copy=*/false);

    // Add the track title
    ok &= FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&m_entry,
            "TITLE",
            qPrintable(emsTrack->name));
    ok &= FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0],
            m_entry, /*copy=*/false);

    // Add the track number
    ok &= FLAC__metadata_object_vorbiscomment_entry_from_name_value_pair(&m_entry,
            "TRACKNUMBER",
            qPrintable(QString::number(emsTrack->position)));
    ok &= FLAC__metadata_object_vorbiscomment_append_comment(m_metadata[0],
            m_entry, /*copy=*/false);

    if (!ok)
    {
        qCritical() << "FlacEncoder: Metadata ERROR: out of memory or tag error";
        return false;
    }

    m_metadata[1]->length = 1234; /* set the padding length */

    return (m_internalEncoder.set_metadata(m_metadata, 2));
}

bool FlacEncoder::initializeInternalEncoder()
{
    m_initStatus = m_internalEncoder.init(qPrintable(m_outputFilename));
    if (m_initStatus != FLAC__STREAM_ENCODER_INIT_STATUS_OK)
    {
        qCritical() << "FlacEncoder: ERROR: initializing encoder: "
                    << FLAC__StreamEncoderInitStatusString[m_initStatus];
        return false;
    }
    return true;
}

bool FlacEncoder::processEncoding()
{
    bool ok = true;

    size_t left = (size_t)m_totalSamples;
    while (ok && left)
    {
        size_t need = (left>READSIZE? (size_t)READSIZE : (size_t)left);
        if (fread(m_buffer, m_channels * (m_bps/8), need, m_fileIn) != need)
        {
            qCritical() << "FlacEncoder: ERROR: reading from WAVE file";
            ok = false;
        }
        else
        {
            /* convert the packed little-endian 16-bit PCM samples from WAVE
             * into an interleaved FLAC__int32 buffer for libFLAC */
            size_t i;
            for (i = 0; i < need * m_channels; i++)
            {
                /* inefficient but simple and works on big- or little-endian machines */
                m_pcm[i] = (FLAC__int32)(((FLAC__int16)(FLAC__int8)m_buffer[2*i+1] << 8) |
                                         (FLAC__int16)m_buffer[2*i]);
            }
            /* feed samples to encoder */
            ok = m_internalEncoder.process_interleaved(m_pcm, need);
        }
        left -= need;
    }
    return ok;
}

void FlacEncoder::closeEncoder()
{
    m_internalEncoder.finish();
    qDebug() << "FlacEncoder:   state: "
             <<  m_internalEncoder.get_state().resolved_as_cstring(m_internalEncoder);

    FLAC__metadata_object_delete(m_metadata[0]);
    FLAC__metadata_object_delete(m_metadata[1]);

    fclose(m_fileIn);
    m_internalEncoder.m_lastEncodingProgress = 0;
}

FlacEncoder::InternalFlacEncoder::InternalFlacEncoder()
    : FLAC::Encoder::File()
{
    m_lastEncodingProgress = 0;
}

void FlacEncoder::InternalFlacEncoder::progress_callback(FLAC__uint64 bytes_written,
                                                         FLAC__uint64 samples_written,
                                                         unsigned frames_written,
                                                         unsigned total_frames_estimate)
{
    Q_UNUSED(bytes_written);
    Q_UNUSED(samples_written);

    // Print the FLAC encoding progress (every 10%)
    unsigned int progress = ((10 * frames_written) / total_frames_estimate);
    if ( progress > m_lastEncodingProgress)
    {
        m_lastEncodingProgress = progress;
        qDebug() << "FlacEncoder: progress: wrote "
                 << frames_written << "/" << total_frames_estimate << " frames";
    }
}
