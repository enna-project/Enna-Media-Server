#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include "Database.h"
#include "Grabbers.h"

class Scanner : public QObject
{
    Q_OBJECT
public:
    explicit Scanner(QObject *parent = 0);
    ~Scanner();
    void locationAdd(const QString &location);

private:
    Database db;
    QThread *grabbersThread;
    Grabbers *grabbers;

signals:
    void grab(QString, QByteArray);

public slots:
    void fileFound(QString filename, QByteArray sha1);
    void filesProcessed();
    void metadataFound(QByteArray sha1);
};

#endif // SCANNER_H
