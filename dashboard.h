#ifndef DASHBOARD_H
#define DASHBOARD_H

#include <QMainWindow>

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

private:
    Ui::DashBoard *ui;
};
#endif // DASHBOARD_H
