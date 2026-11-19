#pragma once

#include <QList>

class MinecraftInstance;

namespace discord {
class Presence;
};

class LauncherRichPresence {
public:
    LauncherRichPresence();

    void enable(bool enabled);

    void sync(MinecraftInstance* instance, bool running);

private:
    discord::Presence makePresence();

    bool m_running = false;
    struct RunningInstance {
        MinecraftInstance* instance;
        qint64 startTime;
    };
    QList<RunningInstance> m_runningInstances;
};
