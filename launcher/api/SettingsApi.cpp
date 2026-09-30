// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "SettingsApi.h"

#include <QJsonArray>

#include "Application.h"
#include "settings/Setting.h"
#include "settings/SettingsObject.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "SensitiveChange.h"

namespace api {

namespace {

struct SettingSpec {
    const char* key;
    QMetaType::Type type;
    bool sensitive;
    qint64 min = 0;
    qint64 max = 1 << 24;
};

// Keep in sync with `LauncherSettings` in frontend/src/types/settings.ts.
const SettingSpec SettingSpecs[] = {
    { "InstanceDir", QMetaType::QString, false },
    { "DownloadsDir", QMetaType::QString, false },
    { "JavaPath", QMetaType::QString, true },
    { "JvmArgs", QMetaType::QString, true },
    { "MinMemAlloc", QMetaType::Int, false, 128, 1 << 22 },
    { "MaxMemAlloc", QMetaType::Int, false, 128, 1 << 22 },
    { "PermGen", QMetaType::Int, false, 32, 1 << 16 },
    { "AutomaticJavaSwitch", QMetaType::Bool, false },
    { "AutomaticJavaDownload", QMetaType::Bool, false },
    { "IgnoreJavaCompatibility", QMetaType::Bool, false },
    { "LaunchMaximized", QMetaType::Bool, false },
    { "MinecraftWinWidth", QMetaType::Int, false, 1, 1 << 16 },
    { "MinecraftWinHeight", QMetaType::Int, false, 1, 1 << 16 },
    { "ShowConsole", QMetaType::Bool, false },
    { "AutoCloseConsole", QMetaType::Bool, false },
    { "ShowConsoleOnError", QMetaType::Bool, false },
    { "ConsoleMaxLines", QMetaType::Int, false, 1000, 10000000 },
    { "CloseAfterLaunch", QMetaType::Bool, false },
    { "QuitAfterGameStop", QMetaType::Bool, false },
    { "ShowGameTime", QMetaType::Bool, false },
    { "RecordGameTime", QMetaType::Bool, false },
    { "PreLaunchCommand", QMetaType::QString, true },
    { "WrapperCommand", QMetaType::QString, true },
    { "PostExitCommand", QMetaType::QString, true },
    { "NumberOfConcurrentDownloads", QMetaType::Int, false, 1, 64 },
    { "NumberOfConcurrentTasks", QMetaType::Int, false, 1, 64 },
    { "RequestTimeout", QMetaType::Int, false, 5, 3600 },
};

const SettingSpec* findSpec(const QString& key)
{
    for (const auto& spec : SettingSpecs) {
        if (key == QLatin1String(spec.key)) {
            return &spec;
        }
    }
    return nullptr;
}

QJsonObject readAll()
{
    auto* settings = APPLICATION->settings();
    QJsonObject out;
    for (const auto& spec : SettingSpecs) {
        const auto value = settings->get(spec.key);
        switch (spec.type) {
            case QMetaType::Bool:
                out.insert(spec.key, value.toBool());
                break;
            case QMetaType::Int:
                out.insert(spec.key, value.toInt());
                break;
            default:
                out.insert(spec.key, value.toString());
                break;
        }
    }
    return out;
}

}  // namespace

SettingsApi::SettingsApi(ApiRouter* router, QObject* parent) : QObject(parent), m_router(router)
{
    m_changedTimer.setSingleShot(true);
    m_changedTimer.setInterval(100);
    connect(&m_changedTimer, &QTimer::timeout, this, [this] {
        QJsonArray keys;
        for (const auto& key : std::as_const(m_changedKeys)) {
            keys.append(key);
        }
        m_changedKeys.clear();
        m_router->emitEvent("settings.changed", QJsonObject{ { "keys", keys } });
    });
    auto onChanged = [this](const Setting& setting) {
        if (findSpec(setting.id())) {
            m_changedKeys.insert(setting.id());
            m_changedTimer.start();
        }
    };
    connect(APPLICATION->settings(), &SettingsObject::SettingChanged, this, [onChanged](const Setting& s, const QVariant&) { onChanged(s); });
    connect(APPLICATION->settings(), &SettingsObject::settingReset, this, onChanged);

    router->addSync("settings.get", [](const QJsonObject&) { return readAll(); });

    router->addSync("settings.set", [](const QJsonObject& p) {
        const auto values = params::requireObject(p, "values");
        auto* settings = APPLICATION->settings();

        QList<QPair<const SettingSpec*, QVariant>> updates;
        QList<QPair<QString, QString>> sensitive;
        for (auto it = values.begin(); it != values.end(); ++it) {
            const auto* spec = findSpec(it.key());
            if (!spec) {
                throw ApiError::invalidParams(tr("Unknown or read-only setting '%1'").arg(it.key()));
            }
            QVariant value;
            switch (spec->type) {
                case QMetaType::Bool:
                    value = params::requireBool(values, it.key());
                    break;
                case QMetaType::Int:
                    value = static_cast<int>(params::requireInt(values, it.key(), spec->min, spec->max));
                    break;
                default:
                    value = params::requireString(values, it.key(), 8192);
                    break;
            }
            if (spec->sensitive && settings->get(spec->key) != value) {
                sensitive.append({ QString::fromLatin1(spec->key), value.toString() });
            }
            updates.append({ spec, value });
        }
        if (updates.isEmpty()) {
            return readAll();
        }
        if (values.contains("MinMemAlloc") || values.contains("MaxMemAlloc")) {
            const auto minMem = values.contains("MinMemAlloc") ? values.value("MinMemAlloc").toInt() : settings->get("MinMemAlloc").toInt();
            const auto maxMem = values.contains("MaxMemAlloc") ? values.value("MaxMemAlloc").toInt() : settings->get("MaxMemAlloc").toInt();
            if (minMem > maxMem) {
                throw ApiError::invalidParams(tr("The minimum memory allocation must not exceed the maximum"));
            }
        }
        if (!sensitive.isEmpty() && !confirmSensitiveChange(tr("launcher settings"), sensitive)) {
            throw ApiError::permissionDenied(tr("The settings change was not confirmed"));
        }
        {
            SettingsObject::Lock lock(settings);
            for (const auto& [spec, value] : updates) {
                settings->set(spec->key, value);
            }
        }
        return readAll();
    });
}

}  // namespace api
