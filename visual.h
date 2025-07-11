#ifndef VISUAL_H
#define VISUAL_H
#include <QImage>
#include <QString>
#include <opencv2/opencv.hpp>
#include <windows.h>

// Final interface.
// Pass a game window hwnd in it, then it will take a screenshot of game
// and ocr two areas, then return the full-size-screenshot and two text.
bool analyze_game_window(HWND hwnd,
                       QString &ocrResult1,
                       QString &ocrResult2,
                       QImage &pic1,
                       QImage &pic2,
                       QImage &fullImage);

// ____________Helpers____________

cv::Mat QImage_to_cvMat(const QImage &inImage);
QImage CvMat_to_QImage(const cv::Mat &inMat);
// OCR Parasaurolophus and tribe log in once.
bool OCR_image(const QImage &input_image, QString &recognized_text);
bool OCR_area_P(const QImage &input_full_game_window, QString &recognized_text);
bool OCR_area_T(const QImage &input_full_game_window, QString &recognized_text);
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
bool check_connection_health(HWND gameHwnd);

#endif // VISUAL_H
