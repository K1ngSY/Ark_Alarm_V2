#ifndef KSUBTHREAD_H
#define KSUBTHREAD_H

#include <QObject>

class KSubthread : public QObject
{
    Q_OBJECT
public:
    explicit KSubthread(QObject *parent = nullptr);

public slots:
    virtual void start() = 0;
    virtual void stop() = 0;

signals:
    // finished without exception
    void finished_0();
    // finished with exception
    void finished_1(const QString &msg);

    void log_message_Debug(const QString &msg);
    void log_message_User(const QString &msg);
};

#endif // KSUBTHREAD_H
