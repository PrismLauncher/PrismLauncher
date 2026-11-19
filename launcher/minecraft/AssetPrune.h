// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 TheKodeToad <TheKodeToad@proton.me>
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

#include <QList>
#include <QString>
#include <cstdint>
#include <memory>

#include "tasks/Task.h"

class HttpMetaCache;
class InstanceList;

namespace AssetPrune {
std::unique_ptr<Task> prepare(const InstanceList& instances);

struct PruneSet {
    QStringList libs;
    QStringList assetIndexes;
    QStringList assets;
    std::int64_t reclaimableBytes = 0;

    qsizetype count() const { return libs.count() + assetIndexes.count() + assets.count(); }
};

PruneSet collect(const HttpMetaCache& metacache, const InstanceList& instances);

struct PruneResult {
    QStringList errors;
    std::int64_t removedCount = 0;
    std::int64_t reclaimedBytes = 0;
};

PruneResult perform(HttpMetaCache* metacache, const PruneSet& set);
}  // namespace AssetPrune
