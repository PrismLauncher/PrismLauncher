// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

struct lua_State;

namespace PrismApi {

// Adds prism.ui.window to the table at the top of the stack (prism.ui). Windows hold widgets added with :add(kind,
// opts); kinds are label, button, input, text, code, list, row and column. Needs allowed (the ui permission). The
// windows of a state are closed when it is destroyed.
void addWindowFunction(lua_State* L, bool allowed);

}  // namespace PrismApi
