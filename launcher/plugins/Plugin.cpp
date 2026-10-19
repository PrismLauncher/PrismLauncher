// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "Plugin.h"

#include <QDir>
#include <QFile>

#include "plugins/Logging.h"
#include "plugins/lua/LuaState.h"
#include "plugins/lua/PrismApi.h"

namespace {

Result<PluginManifest> readManifest(const QString& directory)
{
    TRY_INTO(PluginManifest manifest, PluginManifest::load(QDir(directory).filePath("plugin.toml")))

    // The id names the plugin's folder, so two plugins can never share it
    const QString folderName = QDir(directory).dirName();
    if (manifest.id != folderName) {
        return std::unexpected(QString("plugin id '%1' does not match its folder name '%2'").arg(manifest.id, folderName));
    }
    return manifest;
}

}  // namespace

Plugin::Plugin(const QString& directory) : m_directory(directory), m_manifest(readManifest(directory))
{
    if (!m_manifest) {
        fail(m_manifest.error());
    }
}

Plugin::~Plugin() = default;

QString Plugin::id() const
{
    return m_manifest ? m_manifest->id : QDir(m_directory).dirName();
}

bool Plugin::load()
{
    if (m_state == State::Loaded) {
        return true;
    }
    if (!m_manifest) {
        fail(m_manifest.error());
        return false;
    }
    m_error.clear();

    const QString& entry = m_manifest->entry;
    QFile file(QDir(m_directory).filePath(entry));
    if (!file.open(QIODevice::ReadOnly)) {
        fail(QString("could not open %1: %2").arg(entry, file.errorString()));
        return false;
    }
    // Read one byte past the limit instead of trusting size(), which is 0 for pipes and devices
    const QByteArray code = file.read(s_maxEntrySize + 1);
    if (code.size() > s_maxEntrySize) {
        fail(QString("%1 is larger than %2 bytes").arg(entry).arg(s_maxEntrySize));
        return false;
    }

    auto lua = std::make_unique<LuaState>();
    if (!lua->isValid()) {
        fail("could not create the Lua state");
        return false;
    }
    if (auto error = PrismApi::install(*lua, *m_manifest, m_directory)) {
        fail(*error);
        return false;
    }
    if (auto error = lua->run(code, entry)) {
        fail(*error);
        return false;
    }

    m_lua = std::move(lua);
    m_state = State::Loaded;
    m_loadCount++;
    return true;
}

void Plugin::unload()
{
    m_lua.reset();
    m_error.clear();
    m_state = State::Disabled;
}

void Plugin::emitEvent(const char* event, const PrismApi::EventFields& fields)
{
    if (m_state != State::Loaded) {
        return;
    }
    if (auto error = PrismApi::emitEvent(*m_lua, event, fields)) {
        qCWarning(pluginLogC).noquote() << QString("[%1] error handling %2: %3").arg(id(), event, *error);
    }
}

void Plugin::fail(const QString& error)
{
    m_lua.reset();
    m_error = error;
    m_state = State::Error;
}
