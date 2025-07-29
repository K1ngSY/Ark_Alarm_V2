#include "crashhandler.h"
#include "motion.h"
#include "utility.h"
#include "visual.h"
#include <QThread>
#include <windows.h>
#include <QDebug>
#include <QDesktopServices>
#include <QUrl>

const QString GAME_TITLE = "ArkAscended";
CrashHandler::CrashHandler(QObject *parent)
    : KWorker{parent}
{
    m_crash_windows_keywords << "UE4-ShooterGame"
                             << "UE-ShooterGame"
                             << "Game has crashed and will close"
                             << "Crash!"
                             << "Error"
                             << "Shooter"
                             << "Shooter Crash Reporter";
}

void CrashHandler::handle_start_signal()
{
    resetCancel();
    emit log_message_Debug("CrashHandler::handle_start_signal:\n开始执行任务!");
    emit log_message_User("开始处理游戏崩溃…");

    close_crash_windows();
    HWND game_hwnd = nullptr;
    if (bind_window(GAME_TITLE, game_hwnd))
    {
        if (!waitForProcessExit(game_hwnd))
        {
            emit log_message_Debug("CrashHandler::handle_start_signal:\n等待游戏进程彻底结束15s");
            emit log_message_User("等待游戏进程彻底结束15s");
            QThread::msleep(15000);
        }
        else
        {
            emit log_message_Debug("CrashHandler::handle_start_signal:\n游戏进程被守护结束");
            emit log_message_User("陪游戏进程走完了最后一秒~");
        }
    }
    else
    {
        emit log_message_Debug("CrashHandler::handle_start_signal:\n未发现游戏进程，准备启动游戏");
        emit log_message_User("未发现游戏进程，准备启动游戏");
    }
    game_hwnd = nullptr;
    emit log_message_Debug("CrashHandler::handle_start_signal:\n正在启动游戏");
    emit log_message_User("正在启动游戏");
    QDesktopServices::openUrl(QUrl("steam://rungameid/2399830"));
    bool game_window_found = false;
    emit log_message_Debug("CrashHandler::handle_start_signal:\n等待游戏窗口出现（120 秒）…");
    emit log_message_User("等待游戏窗口出现（120 秒）…");
    for (int i = 0; i < 20; i++)
    {
        if (m_cancelRequested)
        {
            emit log_message_Debug("CrashHandler::handle_start_signal:\n收到cancel指令，即将退出");
            emit log_message_User("收到cancel指令，CrashHandler即将终止任务");
            return;
        }
        if(scan_window(GAME_TITLE))
        {
            game_window_found = true;
            emit log_message_Debug("CrashHandler::handle_start_signal:\n游戏窗口已出现");
            emit log_message_User("游戏窗口已出现");
            if (!bind_window(GAME_TITLE, game_hwnd))
            {
                emit log_message_Debug("CrashHandler::handle_start_signal:\n绑定游戏窗口句柄失败，重连游戏终止");
                emit log_message_User("绑定游戏窗口句柄失败，重连游戏终止");
                return;
            }
            break;
        }
        QThread::msleep(6000);
    }
    if (!game_window_found)
    {
        emit wait_game_window_timeout();
        emit log_message_Debug("CrashHandler::handle_start_signal:\n游戏窗口等待超时，重连任务终止");
        emit log_message_User("游戏窗口等待超时，重连任务终止");
        return;
    }
    emit log_message_Debug("CrashHandler::handle_start_signal:\n开始扫描开始游戏按钮");
    emit log_message_User("开始扫描开始游戏按钮");
    for (int i = 0; i < 10; i++)
    {
        if (m_cancelRequested)
        {
            emit log_message_Debug("CrashHandler::handle_start_signal:\n收到cancel指令，即将退出");
            emit log_message_User("收到cancel指令，CrashHandler即将终止任务");
            return;
        }
        if (check_start_button(game_hwnd))
        {
            emit log_message_Debug("CrashHandler::handle_start_signal:\n开始游戏按钮已出现，crash handle部分结束");
            emit log_message_User("开始游戏按钮已出现，crash handle部分结束");
            emit finished_0();
            return;
        }
        QThread::msleep(3000);
    }
    emit log_message_Debug("CrashHandler::handle_start_signal:\n开始游戏按钮等待超时，重连任务终止");
    emit log_message_User("开始游戏按钮等待超时，重连任务终止");
    emit wait_start_button_timeout();
}

