#include "scanner.h"
#include "utility.h"
#include "visual.h"
#include "motion.h"
#include <QDebug>
#include <QThread>
#include <QRegularExpression>
#include <QDateTime>
Scanner::Scanner(QObject *parent)
    : KSubthread{parent}
{

}

Scanner::~Scanner()
{

}

void Scanner::set_TPPW(const QString &title, HWND TPPW_hwnd)
{
    this->m_TPPW_title = title;
    emit log_message_Debug("Alarm_set_TPPW:\n已经设置新的微信窗口标题为" + m_TPPW_title);
    emit log_message_Debug("Alarm_set_TPPW:\nSet TPPW title to " + m_TPPW_title);
    this->m_TPPW_hwnd = TPPW_hwnd;
    emit log_message_Debug(QString("Alarm_set_TPPW:\n已经设置新的微信窗口句柄为 %1").arg((qulonglong)m_TPPW_hwnd));
    emit log_message_Debug(QString("Alarm_set_TPPW:\nSet TPPW Hwnd to %1").arg((qulonglong)m_TPPW_hwnd));
}

void Scanner::set_click_coordinates(const int &x, const int &y)
{
    this->m_click_coordinate_x = x;
    this->m_click_coordinate_y = y;
    emit log_message_Debug(QString("Alarm_set_click_coordinates:\n已更新微信电话坐标为 X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
    emit log_message_Debug(QString("Alarm_set_click_coordinates:\nSet click button coordinate to X: %1 Y: %2").arg(m_click_coordinate_x).arg(m_click_coordinate_y));
}

bool Scanner::start()
{
    // Do some checks before emit, check if it's all set.
    emit start_signal();
    qDebug() << "start";
    return true;
}

bool Scanner::stop()
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

bool Scanner::check_windows_and_crash()
{
    if (!bind_game_window(m_game_window_title))
    {
        emit log_message_Debug("Scanner::check_windows_and_crash: \n无法绑定游戏窗口！ // Unable to bind game window!");
        emit find_window_failed(GAME);
        return false;
    }
    if (!m_TPPW_hwnd) {
        emit send_warn("Scanner::check_windows_and_crash: \n通讯平台窗口句柄不存在！ // Alarm platform window handle is missing!");
        emit find_window_failed(TPPW);
        return false;
    }
    if (scan_crash_windows()) {
        emit game_crashed();
        return false;
    }
    if (!scan_window(m_game_window_hwnd)) {
        emit find_window_failed(GAME);
        return false;
    }
    if (!scan_window(m_TPPW_hwnd)) {
        emit find_window_failed(TPPW);
        return false;
    }
    emit log_message_Debug("Scanner::check_windows_and_crash: \n全部验证通过 // All validations passed");
    return true;
}

bool Scanner::bind_game_window(const QString &title)
{
    std::wstring w = title.toStdWString();
    this->m_game_window_hwnd = ::FindWindowW(nullptr, w.c_str());
    if (this->m_game_window_hwnd) return true;
    else return false;
}

bool Scanner::capture_and_analyze(QString &ocr_result_1, QString &ocr_result_2, QImage &pic_1, QImage &pic_2, QImage &screenshot)
{
    RECT rect;
    if (!GetWindowRect(m_game_window_hwnd, &rect))
    {
        emit log_message_Debug("Scanner::capture_and_analyze: \n获取窗口矩形失败 // Failed to get window rect");
        return false;
    }
    if (!analyze_game_window(m_game_window_hwnd, ocr_result_1, ocr_result_2, pic_1, pic_2, screenshot))
    {
        emit log_message_Debug("Scanner::capture_and_analyze: \n分析游戏窗口失败 // analyzeGameWindow failed");
        return false;
    }
    // 自动打开部落日志重试 / Retry opening tribe log if needed
    if (!ensure_tribe_log_open())
        return false;
    // 检测错误关键词（超时/断连） / Detect error keywords
    for (const QString &kw : m_game_timeout_keywords) {
        if (!kw.isEmpty() && ocr_result_2.contains(kw, Qt::CaseInsensitive)) {
            emit log_message_Debug("Scanner::capture_and_analyze: \n检测到连接丢失或超时 // Detected timeout: " + kw);
            emit log_message_User("游戏已掉线！");
            emit game_timeout();
            return false;
        }
    }
    return true;
}

bool Scanner::ensure_tribe_log_open()
{
    int tries = 0;
    QString OCR_result;
    QImage game_screenshot;
    if (!print_window(m_game_window_hwnd, game_screenshot))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\n截图失败 // Capture failed!");
        return false;
    }
    if (!OCR_area_T(game_screenshot, OCR_result))
    {
        emit log_message_Debug("Scanner::ensure_tribe_log_open:\nOCR failed");
        return false;
    }
    while (OCR_result.isEmpty() && tries < 10) {
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
        if (!OCR_area_T(game_screenshot, OCR_result))
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

void Scanner::handle_parasaurolophus_alert(const QString &keyword)
{
    QString message = "Alarm_doOneRound:\n副栉龙警报关键词检测到: " + keyword + " // Parasaurolophus keyword detected";
    send_text(message);
    emit play_alarm_sound_P();
}

void Scanner::handle_tribe_alerts(const QImage &screenshot, const QMap<QString, QPair<QStringList, bool> > &logs_map)
{
    // 遍历 map_KwToLogList，对每个关键词 matched_keyword，只发一次合并预告
    for (auto it = logs_map.constBegin(); it != logs_map.constEnd(); ++it) {
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
        for (const QString &oneLine : logs) {
            tribeText += oneLine + "\n";
        }
        tribeText += "----LOG----\n\n";

        tribeText += "—— K_AlarmBot ——";

        if (m_need_text) {
            emit increase_alarm_count();
            send_text(tribeText);
            send_image(screenshot);
        }
        if (m_need_call && is_serious) {
            if (m_is_group_call) {
                make_group_call();
            }
            else {
                make_call();
            }
        }
        emit play_alarm_sound_log();
    }
}





