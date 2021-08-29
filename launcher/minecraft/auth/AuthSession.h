#pragma once

#include <QString>
#include <QMultiMap>
#include <memory>

class MinecraftAccount;

struct AuthSession
{
    bool MakeOffline(QString offline_playername);

    QString serializeUserProperties();

    enum Status
    {
        Undetermined,
        RequiresOAuth,
        RequiresPassword,
        PlayableOffline,
        PlayableOnline,
        GoneOrMigrated
    } status = Undetermined;

    // client token
    QString client_token;
    // account user name
    QString username;
    // combined session ID
    QString session;
    // volatile auth token
    QString access_token;
    // profile name
    QString player_name;
    // profile ID
    QString uuid;
    // 'legacy' or 'mojang', depending on account type
    QString user_type;
    // Did the auth server reply?
    bool auth_server_online = false;
    // Did the user request online mode?
    bool wants_online = true;
    std::shared_ptr<MinecraftAccount> m_accountPtr;
};

typedef std::shared_ptr<AuthSession> AuthSessionPtr;
