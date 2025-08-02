#include "utility.h"
#include <QDebug>
#include <QStringList>
#include <QScreen>


bool bind_window(const QString &title, HWND &windowHwnd)
{
    std::wstring w = title.toStdWString();
    windowHwnd = ::FindWindowW(nullptr, w.c_str());
    if (windowHwnd) return true;
    else return false;
}

bool scan_window(const HWND &windowHwnd)
{
    // Handle null pointers
    if (windowHwnd == nullptr)
    {
        return false;
    }
    // IsWindow 返回非零即句柄对应的窗口当前存在
    return ::IsWindow(windowHwnd) != FALSE;
}

bool scan_crash_windows()
{
    QStringList window_keywords;
    window_keywords << "UE4-ShooterGame"
                    << "UE-ShooterGame"
                    << "Game has crashed and will close"
                    << "Crash!"
                    << "Error"
                    << "Shooter"
                    << "Shooter Crash Reporter";

    // 枚举所有顶层窗口，找标题包含任一 keyword
    struct EnumData { const QStringList *window_keywords; HWND found; };
    EnumData data{&window_keywords, nullptr};
    ::EnumWindows([](HWND hwnd, LPARAM lparam) -> BOOL {
        auto &d = *reinterpret_cast<EnumData*>(lparam);
        if (!::IsWindowVisible(hwnd)) return TRUE;
        wchar_t buf[512] = {0};
        ::GetWindowTextW(hwnd, buf, _countof(buf));
        QString title = QString::fromWCharArray(buf).trimmed();
        if (title.isEmpty()) return TRUE;
        for (const QString &t : *d.window_keywords) {
            if (!t.isEmpty() && title.contains(t, Qt::CaseInsensitive)) {
                d.found = hwnd;
                return FALSE; // 停止枚举 stop enum
            }
        }
        return TRUE;
    }, reinterpret_cast<LPARAM>(&data));

    if (data.found) {
        return true;
    } else {
        return false;
    }
}

bool scan_window(const QString &window_title)
{
    HWND dummy;
    return bind_window(window_title, dummy);
}



bool get_wechat_window_coordinates(HWND hwnd, int y, QPair<int, int> &pair)
{
    if (!hwnd)
    {
        qDebug() << "get_wechat_window_coordinates:\n窗口句柄无效!";
        return false;
    }
    RECT rect;
    if(GetWindowRect(hwnd, &rect)) {
        int width = rect.right - rect.left;
        int height = rect.bottom - rect.top;
        QScreen *screen = QGuiApplication::primaryScreen();
        if (!screen) {
            qDebug() << "get_wechat_window_coordinates:\n获取屏幕分辨率信息失败!";
            return false;
        }
        QSize screenSize = screen->size();       // 屏幕分辨率，例如 1920x1080
        int   screenW    = screenSize.width();

        int pos_x = width - screenW / 1920 * 35;
        int pos_y = y;
        pair = qMakePair(pos_x, pos_y);
        return true;
    }
    return false;
}
