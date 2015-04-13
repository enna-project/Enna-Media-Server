#include "DsdPlugin.h"
#include <QFile>
#include <QDebug>

DsdPlugin::DsdPlugin()
{
    capabilities << "dsf";
    capabilities << "dff";
}

DsdPlugin::~DsdPlugin()
{

}

static inline uint32_t uint32BEToLE(uint32_t in)
{
    uint32_t ret = 0;
    ret |= (in >> 24 & 0x000000ff);
    ret |= (in >> 8  & 0x0000ff00);
    ret |= (in << 8  & 0x00ff0000);
    ret |= (in << 24 & 0xff000000);
    return ret;
}

static inline uint16_t uint16BEToLE(uint16_t in)
{
    uint16_t ret = 0;
    ret |= (in >> 8  & 0x00ff);
    ret |= (in << 8  & 0xff00);
    return ret;
}

static inline uint64_t dsdUint64toStd(DsdUint64 in, bool bigendian)
{
    uint64_t ret = 0;
    if(!bigendian)
    {
        ret = uint64_t(in.hi << 32) | in.lo;
    }
    else
    {
        ret |= uint32BEToLE(in.lo);
        ret = ret << 32;
        ret |= uint32BEToLE(in.hi);
    }
    return ret;
}

static void printID(DsdId id)
{
    qDebug() << "ID is : " << QString().sprintf("%c%c%c%c", id.value[0], id.value[1], id.value[2], id.value[3]);
}

bool DsdPlugin::getDsfMetadata(EMSTrack *track)
{
    QFile file(track->filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "Unable to open file : " << track->filename;
        return false;
    }

    DsfHeader header;
    if(file.read((char *)&header, sizeof(header)) != sizeof(header))
    {
        qCritical() << "Unable to read DSF header chunk from file : " << track->filename;
        file.close();
        return false;
    }

    if (!header.id.Equals("DSD "))
    {
        /* Not a DSF file, skip it */
        qDebug() << "The file " << track->filename << " is not in the DSF format";
        file.close();
        return false;
    }

    DsfFmtChunk format;
    if(file.read((char *)&format, sizeof(format)) != sizeof(format))
    {
        qCritical() << "Unable to read DSF format chunk from file  : " << track->filename;
        file.close();
        return false;
    }
    file.close();

    if (!format.id.Equals("fmt "))
    {
        /* Not a valid DSF file, skip it */
        qDebug() << "The file " << track->filename << " does not contain format chunk";
        return false;
    }

    track->sample_rate = format.sample_freq;
    track->duration = dsdUint64toStd(format.scnt, false) / format.sample_freq;

    track->format_parameters.clear();
    track->format_parameters.append(QString("channels:%1;").arg(format.channelnum));
    track->format_parameters.append(QString("bits_per_sample:%1;").arg(format.bitssample));

    /* If there is id3tag, set a custom offset. It will be used by the id3tag plugin */
    uint64_t offset = dsdUint64toStd(header.pmeta, false);
    if (offset > 0)
    {
        track->format_parameters.append(QString("id3tag_offset:%1;").arg(offset));
    }

    return true;
}

bool DsdPlugin::getDsdiffMetadata(EMSTrack *track)
{
    QFile file(track->filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "Unable to open file : " << track->filename;
        return false;
    }
    DsdiffHeader header;
    file.read((char *)&header, sizeof(header));

    DsdiffChunkHeader chunk;
    uint16_t channels = 0;
    uint32_t sample_rate = 0;
    DsdUint64 size;
    size.hi = 0;
    size.lo = 0;
    do
    {
        if(file.read((char *)&chunk, sizeof(chunk)) != sizeof(chunk))
        {
            qCritical() << "Unable to read DSDIFF header chunk from file : " << track->filename;
            file.close();
            return false;
        }

        if (chunk.id.Equals("PROP"))
        {
            DsdId id;
            if(!file.read((char *)&id, sizeof(id)) != sizeof(id))
            {
                if (id.Equals("SND "))
                {
                    unsigned int totalReadInChunk = sizeof(id);
                    while (totalReadInChunk < dsdUint64toStd(chunk.size, true))
                    {
                        DsdiffChunkHeader echunk;
                        if(file.read((char *)&echunk, sizeof(echunk)) == sizeof(echunk))
                        {
                            totalReadInChunk += sizeof(echunk);
                            if (echunk.id.Equals("FS  "))
                            {
                                unsigned int read = sizeof(sample_rate);
                                file.read((char *)&sample_rate, read);
                                totalReadInChunk += read;
                                if (dsdUint64toStd(echunk.size, true) > read)
                                {
                                    file.read(dsdUint64toStd(echunk.size, true)-read);
                                    totalReadInChunk += dsdUint64toStd(echunk.size, true)-read;
                                }
                            }
                            else if (echunk.id.Equals("CHNL "))
                            {
                                unsigned int read = sizeof(channels);
                                file.read((char *)&channels, read);
                                totalReadInChunk += read;
                                if (dsdUint64toStd(echunk.size, true) > read)
                                {
                                    file.read(dsdUint64toStd(echunk.size, true)-read);
                                    totalReadInChunk += dsdUint64toStd(echunk.size, true)-read;
                                }
                            }
                            else
                            {
                                file.read(dsdUint64toStd(echunk.size, true));
                                totalReadInChunk += dsdUint64toStd(echunk.size, true);
                            }
                        }
                    }
                }
                else
                {
                    file.read(dsdUint64toStd(chunk.size, true)-sizeof(id));
                }
            }
        }
        else if (chunk.id.Equals("DSD "))
        {
            size = chunk.size;
            break;
        }
        else
        {
            /* Skip this header chunk */
            file.read(dsdUint64toStd(chunk.size, true));
        }
    } while(true);

    if (uint32BEToLE(sample_rate)*uint16BEToLE(channels) > 0)
    {
        track->duration = dsdUint64toStd(size, true)*8 / (uint32BEToLE(sample_rate)*uint16BEToLE(channels));
    }
    track->sample_rate = uint32BEToLE(sample_rate);
    track->format_parameters.append(QString("channels:%1;").arg(uint16BEToLE(channels)));
    /* Guess that bits_per_sample is 1 */
    track->format_parameters.append(QString("bits_per_sample:1;"));
    file.close();
}

bool DsdPlugin::update(EMSTrack *track)
{
    /* Get duration, rates, and others data */
    if (track->format == "dsf")
    {
        return getDsfMetadata(track);
    }
    if (track->format == "dff")
    {
        return getDsdiffMetadata(track);
    }
    return true;
}

