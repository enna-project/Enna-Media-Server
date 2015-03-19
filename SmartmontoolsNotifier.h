#ifndef SMARTMONTOOLSNOTIFIER_H
#define SMARTMONTOOLSNOTIFIER_H

#include <QObject>
#include <QtDBus/QtDBus>

class SmartmontoolsNotifier : public QObject
{
    Q_OBJECT

    QDBusInterface *m_interface;
public:
    explicit SmartmontoolsNotifier(QObject *parent = 0);
    ~SmartmontoolsNotifier();

signals:
    void warnUser();
public slots:
    void messageReceived(QString message);
};

#endif // SMARTMONTOOLSNOTIFIER_H
