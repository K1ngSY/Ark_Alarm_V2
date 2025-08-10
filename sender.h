#ifndef SENDER_H
#define SENDER_H

#include <windows.h>
#include <QImage>
#include <QString>
#include <QTimer>
#include "kworker.h"

class Sender : public KWorker
{
    Q_OBJECT
public:
    explicit Sender(QObject *parent = nullptr);
public slots:
    void send_text(HWND hwnd, QString message);
    void send_image(HWND hwnd, QImage image);
    void click_center_and_ESC(HWND hwnd);

    void make_call(HWND hwnd, int x, int y);
    void make_group_call(HWND hwnd, QString members, int x, int y);

private slots:
    void handle_start_signal() override {}
    void handle_stop_signal() override
    {
        if (m_timer_making_call)
        {
            if (m_timer_making_call->isActive())
                m_timer_making_call->stop();
            disconnect(m_timer_making_call_conn);
            m_timer_making_call->deleteLater();
            m_timer_making_call = nullptr;
        }
    }
    void CD_helper();
private:
    bool m_in_CD_call;
    QTimer *m_timer_making_call;
    QMetaObject::Connection m_timer_making_call_conn;
    void start_CD();

};

#endif // SENDER_H
