#include "scanner.h"

Scanner::Scanner(QObject *parent)
    : KSubthread{parent}
{}

void Scanner::set_TPPW_title(const QString &title)
{
    this->m_TPPW_title = title;
    emit log_message_Debug("Alarm_setAPTitle:\n已经设置新的微信窗口标题为" + m_TPPW_title);
    emit log_message_Debug("Alarm_setAPTitle:\nSet TPPW title to " + m_TPPW_title);
}

void Scanner::set_TPPW_hwnd(HWND TPPW_hwnd)
{
    this->m_TPPW_hwnd = TPPW_hwnd;
    emit log_message_Debug(QString("Alarm_setAPTitle:\n已经设置新的微信窗口句柄为 %1").arg((qulonglong)m_TPPW_hwnd));
    emit log_message_Debug(QString("Alarm_setAPTitle:\nSet TPPW Hwnd to %1").arg((qulonglong)m_TPPW_hwnd));
}

void Scanner::set_click_position(const int &x, const int &y)
{
    this->m_click_position_x = x;
    this->m_click_position_y = y;
    emit log_message_Debug(QString("Alarm_setCallButtonPosition:\n已更新微信电话坐标为 X: %1 Y: %2").arg(m_click_position_x).arg(m_click_position_y));
    emit log_message_Debug(QString("Alarm_setCallButtonPosition:\nSet click button position to X: %1 Y: %2").arg(m_click_position_x).arg(m_click_position_y));
}

void Scanner::start()
{

}
