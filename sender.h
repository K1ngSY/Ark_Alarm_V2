#ifndef SENDER_H
#define SENDER_H

#include <windows.h>
#include <QImage>
#include <QString>
#include "kworker.h"

class Sender : public KWorker
{
    Q_OBJECT
public:
    Sender();
public slots:
    void send_text(HWND hwnd, QString message);
    void send_image(HWND hwnd, QImage image);
private slots:
    void handle_start_signal() override {}
    void handle_stop_signal() override {}
};

#endif // SENDER_H
