// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>

struct lua_State;

namespace PrismApi {

class InstanceSource;

constexpr qint64 s_maxFileReadSize = 16 * 1024 * 1024;

// Pushes the prism.fs table. Folders are handles that only accept relative paths and never reach outside their root,
// not even through symlinks:
// - prism.fs.data(): the plugin's own data folder (dataDir, UTF-8), created on demand. Always available.
// - prism.fs.instance(id): the game folder of an instance. Needs instanceAccess (the fs.instance permission).
// Handles have read, write, append, list, exists, isDir, mkdir and remove; prism.fs.copy and prism.fs.move work across
// handles. Failures raise errors.
void pushFsTable(lua_State* L, const QByteArray& dataDir, bool instanceAccess, InstanceSource* instances);

}  // namespace PrismApi
