#include "dashboard.h"
#include "ui_dashboard.h"
#include "utility.h"
#include "windowselectiondialog.h"
#include <QThread>
#include <QDebug>
#include <QMessageBox>
DashBoard::DashBoard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DashBoard)
    , m_overlay_window(nullptr)
    , m_wechat_window_hwnd(nullptr)
    , m_game_hwnd(nullptr)
    , m_slider_update_timer(nullptr)
    , m_timer_table_CD(nullptr)
    , m_scanner(nullptr)
    , m_crash_handler(nullptr)
    , m_rejoiner(nullptr)
    , m_server_list(nullptr)
    , m_sender(nullptr)
    , m_scanner_thread(nullptr)
    , m_crash_handler_thread(nullptr)
    , m_rejoiner_thread(nullptr)
    , m_server_list_thread(nullptr)
    , m_sender_thread(nullptr)
    , m_wechat_pos_y(0)
    , m_serverComboBoxUpdated(false)
    , m_playerNumberAlarmSent(false)
    , m_effect1(nullptr)
    , m_effect2(nullptr)
    , m_round_count(0)
    , m_alarm_count(0)
{
    ui->setupUi(this);
    ui->lcdNumber_alarm->setDigitCount(5);
    ui->lcdNumber_round->setDigitCount(5);
    ui->lcdNumber_alarm->display(m_alarm_count);
    ui->lcdNumber_round->display(m_round_count);
    ui->label_status->setText(tr("欢迎使用！"));

    // Scanner
    m_scanner = new Scanner();
    m_scanner_thread = new QThread();
    m_scanner->moveToThread(m_scanner_thread);

    // CrashHandler
    m_crash_handler = new CrashHandler();
    m_crash_handler_thread = new QThread();
    m_crash_handler->moveToThread(m_crash_handler_thread);

    // Rejoiner
    m_rejoiner = new Rejoiner();
    m_rejoiner_thread = new QThread();
    m_rejoiner->moveToThread(m_rejoiner_thread);

    // ServerList
    m_server_list = new ServerList();
    m_server_list_thread = new QThread();
    m_server_list->moveToThread(m_server_list_thread);

    // Sender
    m_sender = new Sender();
    m_sender_thread = new QThread();
    m_sender->moveToThread(m_sender_thread);

    // Launch Threads
    m_scanner_thread->start();
    m_crash_handler_thread->start();
    m_rejoiner_thread->start();
    m_server_list_thread->start();
    m_sender_thread->start();

    // connect log_message
    connect(m_scanner, &KWorker::log_message_Debug, this, &DashBoard::append_log_debug);
    connect(m_crash_handler, &KWorker::log_message_Debug, this, &DashBoard::append_log_debug);
    connect(m_rejoiner, &KWorker::log_message_Debug, this, &DashBoard::append_log_debug);
    connect(m_server_list, &KWorker::log_message_Debug, this, &DashBoard::append_log_debug);
    connect(m_sender, &KWorker::log_message_Debug, this, &DashBoard::append_log_debug);

    connect(m_scanner, &KWorker::log_message_User, this, &DashBoard::append_log_user);
    connect(m_crash_handler, &KWorker::log_message_User, this, &DashBoard::append_log_user);
    connect(m_rejoiner, &KWorker::log_message_User, this, &DashBoard::append_log_user);
    connect(m_server_list, &KWorker::log_message_User, this, &DashBoard::append_log_user);
    connect(m_sender, &KWorker::log_message_User, this, &DashBoard::append_log_user);

    m_slider_update_timer = new QTimer(this);
    m_slider_update_timer_conn = connect(m_slider_update_timer, &QTimer::timeout, this, &DashBoard::update_overlay_range);

    ui->image1Label->setScaledContents(true);
    ui->image2Label->setScaledContents(true);


    connect(m_scanner, &Scanner::send_warn, this, &DashBoard::handle_warn);

    connect(ui->pushButton_start, &QPushButton::clicked, this, &DashBoard::start_monitor);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &DashBoard::stop_all);


    connect(m_crash_handler, &CrashHandler::got_game_hwnd, this, &DashBoard::set_game_hwnd);
    connect(m_crash_handler, &CrashHandler::finished_0, this, &DashBoard::proceed_rejoin);
}

DashBoard::~DashBoard()
{
    delete ui;
}

void DashBoard::stop_all()
{
    stop_scanner();
    stop_crash_handler();
    stop_rejoiner();
    emit append_log_debug("DashBoard::stop_all:\n监控关闭");
    emit append_log_user("监控关闭");
}

