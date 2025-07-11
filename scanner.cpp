#include "scanner.h"
#include "utility.h"
#include "visual.h"
#include "motion.h"
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

bool Scanner::check_windows_and_crash()
{
    if (!bind_game_window(m_game_window_title))
    {
        emit log_message_Debug("Scanner::check_windows_and_crash: \n无法绑定游戏窗口！ // Unable to bind game window!");
        emit find_window_failed(GAME);
        return false;
    }
    if (!m_TPPW_hwnd) {
        emit send_warn("Scanner::check_windows_and_crash: \n通讯平台窗口句柄不存在！ // Alarm platform window handle is missing!");
        emit find_window_failed(TPPW);
        return false;
    }
    if (scan_crash_windows()) {
        emit game_crashed();
        return false;
    }
    if (!scan_window(m_game_window_hwnd)) {
        emit find_window_failed(GAME);
        return false;
    }
    if (!scan_window(m_TPPW_hwnd)) {
        emit find_window_failed(TPPW);
        return false;
    }
    emit log_message_Debug("Scanner::check_windows_and_crash: \n全部验证通过 // All validations passed");
    return true;
}

bool Scanner::bind_game_window(const QString &title)
{
    std::wstring w = title.toStdWString();
    this->m_game_window_hwnd = ::FindWindowW(nullptr, w.c_str());
    if (this->m_game_window_hwnd) return true;
    else return false;
}

bool Scanner::capture_and_analyze(QString &ocr_result_1, QString &ocr_result_2, QImage &pic_1, QImage &pic_2, QImage &screenshot)
{
    RECT rect;
    if (!GetWindowRect(m_game_window_hwnd, &rect))
    {
        emit log_message_Debug("Scanner::capture_and_analyze: \n获取窗口矩形失败 // Failed to get window rect");
        return false;
    }
    if (!analyze_game_window(m_game_window_hwnd, ocr_result_1, ocr_result_2, pic_1, pic_2, screenshot))
    {
        emit log_message_Debug("Scanner::capture_and_analyze: \n分析游戏窗口失败 // analyzeGameWindow failed");
        return false;
    }
    // 自动打开部落日志重试 / Retry opening tribe log if needed
    if (!ensure_tribe_log_open())
        return false;
    // 检测错误关键词（超时/断连） / Detect error keywords
    for (const QString &kw : m_game_timeout_keywords) {
        if (!kw.isEmpty() && ocr_result_2.contains(kw, Qt::CaseInsensitive)) {
            emit log_message_Debug("Scanner::capture_and_analyze: \n检测到连接丢失或超时 // Detected timeout: " + kw);
            emit log_message_User("游戏已掉线！");
            emit game_timeout();
            return false;
        }
    }
    return true;
}

bool Scanner::ensure_tribe_log_open()
{
    int tries = 0;
    QString OCR_result;
    QImage game_screenshot;
    if (!print_window(m_game_window_hwnd, game_screenshot))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n截图失败 // Capture failed!");
        return false;
    }
    if (!OCR_area_T(game_screenshot, OCR_result))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\nOCR failed");
        return false;
    }
    while (OCR_result.isEmpty() && tries < 10) {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n部落日志未打开，尝试自动打开 // Tribe log not open, retry");
        click_center_and_keyL(m_game_window_hwnd);
        QThread::msleep(500);
        if (!VisualProcessor::analyzeGameWindow(m_gameWindowHwnd, ocr1, ocr2, pic1, pic2, screenshot)) {
            emit logMessage("Alarm_doOneRound: 截图或 OCR 失败 // capture or OCR failed");
            return false;
        }
        if (!ocr2.isEmpty()) {
            emit gotPic_1(pic1);
            emit gotPic_2(pic2);
            break;
        }
        ++tries;
    }
    if (tries >= 10) {
        emit logMessage("Alarm_doOneRound: 无法打开部落日志 // Unable to open tribe log");
        return false;
    }
    return true;
}
