#include "dashboard.h"
#include "ui_dashboard.h"
#include "utility.h"
#include "windowselectiondialog.h"
#include "togglebutton.h"
#include <QThread>
#include <QDebug>
#include <QMessageBox>
#include <QFile>
#include <QDir>
#include <QUrl>

DashBoard::DashBoard(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::DashBoard)
    , m_overlay_window(nullptr)
    , m_wechat_window_hwnd(nullptr)
    , m_game_hwnd(nullptr)
    , m_timer_table_CD(nullptr)
    , m_timer_slider_range(nullptr)
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
    , m_play_sound(false)
    , m_audioOutput1(new QAudioOutput(this))
    , m_audioOutput2(new QAudioOutput(this))
    , m_player1(new QMediaPlayer(this))
    , m_player2(new QMediaPlayer(this))
    , m_round_count(0)
    , m_alarm_count(0)
{
    ui->setupUi(this);
    ui->lcdNumber_alarm->setDigitCount(5);
    ui->lcdNumber_round->setDigitCount(5);
    ui->lcdNumber_alarm->display(m_alarm_count);
    ui->lcdNumber_round->display(m_round_count);
    ui->label_status->setText(tr("欢迎使用！"));
    ui->thresholdSpinBox->setValue(70);
    ui->horizontalSlider_pos_y->setMinimum(20);
    ui->spinBox_pos_y->setMinimum(20);
    ui->Main_tabWidget->tabBar()->hide();

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

    connect(this, &DashBoard::_send_message, m_sender, &Sender::send_text);

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

    // 捕获图片按比例缩放
    ui->image1Label->setScaledContents(true);
    ui->image2Label->setScaledContents(true);

    // Scanner
    connect(m_scanner, &Scanner::send_warn, this, &DashBoard::handle_warn);
    connect(m_scanner, &Scanner::return_call_member, this, &DashBoard::updateCallMemberLable);
    connect(m_scanner, &Scanner::made_call, this, &DashBoard::on_made_call);

    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    connect(m_scanner, &Scanner::s_send_text, m_sender, &Sender::send_text);
    connect(m_scanner, &Scanner::s_send_image, m_sender, &Sender::send_image);
    connect(m_scanner, &Scanner::s_send_call, m_sender, &Sender::make_call);
    connect(m_scanner, &Scanner::s_send_group_call, m_sender, &Sender::make_group_call);
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!
    // 所有需要发消息到微信的任务都要创建发送消息的信号然后将该信号连接到sender!!!!!!!!!!

    connect(m_scanner, &Scanner::got_picture_P, this, &DashBoard::update_image_p);
    connect(m_scanner, &Scanner::got_picture_log, this, &DashBoard::update_image_t);
    connect(m_scanner, &Scanner::game_timeout, this, &DashBoard::handle_in_game_error);
    connect(m_scanner, &Scanner::increase_round_count, this, &DashBoard::increase_scan_count);
    connect(m_scanner, &Scanner::increase_alarm_count, this, &DashBoard::increase_alarm_count);
    connect(m_scanner, &Scanner::find_window_failed, this, &DashBoard::handle_find_window_fail);
    connect(m_scanner, &Scanner::game_crashed, this, &DashBoard::handle_crash);

    // CrashHandler
    connect(m_crash_handler, &CrashHandler::wait_game_window_timeout, this, &DashBoard::handle_crashHandler_game_window_timeout);
    connect(m_crash_handler, &CrashHandler::wait_start_button_timeout, this, &DashBoard::handle_crashHandler_start_button_timeout);
    connect(m_crash_handler, &CrashHandler::got_game_hwnd, this, &DashBoard::handle_crashHandler_got_game_hwnd);
    connect(m_crash_handler, &CrashHandler::finished_0, this, &DashBoard::proceed_rejoin);

    // Rejoiner

    connect(m_rejoiner, &Rejoiner::finished_1, this, &DashBoard::handle_rejoiner_finished1);
    connect(m_rejoiner, &Rejoiner::finished_0, this, &DashBoard::handle_rejoiner_finished0);

    connect(ui->checkBox_is_group, &QCheckBox::checkStateChanged, m_scanner, &Scanner::update_group_call_status);
    // Toggle Buttons
    connect(ui->toggleButton_call_alarm_T, &ToggleButton::toggled, m_scanner, &Scanner::set_need_call_T);
    connect(ui->toggleButton_call_alarm_P, &ToggleButton::toggled, m_scanner, &Scanner::set_need_call_P);
    connect(ui->toggleButton_text_alarm_T, &ToggleButton::toggled, m_scanner, &Scanner::set_need_text_T);
    connect(ui->toggleButton_text_alarm_P, &ToggleButton::toggled, m_scanner, &Scanner::set_need_call_P);
    connect(ui->toggleButton_auto_respawn, &ToggleButton::toggled, m_scanner, &Scanner::set_enable_auto_respawn);

    // PushButtons
    connect(ui->pushButton_start, &QPushButton::clicked, this, &DashBoard::start_monitor);
    connect(ui->pushButton_stop, &QPushButton::clicked, this, &DashBoard::stop_all);
    connect(ui->pushButton_test, &QPushButton::clicked, m_scanner, &Scanner::full_test);
    connect(ui->pushButton_select_wechat_window, &QPushButton::clicked, this, &DashBoard::select_wechat_window);
    connect(ui->updateDowTablepushButton, &QPushButton::clicked, this, &DashBoard::refresh_server_combo_box);
    connect(ui->pushButton_debug_log_clear, &QPushButton::clicked, ui->textBrowser_debug_log, &QTextBrowser::clear);
    connect(ui->pushButton_user_log_clear, &QPushButton::clicked, ui->textBrowser_user_log, &QTextBrowser::clear);

    // Overlay WeChat Pos Y
    connect(ui->horizontalSlider_pos_y, &QSlider::valueChanged, this, &DashBoard::update_overlay_pos_y_slider);
    connect(ui->spinBox_pos_y, &QSpinBox::valueChanged, this, &DashBoard::update_overlay_pos_y_spinBox);

    // Tab Switching
    connect(ui->pushButton_to_tab1, &QPushButton::clicked, this, &DashBoard::switch_to_tab_1);
    connect(ui->pushButton_to_tab2, &QPushButton::clicked, this, &DashBoard::switch_to_tab_2);
    connect(ui->pushButton_to_tab3, &QPushButton::clicked, this, &DashBoard::switch_to_tab_3);
    connect(ui->pushButton_to_tab4, &QPushButton::clicked, this, &DashBoard::switch_to_tab_4);

    // ServerList
    connect(m_server_list, &ServerList::servers_fetched, this, &DashBoard::handle_server_fetched);


    // 微信覆盖层 / overlayWindow
    connect(ui->checkBox_overlay_visible, &QCheckBox::checkStateChanged, this, &DashBoard::on_overlay_visibility_changed);

    // 报警音控制
    connect(ui->playSoundcheckBox, &QCheckBox::checkStateChanged, this, &DashBoard::change_play_sound);

    // 警报过滤器
    connect(this, &DashBoard::change_keyword_status, m_scanner, &Scanner::set_filter_key);
    connect(ui->checkBox_starved, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_starved);
    connect(ui->checkBox_waskilled, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_waskilled);
    connect(ui->checkBox_demolished, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_demolished);
    connect(ui->checkBox_froze, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_froze);
    connect(ui->checkBox_claimed, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_claimed);
    connect(ui->checkBox_promoted, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_promoted);
    connect(ui->checkBox_added, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_added);
    connect(ui->checkBox_topublic, &QCheckBox::checkStateChanged, this, &DashBoard::changeAlarmKey_topublic);

    // Rejoin Mode Change
    connect(ui->checkBox_force_rejoin_with_mods, &QCheckBox::checkStateChanged, m_rejoiner, &Rejoiner::set_has_mod);

    // Call member
    connect(ui->lineEdit_call_members, &QLineEdit::textChanged, m_scanner, &Scanner::set_call_members);
    // Server ID
    connect(ui->lineEdit_server_ID, &QLineEdit::textChanged, m_rejoiner, &Rejoiner::set_server_ID);
    // Bed Name
    connect(ui->lineEdit_beds, &QLineEdit::textChanged, m_scanner, &Scanner::set_bed_name);


    // —————————— 音频对象配置 —————————— 音频对象配置 —————————— 音频对象配置 ——————————
    // —— 1. 获取可执行文件所在目录 ——
    // applicationDirPath() 返回可执行文件所在的文件夹（不带斜杠尾部）
    QString exeDir = QCoreApplication::applicationDirPath();

    // —— 2. 拼接出各自的 WAV 文件路径 ——
    // 假设同级目录下有 sound1.wav、sound2.wav
    QString wavPath1 = QDir(exeDir).filePath("P_Alarm.wav");
    QString wavPath2 = QDir(exeDir).filePath("Log_Alarm.wav");

    m_player1->setAudioOutput(m_audioOutput1);
    m_player2->setAudioOutput(m_audioOutput2);

    m_audioOutput1->setVolume(1.0);
    m_audioOutput2->setVolume(1.0);
    m_player1->setSource(QUrl::fromLocalFile(wavPath1));
    m_player1->setLoops(2);       // 播放次数，1 表示播放一次；QSoundEffect::Infinite 表示循环播放
    m_player2->setSource(QUrl::fromLocalFile(wavPath2));
    m_player2->setLoops(2);

    // —— 4. 检查文件是否存在（可选） ——
    // 如果文件不存在，setSource 不会报错，但 play() 会静默不出声。
    if (!QFile::exists(wavPath1)) {
        append_log_user("P_Alarm.wav 不存在，请检查可执行目录下是否有该文件！");
    }
    if (!QFile::exists(wavPath2)) {
        append_log_user("Log_Alarm.wav 不存在，请检查可执行目录下是否有该文件！");
    }
    // —— 5. 连接信号槽 ——
    connect(m_scanner, &Scanner::play_alarm_sound_P, this, &DashBoard::onPlayP_AlarmSound);
    connect(m_scanner, &Scanner::play_alarm_sound_log, this, &DashBoard::onPlayLogAlarmSound);
    // —————————— 音频对象配置 —————————— 音频对象配置 —————————— 音频对象配置 ——————————

    if (!test_ocr())
    {
        append_log_user("文字识别模块未通过测试，请重新启动软件或重新获取软件重试！");
    }
    else
    {
        append_log_user("文字识别模块测试成功！");
    }
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
    if (wechatDialog.exec() == QDialog::Accepted)
    {
        auto wechatWin = wechatDialog.selected_window();
        m_wechat_window_hwnd = wechatWin.hwnd;
        m_wechat_window_title = wechatWin.title;  // 保存窗口标题
        append_log_user(QString("选定微信窗口：%1 句柄：%2").arg(wechatWin.title).arg((qulonglong)m_wechat_window_hwnd));
    }
    else
    {
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

void DashBoard::on_overlay_visibility_changed(Qt::CheckState state)
{
    if(m_wechat_window_hwnd)
    {
        if (state == Qt::CheckState::Checked)
        {
            if (!m_overlay_window)
            {
                m_overlay_window = new OverlayWindow(m_wechat_window_hwnd, m_wechat_pos_y, this);
                if (!m_timer_slider_range)
                {
                    m_timer_slider_range = new QTimer(this);
                    m_timer_slider_range_conn = connect(m_timer_slider_range, &QTimer::timeout, this, &DashBoard::update_overlay_range);
                }
                if (!m_timer_slider_range->isActive())
                {
                    m_timer_slider_range->start(200);
                }
            }
            m_overlay_window->show();
            append_log_user("显示微信覆盖层");
        }
        else
        {
            if(m_overlay_window)
            {
                m_overlay_window->hide();
                if (m_timer_slider_range)
                {
                    if (m_timer_slider_range->isActive())
                    {
                        m_timer_slider_range->stop();
                    }
                    disconnect(m_timer_slider_range_conn);
                    m_timer_slider_range->deleteLater();
                    m_timer_slider_range = nullptr;
                }
                delete m_overlay_window;
                m_overlay_window = nullptr;
                append_log_user("隐藏微信覆盖层");
            }
        }
    }
    else {
        append_log_user("未选择微信窗口，请选择后重试");
        ui->checkBox_overlay_visible->setCheckState(Qt::CheckState::Unchecked);
        return;
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
    QString logLine = QString("[%1]\n%2\n").arg(get_time_stamp()).arg(message);
    qDebug() << "debug output: " << logLine;
    if (ui->textBrowser_debug_log)
        ui->textBrowser_debug_log->append(logLine);
}

void DashBoard::append_log_user(const QString &message)
{
    qDebug() << "user output: " << message;
    if (ui->textBrowser_user_log)
    {
        ui->textBrowser_user_log->append(message + "\n");
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

    if (m_timer_table_CD)
    {
        if (m_timer_table_CD->isActive())
        {
            m_timer_table_CD->stop();
        }
        disconnect(m_timer_table_CD_conn);
        m_timer_table_CD->deleteLater();
        m_timer_table_CD = nullptr;
    }

    if (m_timer_slider_range)
    {
        if (m_timer_slider_range->isActive())
        {
            m_timer_slider_range->stop();
        }
        disconnect(m_timer_slider_range_conn);
        m_timer_slider_range->deleteLater();
        m_timer_slider_range = nullptr;
    }

    if (m_audioOutput1)
    {
        m_audioOutput1->deleteLater();
        m_audioOutput1 = nullptr;
    }

    if (m_audioOutput2)
    {
        m_audioOutput2->deleteLater();
        m_audioOutput2 = nullptr;
    }

    if (m_player1)
    {
        if (m_player1->isPlaying())
        {
            m_player1->stop();
        }
        m_player1->deleteLater();
        m_player1 = nullptr;
    }

    if (m_player2)
    {
        if (m_player2->isPlaying())
        {
            m_player2->stop();
        }
        m_player2->deleteLater();
        m_player2 = nullptr;
    }
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

void DashBoard::refresh_server_combo_box()
{
    if (m_serverComboBoxUpdated)
    {
        m_serverComboBoxUpdated = false;
        append_log_debug("DashBoard::refresh_server_combo_box:\n已将m_serverComboBoxUpdated设置为false");
    }
    if (!m_serverComboBoxUpdated)
    {
        append_log_user("服务器列表下拉菜单将在几秒后刷新！");
    }
}

void DashBoard::handle_made_calls(call_type type)
{
    if (type == call_type::GROUP)
    {
        append_log_user("已呼出微信群语音");
    }
    else
    {
        append_log_user("已呼出微信单人语音");
    }
}

void DashBoard::handle_crash()
{
    append_log_user("检测到游戏崩溃，开始处理崩溃……");
    stop_scanner();
    launch_crash_handler();
}

void DashBoard::handle_crashHandler_game_window_timeout()
{
    append_log_user("等待游戏窗口超时，崩溃处理失败，请您手动重连游戏重启监控");
    emit _send_message(m_wechat_window_hwnd, "等待游戏窗口超时，崩溃处理失败，请您手动重连游戏重启监控");
    stop_all();
}

void DashBoard::handle_crashHandler_start_button_timeout()
{
    append_log_user("等待游戏加载超时，崩溃处理失败，请您手动重连游戏重启监控");
    emit _send_message(m_wechat_window_hwnd, "等待游戏加载超时，崩溃处理失败，请您手动重连游戏重启监控");
    stop_all();
}

void DashBoard::handle_crashHandler_got_game_hwnd(HWND game_hwnd)
{
    append_log_debug("DashBoard::handle_crashHandler_got_game_hwnd:\n主窗拿到崩溃处理器传入的游戏窗口句柄");
    m_game_hwnd = game_hwnd;
}

void DashBoard::handle_rejoiner_finished1(QString msg)
{
    append_log_user("重连游戏失败，请您手动重连游戏，原因：\n" + msg);
    emit _send_message(m_wechat_window_hwnd, "重连游戏失败，请您手动重连游戏，原因：\n" + msg);
}

void DashBoard::handle_rejoiner_finished0()
{
    append_log_user("重连游戏成功，重启监控！");
    emit _send_message(m_wechat_window_hwnd, "重连游戏成功，重启监控！");
    start_monitor();
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
    if (!m_play_sound)
    {
        append_log_debug("尝试播放报警音频P,但用户未禁用了该功能,已取消");
        return;
    }
    if (m_player1->isPlaying()) {
        m_player1->stop();
    }
    if (ui->playSoundcheckBox->isChecked()) {
        m_player1->play();
    }
}

void DashBoard::onPlayLogAlarmSound()
{
    // 如果当前正在播放，则先停止再重新播放（可选）
    if (!m_play_sound)
    {
        append_log_debug("尝试播放报警音频T,但用户未禁用了该功能,已取消");
        return;
    }
    if (m_player2->isPlaying()) {
        m_player2->stop();
    }
    if (ui->playSoundcheckBox->isChecked()) {
        m_player2->play();
    }
}

void DashBoard::updateCallMemberLable(QString member)
{
    ui->label_call_members->setText(member);
}

void DashBoard::handle_warn(QString msg)
{
    QMessageBox::warning(this, "Warning!", msg);
}

void DashBoard::switch_to_tab_1()
{
    ui->Main_tabWidget->setCurrentWidget(ui->tab_1);
}

void DashBoard::switch_to_tab_2()
{
    ui->Main_tabWidget->setCurrentWidget(ui->tab_2);
}

void DashBoard::switch_to_tab_3()
{
    ui->Main_tabWidget->setCurrentWidget(ui->tab_3);
}

void DashBoard::switch_to_tab_4()
{
    ui->Main_tabWidget->setCurrentWidget(ui->tab_4);
}

void DashBoard::change_play_sound(Qt::CheckState checkState)
{
    m_play_sound = checkState == Qt::CheckState::Checked;
    emit append_log_user(QString("已%1警报音功能").arg(m_play_sound? "启用" : "禁用"));
}
