// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Octol1ttle <l1ttleofficial@outlook.com>
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

namespace Net {

class DummySink : public Sink {
   public:
    explicit DummySink() {}
    ~DummySink() override {}
    auto init(QNetworkRequest& request) -> Task::State override { return Task::State::Running; }
    auto write(QByteArray& data) -> Task::State override { return Task::State::Succeeded; }
    auto abort() -> Task::State override { return Task::State::AbortedByUser; }
    auto finalize(QNetworkReply& reply) -> Task::State override { return Task::State::Succeeded; }
    auto hasLocalData() -> bool override { return false; }
};

}  // namespace Net
