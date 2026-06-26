// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 */

#include "BabricCreationTask.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileInfo>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "settings/INISettingsObject.h"

// ---------------------------------------------------------------------------
// Babric component stack (mirrors babric/prism-instance on GitHub):
//
//   org.lwjgl                  2.9.4+legacyfabric.17  [local patch]
//   net.minecraft              b1.7.3                 [local patch]
//   babric                     b1.7.3                 [local patch]
//   net.fabricmc.intermediary  b1.7.3                 [local patch]
//   net.fabricmc.fabric-loader 0.18.4                 [Prism meta server]
//
// The four local patches are embedded directly as C++ raw string literals
// so they are always compiled into the binary without any QRC dependency.
// To update, replace the JSON text below and rebuild.
// ---------------------------------------------------------------------------

static constexpr const char* FABRIC_LOADER_VERSION = "0.18.4";

static const char* PATCH_ORG_LWJGL = R"({
    "formatVersion": 1,
    "libraries": [
        {
            "downloads": {
                "classifiers": {
                    "natives-linux": {
                        "sha1": "7ff832a6eb9ab6a767f1ade2b548092d0fa64795",
                        "size": 10362,
                        "url": "https://libraries.minecraft.net/net/java/jinput/jinput-platform/2.0.5/jinput-platform-2.0.5-natives-linux.jar"
                    },
                    "natives-linux-arm32": {
                        "sha1": "f3c455b71c5146acb5f8a9513247fc06db182fd5",
                        "size": 4521,
                        "url": "https://github.com/theofficialgman/lwjgl3-binaries-arm32/raw/lwjgl-2.9.4/jinput-platform-2.0.5-natives-linux.jar"
                    },
                    "natives-linux-arm64": {
                        "sha1": "42b388ccb7c63cec4e9f24f4dddef33325f8b212",
                        "size": 10932,
                        "url": "https://github.com/theofficialgman/lwjgl3-binaries-arm64/raw/lwjgl-2.9.4/jinput-platform-2.0.5-natives-linux.jar"
                    },
                    "natives-osx": {
                        "sha1": "53f9c919f34d2ca9de8c51fc4e1e8282029a9232",
                        "size": 12186,
                        "url": "https://libraries.minecraft.net/net/java/jinput/jinput-platform/2.0.5/jinput-platform-2.0.5-natives-osx.jar"
                    },
                    "natives-osx-arm64": {
                        "sha1": "5189eb40db3087fb11ca063b68fa4f4c20b199dd",
                        "size": 10031,
                        "url": "https://github.com/r58Playz/jinput-m1/raw/main/plugins/OSX/bin/jinput-platform-2.0.5.jar"
                    },
                    "natives-windows": {
                        "sha1": "385ee093e01f587f30ee1c8a2ee7d408fd732e16",
                        "size": 155179,
                        "url": "https://libraries.minecraft.net/net/java/jinput/jinput-platform/2.0.5/jinput-platform-2.0.5-natives-windows.jar"
                    }
                }
            },
            "extract": { "exclude": ["META-INF/"] },
            "name": "net.java.jinput:jinput-platform:2.0.5",
            "natives": {
                "linux": "natives-linux",
                "linux-arm32": "natives-linux-arm32",
                "linux-arm64": "natives-linux-arm64",
                "osx": "natives-osx",
                "osx-arm64": "natives-osx-arm64",
                "windows": "natives-windows"
            }
        },
        {
            "downloads": {
                "artifact": {
                    "sha1": "39c7796b469a600f72380316f6b1f11db6c2c7c4",
                    "size": 208338,
                    "url": "https://libraries.minecraft.net/net/java/jinput/jinput/2.0.5/jinput-2.0.5.jar"
                }
            },
            "name": "net.java.jinput:jinput:2.0.5"
        },
        {
            "downloads": {
                "artifact": {
                    "sha1": "e12fe1fda814bd348c1579329c86943d2cd3c6a6",
                    "size": 7508,
                    "url": "https://libraries.minecraft.net/net/java/jutils/jutils/1.0.0/jutils-1.0.0.jar"
                }
            },
            "name": "net.java.jutils:jutils:1.0.0"
        },
        {
            "extract": { "exclude": ["META-INF/"] },
            "name": "org.lwjgl.lwjgl:lwjgl-platform:2.9.4+legacyfabric.17",
            "url": "https://maven.legacyfabric.net/",
            "natives": {
                "linux": "natives-linux",
                "linux-arm32": "natives-linux",
                "linux-arm64": "natives-linux",
                "osx": "natives-osx",
                "osx-arm64": "natives-osx",
                "windows": "natives-windows"
            }
        },
        {
            "name": "org.lwjgl.lwjgl:lwjgl:2.9.4+legacyfabric.17",
            "url": "https://maven.legacyfabric.net/"
        },
        {
            "name": "org.lwjgl.lwjgl:lwjgl_util:2.9.4+legacyfabric.17",
            "url": "https://maven.legacyfabric.net/"
        }
    ],
    "name": "LWJGL 2",
    "type": "release",
    "uid": "org.lwjgl",
    "version": "2.9.4+legacyfabric.17",
    "volatile": true
})";

