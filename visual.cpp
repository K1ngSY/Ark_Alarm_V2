#include "visual.h"

#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

#include <QDebug>
#include <tesseract/baseapi.h>
#include <leptonica/allheaders.h>
#include <QGuiApplication>
#include <QDir>
#include <QStringList>

cv::Mat QImage_to_cvMat(const QImage &in_image)
{
    return cv::Mat(in_image.height(),
                   in_image.width(),
                   CV_8UC4,
                   const_cast<uchar*>(in_image.bits()),
                   in_image.bytesPerLine()).clone();
}

QImage CvMat_to_QImage(const cv::Mat &in_mat)
{
    if (in_mat.type() == CV_8UC1) {
        QImage img(in_mat.data, in_mat.cols, in_mat.rows,
                   in_mat.step, QImage::Format_Grayscale8);
        return img.copy();
    } else if (in_mat.type() == CV_8UC4) {
        QImage img(in_mat.data, in_mat.cols, in_mat.rows,
                   in_mat.step, QImage::Format_ARGB32);
        return img.copy();
    }
    qDebug() << "CvMat_to_QImage:\n不支持的 Mat 类型";
    return {};
}

bool OCR_image(const QImage &input_image, QString &recognized_text)
{
    QString tessDir = QDir(QCoreApplication::applicationDirPath()).filePath("tessdata");
    tesseract::TessBaseAPI ocr;
    ocr.SetVariable("tessedit_use_mmap", "0");
    QByteArray localAnsi = tessDir.toLocal8Bit();
    std::string ansiPath(localAnsi.constData(), localAnsi.size());

    if (ocr.Init(ansiPath.c_str(), "chi_sim+eng", tesseract::OEM_LSTM_ONLY)) {
        qDebug() << "OCR_image:\n初始化失败，请检查 tessdataPath";
        return false;
    }
    ocr.SetPageSegMode(tesseract::PSM_AUTO);

    // OCR Process
    int bpp = input_image.depth()/8;
    ocr.SetImage(const_cast<uchar*>(input_image.bits()),
                 input_image.width(),
                 input_image.height(),
                 bpp,
                 input_image.bytesPerLine());
    char *out = ocr.GetUTF8Text();
    recognized_text = QString::fromUtf8(out);
    delete[] out;
    ocr.End();
    return true;
}

bool OCR_area_P(const QImage &input_full_game_window, QString &recognized_text, QImage &debug_roi)
{
    int w = input_full_game_window.width();
    int h = input_full_game_window.height();
    QImage pic = input_full_game_window.copy(int(w * 0.08),    int(h * 0.01203),
                                             int(w * 0.2661),  int(h * 0.02778));
    cv::Mat m = QImage_to_cvMat(pic);
    std::vector<cv::Mat> ch;
    cv::split(m, ch);
    if (ch.size() < 3)
    {
        qDebug() << "OCR_area_P:\npic 通道不足";
        return false;
    }
    // 拆蓝色通道并赋值回 pic
    // 蓝色通道 索引 0
    pic = CvMat_to_QImage(ch[0]);
    debug_roi = pic;
    if (!OCR_image(pic, recognized_text))
    {
        qDebug() << "OCR_area_P:\nOCR识别失败！";
        return false;
    }
    return true;
}

