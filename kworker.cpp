#include "kworker.h"

KWorker::KWorker(QObject *parent)
    : QObject{parent}
{
    connect(this, &KWorker::start_signal, this, &KWorker::handle_start_signal);
    connect(this, &KWorker::stop_signal, this, &KWorker::handle_stop_signal);
}
