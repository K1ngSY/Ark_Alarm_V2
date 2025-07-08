#ifndef SCANNER_H
#define SCANNER_H

#include "ksubthread.h"

#include <windows.h>

class Scanner : public KSubthread
{
    Q_OBJECT
public:
    explicit Scanner(QObject *parent = nullptr);
    // setters
    // Third Party Platform Window Title
    void set_TPPW_title(const QString &title);
    // Third Party Platform Window Hwnd
    void set_TPPW_hwnd(HWND TPPW_hwnd);
    // Button Position on TPPW
    void set_click_position(const int &x, const int &y);

private:
    HWND m_TPPW_hwnd;
    QString m_TPPW_title;
    int m_click_position_x;
    int m_click_position_y;
    bool m_first_round;
public slots:
    void start();
};

#endif // SCANNER_H
