#ifndef UTILITY_H
#define UTILITY_H
#include <QString>
#include <windows.h>
#include <QString>
#include <QPair>

// Pass in Hwnd, bind the aim window to it.
bool bind_window(const QString &title, HWND &windowHwnd);
// Check if there is a window with this Hwnd.
bool scan_window(const HWND &windowHwnd);
bool scan_window(const QString &window_title);
// Check if there are some crash handler windows.
bool scan_crash_windows();

bool get_wechat_window_coordinates(HWND hwnd, int y, QPair<int, int> &pair);

#endif // UTILITY_H
