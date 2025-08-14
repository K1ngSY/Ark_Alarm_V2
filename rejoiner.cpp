#include "rejoiner.h"
#include "utility.h"
#include "motion.h"
#include "visual.h"
#include <windows.h>
#include <QThread>
Rejoiner::Rejoiner(QObject *parent)
    : KWorker{parent}
{
    m_cancelRequested.store(false);
    m_game_hwnd = nullptr;
    m_server_ID = "";
    m_has_mod = false;
}

bool Rejoiner::start_work()
{
    if (!m_game_hwnd)
    {
        emit log_message_Debug("Rejoiner::start_work:\nGame HWND is unknown!");
        emit log_message_User("Game HWND is unknown!");
        return false;
    }
    if (m_server_ID.isEmpty())
    {
        emit log_message_Debug("Rejoiner::start_work:\nServer ID is unknown!");
        emit log_message_User("Server ID is unknown!");
        return false;
    }
    emit start_signal();
    return true;
}

bool Rejoiner::click_start()
{
    if (!scan_window(m_game_hwnd))
    {
        emit log_message_Debug("Rejoiner::click_start:\n游戏窗口不存在");
        emit log_message_User("游戏窗口不存在");
        return false;
    }
    RECT rc;
    if (!GetWindowRect(m_game_hwnd, &rc)) {
        emit log_message_Debug("Rejoiner::click_start:\n获取窗口尺寸失败");
        emit log_message_User("获取窗口尺寸失败");
        return false;
    }
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;

    left_click(m_game_hwnd, w/2, h*43/54);
    emit log_message_Debug("Rejoiner::click_start:\n已点击开始游戏按钮");
    emit log_message_User("已点击开始游戏按钮");
    return true;
}

bool Rejoiner::click_join_card()
{
    if (!scan_window(m_game_hwnd))
    {
        emit log_message_Debug("Rejoiner::click_join_card:\n游戏窗口不存在");
        emit log_message_User("重连:游戏窗口不存在");
        return false;
    }
    if (check_5_cards(m_game_hwnd))
    {
        emit log_message_User("重连:检测到有五张加入游戏卡片，使用5卡模式");
        click_center(m_game_hwnd);
    }
    else
    {
        RECT rc; GetWindowRect(m_game_hwnd, &rc);
        int w = rc.right - rc.left, h = rc.bottom - rc.top;
        int x = w * 0.32, y = h / 2;
        emit log_message_User("重连:未检测到有五张加入游戏卡片，使用普通模式");
        left_click(m_game_hwnd, x, y);
    }
    emit log_message_Debug("Rejoiner::click_join_card:\n已点击加入游戏卡片");
    emit log_message_User("重连:已点击加入游戏卡片");
    return true;
}

