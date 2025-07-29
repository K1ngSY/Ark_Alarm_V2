#ifndef REJOINER_H
#define REJOINER_H

#include "kworker.h"
#include <windows.h>

class Rejoiner : public KWorker
{
    Q_OBJECT
public:
    explicit Rejoiner(QObject *parent = nullptr);
    bool start_work() override;
    inline void set_game_hwnd(HWND hwnd)
    {
        this->m_game_hwnd = hwnd;
        emit log_message_Debug(QString("Rejoiner::set_game_hwnd:\n已设置游戏窗口句柄为%1").arg((qulonglong)m_game_hwnd));
        emit log_message_User(QString("已设置游戏窗口句柄为%1").arg((qulonglong)m_game_hwnd));
    }
    inline void set_server_ID(const QString &ID)
    {
        this->m_server_ID = ID;
        emit log_message_Debug(QString("Rejoiner::set_game_hwnd:\n已设置服务器ID为%1").arg(m_server_ID));
        emit log_message_User(QString("已设置服务器ID为%1").arg(m_server_ID));
    }
    inline void set_has_mod(const bool &has_mod)
    {
        this->m_has_mod = has_mod;
        emit log_message_Debug(QString("Rejoiner::set_game_hwnd:\n已设置服务器Mod状态为[%1]").arg(m_has_mod? "含Mod" : "不含Mode"));
        emit log_message_User(QString("已设置服务器Mod状态为[%1]").arg(m_has_mod? "含Mod" : "不含Mode"));
    }
    inline void cancel() override
    {
        m_cancelRequested.store(true);
        emit log_message_Debug(QString("Rejoiner::cancel:\n原子状态为[%1]").arg(m_cancelRequested.load()? "canceled" : "not canceled"));
        emit log_message_User(QString("原子状态为[%1]").arg(m_cancelRequested.load()? "canceled" : "not canceled"));
    }
private:
    HWND m_game_hwnd;
    QString m_server_ID;
    bool m_has_mod;

    // 拆分子步骤
    bool click_start();
    bool click_join_card();
    bool search_server();
    bool select_first_server();
    bool check_mod(bool &has, QImage &debug_img);
    void join_with_mod(bool &conn_failed);
    void join_without_mod(bool &conn_fail);
private slots:
    void handle_start_signal() override;
    void handle_stop_signal() override;
};

#endif // REJOINER_H
