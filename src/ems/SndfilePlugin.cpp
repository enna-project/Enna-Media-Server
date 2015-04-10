#include "SndfilePlugin.h"
#include <sndfile.h>

SndfilePlugin::SndfilePlugin()
{
    capabilities << "wav";
    capabilities << "aiff";
    capabilities << "raw";
    capabilities << "wavex";
    capabilities << "rf64";
    // Libsndfile can handle much more formats...
    // See: http://www.mega-nerd.com/libsndfile
}

SndfilePlugin::~SndfilePlugin()
{

}


bool SndfilePlugin::update(EMSTrack *track)
{
    SNDFILE* sndFile;
    SF_INFO sndFileInfo;

    sndFile = sf_open(track->filename.toUtf8().data(), SFM_READ, &sndFileInfo);
    if (sndFile == NULL)
    {
        return false;
    }

    track->duration = sndFileInfo.frames / sndFileInfo.samplerate;
    track->sample_rate = sndFileInfo.samplerate;
    track->format_parameters.clear();
    track->format_parameters.append(QString("channels:%1;").arg(sndFileInfo.channels));

    /* Retrieve bits per sample (for PCM stream only) */
    unsigned int formatSub = sndFileInfo.format & SF_FORMAT_SUBMASK;
    switch (formatSub)
    {
        case SF_FORMAT_PCM_S8:
            track->format_parameters.append("bits_per_sample:8;");
            break;
        case SF_FORMAT_PCM_16:
            track->format_parameters.append("bits_per_sample:16;");
            break;
        case SF_FORMAT_PCM_24:
            track->format_parameters.append("bits_per_sample:24;");
            break;
        case SF_FORMAT_PCM_32:
            track->format_parameters.append("bits_per_sample:32;");
            break;
        case SF_FORMAT_PCM_U8:
            track->format_parameters.append("bits_per_sample:8;");
            break;
        default:
            break;
    }

    return true;
}