bool OCR_area_T(const QImage &input_full_game_window, QString &recognized_text, QImage &debug_roi)
{
    int w = input_full_game_window.width();
    int h = input_full_game_window.height();
    QImage pic = input_full_game_window.copy(int(w * 0.39531), int(h * 0.17778),
                                             int(w * 0.209375), int(h * 0.584259));
    cv::Mat m = QImage_to_cvMat(pic);
    // 查看 m2 的通道数和类型
    qDebug() << "OCR_area_T_Debug:\nm channels =" << m.channels() << ", type =" << m.type();
    if (m.channels() == 4) {
        cv::cvtColor(m, m, cv::COLOR_BGRA2BGR);
        qDebug() << "Debug: m 从 BGRA 转为 BGR";
    }
    // ——— 对 pic 做 HSV 滤色 → 二值掩膜 ———
    // 1. 先把 m (BGR) 转成 HSV
    cv::Mat hsv;
    cv::cvtColor(m, hsv, cv::COLOR_BGR2HSV);
    // ——— 调试3：拆分并显示 HSV 三通道 ———
    std::vector<cv::Mat> hsv_channels;
    cv::split(hsv, hsv_channels);

    // cv::imshow("Debug_H_channel", hsv_channels[0]);  // H 范围 0–179
    // cv::imshow("Debug_S_channel", hsv_channels[1]);  // S 范围 0–255
    // cv::imshow("Debug_V_channel", hsv_channels[2]);  // V 范围 0–255
    // cv::waitKey(0);

    // —— 灰黑色对应的 OpenCV HSV 范围 —— (灰黑色现在用白色替代)
    int h_gray_min = 70; // 0
    int h_gray_max = 115; // 179
    int s_gray_min = 0;
    int s_gray_max = 131;
    int v_gray_min = 94;
    int v_gray_max = 187;
    cv::Scalar lower_gray(h_gray_min, s_gray_min, v_gray_min);
    cv::Scalar upper_gray(h_gray_max, s_gray_max, v_gray_max);

    // —— 绿色对应的 OpenCV HSV 范围 ——
    int h_green_min = 60;  // 60
    int h_green_max = 78;  // 75
    int s_green_min = 225;  // 212
    int s_green_max = 255;  // 252
    int v_green_min = 76;  // 97
    int v_green_max = 255;  // 250
    cv::Scalar lower_green(h_green_min, s_green_min, v_green_min);
    cv::Scalar upper_green(h_green_max, s_green_max, v_green_max);

    // —— 红色对应的 OpenCV HSV 范围 ——
    int h_red_min = 150;
    int h_red_max = 180;
    int s_red_min = 190;
    int s_red_max = 255;
    int v_red_min = 130;
    int v_red_max = 255;
    cv::Scalar lower_red(h_red_min, s_red_min, v_red_min);
    cv::Scalar upper_red(h_red_max, s_red_max, v_red_max);

    // —— 红色辅助膜 只拿H=0的部分对应的 OpenCV HSV 范围 ——
    int h_red_1_min = 0;
    int h_red_1_max = 0;
    int s_red_1_min = 190;
    int s_red_1_max = 255;
    int v_red_1_min = 130;
    int v_red_1_max = 255;
    cv::Scalar lower_red_1(h_red_1_min, s_red_1_min, v_red_1_min);
    cv::Scalar upper_red_1(h_red_1_max, s_red_1_max, v_red_1_max);

    // —— 青蓝色对应的 OpenCV HSV 范围 ——
    int h_cyan_min = 90;    // 90
    int h_cyan_max = 95;    // 95
    int s_cyan_min = 230;    // 194
    int s_cyan_max = 255;    // 252
    int v_cyan_min = 100;    // 97
    int v_cyan_max = 255;    // 242
    cv::Scalar lower_cyan(h_cyan_min, s_cyan_min, v_cyan_min);
    cv::Scalar upper_cyan(h_cyan_max, s_cyan_max, v_cyan_max);

    /////////////////下方为待处理掩膜/////////////////下方为待处理掩膜/////////////////下方为待处理掩膜/////////////////
    // —— 白色对应的 OpenCV HSV 范围 —— (已完工)
    int h_white_min = 0;
    int h_white_max = 255;
    int s_white_min = 0;
    int s_white_max = 130;
    int v_white_min = 117;
    int v_white_max = 255;
    cv::Scalar lower_white(h_white_min, s_white_min, v_white_min);
    cv::Scalar upper_white(h_white_max, s_white_max, v_white_max);

    // —— 黄色对应的 OpenCV HSV 范围 ——
    int h_yellow_min = 30;
    int h_yellow_max = 50;
    int s_yellow_min = 130;
    int s_yellow_max = 255;
    int v_yellow_min = 145;
    int v_yellow_max = 255;
    cv::Scalar lower_yellow(h_yellow_min, s_yellow_min, v_yellow_min);
    cv::Scalar upper_yellow(h_yellow_max, s_yellow_max, v_yellow_max);

    // —— 卡其色对应的 OpenCV HSV 范围 ——
    int h_khaki_min = 20;
    int h_khaki_max = 35;
    int s_khaki_min = 74;
    int s_khaki_max = 110;
    int v_khaki_min = 125;
    int v_khaki_max = 250;
    cv::Scalar lower_khaki(h_khaki_min, s_khaki_min, v_khaki_min);
    cv::Scalar upper_khaki(h_khaki_max, s_khaki_max, v_khaki_max);

    // —— 深蓝色对应的 OpenCV HSV 范围 ——
    int h_blue_min = 110;
    int h_blue_max = 121;
    int s_blue_min = 249;
    int s_blue_max = 255;
    int v_blue_min = 150;
    int v_blue_max = 255;
    cv::Scalar lower_blue(h_blue_min, s_blue_min, v_blue_min);
    cv::Scalar upper_blue(h_blue_max, s_blue_max, v_blue_max);

    // —— 屎黄色对应的 OpenCV HSV 范围 ——
    int h_shiHuang_min = 20;
    int h_shiHuang_max = 25;
    int s_shiHuang_min = 190;
    int s_shiHuang_max = 255;
    int v_shiHuang_min = 128;
    int v_shiHuang_max = 255;
    cv::Scalar lower_shiHuang(h_shiHuang_min, s_shiHuang_min, v_shiHuang_min);
    cv::Scalar upper_shiHuang(h_shiHuang_max, s_shiHuang_max, v_shiHuang_max);

    // 4. 分别做 inRange，得到8个二值掩膜
    cv::Mat mask_green, mask_red, mask_red_1, mask_cyan,
        mask_white, mask_yellow, mask_khaki, mask_blue, mask_shiHuang;

    cv::inRange(hsv, lower_green, upper_green, mask_green);
    cv::inRange(hsv, lower_red,   upper_red,   mask_red);
    cv::inRange(hsv, lower_red_1,   upper_red_1,   mask_red_1);
    cv::inRange(hsv, lower_cyan,  upper_cyan,  mask_cyan);

    cv::inRange(hsv, lower_white,  upper_white,  mask_white);
    cv::inRange(hsv, lower_yellow,  upper_yellow,  mask_yellow);
    cv::inRange(hsv, lower_khaki,  upper_khaki,  mask_khaki);
    cv::inRange(hsv, lower_blue,  upper_blue,  mask_blue);
    cv::inRange(hsv, lower_shiHuang,  upper_shiHuang,  mask_shiHuang);


    // ——— 调试4：分别显示8个掩膜 ———
    // cv::imshow("Debug_mask_green", mask_green);
    // cv::imshow("Debug_mask_red",   mask_red);
    // cv::imshow("Debug_mask_red_1",   mask_red_1);
    // cv::imshow("Debug_mask_cyan",  mask_cyan);

    // cv::imshow("Debug_mask_white",  mask_white);
    // cv::imshow("Debug_mask_yellow",  mask_yellow);
    // cv::imshow("Debug_mask_khaki",  mask_khaki);
    // cv::imshow("Debug_mask_blue",  mask_blue);
    // cv::imshow("Debug_mask_shiHuang",  mask_shiHuang);
    // cv::waitKey(0);

    // 5. 合并8张掩膜（满足任一颜色 → 保留）
    cv::Mat mask_all = mask_green | mask_red | mask_red_1 | mask_cyan | mask_white | mask_yellow | mask_khaki | mask_blue | mask_shiHuang;

    // ——— 调试5：显示最终合并掩膜 ———
    // cv::imshow("Debug_mask_all", mask_all);
    // cv::waitKey(0);

    // // 6. （可选）形态学去噪/闭运算，提高文字连通性
    // {
    //     cv::Mat kernel_open  = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3, 3));
    //     cv::Mat kernel_close = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    //     cv::morphologyEx(mask_all, mask_all, cv::MORPH_OPEN,  kernel_open);
    //     cv::morphologyEx(mask_all, mask_all, cv::MORPH_CLOSE, kernel_close);
    // }


    // 6.5. 锐化掩膜：使用 3x3 锐化核做 filter2D
    {
        // 经典锐化核（中心 5，周围 -1）
        cv::Mat sharpen_kernel = (cv::Mat_<float>(3,3) <<
                                      0, -1,  0,
                                  -1,  5, -1,
                                  0, -1,  0);
        cv::Mat mask_sharp;
        cv::filter2D(mask_all, mask_sharp, mask_all.depth(), sharpen_kernel);
        mask_all = mask_sharp;  // 用锐化结果替换原来的掩膜
    }

    // 把黑底白字转换为白底黑字
    cv::Mat mask_inv;
    cv::bitwise_not(mask_all, mask_inv);

    // 7. 把最终的 mask_all（二值图）转回 QImage，并赋给 pic2
    pic = CvMat_to_QImage(mask_inv);
    // ——— 结束滤色生成掩膜部分 ———
    debug_roi = pic;
    if (!OCR_image(pic, recognized_text))
    {
        qDebug() << "OCR_area_T:\nOCR识别失败！";
        return false;
    }
    return true;
}

