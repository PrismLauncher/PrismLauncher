// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <compare>
#include <type_traits>

template <typename Derived, typename EnumV>
struct EnumWrapper {
    using Enum = EnumV;
    using EnumBase = std::underlying_type_t<Enum>;

    constexpr EnumWrapper(Enum e = Derived::invalid()) : m_value(e) {}

    constexpr bool isValid() const { return m_value != Derived::invalid(); }

    bool operator==(const EnumWrapper&) const = default;

    std::strong_ordering operator<=>(const EnumWrapper& other) const { return m_value <=> other.m_value; };

    std::strong_ordering operator<=>(Enum other) const { return m_value <=> other; }

    QString toString() const
    {
        for (auto&& [e, name] : Derived::mapping()) {
            if (e == m_value)
                return QString(name);
        }
        Q_UNREACHABLE();
        return {};
    }

    static constexpr Derived fromString(const QString& str)
    {
        for (auto&& [e, name] : Derived::mapping()) {
            if (str == name)
                return Derived(e);
        }
        return Derived(Derived::invalid());
    }

    explicit operator EnumBase() const { return static_cast<EnumBase>(m_value); }

    explicit operator Enum() const { return m_value; }

    Enum value() const { return m_value; }

   protected:
    Enum m_value;
};