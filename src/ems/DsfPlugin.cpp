#include "DsfPlugin.h"
#include <QFile>
#include <QDebug>

DsfPlugin::DsfPlugin()
{
    capabilities << "dsf";
    capabilities << "dff";
}

DsfPlugin::~DsfPlugin()
{

}


bool DsfPlugin::update(EMSTrack *track)
{
    QFile file(track->filename);
    if (!file.open(QIODevice::ReadOnly))
    {
        qCritical() << "Unable to open file : " << track->filename;
        return false;
    }

    DsfHeader header;
    if(!file.read((char *)&header, sizeof(header)) != sizeof(header))
    {
        qCritical() << "Unable to read DSF header chunk from file : " << track->filename;
    }

    if (!header.id.Equals("DSD "))
    {
        /* Not a DSF file, skip it */
        qDebug() << "The file " << track->filename << " is not in the DSF format";
        return true;
    }

    DsfFmtChunk format;
    if(!file.read((char *)&format, sizeof(format)) != sizeof(format) || !format.id.Equals("fmt "))
    {
        qCritical() << "Unable to read DSF format chunk from file : " << track->filename;
    }

    track->sample_rate = format.sample_freq;
    track->duration = format.scnt.Read() / format.sample_freq;

    track->format_parameters.clear();
    track->format_parameters.append(QString("channels:%1;").arg(format.channelnum));
    track->format_parameters.append(QString("bits_per_sample:%1;").arg(format.bitssample));

    /* If there is id3tag, set a custom offset. It will be used by the id3tag plugin */
    uint64_t offset = header.pmeta.Read();
    if (offset > 0)
    {
        track->format_parameters.append(QString("id3tag_offset:%1;").arg(offset));
    }

    return true;
}

