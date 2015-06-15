#include "TagLibPlugin.h"
#include <QDebug>
#include <taglib/tag.h>
#include <taglib/fileref.h>

void TagLibPlugin::addArtist(EMSTrack *track, QString str)
{
    EMSArtist artist;
    artist.name = str;
    if (artist.name.isEmpty())
    {
        return;
    }

    bool found = false;
    foreach(EMSArtist existingArtist, track->artists)
    {
        if (existingArtist.name == artist.name)
        {
            found = true;
            break;
        }

    }
    if (!found)
    {
        track->artists.append(artist);
    }
}

void TagLibPlugin::addGenre(EMSTrack *track, QString str)
{
    EMSGenre genre;
    genre.name = str;
    if (genre.name.isEmpty())
    {
        return;
    }

    bool found = false;
    foreach(EMSGenre existingGenre, track->genres)
    {
        if (existingGenre.name == genre.name)
        {
            found = true;
            break;
        }

    }
    if (!found)
    {
        track->genres.append(genre);
    }
}


TagLibPlugin::TagLibPlugin()
{
    capabilities << "mp3";
    capabilities << "mpc";
    capabilities << "flac";
    capabilities << "mp4";
    capabilities << "asf";
    capabilities << "aiff";
    capabilities << "wav";
    capabilities << "ogg";
}

TagLibPlugin::~TagLibPlugin()
{

}


bool TagLibPlugin::update(EMSTrack *track)
{
    TagLib::FileRef file(track->filename.toUtf8().data());
    if (file.isNull())
    {
        return false;
    }

    /* Get track parameters (duration, ...) */
    if (track->duration == 0)
    {
        if (file.audioProperties()->length() > 0)
        {
            track->duration = (unsigned int)file.audioProperties()->length();
        }
    }
    if (track->sample_rate == 0)
    {
        if (file.audioProperties()->sampleRate() > 0)
        {
            track->sample_rate = (unsigned int)file.audioProperties()->sampleRate();
        }
    }
    if (track->format_parameters.isEmpty())
    {
        track->format_parameters += QString("channels:%1;").arg(file.audioProperties()->channels());
    }

    /* Get metatdata */
    if (track->name.isEmpty())
    {
        QString title = TStringToQString(file.tag()->title()).trimmed();

        if (!title.isEmpty())
        {
            track->name = title;
        }
    }

    if (track->position == 0)
    {
        track->position = file.tag()->track();
    }

    if (track->album.name.isEmpty())
    {
        track->album.name = TStringToQString(file.tag()->album()).trimmed();
    }

    addArtist(track, TStringToQString(file.tag()->artist()).trimmed());
    addGenre(track, TStringToQString(file.tag()->genre()).trimmed());

    return true;
}


