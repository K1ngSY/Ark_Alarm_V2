#ifndef SCANNER_H
#define SCANNER_H

#include "kworker.h"
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QImage>
#include <windows.h>

enum window_type
{
    GAME,
    TPPW
};
enum call_type
{
    GROUP,
    SINGLE
};
/**
 * @class Scanner
 * @brief Manages game monitoring and alerting via a messaging platform.
 *        通过消息平台管理游戏监控和警报。
 */
class Scanner : public KWorker
{
    Q_OBJECT
public:
    /**
     * @brief Constructs an Scanner object.
     *        构造 Scanner 对象。
     * @param parent        Parent QObject pointer. 父对象指针。
     * @param TPPW_title       Title of the alarm platform window. 警报平台窗口标题。
     * @param TPPW_hwnd        Handle of the alarm platform window. 警报平台窗口句柄。
     */
    explicit Scanner(QObject *parent = nullptr);
    /**
     * @brief Destructor; stops monitoring and cleans up resources.
     *        析构函数；停止监控并清理资源。
     */
    ~Scanner();
    // setters

    /// Sets Third Party Platform Window title and Hwnd. 一次性更新标题和句柄。
    void set_TPPW(const QString &title, HWND TPPW_hwnd);
    /// Sets the click coordinates for making calls. 设置发起电话的坐标。
    void set_click_coordinates(const int &x, const int &y);

    /// Reminder: start_work function alse starts call members monitoring.
    bool start_work() override;
    bool stop_work() override;

private:
    // _______private members_______
    int m_mCycle_timer_interval;
    int m_mCall_member_timer_interval;
    // Third Party Platform Window Hwnd.
    HWND m_TPPW_hwnd;
    HWND m_game_window_hwnd;
    // Third Party Platform Window title.
    QString m_TPPW_title;
    QString m_game_window_title;
    QString m_call_members;
    QString m_log_file_path;

    QStringList m_all_log_keywords;
    QStringList m_serious_log_keywords;
    QStringList m_nonSerious_log_keywords;
    QStringList m_P_keywords;
    QStringList m_error_window_title_flags;
    QStringList m_game_timeout_keywords;

    int m_click_coordinate_x;
    int m_click_coordinate_y;

    bool m_first_round;
    bool m_is_group_call;
    bool m_need_text;
    bool m_need_call;

    QTimer *m_cycle_timer;
    QTimer *m_call_member_check_timer;

    QMap<QString, bool> m_alarm_filter;
    QMap<QString, QString> m_alarm_promts_Chinese;

    QMetaObject::Connection m_cycle_timer_conn;
    QMetaObject::Connection m_call_member_check_timer_conn;

    // _______private functions_______

    /// Reads and splits tribe log by time stamp
    // 部落日志持久化去重 & 分割 / Persist & split tribe logs.
    // 执行此函数之前手动检查部落日志识别结果是否为空.
    // Returns a QMap.
    // boolean(true) means it's serious.
    QMap<QString, QPair<QStringList, bool>> split_tribe_logs(const QString &raw_tribe_log);
    /// It appends log to the locale file if it's new log.
    bool append_new_log(const QString &ts, const QString &content);
    void send_text (const QString &msg) const;
    void send_image(const QImage &img) const;
    void make_call();
    void make_group_call();
    bool allow_this_keyword (const QString &key);

    // ______________Helpers for "scan" function______________
    // Returns true if passed all tests.
    // 无需收到false之后handle问题emit信号
    // 本函数内部检测到对应问题时就已经发出了对应错误信号.
    bool check_windows_and_crash();
    // Binds the Hwnd of the window has the entered title to m_game_window_hwnd
    bool bind_game_window(const QString &title);
    // 本函数内部检测到对应问题时就已经发出了对应错误信号.
    inline bool in_game_error(const QString &OCR_result_tribe)
    {
        // 检测错误关键词（超时/断连） / Detect error keywords
        for (const QString &kw : m_game_timeout_keywords) {
            if (!kw.isEmpty() && OCR_result_tribe.contains(kw, Qt::CaseInsensitive)) {
                emit log_message_Debug("Scanner::in_game_error: \n检测到连接丢失或超时 // Detected timeout: " + kw);
                emit log_message_User("游戏已掉线！");
                emit game_timeout();
                return true;
            }
        }
        return false;
    }
    // Only ensure, doesn't do anything else.
    bool ensure_tribe_log_open();

    bool check_parasaurolophus_alarm(const QString &ocr_result, QString &keyword_out);
    inline bool first_round()
    {
        if (this->m_first_round)
        {
            emit log_message_User("本轮为第一轮检测 仅入栈 不播报");
            this->m_first_round = false;
            return true;
        }
        else
        {
            return false;
        }
    }
    void handle_parasaurolophus_alert(const QString &keyword);
    // 本函数已经包含了对用户是否选择Text和call的判断.
    void handle_tribe_alerts(const QImage &screenshot, const QMap<QString, QPair<QStringList, bool>> &logs_map);
    QString make_time_stamp();
public slots:

private slots:
    /// The real function launches the main assign.
    void handle_start_signal() override;
    /// The real function stops the main assign.
    void handle_stop_signal() override;
    // Main function!
    void scan();
    void refresh_call_member();
    void initialize();

signals:
    void text_alarm_sent(const QString &keyword);
    void image_sent(const QImage &img);
    // window_type == GAME / TPPW.
    void find_window_failed(window_type failed_window_type);
    void TPPW_hwnd_failed();
    void game_crashed();
    void got_picture_P(const QImage &pic);
    void got_picture_log(const QImage &pic);
    void send_warn(const QString &msg);
    void game_timeout();
    void play_alarm_sound_P();
    void play_alarm_sound_log();
    void return_call_member(const QString &members);
    // type == GROUP / SINGLE.
    void made_call(call_type);
    void increase_round_count();
    void increase_alarm_count();
};

#endif // SCANNER_H
