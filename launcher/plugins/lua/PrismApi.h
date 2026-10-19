// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QString>

#include <optional>

class LuaState;
struct PluginManifest;

namespace PrismApi {

// Longer log messages are truncated
constexpr qsizetype s_maxLogMessageLength = 4096;

// Creates the global `prism` table for the plugin described by the manifest, whose folder is directory, and routes
// `print` to prism.log.info. Returns the error message on failure.
std::optional<QString> install(LuaState& state, const PluginManifest& manifest, const QString& directory);

}  // namespace PrismApi
