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