bool Rejoiner::search_server()
{
    if (!scan_window(m_game_hwnd))
    {
        emit log_message_Debug("Rejoiner::search_server:\n游戏窗口不存在");
        emit log_message_User("游戏窗口不存在");
        return false;
    }
    RECT rc; GetWindowRect(m_game_hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int x = w * 0.88, y = h * 0.18;
    left_click(m_game_hwnd, x, y);
    emit log_message_Debug("Rejoiner::search_server:\n已点击搜索框");
    emit log_message_User("已点击搜索框");
    QThread::msleep(200);
    paste_text(m_server_ID);
    emit log_message_User("已粘贴服务器代码" + m_server_ID);
    return true;
}

bool Rejoiner::select_first_server()
{
    RECT rc; GetWindowRect(m_game_hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    int x = w/2, y = h*31/100;  // 0.31
    left_click(m_game_hwnd, x, y);
    emit log_message_Debug("Rejoiner::select_first_serve:\n已点击列表第一个服务器");
    emit log_message_User("已点击列表第一个服务器");
    return true;
}

bool Rejoiner::check_mod(bool &has, QImage &debug_img)
{
    if (!scan_window(m_game_hwnd))
    {
        emit log_message_Debug("Rejoiner::check_mod:\n游戏窗口不存在");
        emit log_message_User("游戏窗口不存在");
        return false;
    }
    has = false;
    if (check_server_mod(m_game_hwnd, debug_img))
    {
        emit log_message_Debug("Rejoiner::check_mod:\n服务器含Mod");
        emit log_message_User("服务器含Mod");
        has = true;
    } else {
        emit log_message_Debug("Rejoiner::check_mod:\n服务器不含Mod");
        emit log_message_User("服务器不含Mod");
    }
    return true;
}

void Rejoiner::join_with_mod(bool &conn_failed)
{
    conn_failed = false;
    emit log_message_Debug("Rejoiner::join_with_mod:\n点击 Join");
    emit log_message_User("点击 Join");
    RECT rc; GetWindowRect(m_game_hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    left_click(m_game_hwnd, w*88/100, h*88/100);

    QThread::msleep(3000);

    left_click(m_game_hwnd, w*535/1920, h*935/1080);
    emit log_message_Debug("Rejoiner::join_with_mod:\n点击 Mod 下载确认按钮");
    emit log_message_User("点击 Mod 下载确认按钮");

    // 等待下载完成
    while (check_donwload(m_game_hwnd) && !m_cancelRequested.load())
    {
        emit log_message_Debug("Rejoiner::join_with_mod:\n正在下载 Mod...");
        emit log_message_User("正在下载 Mod...");
        QThread::msleep(500);
    }

    if (m_cancelRequested.load()) return;

    emit log_message_Debug("Rejoiner::join_with_mod:\n加入游戏中，30秒后重启监控进程！");
    emit log_message_User("加入游戏中，30秒后重启监控进程！");
    int tries = 0;
    while (!m_cancelRequested.load() && tries++ < 40 && !conn_failed)
    {
        conn_failed = !check_connection_health(m_game_hwnd);
        QThread::msleep(1500);
    }

    if (m_cancelRequested.load()) return;
    if (conn_failed) return;
}

void Rejoiner::join_without_mod(bool &conn_failed)
{
    conn_failed = false;
    emit log_message_Debug("Rejoiner::join_with_mod:\n点击 Join");
    emit log_message_User("点击 Join");
    RECT rc; GetWindowRect(m_game_hwnd, &rc);
    int w = rc.right - rc.left, h = rc.bottom - rc.top;
    left_click(m_game_hwnd, w*88/100, h*88/100);
    QThread::msleep(3000);

    if (m_cancelRequested.load()) return;

    emit log_message_Debug("Rejoiner::join_with_mod:\n加入游戏中，30秒后重启监控进程！");
    emit log_message_User("加入游戏中，30秒后重启监控进程！");

    int tries = 0;
    while (!m_cancelRequested.load() && tries++ < 40 && !conn_failed)
    {
        conn_failed = !check_connection_health(m_game_hwnd);
        QThread::msleep(1500);
    }
    if (m_cancelRequested.load()) return;
    if (conn_failed) return;
}

void Rejoiner::handle_start_signal()
{
    resetCancel();
    if (!scan_window(m_game_hwnd))
    {
        emit log_message_Debug("Rejoiner::handle_start_signal:\n游戏窗口不存在，重连终止！");
        emit log_message_User("游戏窗口不存在");
        emit finished_1("Rejoiner::handle_start_signal:游戏窗口不存在");
        return;
    }
    bool conn_failed = false;
    do
    {
        if (!click_start())
        {
            emit log_message_Debug("Rejoiner::handle_start_signal:\n点击开始游戏按钮失败，重连终止！");
            emit log_message_User("点击开始游戏按钮失败");
            emit finished_1("Rejoiner::handle_start_signal:点击开始游戏按钮失败");
            return;
        }
        if (!click_join_card())
        {
            emit log_message_Debug("Rejoiner::handle_start_signal:\n点击加入游戏卡片失败，重连终止！");
            emit log_message_User("点击加入游戏卡片失败");
            emit finished_1("Rejoiner::handle_start_signal:点击加入游戏卡片失败");
            return;
        }
        if (!search_server())
        {
            emit log_message_Debug("Rejoiner::handle_start_signal:\n搜索服务器失败，重连终止！");
            emit log_message_User("搜索服务器失败");
            emit finished_1("Rejoiner::handle_start_signal:搜索服务器失败");
            return;
        }
        emit log_message_Debug("Rejoiner::handle_start_signal:\nsleep 5秒等待服务器搜索加载");
        emit log_message_User("等待服务器列表加载5秒");
        QThread::msleep(5000);
        if (!select_first_server())
        {
            emit log_message_Debug("Rejoiner::handle_start_signal:\n选择第一个服务器失败，重连终止！");
            emit log_message_User("选择第一个服务器失败");
            emit finished_1("Rejoiner::handle_start_signal:选择第一个服务器失败");
            return;
        }
        bool has_mod = m_has_mod;
        if (!has_mod)
        {
            QImage debug_img;
            if (!check_mod(has_mod, debug_img))
            {
                emit log_message_Debug("Rejoiner::handle_start_signal:\n检测是否存在mod失败，重连终止！");
                emit log_message_User("检测是否存在mod失败");
                emit finished_1("Rejoiner::handle_start_signal:检测是否存在mod失败");
                return;
            }
        }
        // 此处不是重复检测 仔细读这段的逻辑
        if (has_mod)
        {
            join_with_mod(conn_failed);
        }
        else
        {
            join_without_mod(conn_failed);
        }

        if (conn_failed)
        {
            if (!scan_window(m_game_hwnd))
            {
                emit log_message_Debug("Rejoiner::handle_start_signal:\n游戏窗口不存在，重连终止！");
                emit log_message_User("游戏窗口不存在");
                emit finished_1("Rejoiner::handle_start_signal:游戏窗口不存在");
                return;
            }
            RECT rc; GetWindowRect(m_game_hwnd, &rc);
            int w = rc.right - rc.left, h = rc.bottom - rc.top;
            press_key(VK_ESCAPE);
            // 点击左下角BACK
            left_click(m_game_hwnd, w / 1920 * 165, h / 1080 * 882);
        }
    }
    while (conn_failed && !m_cancelRequested.load());
    if (!conn_failed)
    {
        emit finished_0();
    }
    else
    {
        emit log_message_Debug("Rejoiner::handle_start_signal:\n尝试处理重连异常失败，重连终止！");
        emit log_message_User("尝试处理重连异常失败，请您手动重连对局");
        emit finished_1("Rejoiner::handle_start_signal:尝试处理重连异常失败");
        return;
    }
}

void Rejoiner::handle_stop_signal()
{
    emit log_message_Debug("Rejoiner::handle_stop_signal:\n收到stop信号但不作相应！本worker请使用cancel()成员函数中止！");
}
