// SPDX-License-Identifier: GPL-3.0-only AND Apache-2.0
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 TheKodeToad <TheKodeToad@proton.me>
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
 *  Copyright (C) 2026 Trial97 <alexandru.tripon97@gmail.com>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#pragma once

#include <QDebug>
#include <QString>
#include <algorithm>
#include <concepts>
#include <expected>
#include <iterator>

template <typename T = void, typename E = QString>
using Result = std::expected<T, E>;

#define TRY(expected)                                \
    if (const auto _result = (expected); !_result) { \
        return std::unexpected{ _result.error() };   \
    }

#define RESULT_H_CONCAT_(x, y) x##y

#define RESULT_H_CONCAT(x, y) RESULT_H_CONCAT_(x, y)

#define TRY_INTO_VAR_ RESULT_H_CONCAT(_try_tmp_, __LINE__)

#define TRY_INTO(decl, expr)       \
    auto&& TRY_INTO_VAR_ = (expr); \
    TRY(TRY_INTO_VAR_)             \
    decl = TRY_INTO_VAR_.value();

namespace Try {

namespace detail {

template <class ContainerT>
concept mapContainer = requires(ContainerT a, const ContainerT::key_type& k) {
    requires std::ranges::range<ContainerT>;
    { a.find(k) } -> std::same_as<std::ranges::iterator_t<ContainerT>>;
};

}  // namespace detail

/// @breif try find a value in a mapping container by key
template <detail::mapContainer Container>
Result<typename Container::const_iterator::value_type> findByMapKey(const Container& container,
                                                                    const typename Container::key_type& key,
                                                                    const QString& msg)
{
    auto&& var = container.find(key);
    if (var == container.end()) {
        return std::unexpected{ msg.arg(key) };
    }
    return *var;
}

/// @breif try to find a value in a container by key projection (std::ranges::find)
template <std::ranges::range Container, class T, class Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<typename Container::const_iterator, Proj>, const T*>
Result<typename Container::const_iterator::value_type> find(const Container& container, const T& key, const QString& msg, Proj proj = {})
{
    auto&& var = std::ranges::find(container, key, proj);
    if (var == container.end()) {
        return std::unexpected{ msg.arg(key) };
    }
    return *var;
}

/// @breif try to find a value in a range by key projection (std::ranges::find)
template <std::input_iterator I, std::sentinel_for<I> S, class T, class Proj = std::identity>
    requires std::indirect_binary_predicate<std::ranges::equal_to, std::projected<I, Proj>, const T*>
Result<typename I::value_type> find(I first, S last, const T& key, const QString& msg, Proj proj = {})
{
    auto&& var = std::ranges::find(first, last, key, proj);
    if (var == last) {
        return std::unexpected{ msg.arg(key) };
    }
    return *var;
}

/// @breif try to find a value in a container by predicate
template <std::ranges::range Container,
          class Proj = std::identity,
          std::indirect_unary_predicate<std::projected<typename Container::const_iterator, Proj>> Pred>
Result<typename Container::const_iterator::value_type> findIf(const Container& container, Pred pred, const QString& msg, Proj proj = {})
{
    auto&& var = std::ranges::find_if(container, pred, proj);
    if (var == container.end()) {
        return std::unexpected{ msg };
    }
    return *var;
}

/// @breif try to find a value in a range by predicate
template <std::input_iterator I,
          std::sentinel_for<I> S,
          class Proj = std::identity,
          std::indirect_unary_predicate<std::projected<I, Proj>> Pred>
Result<typename I::value_type> findIf(I first, S last, Pred pred, const QString& msg, Proj proj = {})
{
    auto&& var = std::ranges::find_if(first, last, pred, proj);
    if (var == last) {
        return std::unexpected{ msg };
    }
    return *var;
}

}  // namespace Try
