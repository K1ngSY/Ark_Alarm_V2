#ifndef KWORKER_H
#define KWORKER_H

#include <QObject>
#include <atomic>

class KWorker : public QObject
{
    Q_OBJECT
public:
    explicit KWorker(QObject *parent = nullptr);
    // only access to start its work.
    // must emit "start_signal".
    virtual bool start_work()
    {
        emit start_signal();
        return true;
    }
    // only access to stop itself.
    // must emit "stop_signal".
    virtual bool stop_work()
    {
        emit stop_signal();
        return true;
    }

    // 在主线程中调用，用来通知取消
    virtual void cancel() { m_cancelRequested.store(true); }

    // 每次启动新任务前，都要重置
    virtual void resetCancel() { m_cancelRequested.store(false); }

private slots:
    virtual void handle_start_signal() = 0;
    virtual void handle_stop_signal() = 0;

protected:
    std::atomic<bool> m_cancelRequested;

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

#endif // KWORKER_H
