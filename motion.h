#ifndef MOTION_H
#define MOTION_H
#include <windows.h>
#include <QImage>
#include <QString>

void click_center_and_keyL(HWND aim_hwnd);

void click_center_and_ESC(HWND aim_hwnd);

void left_click(HWND hwnd, int pos_x, int pos_y);

void paste_text(const QString &message);

void paste_image(const QImage &img);

void _send_text(HWND hwnd, const QString &message);

void _send_image(HWND hwnd, const QImage &image);

void press_key(WORD vk);

void ctrl_v();

#endif // MOTION_H
