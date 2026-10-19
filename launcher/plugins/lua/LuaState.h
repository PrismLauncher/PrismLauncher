// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>
#include <optional>

struct lua_State;

// Owns a sandboxed lua_State: only base (without dofile/loadfile, and load restricted to text chunks), coroutine,
// math, string, table and utf8 are available.
// Allocations beyond the memory limit fail, which Lua reports as a "not enough memory" error.
class LuaState {
   public:
    static constexpr std::size_t s_defaultMemoryLimit = 32 * 1024 * 1024;

    explicit LuaState(std::size_t memoryLimit = s_defaultMemoryLimit);
    ~LuaState();

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    bool isValid() const { return m_state != nullptr; }
    lua_State* get() const { return m_state; }

    std::size_t memoryUsed() const { return m_memory.used; }
    std::size_t memoryLimit() const { return m_memory.limit; }

    // Runs a text chunk. Returns the error message on failure.
    std::optional<QString> run(const QByteArray& code, const QString& chunkName);

   private:
    struct Memory {
        std::size_t used = 0;
        std::size_t limit = 0;
    };

    static void* allocate(void* ud, void* ptr, std::size_t osize, std::size_t nsize);

    // Must outlive m_state: the allocator keeps a pointer to it.
    Memory m_memory;
    lua_State* m_state = nullptr;
};
