#include "scanner.h"
#include <QDebug>
#include <QThread>
Scanner::Scanner(QObject *parent)
    : KSubthread{parent}
{

}

Scanner::~Scanner()
{

}

void Scanner::set_TPPW(const QString &title, HWND TPPW_hwnd)
{
    this->m_TPPW_title = title;
    emit log_message_Debug("Alarm_set_TPPW:\n已经设置新的微信窗口标题为" + m_TPPW_title);
    emit log_message_Debug("Alarm_set_TPPW:\nSet TPPW title to " + m_TPPW_title);
    this->m_TPPW_hwnd = TPPW_hwnd;
    emit log_message_Debug(QString("Alarm_set_TPPW:\n已经设置新的微信窗口句柄为 %1").arg((qulonglong)m_TPPW_hwnd));
    emit log_message_Debug(QString("Alarm_set_TPPW:\nSet TPPW Hwnd to %1").arg((qulonglong)m_TPPW_hwnd));
}

void Scanner::set_click_coordinates(const int &x, const int &y)
{
    this->m_click_coordinate_x = x;
    this->m_click_coordinate_y = y;
    emit log_message_Debug(QString("Alarm_set_click_coordinates:\n已更新微信电话坐标为 X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
    emit log_message_Debug(QString("Alarm_set_click_coordinates:\nSet click button coordinate to X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
}

bool Scanner::start()
{
    // Do some checks before emit, check if it's all set.
    emit start_signal();
    qDebug() << "start";
    return true;
}

bool Scanner::stop()
{
    // Do some checks before emit, check if it's all set.
    emit stop_signal();
    qDebug() << "stop";
    return true;
}
