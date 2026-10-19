// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>

struct lua_State;

namespace PrismApi {

constexpr qint64 s_maxModuleSize = 1024 * 1024;

// Adds the helpers written in Lua (prism.json, prism.path, prism.text, prism.inspect) to the prism table at the top of
// the stack and, unless the state already has package, a require that loads text modules from pluginDir (UTF-8) only:
// require("a.b") loads a/b.lua or a/b/init.lua.
void installLibrary(lua_State* L, const QByteArray& pluginDir);

}  // namespace PrismApi
