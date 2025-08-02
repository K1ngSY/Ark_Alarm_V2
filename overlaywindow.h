#ifndef OVERLAYWINDOW_H
#define OVERLAYWINDOW_H

#include <QWidget>
#include <QTimer>
#include <windows.h>

class OverlayWindow : public QWidget
{
    Q_OBJECT
public:
    explicit OverlayWindow(HWND targetHwnd, int pos_y, QWidget *parent = nullptr);
    inline void set_dot_pos_y(int y)
    {
        m_pos_y = y;
        update();
        emit overlay_updated();
    }
private:
    HWND m_target_hwnd;
    int m_pos_x;
    int m_pos_y;
    QTimer m_timer;

    void refresh();

protected:
    void paintEvent(QPaintEvent *event) override;

signals:
    void overlay_updated();
};

#endif // OVERLAYWINDOW_H
