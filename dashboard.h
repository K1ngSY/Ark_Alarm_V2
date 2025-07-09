#ifndef DASHBOARD_H
#define DASHBOARD_H

#include "scanner.h"
#include <QMainWindow>
#include <QThread>
QT_BEGIN_NAMESPACE
namespace Ui {
class DashBoard;
}
QT_END_NAMESPACE

class DashBoard : public QMainWindow
{
    Q_OBJECT

public:
    DashBoard(QWidget *parent = nullptr);
    ~DashBoard();

private slots:

private:
    Ui::DashBoard *ui;
};
#endif // DASHBOARD_H
