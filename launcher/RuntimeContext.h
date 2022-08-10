#pragma once

#include <QSet>
#include <QString>
#include "settings/SettingsObject.h"

struct RuntimeContext {
    QString javaArchitecture;
    QString javaRealArchitecture;
    QString javaPath;
    QString system;

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
        system = currentSystem();
    }

    QString getClassifier() const {
        return system + "-" + mappedJavaRealArchitecture();
    }

    // "Legacy" refers to the fact that Mojang assumed that these are the only two architectures
    bool isLegacyArch() const {
        QSet<QString> legacyArchitectures{"amd64", "x86_64", "i386", "i686", "x86"};
        return legacyArchitectures.contains(mappedJavaRealArchitecture());
    }

    bool classifierMatches(QString target) const {
        // try to match precise classifier "[os]-[arch]"
        bool x = target == getClassifier();
        // try to match imprecise classifier on legacy architectures "[os]"
        if (!x && isLegacyArch())
            x = target == system;

        return x;
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
