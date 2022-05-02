// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
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

#include "Sink.h"

namespace Net {

/*
 * Sink object for downloads that uses an external QByteArray it doesn't own as a target.
 * FIXME: It is possible that the QByteArray is freed while we're doing some operation on it,
 * causing a segmentation fault.
 */
class ByteArraySink : public Sink {
   public:
    ByteArraySink(QByteArray* output) : m_output(output){};

    virtual ~ByteArraySink() = default;

   public:
    auto init(QNetworkRequest& request) -> Task::State override
    {
        m_output->clear();
        if (initAllValidators(request))
            return Task::State::Running;
        return Task::State::Failed;
    };

    auto write(QByteArray& data) -> Task::State override
    {
        m_output->append(data);
        if (writeAllValidators(data))
            return Task::State::Running;
        return Task::State::Failed;
    }

    auto abort() -> Task::State override
    {
        m_output->clear();
        failAllValidators();
        return Task::State::Failed;
    }

    auto finalize(QNetworkReply& reply) -> Task::State override
    {
        if (finalizeAllValidators(reply))
            return Task::State::Succeeded;
        return Task::State::Failed;
    }

    auto hasLocalData() -> bool override { return false; }

   private:
    QByteArray* m_output;
};
}  // namespace Net
