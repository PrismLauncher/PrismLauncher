// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Octol1ttle <l1ttleofficial@outlook.com>
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

#ifdef Q_OS_WIN32
#define UNEXPECTED_WIN32_ERROR_LAST (std::unexpected{ std::error_code{ static_cast<int>(GetLastError()), std::system_category() } })
#define UNEXPECTED_WIN32_ERROR(x) (std::unexpected{ std::error_code{ static_cast<int>(x), std::system_category() } })
#endif

#define TRY(expected) if (const auto _result = expected; !_result) { return std::unexpected{ _result.error() }; }