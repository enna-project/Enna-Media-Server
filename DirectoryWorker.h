#ifndef DIRECTORYWORKER_H
#define DIRECTORYWORKER_H

#include <QObject>
#include <QDir>

class DirectoryWorker : public QObject
{
    Q_OBJECT
public:
    explicit DirectoryWorker(QString path, QString extensions, QObject *parent = nullptr);
    ~DirectoryWorker();

private:
    QString m_path;
    QStringList m_extensions;
    void scanDir(QDir dir);
    bool sha1Compute(QString filename, unsigned char *sha1);

signals:
    void finished(DirectoryWorker* me);
    void fileFound(QString, QByteArray);

public slots:
    void process();
};

#endif // DIRECTORYWORKER_H
