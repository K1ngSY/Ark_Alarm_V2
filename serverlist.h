#ifndef SERVERLIST_H
#define SERVERLIST_H

#include "kworker.h"
#include <QTimer>
#include <QNetworkAccessManager>
#include <QNetworkReply>

// 用于承载单个服务器信息
struct ServerInfo {
    QString sessionName;
    int numPlayers;
};
Q_DECLARE_METATYPE(QList<ServerInfo>)
const int INTERVAL = 10000;
class ServerList : public KWorker
{
    Q_OBJECT
public:
    explicit ServerList(QObject *parent = nullptr);

private:
    QTimer *m_timer;
    QNetworkAccessManager *m_manager;
    QMetaObject::Connection m_timer_conn;
    QMetaObject::Connection m_manager_conn;

private slots:
    void fetch();
    void return_list(QNetworkReply *reply);

    void handle_start_signal() override;
    void handle_stop_signal() override;

signals:
    void servers_fetched(const QList<ServerInfo>);
};

#endif // SERVERLIST_H
