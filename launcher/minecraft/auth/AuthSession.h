#pragma once

#include <QString>
#include <memory>

#include "LaunchMode.h"

class MinecraftAccount;

struct AuthSession {
    QString serializeUserProperties();

    // combined session ID
    QString session;
    // volatile auth token
    QString access_token;
    // profile name
    QString player_name;
    // profile ID
    QString uuid;
    // Minecraft-compatible user type for modern launch arguments.
    QString user_type;
    // Apply the Ely.by Authlib-compatible session patch before launching the game.
    bool wantsElyPatch = false;
    // the actual launch mode for this session
    LaunchMode launchMode;
};

using AuthSessionPtr = std::shared_ptr<AuthSession>;
