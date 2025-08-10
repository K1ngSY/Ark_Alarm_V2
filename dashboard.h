#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "overlaywindow.h"
#include "scanner.h"
#include "rejoiner.h"
#include "crashhandler.h"
#include "sender.h"
#include "serverlist.h"
#include "ui_dashboard.h"
#include "utility.h"
#include <QMainWindow>
#include <QThread>
#include <QDateTime>
#include <windows.h>
#include <QTimer>
#include <QMediaPlayer>
#include <QAudioOutput>

QT_BEGIN_NAMESPACE
namespace Ui {
class DashBoard;
}
QT_END_NAMESPACE

class DashBoard : public QMainWindow
{
    Q_OBJECT

public:
    DashBoard(QWidget *parent = nullptr);
    ~DashBoard();

private slots:
    void stop_all();

    void append_log_user(const QString &message);
    void append_log_debug(const QString &message);

    void select_wechat_window();

    void start_monitor();

    void update_overlay_pos_y_slider(int value);
    void update_overlay_pos_y_spinBox(int value);
    void update_overlay_range();
    void on_overlay_visibility_changed(Qt::CheckState state);

    // Debug Images.
    inline void update_image_p(QImage pic)
    {
        ui->image1Label->setPixmap(QPixmap::fromImage(pic));
    }
    inline void update_image_t(QImage pic)
    {
        ui->image2Label->setPixmap(QPixmap::fromImage(pic));
    }

    void handle_find_window_fail(window_type failed_window_type);

    void on_made_call(call_type type);

    // connect to crash handler finishen_0.
    void proceed_rejoin();

    void handle_crash_handler_bad_finish(QString msg);

    // this means rejoiner has a good finish.
    // connect to rejoiner finished_0.
    void restart_mornitor();

    void handle_rejoiner_bad_finish(QString msg);

    // inline void handle_expired() {}

    void handle_server_fetched(const QList<ServerInfo> &servers);

    void refresh_table_next_round();

    void block_player_number_alarm();

    // 警报过滤器关键词修改
    void changeAlarmKey_starved(int status);
    void changeAlarmKey_waskilled(int status);
    void changeAlarmKey_demolished(int status);
    void changeAlarmKey_froze(int status);
    void changeAlarmKey_claimed(int status);
    void changeAlarmKey_promoted(int status);
    void changeAlarmKey_added(int status);
    void changeAlarmKey_topublic(int status);

    // 音频播放处理
    void onPlayP_AlarmSound();
    void onPlayLogAlarmSound();

    void updateCallMemberLable(QString member);

    void handle_warn(QString msg);

    void switch_to_tab_1();
    void switch_to_tab_2();
    void switch_to_tab_3();
    void switch_to_tab_4();

    void change_play_sound(Qt::CheckState checkState);

    void refresh_server_combo_box();

    // connect to scanner game_timeout.
    inline void handle_in_game_error()
    {
        stop_scanner();
        if (!game_back_to_home())
        {
            return;
        }
        launch_rejoiner();
    }

    void handle_made_calls(call_type type);

    void handle_crash();

    void handle_crashHandler_game_window_timeout();
    void handle_crashHandler_start_button_timeout();
    void handle_crashHandler_got_game_hwnd(HWND game_hwnd);

    void handle_rejoiner_finished1(QString msg);
    void handle_rejoiner_finished0();

private:
    Ui::DashBoard *ui;

    OverlayWindow *m_overlay_window;

    HWND m_wechat_window_hwnd;
    HWND m_game_hwnd;

    QString m_wechat_window_title;

    QTimer *m_timer_table_CD;
    QTimer *m_timer_slider_range;
    QMetaObject::Connection m_timer_table_CD_conn;
    QMetaObject::Connection m_timer_slider_range_conn;

    Scanner *m_scanner;
    CrashHandler *m_crash_handler;
    Rejoiner *m_rejoiner;
    ServerList *m_server_list;
    Sender *m_sender;

    QThread *m_scanner_thread;
    QThread *m_crash_handler_thread;
    QThread *m_rejoiner_thread;
    QThread *m_server_list_thread;
    QThread *m_sender_thread;

    // QDateTime m_expire_DateTime;

    int m_wechat_pos_y;

    bool m_serverComboBoxUpdated;
    bool m_playerNumberAlarmSent;
    bool m_play_sound;

    // ——— 报警音配置 ———
    // 1为副栉龙警报
    // 2为部落日志警报
    QAudioOutput *m_audioOutput1;
    QAudioOutput* m_audioOutput2;
    QMediaPlayer *m_player1;
    QMediaPlayer *m_player2;

    // 检测次数计数器
    int m_round_count;
    // 警报发送计数器
    int m_alarm_count;

    inline void launch_scanner()
    {
        append_log_debug("launch_scanner()");
        append_log_user("监控启动");
        m_scanner->start_work();
    }
    inline void launch_crash_handler()
    {
        append_log_debug("launch_crash_handler()");
        append_log_user("开始处理崩溃");
        m_crash_handler->start_work();
    }
    inline void launch_rejoiner()
    {
        append_log_debug("launch_rejoiner()");
        append_log_user("开始重连");
        m_rejoiner->start_work();
    }
    inline void launch_server_list()
    {
        append_log_debug("launch_server_list()");
        append_log_user("启动服务器列表");
        m_server_list->start_work();
    }
    inline void launch_sender()
    {
        append_log_debug("launch_sender()");
        append_log_user("启动消息发送器");
        m_sender->start_work();
    }

    inline void stop_scanner()
    {
        append_log_debug("stop_scanner()");
        append_log_user("关闭监控进程");
        m_scanner->stop_work();
    }
    inline void stop_crash_handler()
    {
        append_log_debug("stop_crash_handler()");
        append_log_user("停止崩溃处理");
        m_crash_handler->cancel();
    }
    inline void stop_rejoiner()
    {
        append_log_debug("stop_rejoiner()");
        append_log_user("停止重连");
        m_rejoiner->cancel();
    }
    inline void stop_serverlist()
    {
        append_log_debug("stop_serverlist()");
        append_log_user("停止更新服务器列表");
        m_server_list->stop_work();
    }
    inline void stop_sender()
    {
        append_log_debug("stop_sender()");
        append_log_user("停止消息发送器");
        m_sender->stop_work();
    }

    inline bool game_back_to_home()
    {
        if (!scan_window(m_game_hwnd))
        {
            append_log_debug("game_back_to_home:\n游戏窗口不存在，即将进入崩溃处理流程");
            append_log_user("游戏窗口不存在，即将进入崩溃处理流程");
            stop_scanner();
            launch_crash_handler();
            return false;
        }
        m_sender->click_center_and_ESC(m_game_hwnd);
        launch_rejoiner();
        return true;
    }

    inline void send_message(const QString &msg)
    {
        if (!scan_window(m_wechat_window_hwnd))
        {
            append_log_debug("send_message:\n微信窗口不存在！");
            append_log_user("微信窗口不存在！");
            stop_all();
        }
        emit _send_message(m_wechat_window_hwnd, msg);
    }

    inline void increase_alarm_count()
    {
        m_alarm_count++;
        ui->lcdNumber_alarm->display(m_alarm_count);
    }

    inline void increase_scan_count()
    {
        m_round_count++;
        ui->lcdNumber_round->display(m_round_count);
    }

    void table_CD_helper();

protected:
    void closeEvent(QCloseEvent *event) override;

signals:
    void _send_message(HWND hwnd, QString msg);
    void change_keyword_status(QString key, bool status);
};
#endif // DASHBOARD_H
