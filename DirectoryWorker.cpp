#include <QDebug>
#include <QSettings>
#include <QThread>
#include <QDebug>
#include <QCoreApplication>
#include "sha1.h"
#include "DirectoryWorker.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

DirectoryWorker::DirectoryWorker(QString path, QString extensions, QObject *parent) :
    QObject(parent),
    m_path(path)

{
    m_extensions = extensions.split(",");
}

DirectoryWorker::~DirectoryWorker()
{

}

void DirectoryWorker::process()
{
    qDebug() << Q_FUNC_INFO << QThread::currentThreadId();
    QDir rootDir(m_path);
    scanDir(rootDir);
    emit finished();
    QCoreApplication::processEvents();
}

void DirectoryWorker::scanDir(QDir dir)
{

    dir.setNameFilters(m_extensions);
    dir.setFilter(QDir::Files | QDir::NoDotAndDotDot | QDir::NoSymLinks);

    //qDebug() << "Scanning: " << dir.path();

    QStringList fileList = dir.entryList();
    for (int i=0; i<fileList.count(); i++)
    {
        char sha1[20];

        QString fullPath = dir.absolutePath() + "/" + fileList[i];
        sha1Compute(fullPath, (unsigned char*)sha1);
        QByteArray byteArray(sha1);
        //qDebug() << Q_FUNC_INFO << QThread::currentThreadId();
        emit fileFound(fullPath, byteArray);
        //qDebug() << "Found file: " << fullPath;
    }

    dir.setFilter(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::NoSymLinks);
    QStringList dirList = dir.entryList();
    for (int i=0; i<dirList.size(); ++i)
    {
        QString newPath = QString("%1/%2").arg(dir.absolutePath()).arg(dirList.at(i));
        scanDir(QDir(newPath));
    }
}

/* Create a (unique?) sha1 based on content of the file */
bool
DirectoryWorker::sha1Compute(QString filename, unsigned char *sha1)
{
    FILE *fd;
    quint64 fsize;
    unsigned int rsize = 0;
    quint8 *tmp;
    SHA1 ctx;

    fd = fopen(filename.toUtf8().data(), "rb");
    if (!fd)
      {
         qDebug() << "Error opening " << filename;
         return false;
      }

    if (!sha1)
      return false;

    sha1_init(&ctx);

    fseek(fd, 0, SEEK_END);
    fsize = ftell(fd);
    fseek(fd, 0, SEEK_SET);

    rsize = MIN(65536, fsize);
    tmp = (quint8*)calloc(rsize, sizeof(uint8_t));

    fread(tmp, 1, rsize, fd);
    sha1_update(&ctx, tmp, rsize);
    fseek(fd, (long)(fsize - 65536), SEEK_SET);
    fread(tmp, 1, rsize, fd);
    sha1_update(&ctx, tmp, rsize);
    sha1_final(&ctx, sha1);

    free(tmp);
    fclose(fd);

    return true;
}
