// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QByteArray>
#include <QString>

#include <cstddef>
#include <cstdint>
#include <optional>

struct lua_State;
struct lua_Debug;

// Owns a sandboxed lua_State: only base (without dofile/loadfile, load restricted to text chunks and setmetatable
// rejecting __gc), coroutine, math, string, table and utf8 are available.
// Allocations beyond the memory limit fail, which Lua reports as a "not enough memory" error.
// Each call into Lua may run up to the instruction limit; past it, Lua raises "instruction limit exceeded".
class LuaState {
   public:
    static constexpr std::size_t s_defaultMemoryLimit = 32 * 1024 * 1024;
    static constexpr std::uint64_t s_defaultInstructionLimit = 10'000'000;

    explicit LuaState(std::size_t memoryLimit = s_defaultMemoryLimit);
    ~LuaState();

    LuaState(const LuaState&) = delete;
    LuaState& operator=(const LuaState&) = delete;

    bool isValid() const { return m_state != nullptr; }
    lua_State* get() const { return m_state; }

    std::size_t memoryUsed() const { return m_memory.used; }
    std::size_t memoryLimit() const { return m_memory.limit; }

    std::uint64_t instructionLimit() const { return m_instructionLimit; }
    void setInstructionLimit(std::uint64_t limit) { m_instructionLimit = limit; }

    // Runs a text chunk. Returns the error message on failure.
    std::optional<QString> run(const QByteArray& code, const QString& chunkName);
    // Calls the function below the nargs arguments at the top of the stack with a fresh instruction budget, and pops
    // them, discarding any results. Returns the error message, with a traceback, on failure.
    std::optional<QString> call(int nargs);

   private:
    struct Memory {
        std::size_t used = 0;
        std::size_t limit = 0;
    };

    static void* allocate(void* ud, void* ptr, std::size_t osize, std::size_t nsize);
    static void instructionHook(lua_State* L, lua_Debug* ar);

    // Starts a new instruction budget for the next call into Lua.
    void resetInstructionBudget();

    // Must outlive m_state: the allocator keeps a pointer to it.
    Memory m_memory;
    std::uint64_t m_instructionLimit = s_defaultInstructionLimit;
    std::uint64_t m_instructionsUsed = 0;
    lua_State* m_state = nullptr;
};