void CrashHandler::handle_stop_signal()
{
    emit log_message_Debug("CrashHandler::handle_stop_signal:\n收到stop信号但不作相应！本worker请使用cancel()成员函数中止！");
}

void CrashHandler::close_crash_windows()
{
    // 1. 优先检查 Shooter Crash Reporter 窗口
    HWND shooter_reporter = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
    if (shooter_reporter)
    {
        emit log_message_Debug("CrashHandler::close_crash_windows:\n发现Shooter Crash Reporter崩溃窗口\n即将点击“send and close”…");
        // TODO：将下面坐标替换为实际的按钮坐标
        RECT r;
        ::GetWindowRect(shooter_reporter, &r);
        int x = (r.right - r.left);
        int y = (r.bottom - r.top);
        int sendAndCloseX = static_cast<int>(x * 0.74);  // 按钮相对于窗口左上角的 X 偏移
        int sendAndCloseY = static_cast<int>(y * 0.96);  // 按钮相对于窗口左上角的 Y 偏移
        left_click(shooter_reporter, sendAndCloseX, sendAndCloseY);
        QThread::msleep(500);
        HWND reporterHwnd = ::FindWindowW(nullptr, L"Shooter Crash Reporter");
        // 如果报告器不在了，就结束；否则继续尝试关闭
        if (reporterHwnd)
        {
            close_crash_windows();
        }
    }
    // 2. 如果没有 Shooter Crash Reporter，再用原有关键词逻辑关闭其它崩溃框
    struct EnumData
    {
        const QStringList *tokens;
        HWND found;
    };
    EnumData data
    {
        &m_crash_windows_keywords,
        nullptr
    };
    ::EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
        auto &d = *reinterpret_cast<EnumData*>(lparam);
        if (!::IsWindowVisible(hwnd)) return TRUE;
        wchar_t buf[512] = {0};
        ::GetWindowTextW(hwnd, buf, _countof(buf));
        QString title = QString::fromWCharArray(buf).trimmed();
        if (title.isEmpty()) return TRUE;
        for (const QString &t : *d.tokens)
        {
            if (!t.isEmpty() && title.contains(t, Qt::CaseInsensitive))
            {
                d.found = hwnd;
                return FALSE; // 停止枚举
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    // 关闭匹配到的窗口
    if (data.found) {
        wchar_t buf[512] = {0};
        ::GetWindowTextW(data.found, buf, _countof(buf));
        QString winTitle = QString::fromWCharArray(buf);
        emit log_message_Debug(QString("CrashHandler::close_crash_windows:\n检测到崩溃弹窗“%1”，准备关闭…").arg(winTitle));
        emit log_message_User(QString("检测到崩溃弹窗\"%1\"").arg(winTitle));
        ::SendMessageW(data.found, WM_CLOSE, 0, 0);
        QThread::msleep(500);
        close_crash_windows();
    }
    else {
        emit log_message_Debug("CrashHandler::close_crash_windows:\n崩溃弹窗已全部清理");
        emit log_message_User("崩溃弹窗已全部清理");
    }
    // end
}

bool CrashHandler::reboot_game()
{

}

bool CrashHandler::waitForProcessExit(HWND hwnd)
{
    // 1. 取出该窗口对应的进程 PID
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        qWarning() << "无法获取 PID";
        return false;
    }

    // 2. 打开该进程，以便等待它结束
    HANDLE hProcess = OpenProcess(SYNCHRONIZE, FALSE, pid);
    if (hProcess == NULL) {
        qWarning() << "OpenProcess 失败，错误码：" << GetLastError();
        return false;
    }

    // 3. 发送 WM_CLOSE 关闭窗口
    ::SendMessageW(hwnd, WM_CLOSE, 0, 0);

    // 4. 等待进程退出（无限期）
    DWORD waitResult = WaitForSingleObject(hProcess, INFINITE);
    if (waitResult == WAIT_OBJECT_0) {
        // 进程已经退出
        qDebug() << "进程 PID =" << pid << "已退出";
        // 在这里执行后续操作
    } else {
        qWarning() << "WaitForSingleObject 返回：" << waitResult;
    }

    // 5. 释放句柄
    CloseHandle(hProcess);
    return true;
}