bool print_window(HWND hwnd, QImage &image)
{
    RECT r;
    GetWindowRect(hwnd, &r);
    int w = r.right - r.left, h = r.bottom - r.top;
    HDC hdcWin = GetDC(hwnd);
    HDC hdcMem = CreateCompatibleDC(hdcWin);
    HBITMAP hBmp = CreateCompatibleBitmap(hdcWin, w, h);
    SelectObject(hdcMem, hBmp);
    if (!PrintWindow(hwnd, hdcMem, PW_RENDERFULLCONTENT))
    {
        DeleteObject(hBmp);
        DeleteDC(hdcMem);
        ReleaseDC(hwnd, hdcWin);
        return false;
    }
    BITMAPINFOHEADER bi{};
    bi.biSize = sizeof(bi);
    bi.biWidth = w;
    bi.biHeight = -h;
    bi.biPlanes = 1;
    bi.biBitCount = 32;
    bi.biCompression = BI_RGB;

    QImage tmp(w, h, QImage::Format_ARGB32);
    GetDIBits(hdcWin, hBmp, 0, h, tmp.bits(),
              reinterpret_cast<BITMAPINFO*>(&bi), DIB_RGB_COLORS);

    image = tmp;
    DeleteObject(hBmp);
    DeleteDC(hdcMem);
    ReleaseDC(hwnd, hdcWin);
    return true;
}

