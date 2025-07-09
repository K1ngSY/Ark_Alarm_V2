#include "ksubthread.h"

KSubthread::KSubthread(QObject *parent)
    : QObject{parent}
{
    connect(this, &KSubthread::start_signal, this, &KSubthread::handle_start_signal);
    connect(this, &KSubthread::stop_signal, this, &KSubthread::handle_stop_signal);
}
