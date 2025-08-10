#ifndef CRASHHANDLER_H
#define CRASHHANDLER_H

#include "kworker.h"
#include <QStringList>
#include <QTimer>
#include <QElapsedTimer>
#include <windows.h>

class CrashHandler : public KWorker
{
    Q_OBJECT
public:
    explicit CrashHandler(QObject *parent = nullptr);
    inline void cancel() override
    {
        m_cancelRequested.store(true);
        emit log_message_Debug(QString("CrashHandler::cancel:\n原子状态为[%1]").arg(m_cancelRequested.load()? "canceled" : "not canceled"));
        // emit log_message_User(QString("原子状态为[%1]").arg(m_cancelRequested.load()? "canceled" : "not canceled"));
    }
private slots:
    // handle_start_signal() 是总流程.
    void handle_start_signal() override;
    void handle_stop_signal() override;


private:
    QStringList m_crash_windows_keywords;

    // Private functions.
    // 此函数只关闭所有崩溃弹窗不关闭游戏.
    void close_crash_windows();
    void start_game();
    bool waitForProcessExit(HWND hwnd);

signals:
    void wait_game_window_timeout();
    void wait_start_button_timeout();
    void got_game_hwnd(HWND game_hwnd);
};

#endif // CRASHHANDLER_H
