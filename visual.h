#ifndef VISUAL_H
#define VISUAL_H
#include <QImage>
#include <QString>
#include <opencv2/opencv.hpp>
#include <windows.h>

cv::Mat QImage_to_cvMat(const QImage &in_image);
QImage CvMat_to_QImage(const cv::Mat &in_mat);
// OCR Parasaurolophus and tribe log in once.
bool OCR_image(const QImage &input_image, QString &recognized_text);
bool OCR_area_P(const QImage &input_full_game_window, QString &recognized_text, QImage &debug_roi);
bool OCR_area_T(const QImage &input_full_game_window, QString &recognized_text, QImage &debug_roi);
// It will call windows API.
bool print_window(HWND hwnd, QImage &image);
// Scan game window, looking for "Start Game" button.
bool check_start_button(HWND game_hwnd);
// Scan game window, check current if server column has mod.
// The QImage is for Debugging.
bool check_server_mod(HWND game_hwnd, QImage &img);
// When the server contains mods,
// the game may download the mod file
// after clicking Join. At this time,
// scan the screen to see if there is
// a "BACK" button
// (a sign that the mod is being downloaded).
bool check_donwload(HWND game_hwnd);
// Check if there is a connection failed dialog (OCR based).
bool check_connection_health(HWND game_hwnd);

bool check_5_cards(HWND game_hwnd);

bool _test_ocr();

#endif // VISUAL_H