bool check_start_button(HWND game_hwnd)
{
    QImage fullImage;
    if (!print_window(game_hwnd, fullImage))
    {
        qDebug() << "check_start_button:\nFailed to capture full image using PrintWindow";
        return false;
    }
    qDebug() << "check_start_button:\nFull image captured, size:" << fullImage.width() << "x" << fullImage.height();
    RECT rect;
    if (!GetWindowRect(game_hwnd, &rect))
    {
        qDebug() << "check_start_button:\nFailed to get window rect";
        return false;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    qDebug() << "check_start_button:\nWindow rect:" << rect.left << rect.top << width << height;
    QImage roi = fullImage.copy(static_cast<int>(width * 760 / 1920),
                                static_cast<int>(height * 820 / 1080),
                                static_cast<int>(width * 400 / 1920),
                                static_cast<int>(height * 80 / 1080));
    qDebug() << "check_start_button:\nROI size:" << roi.width() << "x" << roi.height();

    // 进入OCR流程
    QString recognizedText;
    if (!OCR_image(roi, recognizedText))
    {
        qDebug() << "check_start_button:\nOCR识别失败！";
        return false;
    }
    QStringList keywordsofStartbutton = {"PRESS", "START", "按下", "开始"};

    qDebug() << "尝试检测开始游戏按钮是否存在……";
    if(!recognizedText.isEmpty())
    {
        qDebug() << "OCR已识别到内容: " << recognizedText << "尝试检测是否为开始游戏按钮……";
        for (const QString &keyword : keywordsofStartbutton)
        {
            if (!keyword.isEmpty() && recognizedText.contains(keyword, Qt::CaseInsensitive)) {
                qDebug() << "已检测到开始游戏按钮！";
                return true;
            }
        }
        qDebug() << "未检测到开始游戏按钮";
        return false;
    }
    else
    {
        qDebug() << "OCR未识别到任何内容！";
        return false;
    }
}

bool check_server_mod(HWND game_hwnd, QImage &img)
{
    QImage fullScreen;
    print_window(game_hwnd, fullScreen);
    RECT rect;
    if (!GetWindowRect(game_hwnd, &rect))
    {
        qDebug() << "check_server_mod:\nFailed to get window rect";
        return false;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    QImage roi = fullScreen.copy(width * 1596 / 1920,
                                 height * 307 / 1080,
                                 width * 196 / 1920,
                                 height * 277 / 1080);
    qDebug() << "check_server_mod:\nROI size:" << roi.width() << "x" << roi.height();

    // 转 cv::Mat 并拆通道
    cv::Mat m = QImage_to_cvMat(roi);
    std::vector<cv::Mat> ch;
    cv::split(m, ch);
    if (ch.size() < 3)
    {
        qDebug() << "check_server_mod:\n拆通道时通道不足";
        return false;
    }

    // 用蓝色通道(索引 0)
    roi = CvMat_to_QImage(ch[0]);
    img = roi;
    // 转换负色 白底黑字
    img.invertPixels(QImage::InvertRgb);
    // 进入OCR流程
    QString recognizedText;
    if (!OCR_image(img, recognizedText))
    {
        qDebug() << "check_server_mod:\nOCR识别失败！";
        return false;
    }
    if (!recognizedText.isEmpty())
    {
        recognizedText = recognizedText.toUpper();
        QStringList keywords = {"YES", "Y", "E", "S"};
        for (QString key : keywords)
        {
            if (recognizedText.contains(key, Qt::CaseInsensitive))
                return true;
        }
    }
    else
    {
        qDebug() << "check_server_mod:\nocr是否存在Mod时识别内容为空";
    }
    return false;
}

bool check_donwload(HWND game_hwnd)
{
    QImage fullScreen;
    print_window(game_hwnd, fullScreen);
    RECT rect;
    if (!GetWindowRect(game_hwnd, &rect))
    {
        qDebug() << "check_donwload:\nFailed to get window rect";
        return false;
    }
    int width = rect.right - rect.left;
    int height = rect.bottom - rect.top;
    QImage roi = fullScreen.copy(width * 876 / 1920,
                                 height * 603 / 1080,
                                 width * 168 / 1920,
                                 height * 43 / 1080);
    qDebug() << "check_donwload:\nROI size:" << roi.width() << "x" << roi.height();

    // 转 cv::Mat 并拆通道
    cv::Mat m = QImage_to_cvMat(roi);
    std::vector<cv::Mat> ch;
    cv::split(m, ch);
    if (ch.size() < 3) {
        qDebug() << "check_donwload:\n拆通道时通道不足";
        return false;
    }

    // 用Red通道(索引 2)
    roi = CvMat_to_QImage(ch[2]);
    // 转换负色
    roi.invertPixels(QImage::InvertRgb);
    // 进入OCR流程
    QString recognizedText;
    if (!OCR_image(roi, recognizedText))
    {
        qDebug() << "check_donwload:\nOCR识别失败";
        return false;
    }
    if(recognizedText.isEmpty())
    {
        qDebug() << "check_donwload:\nOCR识别内容为空";
        return false;
    }
    QStringList downloadKeyWords;
    downloadKeyWords << "返回"
                     << "BACK"
                     << "B";
    for (QString key : downloadKeyWords)
    {
        if (recognizedText.contains(key, Qt::CaseInsensitive))
            return true;
    }
    return false;
}

bool check_connection_health(HWND game_hwnd)
{
    // 此函数检测的是重连游戏的时候的框框
    // 等待进一步完善
}
