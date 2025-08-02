#include "overlaywindow.h"
#include <QPainter>
#include <QColor>
#include <QPen>
#include <QPoint>
#include <QDebug>
#include <QGuiApplication>
#include <QScreen>
#include <windows.h>

OverlayWindow::OverlayWindow(HWND targetHwnd, int pos_y, QWidget *parent)
    : QWidget(parent), m_target_hwnd(targetHwnd), m_pos_y(pos_y)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Tool | Qt::WindowStaysOnTopHint);
    setAttribute(Qt::WA_TranslucentBackground);
    setAttribute(Qt::WA_NoSystemBackground);
    setAttribute(Qt::WA_TransparentForMouseEvents); // 不拦截鼠标
    setAttribute(Qt::WA_ShowWithoutActivating);
    resize(300, 200);
    connect(&m_timer, &QTimer::timeout, this, &OverlayWindow::refresh);
    m_timer.start(10); // 定时追踪位置
    qDebug() << "OverLayWindow Created";
    RECT rect;
    if (GetWindowRect(m_target_hwnd, &rect)) {
        int width = rect.right - rect.left;
        this->m_pos_x = width * 0.95;
    }
    show();

    HWND hwnd = reinterpret_cast<HWND>(winId());
    LONG exStyle = GetWindowLong(hwnd, GWL_EXSTYLE);
    SetWindowLong(hwnd, GWL_EXSTYLE, exStyle | WS_EX_TRANSPARENT);
}

void OverlayWindow::refresh()
{
    if (!IsWindow(m_target_hwnd)) {
        close();
        return;
    }
    RECT rect;
    if (GetWindowRect(m_target_hwnd, &rect)) {
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            return;
        }
        QSize screenSize = screen->size();       // 屏幕分辨率，例如 1920x1080
        int   screenW    = screenSize.width();

        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        this->m_pos_x = width - (screenW / 1920 * 35);
        move(rect.left, rect.top);
        resize(width, height);
        update();
    }
}



void OverlayWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    QPen pen;
    QPoint pointHL, pointHR, pointVU, pointVL;
    int L = 7;
    pointHL.setX(m_pos_x-L);
    pointHL.setY(m_pos_y);
    pointHR.setX(m_pos_x+L);
    pointHR.setY(m_pos_y);
    pointVU.setX(m_pos_x);
    pointVU.setY(m_pos_y+L);
    pointVL.setX(m_pos_x);
    pointVL.setY(m_pos_y-L);
    pen.setWidth(2);
    pen.setColor(Qt::red);
    pen.setCapStyle(Qt::SquareCap);
    painter.setPen(pen);
    // painter.setCompositionMode(QPainter::CompositionMode_Source);
    painter.fillRect(rect(), QColor(0, 0, 0, 1));  // 几乎完全透明，但不是0
    painter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    painter.drawLine(pointHL,pointHR);
    painter.drawLine(pointVU,pointVL);
}

