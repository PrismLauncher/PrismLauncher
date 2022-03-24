#include "VerifyJavaInstall.h"

#include "java/JavaVersion.h"
#include "minecraft/PackProfile.h"
#include "minecraft/MinecraftInstance.h"

void VerifyJavaInstall::executeTask() {
    auto instance = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());
    auto packProfile = instance->getPackProfile();
    auto settings = instance->settings();
    auto storedVersion = settings->get("JavaVersion").toString();
    auto ignoreCompatibility = settings->get("IgnoreJavaCompatibility").toBool();

    auto compatibleMajors = packProfile->getProfile()->getCompatibleJavaMajors();

    JavaVersion javaVersion(storedVersion);

    if (compatibleMajors.isEmpty() || compatibleMajors.contains(javaVersion.major()))
    {
        emitSucceeded();
        return;
    }


    if (ignoreCompatibility)
    {
        emit logLine(tr("Java major version is incompatible. Things might break."), MessageLevel::Warning);
        emitSucceeded();
        return;
    }

    emit logLine(tr("Instance not compatible with Java major version %1.\n"
                    "Switch the Java version of this instance to one of the following:").arg(javaVersion.major()),
                 MessageLevel::Error);
    for (auto major: compatibleMajors)
    {
        emit logLine(tr("Java %1").arg(major), MessageLevel::Error);
    }
    emitFailed(QString("Incompatible Java major version"));
}