static const char* PATCH_NET_MINECRAFT = R"({
    "+traits": ["legacyLaunch", "legacyServices", "texturepacks"],
    "assetIndex": {
        "id": "pre-1.6",
        "sha1": "3d8e55480977e32acd9844e545177e69a52f594b",
        "size": 74091,
        "totalSize": 49505710,
        "url": "https://piston-meta.mojang.com/v1/packages/3d8e55480977e32acd9844e545177e69a52f594b/pre-1.6.json"
    },
    "compatibleJavaMajors": [8, 17, 21, 25],
    "compatibleJavaName": "java-runtime-delta",
    "formatVersion": 1,
    "mainJar": {
        "downloads": {
            "artifact": {
                "sha1": "43db9b498cb67058d2e12d394e6507722e71bb45",
                "size": 1465375,
                "url": "https://launcher.mojang.com/v1/objects/43db9b498cb67058d2e12d394e6507722e71bb45/client.jar"
            }
        },
        "name": "com.mojang:minecraft:b1.7.3:client"
    },
    "minecraftArguments": "${auth_player_name} ${auth_session} --gameDir ${game_directory} --assetsDir ${game_assets}",
    "name": "Minecraft",
    "releaseTime": "2011-07-08T00:00:00+02:00",
    "requires": [{"suggests": "2.9.4+legacyfabric.17", "uid": "org.lwjgl"}],
    "type": "old_beta",
    "uid": "net.minecraft",
    "version": "b1.7.3"
})";

static const char* PATCH_BABRIC = R"({
    "formatVersion": 1,
    "name": "Babric",
    "uid": "babric",
    "version": "b1.7.3",
    "minecraftArguments": "--username ${auth_player_name} --session ${auth_session} --gameDir ${game_directory} --assetsDir ${game_assets}",
    "+traits": ["noapplet"],
    "libraries": [
        {"name": "babric:log4j-config:1.0.0",                         "url": "https://maven.glass-launcher.net/babric/"},
        {"name": "net.minecrell:terminalconsoleappender:1.2.0",        "url": "https://repo1.maven.org/maven2/"},
        {"name": "org.slf4j:slf4j-api:1.8.0-beta4"},
        {"name": "org.apache.logging.log4j:log4j-slf4j18-impl:2.16.0"},
        {"name": "org.apache.logging.log4j:log4j-api:2.16.0"},
        {"name": "org.apache.logging.log4j:log4j-core:2.16.0"},
        {"name": "com.google.code.gson:gson:2.8.9"},
        {"name": "com.google.guava:guava:31.0.1-jre"},
        {"name": "com.google.guava:failureaccess:1.0.1"},
        {"name": "org.apache.commons:commons-lang3:3.12.0"},
        {"name": "commons-io:commons-io:2.11.0"},
        {"name": "commons-codec:commons-codec:1.15"}
    ]
})";

static const char* PATCH_INTERMEDIARY = R"({
    "formatVersion": 1,
    "libraries": [
        {"name": "babric:intermediary-upstream:b1.7.3", "url": "https://maven.glass-launcher.net/babric"}
    ],
    "name": "Intermediary Mappings",
    "releaseTime": "2024-10-18T18:57:03.000Z",
    "requires": [{"equals": "b1.7.3", "uid": "net.minecraft"}],
    "type": "release",
    "uid": "net.fabricmc.intermediary",
    "version": "b1.7.3",
    "volatile": true
})";

// Ordered: org.lwjgl before net.minecraft so LWJGL 2 is on the classpath first.
static const QList<QPair<QString, const char*>> LOCAL_PATCHES = {
    { "org.lwjgl",                 PATCH_ORG_LWJGL    },
    { "net.minecraft",             PATCH_NET_MINECRAFT },
    { "babric",                    PATCH_BABRIC        },
    { "net.fabricmc.intermediary", PATCH_INTERMEDIARY  },
};

