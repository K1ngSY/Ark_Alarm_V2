#include "scanner.h"
#include "utility.h"
#include "visual.h"
#include "motion.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <QDateTime>
#include <QCoreApplication>
#include <QFile>
Scanner::Scanner(QObject *parent)
    : KWorker{parent}
{
    // 所有指针在构造函数里需要全部赋值为 nullptr
    this->m_mCycle_timer_interval       = 10000; // 主任务循环时间暂时定为10秒一次
    this->m_mCall_member_timer_interval = 2000;  // 群呼成员刷新Timer

    this->m_call_member_check_timer = nullptr;
    this->m_cycle_timer             = nullptr;

    this->m_is_group_call = false;
    this->m_need_call     = false;
    this->m_need_text     = false;
    this->m_first_round   = true;

    this->m_click_coordinate_x = 20;
    this->m_click_coordinate_y = 20;

    // 设置持久化文件存放路径（程序所在目录下）
    m_log_file_path = QCoreApplication::applicationDirPath() + "/tribe_logs.txt";

    this->m_call_members = "[占位符]注意：您仍未设置群呼成员！";
    this->m_game_timeout_keywords << "HOST"
                                  << "CONNECTION"
                                  << "TIMEOUT"
                                  << "主机连接超时";
    m_P_keywords << "dino"
                 << "发现"
                 << "detected"
                 << "an"
                 << "enemy";
    m_serious_log_keywords << "被摧毁"
                           << "击杀"
                           << "was destroyed"
                           << "was killed by";
    m_nonSerious_log_keywords << "饿死"
                              << "已死亡"
                              << "认养"
                              << "放生"
                              << "拆除"
                              << "提升"
                              << "你的部落击杀了"
                              << "被加入"
                              << "starved"
                              << "died"
                              << "promoted"
                              << "demoted"
                              << "added"
                              << "demolished"
                              << "was killed"
                              << "was removed"
                              << "claimed"
                              << "unclaimed"
                              << "to public"
                              << "to private"
                              << "froze"
                              << "Your Tribe killed";
    m_all_log_keywords.append(m_serious_log_keywords);
    m_all_log_keywords.append(m_nonSerious_log_keywords);

    // ---------初始化过滤器字典---------
    m_alarm_filter["starved"] = false;
    m_alarm_filter["饿死"] = false;

    m_alarm_filter["was killed"] = false;
    m_alarm_filter["已死亡"] = false;

    m_alarm_filter["demolished"] = false;
    m_alarm_filter["拆除"] = false;

    m_alarm_filter["claimed"] = false;
    m_alarm_filter["认养"] = false;

    m_alarm_filter["unclaimed"] = false;
    m_alarm_filter["放生"] = false;

    m_alarm_filter["promoted"] = false;
    m_alarm_filter["提升"] = false;

    m_alarm_filter["added"] = false;
    m_alarm_filter["加入"] = false;

    m_alarm_filter["was removed"] = false;
    m_alarm_filter["踢出"] = false;

    m_alarm_filter["demoted"] = false;
    m_alarm_filter["降职"] = false;

    // 公开盘子
    m_alarm_filter["to public"] = false;
    // 私有盘子
    m_alarm_filter["to private"] = false;
    // 冻龙
    m_alarm_filter["froze"] = false;
    // ---------初始化过滤器字典end---------

    // ---------初始化警报中文提示词字典---------
    m_alarm_promts_Chinese["被摧毁"] = "您的建筑被摧毁！";
    m_alarm_promts_Chinese["击杀"] = "您的成员被杀！";
    m_alarm_promts_Chinese["was destroyed"] = "您的建筑被摧毁！";
    m_alarm_promts_Chinese["was killed by"] = "您的成员被杀！";
    m_alarm_promts_Chinese["饿死"] = "您的龙被饿死了！";
    m_alarm_promts_Chinese["已死亡"] = "自然死亡！";
    m_alarm_promts_Chinese["认养"] = "您的龙被认养了！";
    m_alarm_promts_Chinese["放生"] = "您的龙被放生了！";
    m_alarm_promts_Chinese["拆除"] = "您的建筑被拆除了！";
    m_alarm_promts_Chinese["提升"] = "您的某成员被升职！";
    m_alarm_promts_Chinese["你的部落击杀了"] = "您的部落击杀了敌方目标！";
    m_alarm_promts_Chinese["被加入"] = "有新成员加入了您的部落！";
    m_alarm_promts_Chinese["starved"] = "您的龙被饿死了！";
    m_alarm_promts_Chinese["died"] = "自然死亡！";
    m_alarm_promts_Chinese["promoted"] = "您的某成员被升职！";
    m_alarm_promts_Chinese["demoted"] = "您的某成员被降职！";
    m_alarm_promts_Chinese["added"] = "有新成员加入了您的部落！";
    m_alarm_promts_Chinese["demolished"] = "您的建筑被拆除了！";
    m_alarm_promts_Chinese["was killed"] = "自然死亡！";
    m_alarm_promts_Chinese["was removed"] = "某成员被踢出部落！";
    m_alarm_promts_Chinese["claimed"] = "您的龙被认养了！";
    m_alarm_promts_Chinese["unclaimed"] = "您的龙被放生了！";
    m_alarm_promts_Chinese["to public"] = "您的某建筑权限被设置为公开！";
    m_alarm_promts_Chinese["to private"] = "您的某建筑权限被设置为私有！";
    m_alarm_promts_Chinese["froze"] = "您的某龙被收！";
    m_alarm_promts_Chinese["Your Tribe killed"] = "您的部落击杀了敌方目标！";
}

