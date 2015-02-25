#ifndef GRABBERS_H
#define GRABBERS_H

#include <QObject>
#include <QJsonObject>
#include <QMap>
#include <QMutex>

class Grabbers : public QObject
{
    Q_OBJECT
public:
    explicit Grabbers(QObject *parent = 0);
    ~Grabbers();
   QJsonObject *metadataGet(QByteArray sha1);

private:
   QMap<QString, QJsonObject*> metadatas;
   QMutex m_mutex;

public slots:
   void grab(QString filename, QByteArray sha1);

signals:
   void metadataFound(QByteArray sha1);
};

#endif // GRABBERS_H
