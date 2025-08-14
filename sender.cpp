#include "sender.h"
#include "motion.h"
#include "utility.h"
#include <QThread>
Sender::Sender(QObject *parent)
    : KWorker{parent}, m_in_CD_call(false), m_timer_making_call(nullptr)
{}

void Sender::send_text(HWND hwnd, QString message)
{
    if (!scan_window(hwnd))
    {
        emit log_message_Debug("Sender::send_text:\n尝试发送文本但窗口不存在！");
        emit log_message_User("尝试发送文本但窗口不存在！");
        return;
    }
    _send_text(hwnd, message);
}

void Sender::send_image(HWND hwnd, QImage image)
{
    if (!scan_window(hwnd))
    {
        emit log_message_Debug("Sender::send_image:\n尝试发送图片但窗口不存在！");
        emit log_message_User("尝试发送图片但窗口不存在！");
        return;
    }
    _send_image(hwnd, image);
}

void Sender::click_center_and_ESC(HWND hwnd)
{
    if (!scan_window(hwnd))
    {
        emit log_message_Debug("Sender::click_center_and_ESC:\n尝试点击但窗口不存在！");
        emit log_message_User("尝试点击但窗口不存在！");
        return;
    }
    ::click_center_and_ESC(hwnd);
}

void Sender::make_call(HWND hwnd, int x, int y)
{
    if (m_in_CD_call)
    {
        emit log_message_Debug("Sender::make_call:\n Sender is in CD");
        emit log_message_User("微信语音功能冷却中！");
    }
    else
    {
        emit log_message_Debug("Sender::make_call:\n Sender is not in CD");
        emit log_message_Debug("Sender::make_call(HWND hwnd, int x, int y)");
        left_click(hwnd, x, y);
        start_CD();
    }
}

void Sender::make_group_call(HWND hwnd, QString members, int x, int y)
{
    if (m_in_CD_call)
    {
        emit log_message_Debug("Sender::make_group_call:\n Sender is in CD");
        emit log_message_User("微信语音功能冷却中！");
        return;
    }
    emit log_message_Debug("Sender::make_group_call(HWND hwnd, QString members, int x, int y)");
    if (members == "[占位符]注意：您仍未设置群呼成员！")
    {
        emit log_message_User("您试图发起微信群语音，但未配置群呼成员，已被终止！");
        return;
    }
    left_click(hwnd, x, y);
    //等待弹窗完全出现.
    QThread::msleep(300);
    // 1) 找到“微信选择成员”对话框.
    std::wstring title = QStringLiteral("微信选择成员").toStdWString();
    HWND dlg = FindWindowW(nullptr, title.c_str());
    if (!dlg)
    {
        emit log_message_Debug("Sender::make_group_call:\n无法找到“微信选择成员”窗口");
        title = QStringLiteral("WeChat选择成员").toStdWString();
        dlg = FindWindowW(nullptr, title.c_str());
        if (!dlg)
        {
            emit log_message_Debug("Sender::make_group_call:\n无法找到“WeChat选择成员”窗口");
            return;
        }
        else
        {
            emit log_message_Debug("Sender::make_group_call:\n已找到“WeChat选择成员”窗口");
        }
    }
    else
    {
        emit log_message_Debug("Sender::make_group_call:\n已找到“微信选择成员”窗口");
    }

    SetForegroundWindow(dlg);
    QThread::msleep(30);

    // 2) 获取当前屏幕分辨率，计算横/纵缩放比例.
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    double scaleX = screenW / 1920.0;
    double scaleY = screenH / 1080.0;

    // 3) 解析用户输入的成员并依次搜索点击.
    QStringList list = members.split(",", Qt::SkipEmptyParts);
    for (const QString &s : list) {
        int baseX = 183;
        int baseY = 63;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Sender::make_group_call:\n点击搜索框，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
        paste_text(s.trimmed());
        emit log_message_Debug(QString("Sender::make_group_call:\n粘贴用户名 %1").arg(s.trimmed()));
        baseX = 127;
        baseY = 115;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Sender::make_group_call:\n点击首位，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
        baseX = 313;
        baseY = 59;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Sender::make_group_call:\n点击清除，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
    }

    // 4) 点击“确定”按钮（基准坐标 X,Y）.
    int btnX = int(447 * scaleX);
    int btnY = int(524 * scaleY);
    emit log_message_Debug(QString("Sender::make_group_call:\n点击确定呼叫按钮，坐标=(%1, %2)").arg(btnX).arg(btnY));
    left_click(dlg, btnX, btnY);

    emit log_message_Debug("群呼完成");
    start_CD();
}

void Sender::CD_helper()
{
    m_in_CD_call = false;
    if (m_timer_making_call)
    {
        if (m_timer_making_call->isActive())
        {
            m_timer_making_call->stop();
        }
        disconnect(m_timer_making_call_conn);
        m_timer_making_call->deleteLater();
        m_timer_making_call = nullptr;
    }
}

void Sender::start_CD()
{
    m_in_CD_call = true;
    if (!m_timer_making_call)
    {
        m_timer_making_call = new QTimer(this);
        m_timer_making_call->setSingleShot(true);
        m_timer_making_call_conn = connect(m_timer_making_call, &QTimer::timeout, this, &Sender::CD_helper);
    }
    if (!m_timer_making_call->isActive())
    {
        m_timer_making_call->start(180000);
        emit log_message_User("微信语音功能进入冷却(3min)！");
        emit log_message_Debug("Sender::start_CD:\n微信语音功能进入冷却(3min)！");
    }
}
