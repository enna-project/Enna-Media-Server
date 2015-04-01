#include "FlacPlugin.h"
#include <FLAC/all.h>

FlacPlugin::FlacPlugin()
{
    capabilities << "flac";
}

FlacPlugin::~FlacPlugin()
{

}

bool FlacPlugin::update(EMSTrack *track)
{
    FLAC__StreamMetadata *tags = 0;
    char *str;

    if (!FLAC__metadata_get_tags(track->filename.toUtf8().data(), &tags) || !tags)
    {
        return false;
    }

    for (unsigned int i = 0; i < tags->data.vorbis_comment.num_comments; i++)
    {

        str = (char *) tags->data.vorbis_comment.comments[i].entry;
        char *tmp_s = strdup(str);
        char *tmp = tmp_s;
        while (*tmp)
        {
            if ((*tmp >= 'a' ) && (*tmp <= 'z'))
                *tmp -= ('a'-'A');
            tmp++;
        }

        if (!strncmp(tmp_s, "TITLE=", 6))
        {
            if (track->name.isEmpty())
            {
                track->name = str + 6;
            }
        }
        else if (!strncmp(tmp_s, "ARTIST=", 7))
        {
            EMSArtist artist;
            artist.name = str + 7;
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
        else if (!strncmp(tmp_s, "ALBUM=", 6))
        {
            if (track->album.name.isEmpty())
            {
                EMSAlbum album;
                album.name = str + 6;
                track->album = album;
            }
        }
        else if (!strncmp(tmp_s, "GENRE=", 6))
        {
            EMSGenre genre;
            genre.name = str + 6;
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
        else if (!strncmp(tmp_s, "TRACKNUMBER=", 12))
        {
            int track_nb = atoi(str + 12);
            if (track_nb >= 0)
            {
                track->position = track_nb;
            }
        }
        else if (!strncmp(tmp_s, "DATE=", 5))
        {
            //int date = atoi(str + 5);
            //Not stored.
        }
        else if (!strncmp(tmp_s, "COMMENT=", 8))
        {
            //Not stored.
        }
    }

    return true;
}
