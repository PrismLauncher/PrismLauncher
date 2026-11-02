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

#include "AssetPrune.h"

#include <QDebug>
#include <QFile>
#include <map>
#include <unordered_set>

#include "AssetsUtils.h"
#include "InstanceList.h"
#include "PackProfile.h"
#include "tasks/ConcurrentTask.h"

namespace AssetPrune {
namespace {
class ReloadPackProfileTask : public ConcurrentTask {
    Q_OBJECT
   public:
    explicit ReloadPackProfileTask(PackProfile* components) : m_components(components) {}

    void executeTask() override
    {
        if (auto result = m_components->reload(Net::Mode::Offline); !result) {
            emitFailed(result.error());
            return;
        }

        auto* reloadTask = m_components->getCurrentTask().get();
        if (reloadTask == nullptr) {
            emitSucceeded();
            return;
        }

        connect(reloadTask, &Task::succeeded, this, &ReloadPackProfileTask::emitSucceeded);
        connect(reloadTask, &Task::failed, this, &ReloadPackProfileTask::emitFailed);
        connect(reloadTask, &Task::failed, this, &ReloadPackProfileTask::emitFailed);
        connect(reloadTask, &Task::aborted, this, &ReloadPackProfileTask::emitAborted);
        propagateFromOther(reloadTask);
    }

   private:
    PackProfile* m_components;
};
}  // namespace

std::unique_ptr<Task> prepare(const InstanceList& instances)
{
    auto result = std::make_unique<ConcurrentTask>();

    for (int i = 0; i < instances.count(); ++i) {
        const auto* instance = instances.at(i);
        auto* components = instance->getPackProfile();
        result->addTask(makeShared<ReloadPackProfileTask>(components));
    }

    return result;
}

PruneSet collect(const HttpMetaCache& metacache, const InstanceList& instances)
{
    PruneSet result;

    std::unordered_set<QString> usedLibs;
    std::unordered_set<QString> usedAssetIndexes;

    for (int i = 0; i < instances.count(); ++i) {
        const auto* instance = instances.at(i);
        const auto* components = instance->getPackProfile();
        auto profile = components->getProfile();

        auto libs = profile->getArtifacts();
        libs.append(profile->getLibraries());
        for (const auto& lib : libs) {
            if (lib == nullptr) {
                continue;
            }
            if (lib->isLocal()) {
                continue;
            }

            lib->visitDownloads(
                [&usedLibs, instance](const QString& storage, const QString& /*url*/, const QString& /*sha1*/) {
                    if (usedLibs.insert(storage).second) {
                        qDebug() << "Won't prune library" << storage << "used in" << instance->id();
                    }
                },
                instance->runtimeContext());
        }

        const auto assetIndex = profile->getMinecraftAssets();
        const auto assetIndexFilename = assetIndex->id + ".json";
        if (usedAssetIndexes.insert(assetIndexFilename).second) {
            qDebug() << "Won't prune asset index" << assetIndexFilename << "used in" << instance->id();
        }
    }

    const auto libsBasePath = metacache.getBasePath("libraries");
    for (const auto& key : metacache.getPaths("libraries")) {
        if (usedLibs.contains(key)) {
            continue;
        }

        const auto path = FS::PathCombine(libsBasePath, key);
        const QFileInfo info(path);
        if (!info.exists()) {
            continue;
        }

        result.libs.append(key);
        result.reclaimableBytes += info.size();
    }

    // hash -> reachable
    std::map<QString, bool> assetObjects;

    const auto assetIndexesBasePath = metacache.getBasePath("asset_indexes");
    for (const auto& key : metacache.getPaths("asset_indexes")) {
        const bool used = usedAssetIndexes.contains(key);

        const auto& path = FS::PathCombine(assetIndexesBasePath, key);
        const QFileInfo info(path);
        if (!info.exists()) {
            continue;
        }

        AssetsIndex index;
        if (!AssetsUtils::loadAssetsIndexJson("", path, index)) {
            qWarning() << "Failed to load asset index under" << key;
            continue;
        }

        for (const auto& obj : index.objects) {
            assetObjects[obj.hash] |= used;
        }

        if (!used) {
            result.assetIndexes.append(key);
            result.reclaimableBytes += info.size();
        }
    }

    for (const auto& [hash, used] : assetObjects) {
        if (used) {
            continue;
        }

        const QFileInfo info(AssetObject(hash).getLocalPath());
        if (!info.exists()) {
            continue;
        }

        result.assets.append(hash);
        result.reclaimableBytes += info.size();
    }

    return result;
}

PruneResult perform(HttpMetaCache* metacache, const PruneSet& set)
{
    PruneResult result;

    auto tryRemoveFile = [&](const QString& path) {
        if (!QFile::remove(path)) {
            result.errors.append(QObject::tr("Could not delete %1").arg(path));
            return false;
        }
        return true;
    };

    auto pruneFromMetacache = [&](const QString& base, const QStringList& keys) {
        const auto basePath = metacache->getBasePath(base);

        for (const auto& key : keys) {
            const auto path = FS::PathCombine(basePath, key);
            const auto fileSize = QFileInfo(path).size();
            if (!tryRemoveFile(path)) {
                continue;
            }

            ++result.removedCount;
            result.reclaimedBytes += fileSize;

            if (!metacache->forgetEntry(base, key)) {
                result.errors.append(QObject::tr("Could not remove metacache entry for %1").arg(path));
                continue;
            }

            qDebug() << "Pruned" << key << "from filesystem and metacache under" << base;
        }
    };

    pruneFromMetacache("libraries", set.libs);
    pruneFromMetacache("asset_indexes", set.assetIndexes);

    for (const auto& hash : set.assets) {
        const auto& path = AssetObject(hash).getLocalPath();
        const auto fileSize = QFileInfo(path).size();
        if (!tryRemoveFile(path)) {
            continue;
        }

        ++result.removedCount;
        result.reclaimedBytes += fileSize;

        qDebug() << "Pruned asset object" << hash << "from filesystem";
    }

    metacache->SaveNow();

    return result;
}
}  // namespace AssetPrune

#include "AssetPrune.moc"
