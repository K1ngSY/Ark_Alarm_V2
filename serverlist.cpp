#include "serverlist.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>

ServerList::ServerList(QObject *parent)
    : KWorker{parent}, m_timer(nullptr), m_manager(nullptr)
{}

void ServerList::fetch()
{
    QNetworkRequest req(QUrl("https://cdn2.arkdedicated.com/servers/asa/officialserverlist.json"));
    m_manager->get(req);
}

void ServerList::return_list(QNetworkReply *reply)
{
    QList<ServerInfo> list;
    if (!reply->error()) {
        auto json = QJsonDocument::fromJson(reply->readAll());
        if (json.isArray()) {
            for (auto v : json.array()) {
                auto obj = v.toObject();
                ServerInfo info;
                info.sessionName = obj.value("SessionName").toString();
                info.numPlayers  = obj.value("NumPlayers").toInt();
                list.append(info);
            }
        }
    }
    reply->deleteLater();
    emit servers_fetched(list);
}

void ServerList::handle_start_signal()
{
    emit log_message_Debug("ServerList::handle_start_signal()");
    if (!m_timer)
    {
        emit log_message_Debug("ServerList::handle_start_signal:\n创建新Timer");
        m_timer = new QTimer(this);
        m_timer_conn = connect(m_timer, &QTimer::timeout, this, &ServerList::fetch);
    }
    if (!m_manager)
    {
        emit log_message_Debug("ServerList::handle_start_signal:\n创建新Manager");
        m_manager = new QNetworkAccessManager(this);
        m_manager_conn = connect(m_manager, &QNetworkAccessManager::finished, this, &ServerList::return_list);
    }
    if (!m_timer->isActive())
    {
        m_timer->start(INTERVAL);
        emit log_message_User("启动服务器监控列表！");
    }
    else
    {
        emit log_message_User("服务器监控列表已在运行！");
    }
    fetch();
}

void ServerList::handle_stop_signal()
{
    if (m_timer->isActive())
        m_timer->stop();
    if (m_timer)
    {
        if (disconnect(m_timer_conn))
        {
            emit log_message_Debug("ServerList::handle_stop_signal:\nTimer断开连接成功");
        }
        else
        {
            emit log_message_Debug("ServerList::handle_stop_signal:\nTimer断开连接失败");
        }
        m_timer->deleteLater();
        m_timer = nullptr;
    }
    if (m_manager)
    {
        if (disconnect(m_manager_conn))
        {
            emit log_message_Debug("ServerList::handle_stop_signal:\nNetworkAccessManager断开连接成功");
        }
        else
        {
            emit log_message_Debug("ServerList::handle_stop_signal:\nNetworkAccessManager断开连接失败");
        }
        m_manager->deleteLater();
        m_manager = nullptr;
    }
}