void DashBoard::select_wechat_window()
{
    append_log_user("开始选择窗口…");
    ui->label_status->setText("选择您的窗口");
    // 选择微信窗口
    WindowSelectionDialog wechatDialog(this);
    wechatDialog.setWindowTitle("选择微信窗口");
    wechatDialog.setLabel1Text("选择微信窗口");
    if (wechatDialog.exec() == QDialog::Accepted) {
        auto wechatWin = wechatDialog.selected_window();
        m_wechat_window_hwnd = wechatWin.hwnd;
        m_wechat_window_title = wechatWin.title;  // 保存窗口标题
        append_log_user(QString("选定微信窗口：%1 句柄：%2").arg(wechatWin.title).arg((qulonglong)m_wechat_window_hwnd));
    } else {
        append_log_user("未选择微信窗口");
        QMessageBox::warning(this, "提示", "未选择微信窗口");
        return;
    }
    m_scanner->set_TPPW(m_wechat_window_title, m_wechat_window_hwnd);
    append_log_user("窗口选择完成");
    ui->label_status->setText("已完成微信窗口选择");
    launch_server_list();
    append_log_user("已启动服务器轮询");
}

void DashBoard::start_monitor()
{
    if(!m_wechat_window_title.isEmpty() && m_wechat_window_hwnd && scan_window(m_wechat_window_hwnd) && scan_window(m_wechat_window_title))
    {
        m_scanner->set_TPPW(m_wechat_window_title, m_wechat_window_hwnd);
        launch_scanner();
        ui->label_status->setText("监控已启动！");
        append_log_user("监控已启动！");
    }
    else {
        QMessageBox::warning(this, "Warning!", "请先选择微信窗口再启动监控！");
    }
}

void DashBoard::update_overlay_pos_y_slider(int value)
{
    if (!m_overlay_window)
    {
        append_log_user("微信语音坐标修改无效: 未显示覆盖层");
        ui->horizontalSlider_pos_y->setValue(m_wechat_pos_y);
        return;
    }
    if (!m_wechat_window_hwnd)
    {
        append_log_user("微信语音坐标修改无效: 请先选择微信窗口");
        ui->horizontalSlider_pos_y->setValue(m_wechat_pos_y);
        return;
    }
    m_wechat_pos_y = value;
    ui->spinBox_pos_y->setValue(m_wechat_pos_y);
    m_overlay_window->set_dot_pos_y(m_wechat_pos_y);
    m_scanner->set_click_pos_y(m_wechat_pos_y);
    append_log_debug(QString("DashBoard::update_overlay_range_slider:\n微信语音Y坐标设置为%1").arg(m_wechat_pos_y));
}

void DashBoard::update_overlay_pos_y_spinBox(int value)
{
    if (!m_overlay_window)
    {
        append_log_user("微信语音坐标修改无效: 未显示覆盖层");
        ui->spinBox_pos_y->setValue(m_wechat_pos_y);
        return;
    }
    if (!m_wechat_window_hwnd)
    {
        append_log_user("微信语音坐标修改无效: 请先选择微信窗口");
        ui->spinBox_pos_y->setValue(m_wechat_pos_y);
        return;
    }
    m_wechat_pos_y = value;
    ui->horizontalSlider_pos_y->setValue(m_wechat_pos_y);
    m_overlay_window->set_dot_pos_y(m_wechat_pos_y);
    m_scanner->set_click_pos_y(m_wechat_pos_y);
    append_log_debug(QString("DashBoard::update_overlay_range_spinBox:\n微信语音Y坐标设置为%1").arg(m_wechat_pos_y));
}

void DashBoard::update_overlay_range()
{
    // 如果微信窗口句柄有效，则更新滑块范围
    if (!m_wechat_window_hwnd) {
        return;
    }

    RECT rect;
    if (GetWindowRect(m_wechat_window_hwnd, &rect)) {
        int winHeight = rect.bottom - rect.top;
        ui->horizontalSlider_pos_y->setMaximum(winHeight - 20);
        ui->spinBox_pos_y->setMaximum(winHeight - 20);
        // qDebug() << QString("更新微信按钮坐标范围: 宽=%1, 高=%2").arg(winWidth).arg(winHeight);
    }
}

