// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024
 *  Copyright (c) 2024 Abhinav Acharya <114682464+abhicommands@users.noreply.github.com>
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

#include "ModFolderLoadTask.h"

#include "Application.h"
#include "FileSystem.h"
#include "minecraft/mod/MetadataHandler.h"

#include <QFileInfo>
#include <QThread>

static bool isKnownModSuffix(const QString& suffix)
{
    return suffix == "jar" || suffix == "zip" || suffix == "litemod" || suffix == "nilmod";
}

static bool isModFile(const QFileInfo& info)
{
    if (!info.isFile()) {
        return false;
    }

    auto suffix = info.suffix().toLower();
    if (suffix == "disabled") {
        auto complete_suffix = info.completeSuffix().toLower();
        const auto parts = complete_suffix.split('.');
        if (parts.size() < 2) {
            return false;
        }
        auto inner_suffix = parts.at(parts.size() - 2);
        return isKnownModSuffix(inner_suffix);
    }

    return isKnownModSuffix(suffix);
}

ModFolderLoadTask::ModFolderLoadTask(const QDir& root_dir,
                                     bool is_indexed,
                                     bool clean_orphan,
                                     std::function<Resource*(const QFileInfo&)> create_function)
    : Task(false)
    , m_rootDir(root_dir)
    , m_isIndexed(is_indexed)
    , m_cleanOrphan(clean_orphan)
    , m_createFunc(create_function)
    , m_result(new Result())
    , m_threadToSpawnInto(thread())
{}

void ModFolderLoadTask::executeTask()
{
    if (thread() != m_threadToSpawnInto) {
        connect(this, &Task::finished, this->thread(), &QThread::quit);
    }

    if (m_isIndexed) {
        getFromMetadata(m_rootDir);
    }

    getFromFiles(m_rootDir, 0);

    if (m_cleanOrphan) {
        QMutableMapIterator iter(m_result->resources);
        while (iter.hasNext()) {
            auto resource = iter.next().value();
            if (resource->status() == ResourceStatus::NOT_INSTALLED) {
                QDir index_dir(QDir(resource->fileinfo().absolutePath()).filePath(".index"));
                resource->destroy(index_dir, false, false);
                iter.remove();
            }
        }
    }

    for (auto mod : m_result->resources) {
        mod->moveToThread(m_threadToSpawnInto);
    }

    if (m_aborted) {
        emitAborted();
        return;
    }
    emitSucceeded();
}

void ModFolderLoadTask::getFromMetadata(const QDir& dir)
{
    QDir index_dir(dir.filePath(".index"));
    if (index_dir.exists()) {
        index_dir.refresh();
        for (auto entry : index_dir.entryList(QDir::Files)) {
            auto metadata = Metadata::get(index_dir, entry);
            if (!metadata.isValid()) {
                continue;
            }

            QFileInfo file_info(dir.filePath(metadata.filename));
            auto* resource = m_createFunc(file_info);
            resource->setInternalId(internalIdForFile(file_info));
            resource->setMetadata(metadata);
            resource->setStatus(ResourceStatus::NOT_INSTALLED);
            m_result->resources[resource->internal_id()].reset(resource);
        }
    }

    for (auto entry : dir.entryInfoList(QDir::Dirs | QDir::NoDotAndDotDot)) {
        if (entry.fileName() == ".index") {
            continue;
        }
        getFromMetadata(QDir(entry.absoluteFilePath()));
    }
}

void ModFolderLoadTask::getFromFiles(const QDir& dir, int depth)
{
    dir.refresh();
    for (auto entry : dir.entryInfoList(QDir::NoDotAndDotDot | QDir::AllEntries)) {
        if (entry.isDir()) {
            if (entry.fileName() == ".index") {
                continue;
            }
            if (depth == 0) {
                Resource* resource = m_createFunc(entry);
                resource->setInternalId(internalIdForFile(entry));
                m_result->resources[resource->internal_id()].reset(resource);
                m_result->resources[resource->internal_id()]->setStatus(ResourceStatus::NO_METADATA);
            }
            getFromFiles(QDir(entry.absoluteFilePath()), depth + 1);
            continue;
        }

        if (!isModFile(entry)) {
            continue;
        }

        auto filePath = entry.absoluteFilePath();
        if (auto app = APPLICATION_DYN; app && app->checkQSavePath(filePath)) {
            continue;
        }

        auto newFilePath = FS::getUniqueResourceName(filePath);
        if (newFilePath != filePath) {
            FS::move(filePath, newFilePath);
            entry = QFileInfo(newFilePath);
        }

        Resource* resource = m_createFunc(entry);
        resource->setInternalId(internalIdForFile(entry));

        if (resource->enabled()) {
            if (m_result->resources.contains(resource->internal_id())) {
                m_result->resources[resource->internal_id()]->setStatus(ResourceStatus::INSTALLED);
                delete resource;
            } else {
                m_result->resources[resource->internal_id()].reset(resource);
                m_result->resources[resource->internal_id()]->setStatus(ResourceStatus::NO_METADATA);
            }
        } else {
            QString chopped_id = resource->internal_id();
            QFileInfo chopped_info(chopped_id);
            if (chopped_info.suffix().compare("disabled", Qt::CaseInsensitive) == 0) {
                chopped_id.chop(9);
            }

            if (m_result->resources.contains(chopped_id)) {
                m_result->resources[resource->internal_id()].reset(resource);

                auto metadata = m_result->resources[chopped_id]->metadata();
                if (metadata) {
                    resource->setMetadata(*metadata);
                    m_result->resources[resource->internal_id()]->setStatus(ResourceStatus::INSTALLED);
                    m_result->resources.remove(chopped_id);
                }
            } else {
                m_result->resources[resource->internal_id()].reset(resource);
                m_result->resources[resource->internal_id()]->setStatus(ResourceStatus::NO_METADATA);
            }
        }
    }
}

QString ModFolderLoadTask::internalIdForFile(const QFileInfo& info) const
{
    auto rel_path = m_rootDir.relativeFilePath(info.absoluteFilePath());
    rel_path = QDir::cleanPath(QDir::fromNativeSeparators(rel_path));
    if (rel_path.startsWith("..")) {
        return info.fileName();
    }
    return rel_path;
}
