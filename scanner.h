#ifndef SCANNER_H
#define SCANNER_H

#include "ksubthread.h"
#include <QObject>
#include <QTimer>
#include <QMap>
#include <QSet>
#include <QString>
#include <QStringList>
#include <QImage>
#include <windows.h>

enum window_type {
    GAME,
    TPPW
};

/**
 * @class Scanner
 * @brief Manages game monitoring and alerting via a messaging platform.
 *        通过消息平台管理游戏监控和警报。
 */
class Scanner : public KSubthread
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

    /// Reminder: start function alse starts call members monitoring.
    bool start() override;
    bool stop() override;

private:
    // _______private members_______

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

    // _______private functions_______

    /// Reads and splits tribe log by time stamp
    QList<QPair<QString, QString>> split_tribe_log_entries(const QString &raw_tribe_log);
    /// It appends log to the locale file if it's new log.
    bool append_new_log(const QString &ts, const QString &content);
    void send_text (const QString &msg) const;
    void send_image(const QImage &img) const;
    void make_call();
    void make_group_call();
    bool allow_this_keyword (const QString &key);

    // ______________Helpers for "scan" function______________
    bool check_windows_and_crash();
    // Binds the Hwnd of the window has the entered title to m_game_window_hwnd
    bool bind_game_window(const QString &title);
    bool capture_and_analyze(QString &ocr_result1,
                             QString &ocr_result2,
                             QImage &pic1,
                             QImage &pic2,
                             QImage &screenshot);
    bool ensure_tribe_log_open(QString &ocr_result1,
                               QString &ocr_result2,
                               QImage &pic1,
                               QImage &pic2,
                               QImage &screenshot);
    bool detect_parasaurolophus(const QString &ocr_result1,
                                QString &keyword_out);
    bool handle_first_round();
    void handle_parasaurolophus_alert(const QString &ocr_result1,
                                      const QString &paras_keyword);
    void handle_tribe_alerts(const QString &ocr_result2,
                             const QImage &screenshot,
                             const QMap<QString, QList<QString>> &logs_map);
public slots:

private slots:
    /// The real function launches the main assign.
    void handle_start_signal() override;
    /// The real function stops the main assign.
    void handle_stop_signal() override;
    void scan();
    void refresh_call_member();
    void initialize();

signals:
    void text_alarm_sent(const QString &keyword);
    void image_sent(const QImage &img);
    /// window_type == GAME / TPPW
    void find_window_failed(window_type failed_window_type);
    void game_crashed();
    void got_picture_P(const QImage &pic);
    void got_picture_log(const QImage &pic);
    void send_warn(const QString &msg);
    void game_timeout();
    void play_slarm_sound_P();
    void play_slarm_sound_log();
    void return_call_member(const QString &members);
    /// type == GROUP / SINGLE
    void made_call(const QString &type);
    void increase_round_count();
    void increase_alarm_count();
};

#endif // SCANNER_H