void DashBoard::handle_find_window_fail(window_type failed_window_type)
{
    if (failed_window_type == window_type::GAME)
    {
        append_log_user("未发现游戏窗口，即将开始处理游戏崩溃");
        stop_scanner();
        launch_crash_handler();
    }
    else if (failed_window_type == window_type::TPPW)
    {
        append_log_user("未发现目标微信窗口，监控终止，请在配置微信窗口后重新开始监控");
        stop_scanner();
    }
    else
    {
        append_log_user("找不到窗口：未知错误，监控终止 请重新手动打开");
        stop_scanner();
    }
}

void DashBoard::on_made_call(call_type type)
{
    append_log_user(QString("已发起%1微信语音").arg(type == call_type::GROUP? "群聊" : "单人"));
}

void DashBoard::set_game_hwnd(HWND hwnd)
{
    m_game_hwnd = hwnd;
}

void DashBoard::proceed_rejoin()
{
    m_rejoiner->set_game_hwnd(m_game_hwnd);
    launch_rejoiner();
}

void DashBoard::handle_crash_handler_bad_finish(QString msg)
{
    append_log_debug(QString("尝试处理游戏崩溃失败，错误原因:\n%1").arg(msg));
    append_log_user(QString("尝试处理游戏崩溃失败，错误原因:\n%1\n请手动重启监控!").arg(msg));
}

void DashBoard::restart_mornitor()
{
    launch_scanner();
}

void DashBoard::handle_rejoiner_bad_finish(QString msg)
{
    append_log_debug(QString("尝试重连游戏失败，错误原因:\n%1").arg(msg));
    append_log_user(QString("尝试重连游戏失败，错误原因:\n%1\n请手动重启监控!").arg(msg));
}

QString get_time_stamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

void DashBoard::append_log_debug(const QString &message)
{
    QString logLine = QString("[%1]\n%2").arg(get_time_stamp()).arg(message);
    qDebug() << "debug output: " << logLine;
    if (ui->textBrowser_debug_log)
        ui->textBrowser_debug_log->append(logLine);
}

void DashBoard::append_log_user(const QString &message)
{
    qDebug() << "user output: " << message;
    if (ui->textBrowser_user_log)
    {
        ui->textBrowser_user_log->append(message);
    }
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

    m_scanner->stop_work();
    m_scanner_thread->quit();
    m_scanner_thread->wait();

    m_crash_handler->cancel();
    m_crash_handler_thread->quit();
    m_crash_handler_thread->wait();

    m_rejoiner->cancel();
    m_rejoiner_thread->quit();
    m_rejoiner_thread->wait();

    m_server_list->stop_work();
    m_server_list_thread->quit();
    m_server_list_thread->wait();

    m_sender->stop_work();
    m_sender_thread->quit();
    m_sender_thread->wait();

    QMainWindow::closeEvent(event);
}
void DashBoard::handle_server_fetched(const QList<ServerInfo> &servers)
{
    // 0) 排序表格
    QList<ServerInfo> sortedServers = servers;
    int size = sortedServers.size();
    bool swapped = false;
    for (int i = 0; i < size - 1; i++) {
        swapped = false;
        for (int f = 0; f < size - i - 1; f++) {
            if (sortedServers[f].sessionName > sortedServers[f + 1].sessionName) {
                sortedServers.swapItemsAt(f, f + 1);
                swapped = true;
            }
        }
        if (!swapped) {
            break;
        }
    }
    
    // 1) 更新表格
    ui->serverTableWidget->clearContents();
    // ui->serverTableWidget->setColumnCount(2);
    ui->serverTableWidget->setHorizontalHeaderLabels(
        QStringList{"Players", "Session"});
    ui->serverTableWidget->setRowCount(sortedServers.size());
    for (int i = 0; i < sortedServers.size(); ++i) {
        ui->serverTableWidget->setItem(
            i, 0,
            new QTableWidgetItem(QString::number(sortedServers[i].numPlayers)));
        ui->serverTableWidget->setItem(
            i, 1, new QTableWidgetItem(sortedServers[i].sessionName));
    }
    ui->serverTableWidget->resizeColumnsToContents();
    
    // 2) 更新下拉列表（单选）
    if (!m_serverComboBoxUpdated) {
        ui->serverComboBox->clear();
        for (auto &info : sortedServers)
            ui->serverComboBox->addItem(info.sessionName);
        m_serverComboBoxUpdated = true;
    }
    
    // 3) 检测阈值
    if (!m_playerNumberAlarmSent)
    {
        QString selected = ui->serverComboBox->currentText();
        int threshold = ui->thresholdSpinBox->value();
        for (auto &info : servers)
        {
            if (info.sessionName == selected && info.numPlayers > threshold)
            {
                QString msg = QString("服务器 %1 当前人数 %2，已超出阈值 %3")
                                  .arg(info.sessionName)
                                  .arg(info.numPlayers)
                                  .arg(threshold);
                emit send_message(msg);
                block_player_number_alarm();
                break;
            }
        }
    }
    else
    {
        append_log_debug("服务器监控警报冷却中!");
    }
}

