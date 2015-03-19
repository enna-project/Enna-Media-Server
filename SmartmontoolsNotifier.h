#ifndef SMARTMONTOOLSNOTIFIER_H
#define SMARTMONTOOLSNOTIFIER_H

#include <QObject>
#include <QtDBus/QtDBus>

class smartmontoolsNotifier : public QObject
{
    Q_OBJECT

    QDBusInterface *m_interface;
public:
    explicit smartmontoolsNotifier(QObject *parent = 0);
    ~smartmontoolsNotifier();

signals:
    void warnUser();
public slots:
    void messageReceived(QString message);
};

#endif // SMARTMONTOOLSNOTIFIER_H
