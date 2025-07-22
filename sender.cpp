#include "sender.h"
#include "motion.h"
Sender::Sender() {}

void Sender::send_text(HWND hwnd, QString message)
{
    _send_text(hwnd, message);
}

void Sender::send_image(HWND hwnd, QImage image)
{
    _send_image(hwnd, image);
}
