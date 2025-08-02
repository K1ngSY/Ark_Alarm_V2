#include "sender.h"
#include "motion.h"
#include "utility.h"
Sender::Sender() {}

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
