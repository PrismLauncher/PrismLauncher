// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>

struct lua_State;

namespace PrismApi {

class HttpClient;

// Sets up the registry used by prism.async and prism.http; pluginId (UTF-8) names the plugin in error messages
void setUpTasks(lua_State* L, const QByteArray& pluginId);

// Pushes prism.async(fn, ...): runs fn in a coroutine ("task") that may wait for slow operations, such as prism.http,
// without blocking the launcher. Errors in a task are logged.
void pushAsyncFunction(lua_State* L);

// Pushes the prism.http table: get(url, [opts]) and post(url, body, [opts]), which wait inside a task and return
// { status, ok, body, headers, error }. opts.headers adds request headers, opts.timeout sets the timeout in seconds.
// Needs allowed (the net permission) and a client; without a client requests fail.
void pushHttpTable(lua_State* L, bool allowed, HttpClient* client);

}  // namespace PrismApi
