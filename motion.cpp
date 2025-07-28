#include "motion.h"
#include <QThread>
#include <QDebug>

void left_click(HWND hwnd, int pos_x, int pos_y)
{
    SetForegroundWindow(hwnd);
    QThread::msleep(200);
    SetForegroundWindow(hwnd);
    QThread::msleep(100);
    SetForegroundWindow(hwnd);
    QThread::msleep(50);
    SetForegroundWindow(hwnd);
    QThread::msleep(30);
    qDebug() << QString("MotionSimulator::LeftClick: received HWND: %1").arg((qulonglong)hwnd);
    // 1) 客户区坐标 → 屏幕坐标
    POINT pt{ pos_x, pos_y };
    if (!ClientToScreen(hwnd, &pt))
    {
        qWarning() << "ClientToScreen failed, error=" << GetLastError();
        return;
    }

    // 2) 计算归一化绝对坐标 (0–65535)
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    UINT normX = MulDiv(pt.x, 65535, screenW - 1);
    UINT normY = MulDiv(pt.y, 65535, screenH - 1);

    // 3) 构造鼠标移动 + 点击事件
    INPUT inputs[3] = {};
    inputs[0].type               = INPUT_MOUSE;
    inputs[0].mi.dwFlags         = MOUSEEVENTF_MOVE | MOUSEEVENTF_ABSOLUTE;
    inputs[0].mi.dx              = normX;
    inputs[0].mi.dy              = normY;
    inputs[1].type               = INPUT_MOUSE;
    inputs[1].mi.dwFlags         = MOUSEEVENTF_LEFTDOWN;
    inputs[2].type               = INPUT_MOUSE;
    inputs[2].mi.dwFlags         = MOUSEEVENTF_LEFTUP;

    // 4) 发送给系统
    UINT sent = SendInput(_countof(inputs), inputs, sizeof(INPUT));
    if (sent != _countof(inputs))
    {
        qWarning() << "SendInput failed, sent=" << sent;
    }
}

void left_click_background(HWND hwnd, int pos_x, int pos_y)
{
    if (!IsWindow(hwnd))
    {
        qWarning() << "left_click_background: invalid window";
        return;
    }
    if (IsIconic(hwnd))
    {
        qWarning() << "left_click_background: window is minimized";
        return;
    }

    LPARAM lParam = MAKELPARAM(pos_x, pos_y);
    PostMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
    PostMessage(hwnd, WM_LBUTTONDOWN, MK_LBUTTON, lParam);
    PostMessage(hwnd, WM_LBUTTONUP, 0, lParam);
}

