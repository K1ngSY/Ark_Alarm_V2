#ifndef KSUBTHREAD_H
#define KSUBTHREAD_H

#include <QObject>

class KSubthread : public QObject
{
    Q_OBJECT
public:
    explicit KSubthread(QObject *parent = nullptr);
    // only access to start its assign.
    // must emit "start_signal".
    virtual bool start()
    {
        emit start_signal();
        return true;
    }
    // only access to stop itself.
    // must emit "stop_signal".
    virtual bool stop()
    {
        emit stop_signal();
        return true;
    }

private slots:
    virtual void handle_start_signal() = 0;
    virtual void handle_stop_signal() = 0;
signals:
    // start signals

    // start mission signal.
    void start_signal();
    // stop mission signal.
    void stop_signal();

    // finish signals

    // finished without exception.
    void finished_0();
    // finished with exception.
    void finished_1(const QString &msg);\

    // send message to dashboard log area
    void log_message_Debug(const QString &msg);
    void log_message_User(const QString &msg);
};

#endif // KSUBTHREAD_H