Scanner::~Scanner()
{}

void Scanner::set_TPPW(const QString &title, HWND TPPW_hwnd)
{
    this->m_TPPW_title = title;
    emit log_message_Debug("Scanner::set_TPPW:\n已经设置新的微信窗口标题为" + m_TPPW_title);
    emit log_message_Debug("Scanner::set_TPPW:\nSet TPPW title to " + m_TPPW_title);
    emit log_message_User("微信窗口标题已更新为" + m_TPPW_title);
    this->m_TPPW_hwnd = TPPW_hwnd;
    emit log_message_Debug(QString("Scanner::set_TPPW:\n已经设置新的微信窗口句柄为 %1").arg((qulonglong)m_TPPW_hwnd));
    emit log_message_Debug(QString("Scanner::set_TPPW:\nSet TPPW Hwnd to %1").arg((qulonglong)m_TPPW_hwnd));
    emit log_message_User(QString("微信窗口句柄已更新为 %1").arg((qulonglong)m_TPPW_hwnd));
}

void Scanner::set_click_coordinates(const int &x, const int &y)
{
    this->m_click_coordinate_x = x;
    this->m_click_coordinate_y = y;
    emit log_message_Debug(QString("Scanner::set_click_coordinates:\n已更新微信电话坐标为 X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
    emit log_message_Debug(QString("Scanner::set_click_coordinates:\nSet click button coordinate to X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
    emit log_message_User(QString("更新微信电话坐标已为 X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
}

bool Scanner::start_work()
{
    // Do some checks before emit, check if it's all set.
    if (m_TPPW_title.isEmpty())
    {
        emit log_message_Debug("微信窗口标题为空");
        return false;
    }
    if (!m_TPPW_hwnd)
    {
        emit log_message_Debug("微信窗口句柄为空");
        return false;
    }
    emit start_signal();
    qDebug() << "Scanner::start";
    return true;
}

bool Scanner::stop_work()
{
    // Do some checks before emit, check if it's all set.
    emit stop_signal();
    qDebug() << "stop";
    return true;
}

QMap<QString, QPair<QStringList, bool>> Scanner::split_tribe_logs(const QString &raw_tribe_log)
{
    QList<QPair<QString, QString>> entries;
    QRegularExpression rx(R"(Day\s*\d+[,，]\s*\d{1,2}:\d{2}:\d{2}:)");
    auto it = rx.globalMatch(raw_tribe_log);

    // 记录每个匹配的起始位置和文本
    QVector<int> positions;
    QStringList stamps;
    while (it.hasNext())
    {
        auto m = it.next();
        positions.append(m.capturedStart());
        stamps.append(m.captured(0));
    }
    // 加上末尾，便于切最后一段
    positions.append(raw_tribe_log.length());

    // 逐段切片：去掉时间戳前缀，只留后续内容
    for (int i = 0; i < stamps.size(); ++i)
    {
        int start = positions[i], end = positions[i+1];
        QString segment = raw_tribe_log.mid(start, end - start).trimmed();
        QString content = segment.mid(stamps[i].length()).trimmed();
        entries.append(qMakePair(stamps[i], content));
    }
    // 将拆分后的条目按时间戳做降序排列（可选，仅保证最新日志先处理）
    std::sort(entries.begin(), entries.end(), [](const auto &a, const auto &b) {return a.first > b.first;});
    // 用于合并同关键词的数据结构：
    // key = matchedKeywords，value = 多条“[时间戳] 内容”的列表.
    QMap<QString, QPair<QStringList, bool>> map_keywords_to_list;

    // 逐条遍历：先持久化去重、再分类、最后积累到 map_KwToLogList
    for (const auto &p : entries)
    {
        const QString &ts      = p.first;    // 例如 "Day 1,10:05:07"
        const QString &content = p.second;   // 该条日志剩余内容

        // —— 持久化文件去重：appendIfNewLogEntry 返回 true 才是“新日志”
        if (!append_new_log(ts, content))
        {
            emit log_message_Debug("Scanner::split_tribe_logs: \n已存在日志，跳过: " + ts + "\n // Log already exists, skip");
            continue;
        }
        // —— 分类检测：检查是否匹配严重关键词或非严重关键词
        bool isSerious = false, isNonSerious = false;
        QString matched_keyword;
        for (const QString &keyword : m_all_log_keywords)
        {
            if (!keyword.isEmpty() && content.contains(keyword, Qt::CaseInsensitive))
            {
                matched_keyword = keyword;
                if (matched_keyword == "destroyed")
                {
                    if (content.contains("auto", Qt::CaseInsensitive) ||
                        content.contains("decay", Qt::CaseInsensitive) ||
                        content.contains("auto-", Qt::CaseInsensitive) ||
                        content.contains("-decay", Qt::CaseInsensitive) ||
                        content.contains("auto-decay", Qt::CaseInsensitive))
                    {
                        isNonSerious = true;
                    }
                    else
                    {
                        isSerious = true;
                    }
                    break;
                }
                if (m_serious_log_keywords.contains(matched_keyword, Qt::CaseInsensitive))
                {
                    isSerious = true;
                }
                else
                {
                    isNonSerious = true;
                }
                break;
            }
        }

        // 如果既不属于严重也不属于非严重，就不做报警，也不累计
        if (!(isSerious || isNonSerious))
        {
            continue;
        }

        // —— 检查该关键词是否被用户设置为“屏蔽”状态，如果屏蔽，则跳过
        bool blocked = !allow_this_keyword(matched_keyword);
        if (blocked) {
            emit log_message_Debug("Scanner::split_tribe_logs:\n关键词 \"" + matched_keyword + "\" 已屏蔽，跳过本条日志 // Keyword blocked, skip: " + matched_keyword);
            continue;
        }

        // —— 把“[时间戳] 内容”格式的字符串，添加到 map_KwToLogList[matchedKw] 列表中
        QString oneLine = QString("[%1] %2").arg(ts).arg(content);
        map_keywords_to_list[matched_keyword].first.append(oneLine);
        map_keywords_to_list[matched_keyword].second = isSerious;
    }
    return map_keywords_to_list;
}

bool Scanner::append_new_log(const QString &ts, const QString &content)
{
    QFile file(m_log_file_path);
    if (!file.open(QIODevice::ReadWrite | QIODevice::Text)) {
        emit log_message_Debug("Scanner::append_new_log:\n无法打开持久化本地日志文件：" + m_log_file_path);
        return false;
    }
    else
    {
        emit log_message_Debug(QString("Scanner::append_new_log:\n已打开位于 %1 的本地日志文件").arg(m_log_file_path));
    }

    // 1) 先检查文件中是否已有该时间戳
    QTextStream in(&file);
    bool exists = false;
    while (!in.atEnd()) {
        QString line = in.readLine();
        if (line.startsWith(ts + "\t")) {
            exists = true;
            break;
        }
    }

    // 2) 如果不存在，则移动到末尾，写入新条目
    if (!exists) {
        QTextStream out(&file);
        file.seek(file.size());
        out << ts << "\t" << content << "\n";
        return true;
    }
    return false;
}

void Scanner::send_text(const QString &msg)
{
    emit s_send_text(m_game_window_hwnd, msg);
    emit log_message_Debug(QString("Scanner::send_text: \n已发送一条信息：\n%1").arg(msg));
}

void Scanner::send_image(const QImage &img)
{
    emit s_send_image(m_game_window_hwnd, img);
    emit log_message_Debug(QString("Scanner::send_text: \n已发送一张图片"));
}

void Scanner::make_call()
{
    left_click(m_game_window_hwnd, m_click_coordinate_x, m_click_coordinate_y);
    emit made_call(SINGLE);
}

void Scanner::make_group_call()
{
    left_click(m_game_window_hwnd, m_click_coordinate_x, m_click_coordinate_y);
    //等待弹窗完全出现.
    QThread::msleep(300);
    // 1) 找到“微信选择成员”对话框.
    std::wstring title = QStringLiteral("微信选择成员").toStdWString();
    HWND dlg = FindWindowW(nullptr, title.c_str());
    if (!dlg) {
        emit log_message_Debug("Scanner::make_group_call:\n无法找到“微信选择成员”窗口");
        return;
    }
    SetForegroundWindow(dlg);
    QThread::msleep(30);

    // 2) 获取当前屏幕分辨率，计算横/纵缩放比例.
    int screenW = GetSystemMetrics(SM_CXSCREEN);
    int screenH = GetSystemMetrics(SM_CYSCREEN);
    double scaleX = screenW / 1920.0;
    double scaleY = screenH / 1080.0;

    // 3) 解析用户输入的成员并依次搜索点击.
    QStringList list = m_call_members.split(",", Qt::SkipEmptyParts);
    for (const QString &s : list) {
        int baseX = 183;
        int baseY = 63;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Scanner::make_group_call:\n点击搜索框，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
        paste_text(s.trimmed());
        emit log_message_Debug(QString("Scanner::make_group_call:\n粘贴用户名 %1").arg(s.trimmed()));
        baseX = 127;
        baseY = 115;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Scanner::make_group_call:\n点击首位，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
        baseX = 313;
        baseY = 59;
        left_click(dlg, int(baseX * scaleX), int(baseY * scaleY));
        emit log_message_Debug(QString("Scanner::make_group_call:\n点击清除，坐标=(%1, %2)").arg(int(baseX * scaleX)).arg(int(baseY * scaleY)));
        QThread::msleep(rand()%10);
    }

    // 4) 点击“确定”按钮（基准坐标 X,Y）.
    int btnX = int(447 * scaleX);
    int btnY = int(524 * scaleY);
    emit log_message_Debug(QString("Scanner::make_group_call:\n点击确定呼叫按钮，坐标=(%1, %2)").arg(btnX).arg(btnY));
    left_click(dlg, btnX, btnY);

    emit log_message_Debug("群呼完成");
    emit made_call(GROUP);
}

bool Scanner::allow_this_keyword(const QString &key)
{
    if (m_alarm_filter.contains(key))
    {
        emit log_message_Debug(QString("Scanner::allow_this_keyword:\n关键词“%1”存在于过滤器列表中，值为").arg((!m_alarm_filter[key])? "Allow" : "Blocked"));
        return !m_alarm_filter[key];
    }
    else
    {
        emit log_message_Debug(QString("Scanner::allow_this_keyword:\n关键词“%1”不在过滤器列表中，已放行").arg(key));
        return false;
    }
}

bool Scanner::check_windows_and_crash()
{
    // Check if game window exists.
    if (!scan_window(m_game_window_hwnd))
    {
        emit log_message_Debug("Scanner::check_windows_and_crash: \n游戏窗口句柄对应窗口不存在！ // Unable to find game window!");
        emit find_window_failed(GAME);
        return false;
    }
    // Check if TPPW exists.
    if (!scan_window(m_TPPW_hwnd))
    {
        emit log_message_Debug("Scanner::check_windows_and_crash: \n通讯平台窗口句柄对应窗口不存在！ // Unable to find TPPW!");
        emit find_window_failed(TPPW);
        return false;
    }
    // Check if crash windows exist.
    if (scan_crash_windows())
    {
        emit game_crashed();
        return false;
    }
    emit log_message_Debug("Scanner::check_windows_and_crash: \n全部验证通过 // All validations passed");
    return true;
}

bool Scanner::bind_game_window(const QString &title)
{
    // std::wstring w = title.toStdWString();
    // this->m_game_window_hwnd = ::FindWindowW(nullptr, w.c_str());
    // if (this->m_game_window_hwnd) return true;
    // else return false;
    if (!bind_window(title, m_game_window_hwnd))
    {
        return false;
    }
    else
    {
        return true;
    }
}

bool Scanner::ensure_tribe_log_open()
{
    int tries = 0;
    QString OCR_result;
    QImage game_screenshot, dummy_image;
    if (!print_window(m_game_window_hwnd, game_screenshot))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n截图失败 // Capture failed!");
        return false;
    }
    if (!OCR_area_T(game_screenshot, OCR_result, dummy_image))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\nOCR failed");
        return false;
    }
    while (OCR_result.isEmpty() && tries < 10)
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n部落日志未打开，尝试自动打开 // Tribe log not open, retry");
        if (!scan_window(m_game_window_hwnd))
        {
            emit log_message_Debug("Scanner::ensure_tribe_log_open:\n游戏窗口不存在！");
            emit log_message_User("检测到游戏窗口不存在");
            return false;
        }
        click_center_and_keyL(m_game_window_hwnd);
        QThread::msleep(400);
        if (!print_window(m_game_window_hwnd, game_screenshot))
        {
            emit log_message_Debug("Scanner::ensure_tribe_log_open:\n截图失败 // Capture failed!");
            return false;
        }
        if (!OCR_area_T(game_screenshot, OCR_result, dummy_image))
        {
            emit log_message_Debug("Scanner::ensure_tribe_log_open:\nOCR failed");
            return false;
        }
        if (!OCR_result.isEmpty()) {
            emit log_message_Debug("Scanner::ensure_tribe_log_open:\n部落日志已打开 // Tribe log is open now.");
            return true;
        }
        ++tries;
    }
    if (tries >= 10 && OCR_result.isEmpty()) {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n无法打开部落日志 // Unable to open tribe log");
        return false;
    }
    return true;
}

bool Scanner::check_parasaurolophus_alarm(const QString &ocr_result, QString &keyword_out)
{
    emit log_message_Debug(QString("Scanner::check_parasaurolophus_alarm:\n复栉龙警报识别结果为：%1").arg(ocr_result));
    for (const QString &keyword : m_P_keywords) {
        if (!keyword.isEmpty() && ocr_result.contains(keyword, Qt::CaseSensitive)) {
            keyword_out = keyword;
            emit log_message_Debug(QString("Scanner::check_parasaurolophus_alarm:\n复栉龙警报识别到关键词：%1").arg(keyword_out));
            return true;
        }
    }
    emit log_message_Debug(QString("Scanner::check_parasaurolophus_alarm:\n复栉龙警报未识别到关键词"));
    return false;
}

void Scanner::handle_parasaurolophus_alert(const QString &OCR_resultconst, const QString &keyword)
{
    QString message = QString("—————— K_AlarmBot ——————\n\n"
                              "%1 \n(Key: %2)\n----------------------------------\n"
                              "Attention: Parasaurolophus has detected the enemy!\n"
                              "副栉龙发现敌人，请留意！\n"
                              "RAW Result\n[%3]\n\n"
                              "—————— K_AlarmBot ——————")
                          .arg(make_time_stamp())
                          .arg(keyword)
                          .arg(OCR_resultconst);
    send_text(message);
    emit increase_alarm_count();
    emit play_alarm_sound_P();
}

void Scanner::handle_tribe_alerts(const QImage &screenshot, const QMap<QString, QPair<QStringList, bool> > &logs_map)
{
    // 遍历 map_KwToLogList，对每个关键词 matched_keyword，只发一次合并预告
    for (auto it = logs_map.constBegin(); it != logs_map.constEnd(); ++it)
    {
        const QString &matched_keyword = it.key();
        const QList<QString> &logs = it.value().first; // e.g. ["[Day 1,10:05:07:] foo", "[Day 1,10:05:10:] bar", ...]

        // 判断本关键词 matched_keyword 属于严重还是非严重：
        bool is_serious = it.value().second;

        // 构造合并后的“部落日志”文本：列出所有 logs 列表里的条目
        // 可以先写好一个头部说明，再循环拼接所有“[ts] 内容”
        QString tribeText;
        tribeText += "—— K_AlarmBot ——\n\n";
        tribeText += "--------\n";
        tribeText += QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
        tribeText += "\n";
        tribeText += "--------\n\n";
        // 英文版提示
        tribeText += "----EN----\n";
        tribeText += QString("Attention: Tribe Log Key Triggered: \"%1\"\n").arg(matched_keyword);
        tribeText += "----EN----\n\n";

        // 中文版提示
        tribeText += "----CN----\n";
        tribeText += QString("部落日志触发：\"%1\"\n").arg(matched_keyword);
        if (m_alarm_promts_Chinese.contains(matched_keyword))
            tribeText += QString("%1\n").arg(m_alarm_promts_Chinese[matched_keyword]);
        tribeText += "----CN----\n\n";

        tribeText += "----LOG----\n";
        // 再把所有实际日志内容一行行拼接进去
        for (const QString &oneLine : logs)
        {
            tribeText += oneLine + "\n";
        }
        tribeText += "----LOG----\n\n";

        tribeText += "—— K_AlarmBot ——";

        if (m_need_text)
        {
            emit increase_alarm_count();
            send_text(tribeText);
            send_image(screenshot);
        }
        if (m_need_call && is_serious)
        {
            if (m_is_group_call) {
                make_group_call();
            }
            else {
                make_call();
            }
        }
    }
    emit increase_alarm_count();
    emit play_alarm_sound_log();
}

QString Scanner::make_time_stamp()
{
    return QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");
}

void Scanner::handle_start_signal()
{
    this->m_first_round = true;
    if (!m_cycle_timer)
    {
        m_cycle_timer = new QTimer(this);
        if (m_cycle_timer) emit log_message_Debug("Scanner::handle_start_signal:\n已在栈上声明了一个新的cycle_timer指针！");
        m_cycle_timer->setInterval(m_mCycle_timer_interval);
        m_cycle_timer_conn = connect(m_cycle_timer, &QTimer::timeout, this, &Scanner::scan);
    }
    if (!m_call_member_check_timer)
    {
        m_call_member_check_timer = new QTimer(this);
        if (m_call_member_check_timer)
            emit log_message_Debug("Scanner::handle_start_signal:\n已在栈上声明了一个新的call_member_checkt_imer指针！");
        m_call_member_check_timer->setInterval(m_mCall_member_timer_interval);
        m_call_member_check_timer_conn = connect(m_call_member_check_timer, &QTimer::timeout, this, &Scanner::refresh_call_member);
    }
    if (!m_cycle_timer->isActive())
    {
        m_cycle_timer->start();
        if (m_cycle_timer->isActive())
        {
            emit log_message_Debug("Scanner::handle_start_signal:\n监控循环启动！");
        }
    }
    else
    {
        emit log_message_Debug("Scanner::handle_start_signal:\n监控循环已在运行！");
    }
    if (!m_call_member_check_timer->isActive())
    {
        m_call_member_check_timer->start();
        if (m_call_member_check_timer->isActive())
        {
            emit log_message_Debug("Scanner::handle_start_signal:\n群呼成员监控启动！");
        }
    }
    else
    {
        emit log_message_Debug("Scanner::handle_start_signal:\n群呼成员监控已在运行！");
    }
}

void Scanner::handle_stop_signal()
{
    if (m_cycle_timer)
    {
        if (m_cycle_timer->isActive())
        {
            m_cycle_timer->stop();
        }
        disconnect(m_cycle_timer_conn);
        m_cycle_timer->deleteLater();
        m_cycle_timer = nullptr;
    }
    if (m_call_member_check_timer)
    {
        if (m_call_member_check_timer->isActive())
        {
            m_call_member_check_timer->stop();
        }
        disconnect(m_call_member_check_timer_conn);
        m_call_member_check_timer->deleteLater();
        m_call_member_check_timer = nullptr;
    }
    if (!(m_cycle_timer || m_call_member_check_timer))
    {
        emit log_message_Debug("Scanner::handle_stop_signal:\n两个QTimer指针均已被delete");
    }
    else
    {
        if (!m_cycle_timer && !m_call_member_check_timer)
        {
            emit log_message_Debug("Scanner::handle_stop_signal:\n两个QTimer指针均未被delete");
        }
        else if (!m_cycle_timer)
        {
            emit log_message_Debug("Scanner::handle_stop_signal:\nm_cycle_timer指针未被delete");
        }
        else
        {
            emit log_message_Debug("Scanner::handle_stop_signal:\nm_call_member_check_timer指针未被delete");
        }
    }
}

void Scanner::scan()
{
    if (!bind_game_window(m_game_window_title))
    {
        emit log_message_Debug("Scanner::scan: \n无法绑定游戏窗口！ // Unable to bind game window!");
        emit find_window_failed(GAME);
        return;
    }
    if (!m_TPPW_hwnd) {
        emit send_warn("Scanner::scan: \n通讯平台窗口句柄不存在！ // Alarm platform window handle is missing!");
        emit TPPW_hwnd_failed();
        return;
    }
    if (!check_windows_and_crash())
    {
        emit log_message_Debug("Scanner::scan:\n游戏窗口未就绪，本轮检测终止");
        return;
    }
    QImage game_screenshot, area_P, area_log;
    QString result_P, result_log, keyword_p;
    if(!print_window(m_game_window_hwnd, game_screenshot))
    {
        emit log_message_Debug("Scanner::scan: \n截图失败 // Unable to take screenshot");
        return;
    }
    if (!OCR_area_P(game_screenshot, result_P, area_P))
    {
        emit log_message_Debug("Scanner::scan: \n副栉龙区域OCR失败 // OCR parasaurolophus area faild");
        return;
    }
    if (!ensure_tribe_log_open())
    {
        emit log_message_Debug("Scanner::scan: \n部落日志无法打开 // Unable to open tribe log");
        return;
    }
    if (!OCR_area_T(game_screenshot, result_log, area_log))
    {
        emit log_message_Debug("Scanner::scan: \n部落日志区域OCR失败 // OCR tribe log area faild");
        return;
    }
    if (check_parasaurolophus_alarm(result_P, keyword_p))
    {
        if (m_need_text)
        {
            handle_parasaurolophus_alert(result_P, keyword_p);
        }
    }
    if (result_log.isEmpty())
    {
        emit log_message_Debug("Scanner::scan: \n部落日志OCR结果为空，本轮检测终止");
        return;
    }
    if (in_game_error(result_log))
    {
        emit log_message_Debug("Scanner::scan: \n部落日志OCR结果中检测到游戏掉线，本轮检测终止");
        return;
    }
    QMap<QString, QPair<QStringList, bool>> tribe_log_result_map = split_tribe_logs(result_log);
    if (tribe_log_result_map.isEmpty())
    {
        emit log_message_Debug("Scanner::scan: \n部落日志OCR结果中未侦测到关键词，本轮检测已正常结束");
    }
    else
    {
        if (!first_round())
        {
            handle_tribe_alerts(game_screenshot, tribe_log_result_map);
        }
    }
    emit log_message_Debug("Scanner::scan: \n函数正常执行完毕！");
}

void Scanner::refresh_call_member()
{
    emit return_call_member(m_call_members);
}





