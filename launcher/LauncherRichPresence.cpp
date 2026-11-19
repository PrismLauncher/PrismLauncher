#include <QObject>
#include <discord-rpc.hpp>

#include "LauncherRichPresence.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

LauncherRichPresence::LauncherRichPresence()
{
    auto& rpc = discord::RPCManager::get();
    // FIXME: doing this stuff in the constructor feels dubious
    // either the library moves away from being a singleton, or perhaps we just make this class a singleton?
    rpc.setClientID(BuildConfig.DISCORD_CLIENT_ID.toStdString());
    rpc.onReady([](const discord::User& user) { qDebug() << "Discord RPC connected on" << user.username; });
    rpc.onErrored([](int errCode, std::string_view message) { qWarning() << "Discord RPC error:" << errCode << message; });
    rpc.onDisconnected([](int errCode, std::string_view message) {
        if (errCode == 0) {
            qDebug() << "Discord RPC disconnected";
            return;
        }
        qWarning() << "Discord RPC disconnected:" << errCode << message;
    });
}

void LauncherRichPresence::enable(bool enabled)
{
    if (m_running == enabled) {
        return;
    }

    auto& rpc = discord::RPCManager::get();
    if (enabled) {
        rpc.initialize();
    } else {
        rpc.shutdown();
    }
    rpc.setPresence(makePresence());
    rpc.refresh();
    m_running = enabled;
}

void LauncherRichPresence::sync(MinecraftInstance* instance, bool running)
{
    erase_if(m_runningInstances, [&](const RunningInstance& inst) { return inst.instance == instance; });
    if (running) {
        m_runningInstances.append({
            .instance = instance,
            .startTime = QDateTime::currentMSecsSinceEpoch(),
        });
    }
    auto& rpc = discord::RPCManager::get();
    rpc.setPresence(makePresence());
    rpc.refresh();
}

discord::Presence LauncherRichPresence::makePresence()
{
    if (m_runningInstances.empty()) {
        discord::Presence presence;
        presence.setStartTimestamp(QDateTime::currentMSecsSinceEpoch());
        presence.setDetails(QObject::tr("Not playing anything").toStdString());
        return presence;
    }
    discord::Presence presence;
    auto info = m_runningInstances.back();
    presence.setStartTimestamp(info.startTime);
    presence.setDetails(QObject::tr("Playing %1").arg(info.instance->name()).toStdString());
    presence.setLargeImageKey(info.instance->iconKey().toStdString());

    const auto gameVersion = info.instance->getPackProfile()->getComponentVersion("net.minecraft");
    presence.setState(QObject::tr("Minecraft %1").arg(gameVersion).toStdString());
    return presence;
}