// ---------------------------------------------------------------------------

static bool writeFile(const QString& path, const QByteArray& data)
{
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile f(path);
    if (!f.open(QFile::WriteOnly | QFile::Truncate)) {
        qCritical() << "[Babric] Cannot write" << path << ":" << f.errorString();
        return false;
    }
    const qint64 written = f.write(data);
    f.close();
    if (written != data.size()) {
        qCritical() << "[Babric] Short write to" << path;
        return false;
    }
    return true;
}


bool BabricCreationTask::installPatches(PackProfile* profile)
{
    qDebug() << "[Babric] installPatches() start";

    // Step 1: Write all four patch files from the embedded string literals.
    QMap<QString, QByteArray> written;
    for (const auto& [uid, json] : LOCAL_PATCHES) {
        const QByteArray data(json);
        const QString target = profile->patchFilePathForUid(uid);
        qDebug() << "[Babric] Writing" << uid << "to" << target;
        if (!writeFile(target, data)) {
            qCritical() << "[Babric] Step 1 FAILED for" << uid;
            return false;
        }
        written[uid] = data;
    }

    // Step 2: Register component versions.
    //
    // setComponentVersion("net.minecraft", ..., important=true) calls
    // Component::revert() internally, which can delete our net.minecraft.json.
    // We re-write it immediately afterwards to be safe.
    profile->setComponentVersion("org.lwjgl", "2.9.4+legacyfabric.17");

    profile->setComponentVersion("net.minecraft", "b1.7.3", /*important=*/true);
    writeFile(profile->patchFilePathForUid("net.minecraft"), written["net.minecraft"]);

    profile->setComponentVersion("babric", "b1.7.3");
    profile->setComponentVersion("net.fabricmc.intermediary", "b1.7.3");

    // Fabric Loader is fetched from the Prism meta server (no local patch).
    profile->setComponentVersion("net.fabricmc.fabric-loader",
                                 QLatin1String(FABRIC_LOADER_VERSION));

    // Step 3: Re-write ALL patch files a second time to survive any
    // Component::revert() calls that setComponentVersion() may have fired above.
    // revert() deletes local patch files for UIDs that exist on the Prism meta
    // server (net.minecraft and possibly org.lwjgl).
    for (const auto& [uid, data] : written.asKeyValueRange()) {
        if (!writeFile(profile->patchFilePathForUid(uid), data))
            qWarning() << "[Babric] Failed to re-write patch for" << uid;
    }

    // Step 4: Save immediately so the profile is consistent on disk.
    profile->saveNow();

    qDebug() << "[Babric] installPatches() done";
    return true;
}


void BabricCreationTask::removePatches(PackProfile* profile)
{
    qDebug() << "[Babric] removePatches() start";

    // Remove the four loader components.
    // Re-scan by UID each iteration because each removal shifts row indices.
    static const QList<QString> loaderUIDs = {
        "net.fabricmc.fabric-loader",
        "babric",
        "net.fabricmc.intermediary",
        "org.lwjgl",
    };
    for (const QString& uid : loaderUIDs) {
        for (int i = 0; i < profile->rowCount(); i++) {
            auto comp = profile->getComponent(i);
            if (comp && comp->getID() == uid) {
                profile->remove(i);
                break;
            }
        }
        // Also delete the local patch file explicitly.
        QFile::remove(profile->patchFilePathForUid(uid));
    }

    // Revert net.minecraft to meta-backed state so the "Custom" label disappears.
    // Component::revert() deletes patches/net.minecraft.json and clears m_file.
    auto netMinecraft = profile->getComponent("net.minecraft");
    if (netMinecraft)
        netMinecraft->revert();
    QFile::remove(profile->patchFilePathForUid("net.minecraft"));

    profile->saveNow();
    qDebug() << "[Babric] removePatches() done";
}


std::unique_ptr<MinecraftInstance> BabricCreationTask::createInstance()
{
    setStatus(tr("Creating Babric instance for Minecraft b1.7.3"));

    auto inst = std::make_unique<MinecraftInstance>(
        m_globalSettings,
        std::make_unique<INISettingsObject>(FS::PathCombine(m_stagingPath, "instance.cfg")),
        m_stagingPath);

    SettingsObject::Lock lock(inst->settings());

    auto* profile = inst->getPackProfile();
    profile->buildingFromScratch();

    if (!installPatches(profile)) {
        setError(tr("Failed to write Babric patch files."));
        return nullptr;
    }

    inst->setName(name());
    inst->setIconKey(m_instIcon);
    return inst;
}
