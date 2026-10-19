// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QString>
#include <QStringList>

#include "Result.h"

// Contents of a plugin's plugin.toml
struct PluginManifest {
    static constexpr int s_apiVersion = 1;
    static const QStringList s_knownPermissions;

    QString id;
    QString name;
    QString version;
    int api = 0;
    // Relative to the plugin folder
    QString entry = "main.lua";
    // Sorted, without duplicates
    QStringList permissions;

    bool hasPermission(const QString& permission) const { return permissions.contains(permission); }

    static Result<PluginManifest> parse(const QByteArray& contents);
    static Result<PluginManifest> load(const QString& path);
};
