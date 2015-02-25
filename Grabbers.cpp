#include <QDebug>
//#include <FLAC/all.h>
#include <QJsonObject>
#include <QJsonDocument>

#include "Grabbers.h"

Grabbers::Grabbers(QObject *parent) : QObject(parent)
{

}

Grabbers::~Grabbers()
{

}


QJsonObject *Grabbers::metadataGet(QByteArray sha1)
{
    m_mutex.lock();
    QJsonObject *obj = metadatas[sha1];
    metadatas.remove(sha1);
    m_mutex.unlock();
    return obj;
}

void Grabbers::grab(QString filename, QByteArray sha1)
{
#if 0
    FLAC__StreamMetadata *tags = 0;
    char *str;

    //qDebug() << "Grabber flac : " << filename;

    if (!FLAC__metadata_get_tags(filename.toUtf8().data(), &tags) || !tags)
    {
        //qDebug() << "cannot retrieve file " << filename << " tag";
        return;
    }

    QJsonObject *metadata = new QJsonObject();

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
            metadata->insert("title", str + 6);
        }
        else if (!strncmp(tmp_s, "ARTIST=", 7))
        {
            metadata->insert("artist", str + 7);
        }
        else if (!strncmp(tmp_s, "ALBUM=", 6))
        {
            metadata->insert("album", str + 6);
        }
        else if (!strncmp(tmp_s, "GENRE=", 6))
        {
            metadata->insert("genre", str + 6);
        }
        else if (!strncmp(tmp_s, "TRACKNUMBER=", 12))
        {
            int track_nb = atoi(str + 12);
            metadata->insert("track_nb", track_nb);
        }
        else if (!strncmp(tmp_s, "DATE=", 5))
        {
            int date = atoi(str + 5);

            metadata->insert("date", date);
        }
        else if (!strncmp(tmp_s, "COMMENT=", 8))
        {
            metadata->insert("comment", str + 8);
        }
    }

    m_mutex.lock();
    metadatas.insert(sha1, metadata);
    m_mutex.unlock();
    emit metadataFound(sha1);
#endif
}