void DashBoard::refresh_table_next_round()
{
    this->m_serverComboBoxUpdated = false;
    if (!m_serverComboBoxUpdated)
    {
        append_log_user("下拉列表将在几秒后刷新！");
    }
}

void DashBoard::block_player_number_alarm()
{
    m_playerNumberAlarmSent = true;
    m_timer_table_CD = new QTimer();
    m_timer_table_CD->setSingleShot(true);
    m_timer_table_CD_conn = connect(m_timer_table_CD, &QTimer::timeout, this, &DashBoard::table_CD_helper);
    m_timer_table_CD->start(120000);
    append_log_debug("DashBoard::table_CD_helper:\n服务器人数警报冷却开始");
    append_log_user("服务器人数警报冷却开始");
}

void DashBoard::table_CD_helper()
{
    if (m_timer_table_CD)
    {
        if (m_timer_table_CD->isActive())
            m_timer_table_CD->stop();
        disconnect(m_timer_table_CD_conn);
        m_timer_table_CD->deleteLater();
        m_timer_table_CD = nullptr;
        m_playerNumberAlarmSent = false;
        append_log_debug("DashBoard::table_CD_helper:\n服务器人数警报冷却结束");
        append_log_user("服务器人数警报冷却结束");
    }
    else
    {
        append_log_debug("DashBoard::table_CD_helper:\n未知错误");
        append_log_user("未知错误");
    }
}

void DashBoard::changeAlarmKey_starved(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("starved", blocked);
    emit change_keyword_status("饿死", blocked);
}

void DashBoard::changeAlarmKey_waskilled(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("was killed", blocked);
    emit change_keyword_status("已死亡", blocked);
}

void DashBoard::changeAlarmKey_demolished(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("demolished", blocked);
    emit change_keyword_status("拆除", blocked);
}

void DashBoard::changeAlarmKey_froze(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("froze", blocked);
}

void DashBoard::changeAlarmKey_claimed(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("claimed", blocked);
    emit change_keyword_status("认养", blocked);
    emit change_keyword_status("unclaimed", blocked);
    emit change_keyword_status("放生", blocked);
}

void DashBoard::changeAlarmKey_promoted(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("promoted", blocked);
    emit change_keyword_status("提升", blocked);
    emit change_keyword_status("demoted", blocked);
    emit change_keyword_status("降职", blocked);
}

void DashBoard::changeAlarmKey_added(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("added", blocked);
    emit change_keyword_status("加入", blocked);
    emit change_keyword_status("was removed", blocked);
    emit change_keyword_status("踢出", blocked);
}

void DashBoard::changeAlarmKey_topublic(int status)
{
    bool blocked = status == Qt::Checked? true:false;
    emit change_keyword_status("to public", blocked);
    emit change_keyword_status("to private", blocked);
}

void DashBoard::onPlayP_AlarmSound()
{
    // 如果当前正在播放，则先停止再重新播放（可选）
    if (m_effect1->isPlaying()) {
        m_effect1->stop();
    }
    if (ui->playSoundcheckBox->isChecked()) {
        m_effect1->play();
    }
}

void DashBoard::onPlayLogAlarmSound()
{
    // 如果当前正在播放，则先停止再重新播放（可选）
    if (m_effect2->isPlaying()) {
        m_effect2->stop();
    }
    if (ui->playSoundcheckBox->isChecked()) {
        m_effect2->play();
    }
}

void DashBoard::updateCallMemberLable(QString member)
{
    ui->label_call_members->setText(member);
}

void DashBoard::onRejoinModeChanged(Qt::CheckState checkState)
{
    if (checkState == Qt::Checked)
    {
        m_rejoiner->set_has_mod(true);
        append_log_user("已启用含Mod服务器重载模式！");
    }
    else
    {
        m_rejoiner->set_has_mod(false);
        append_log_user("已禁用含Mod服务器重载模式！");
    }
}

void DashBoard::handle_warn(QString msg)
{
    QMessageBox::warning(this, "Warning!", msg);
}
