#include "dashboard.h"
#include "ui_dashboard.h"
#include <QThread>
#include <QDebug>
DashBoard::DashBoard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DashBoard)
{
    ui->setupUi(this);
}

DashBoard::~DashBoard()
{
    delete ui;
}

void DashBoard::closeEvent(QCloseEvent *event)
{
    // 此函数替代析构函数
    // 先调用所有worker的stop_work函数
    // 再quit所有线程
    // wait等待线程真正结束
    // 最后调用closeEvent
    // ———————示例代码———————
    // tester->stop_work(); // 调用 worker 的 stop_work() 函数
    // test_thread->quit(); // Quit 线程
    // test_thread->wait(); // Wait till 线程完全结束



    QMainWindow::closeEvent(event);
}
