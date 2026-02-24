#include "AuthSession.h"
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStringList>
#include <utility>

QString AuthSession::serializeUserProperties()
{
    QJsonObject userAttrs;
    /*
    for (auto key : u.properties.keys())
    {
        auto array = QJsonArray::fromStringList(u.properties.values(key));
        userAttrs.insert(key, array);
    }
    */
    QJsonDocument value(userAttrs);
    return value.toJson(QJsonDocument::Compact);
}

bool AuthSession::MakeOffline(QString offline_playername)
{
    session = "-";
    access_token = "0";
    player_name = std::move(offline_playername);
    return true;
}

void AuthSession::MakeDemo(QString name, QString u)
{
    uuid = std::move(u);
    session = "-";
    access_token = "0";
    player_name = std::move(name);
    launchMode = LaunchMode::Demo;
};
