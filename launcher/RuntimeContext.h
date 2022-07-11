#pragma once

#include <QString>
#include "settings/SettingsObject.h"

struct RuntimeContext {
    QString javaArchitecture;
    QString javaRealArchitecture;
    QString javaPath;

    QString mappedJavaRealArchitecture() const {
        if (javaRealArchitecture == "aarch64") {
            return "arm64";
        }
        return javaRealArchitecture;
    }

    void updateFromInstanceSettings(SettingsObjectPtr instanceSettings) {
        javaArchitecture = instanceSettings->get("JavaArchitecture").toString();
        javaRealArchitecture = instanceSettings->get("JavaRealArchitecture").toString();
        javaPath = instanceSettings->get("JavaPath").toString();
    }

    static QString currentSystem() {
#if defined(Q_OS_LINUX)
        return "linux";
#elif defined(Q_OS_MACOS)
        return "osx";
#elif defined(Q_OS_WINDOWS)
        return "windows";
#elif defined(Q_OS_FREEBSD)
        return "freebsd";
#elif defined(Q_OS_OPENBSD)
        return "openbsd";
#else
        return "unknown";
#endif
    }
};
