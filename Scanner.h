#ifndef SCANNER_H
#define SCANNER_H

#include <QObject>
#include <QThread>
#include "Database.h"
#include "Grabbers.h"
#include "DirectoryWorker.h"

class Scanner : public QObject
{
    Q_OBJECT
public:
    explicit Scanner(QObject *parent = 0);
    ~Scanner();

    void locationAdd(const QString &location);
    void startScan();

private:
    QVector<QString> locations;
    QVector<DirectoryWorker*> workers;

    void scanEnd();

public slots:
    void fileFound(QString filename, QByteArray sha1);
    void workerFinished(DirectoryWorker* worker);
};

#endif // SCANNER_H
