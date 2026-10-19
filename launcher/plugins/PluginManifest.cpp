// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "PluginManifest.h"

#include <QFile>
#include <QRegularExpression>

#include <toml++/toml.h>

const QStringList PluginManifest::s_knownPermissions = { "fs.instance", "game.log", "instances.read", "launch.hooks",
                                                         "net",         "ui",       "unrestricted" };

namespace {

const QStringList s_knownKeys = { "id", "name", "version", "api", "entry", "permissions" };

// Used as a folder name, so it must be a safe path component
const QRegularExpression s_idPattern("^[a-z0-9_-]+(\\.[a-z0-9_-]+)*$");
constexpr qsizetype s_maxIdLength = 64;

// Each path segment may not start with a dot, which rules out "." and ".."
const QRegularExpression s_entryPattern("^[A-Za-z0-9_-][A-Za-z0-9_.-]*(/[A-Za-z0-9_-][A-Za-z0-9_.-]*)*\\.lua$");

Result<QString> readString(const toml::table& table, std::string_view key)
{
    const toml::node* node = table.get(key);
    if (!node) {
        return std::unexpected(QString("missing required field '%1'").arg(QString::fromUtf8(key)));
    }
    const auto* value = node->as_string();
    if (!value) {
        return std::unexpected(QString("field '%1' must be a string").arg(QString::fromUtf8(key)));
    }
    return QString::fromStdString(value->get());
}

Result<QStringList> readPermissions(const toml::table& table)
{
    const toml::node* node = table.get("permissions");
    if (!node) {
        return QStringList();
    }
    const auto* array = node->as_array();
    if (!array) {
        return std::unexpected(QString("field 'permissions' must be an array of strings"));
    }

    QStringList permissions;
    for (const toml::node& item : *array) {
        const auto* value = item.as_string();
        if (!value) {
            return std::unexpected(QString("field 'permissions' must be an array of strings"));
        }
        const QString permission = QString::fromStdString(value->get());
        if (!PluginManifest::s_knownPermissions.contains(permission)) {
            return std::unexpected(QString("unknown permission '%1'").arg(permission));
        }
        permissions.append(permission);
    }
    permissions.sort();
    permissions.removeDuplicates();
    return permissions;
}

}  // namespace

Result<PluginManifest> PluginManifest::parse(const QByteArray& contents)
{
    toml::table table;
#if TOML_EXCEPTIONS
    // Catch std::exception instead of toml::parse_error to work around
    // the latter not being caught here when compiled with libc++.
    // See https://github.com/PrismLauncher/PrismLauncher/issues/6047.
    try {
        table = toml::parse(contents.toStdString());
    } catch (const std::exception& err) {
        // May fail for the same reason, then there is no line number
        if (const auto* parseError = dynamic_cast<const toml::parse_error*>(&err)) {
            return std::unexpected(QString("invalid TOML at line %1: %2").arg(parseError->source().begin.line).arg(err.what()));
        }
        return std::unexpected(QString("invalid TOML: %1").arg(err.what()));
    }
#else
    toml::parse_result result = toml::parse(contents.toStdString());
    if (!result) {
        const auto& error = result.error();
        return std::unexpected(QString("invalid TOML at line %1: %2")
                                   .arg(error.source().begin.line)
                                   .arg(QString::fromStdString(std::string(error.description()))));
    }
    table = std::move(result).table();
#endif

    // Reject unknown keys so that typos (e.g. "permision") are not silently ignored
    for (const auto& [key, node] : table) {
        const QString name = QString::fromUtf8(key.str());
        if (!s_knownKeys.contains(name)) {
            return std::unexpected(QString("unknown field '%1'").arg(name));
        }
    }

    PluginManifest manifest;
    TRY_INTO(manifest.id, readString(table, "id"))
    TRY_INTO(manifest.name, readString(table, "name"))
    TRY_INTO(manifest.version, readString(table, "version"))
    TRY_INTO(manifest.permissions, readPermissions(table))

    if (manifest.id.size() > s_maxIdLength || !s_idPattern.match(manifest.id).hasMatch()) {
        return std::unexpected(QString("invalid id '%1': use up to %2 lowercase letters, digits, '_' or '-', separated by dots")
                                   .arg(manifest.id)
                                   .arg(s_maxIdLength));
    }
    if (manifest.name.trimmed().isEmpty()) {
        return std::unexpected(QString("field 'name' must not be empty"));
    }
    if (manifest.version.trimmed().isEmpty()) {
        return std::unexpected(QString("field 'version' must not be empty"));
    }

    const toml::node* api = table.get("api");
    if (!api) {
        return std::unexpected(QString("missing required field 'api'"));
    }
    if (!api->is_integer()) {
        return std::unexpected(QString("field 'api' must be an integer"));
    }
    if (api->as_integer()->get() != s_apiVersion) {
        return std::unexpected(
            QString("unsupported plugin API %1, this launcher supports API %2").arg(api->as_integer()->get()).arg(s_apiVersion));
    }
    manifest.api = s_apiVersion;

    if (table.contains("entry")) {
        TRY_INTO(const QString entry, readString(table, "entry"))
        if (!s_entryPattern.match(entry).hasMatch()) {
            return std::unexpected(QString("invalid entry '%1': must be a relative path to a .lua file inside the plugin").arg(entry));
        }
        manifest.entry = entry;
    }

    return manifest;
}

Result<PluginManifest> PluginManifest::load(const QString& path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return std::unexpected(QString("could not open %1: %2").arg(path, file.errorString()));
    }
    return parse(file.readAll());
}
