// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QString>

#include <optional>

struct lua_State;

// Owns a sandboxed lua_State: only base (without dofile/loadfile, and load restricted to text chunks), coroutine,
// math, string, table and utf8 are available.
class LuaState {
   public:
    LuaState();
    ~LuaState();

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    bool isValid() const { return m_state != nullptr; }
    lua_State* get() const { return m_state; }

    // Runs a text chunk. Returns the error message on failure.
    std::optional<QString> run(const QByteArray& code, const QString& chunkName);

   private:
    lua_State* m_state = nullptr;
};
