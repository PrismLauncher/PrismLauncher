#include "VerifyJavaInstall.h"

#include <launch/LaunchTask.h>
#include <minecraft/MinecraftInstance.h>
#include <minecraft/PackProfile.h>
#include <minecraft/VersionFilterData.h>

void VerifyJavaInstall::executeTask() {
    auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());

    auto javaVersion = m_inst->getJavaVersion();
    auto minecraftComponent = m_inst->getPackProfile()->getComponent("net.minecraft");

    // Java 17 requirement
    if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java17BeginsDate) {
        if (javaVersion.major() < 17) {
            emit logLine("Minecraft 1.18 Pre Release 2 and above require the use of Java 17",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 1.18 Pre Release 2 and above require the use of Java 17"));
            return;
        }
    }
    // Java 16 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java16BeginsDate) {
        if (javaVersion.major() < 16) {
            emit logLine("Minecraft 21w19a and above require the use of Java 16",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 21w19a and above require the use of Java 16"));
            return;
        }
    }
    // Java 8 requirement
    else if (minecraftComponent->getReleaseDateTime() >= g_VersionFilterData.java8BeginsDate) {
        if (javaVersion.major() < 8) {
            emit logLine("Minecraft 17w13a and above require the use of Java 8",
                         MessageLevel::Fatal);
            emitFailed(tr("Minecraft 17w13a and above require the use of Java 8"));
            return;
        }
    }

    emitSucceeded();
}
