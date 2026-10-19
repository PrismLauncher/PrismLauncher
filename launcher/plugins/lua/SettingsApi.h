// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>

#include <cstddef>

struct lua_State;

namespace PrismApi {

constexpr int s_maxSettings = 256;
constexpr std::size_t s_maxSettingStringLength = 64 * 1024;

// Pushes the prism.settings(defaults) function, storing the values in the INI file at path (UTF-8).
//
// It returns a proxy: reading a key gives its stored value (or the default), assigning checks the type against the
// default and stores it, assigning nil restores the default. The file is written once the current call into Lua
// returns, and when the state is closed. It can only be called once per state.
void pushSettingsFunction(lua_State* L, const QByteArray& path);

}  // namespace PrismApi