void click_center_and_keyL(HWND aim_hwnd)
{
    RECT r;
    ::GetWindowRect(aim_hwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    left_click(aim_hwnd, cx, cy);
    // 按键 L
    WORD vk = 'L';
    press_key(vk);
}

void press_key(WORD vk)
{
    INPUT input{};  // 全部初始化为0
    input.type = INPUT_KEYBOARD;
    input.ki.wVk = vk;
    SendInput(1, &input, sizeof(INPUT));

    INPUT inputUp{};
    inputUp.type = INPUT_KEYBOARD;
    inputUp.ki.wVk = vk;
    inputUp.ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(1, &inputUp, sizeof(INPUT));
}

void click_center_and_ESC(HWND aim_hwnd)
{
    RECT r;
    ::GetWindowRect(aim_hwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    left_click(aim_hwnd, cx, cy);
    // 按键 ESC
    press_key(VK_ESCAPE);
}

void _send_text(HWND hwnd, const QString &message)
{
    if (!hwnd) {
        qDebug() << "send_text:\nInvalid WeChat window handle";
        return;
    }
    SetForegroundWindow(hwnd);
    QThread::msleep(100);
    SetForegroundWindow(hwnd);

    QThread::msleep(500);
    // 1) 获取客户区尺寸
    RECT clientRect;
    if (!GetClientRect(hwnd, &clientRect)) {
        qDebug() << "send_text:\nFailed to get client rect";
        return;
    }
    int width  = clientRect.right  - clientRect.left;
    int height = clientRect.bottom - clientRect.top;

    // 2) 计算点击位置（90% 处于底端偏上）
    int clientX = width / 2;
    int clientY = static_cast<int>(height * 0.9);

    // 3) 发起点击、粘贴文本、回车
    left_click(hwnd, clientX, clientY);
    paste_text(message);
    QThread::msleep(500);
    press_key(VK_RETURN);

    qDebug() << "send_text:\nMessage sent";
}

void paste_text(const QString &message)
{
    if (!OpenClipboard(NULL))
    {
        qDebug() << "paste_text:\nUnable to open clipboard";
        return;
    }
    EmptyClipboard();
    // 使用 utf16() 获取指向 UTF-16 数据的指针
    const ushort *data = message.utf16();
    // 计算字节数：每个字符 2 字节，加上终止符
    int byteSize = (message.size() + 1) * sizeof(ushort);
    HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, byteSize);
    if (!hMem) {
        CloseClipboard();
        qDebug() << "paste_text:\nGlobalAlloc failed";
        return;
    }
    void *pMem = GlobalLock(hMem);
    memcpy(pMem, data, byteSize);
    GlobalUnlock(hMem);
    SetClipboardData(CF_UNICODETEXT, hMem);
    CloseClipboard();
    ctrl_v();
}

void paste_image(const QImage &img)
{
    // 将图像转换为 Format_RGB32（无 alpha）以适应 CF_DIB 格式
    QImage image = img.convertToFormat(QImage::Format_RGB32);
    int width = image.width();
    int height = image.height();
    int bytesPerLine = image.bytesPerLine();

    // 构造 BITMAPINFOHEADER，注意对于 CF_DIB，通常使用正高度（表示底部为第一行）
    BITMAPINFOHEADER bih;
    ZeroMemory(&bih, sizeof(bih));
    bih.biSize = sizeof(BITMAPINFOHEADER);
    bih.biWidth = width;
    bih.biHeight = -height;  // 正值表示底部在前（DIB标准格式）
    bih.biPlanes = 1;
    bih.biBitCount = 32;    // 32位格式
    bih.biCompression = BI_RGB;
    bih.biSizeImage = bytesPerLine * height;

    // 总内存大小 = header + 像素数据
    int totalSize = sizeof(BITMAPINFOHEADER) + bih.biSizeImage;
    HGLOBAL hGlobal = GlobalAlloc(GMEM_MOVEABLE, totalSize);
    if (!hGlobal) {
        qDebug() << "paste_image:\nGlobalAlloc failed";
        return;
    }
    void *pGlobal = GlobalLock(hGlobal);
    if (!pGlobal) {
        qDebug() << "paste_image:\nGlobalLock failed";
        GlobalFree(hGlobal);
        return;
    }
    // 将 BITMAPINFOHEADER 复制到全局内存块
    memcpy(pGlobal, &bih, sizeof(bih));
    // 像素数据存放在 header 之后
    void *pPixels = static_cast<char*>(pGlobal) + sizeof(bih);
    memcpy(pPixels, image.bits(), bih.biSizeImage);
    GlobalUnlock(hGlobal);

    // 打开剪贴板并清空
    if (!OpenClipboard(NULL)) {
        qDebug() << "paste_image:\nUnable to open clipboard";
        GlobalFree(hGlobal);
        return;
    }
    EmptyClipboard();
    // 设置剪贴板数据为 CF_DIB 格式
    if (!SetClipboardData(CF_DIB, hGlobal)) {
        qDebug() << "paste_image:\nSetClipboardData failed";
        CloseClipboard();
        GlobalFree(hGlobal);
        return;
    }
    // 成功后剪贴板拥有内存块，不需手动释放
    CloseClipboard();

    // 模拟 Ctrl+V 粘贴操作
    ctrl_v();
}

void ctrl_v()
{
    // 模拟 Ctrl+V 组合键
    INPUT inputs[4]{};
    // Ctrl 按下
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = VK_CONTROL;
    // V 按下
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = 'V';
    // V 释放
    inputs[2].type = INPUT_KEYBOARD;
    inputs[2].ki.wVk = 'V';
    inputs[2].ki.dwFlags = KEYEVENTF_KEYUP;
    // Ctrl 释放
    inputs[3].type = INPUT_KEYBOARD;
    inputs[3].ki.wVk = VK_CONTROL;
    inputs[3].ki.dwFlags = KEYEVENTF_KEYUP;
    SendInput(4, inputs, sizeof(INPUT));
    QThread::msleep(100);
}

void click_center(HWND aim_hwnd)
{
    RECT r;
    ::GetWindowRect(aim_hwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    // 点击
    left_click(aim_hwnd, cx, cy);
}

void click_center_background(HWND aim_hwnd)
{
    RECT r;
    ::GetWindowRect(aim_hwnd, &r);
    int cx = (r.left + r.right) / 2;
    int cy = (r.top  + r.bottom) / 2;
    left_click_background(aim_hwnd, cx, cy);
}
