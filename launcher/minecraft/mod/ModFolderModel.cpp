// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *  Copyright (c) 2023 Trial97 <alexandru.tripon97@gmail.com>
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

#include "ModFolderModel.h"

#include <FileSystem.h>
#include <QDebug>
#include <QFileSystemWatcher>
#include <QHeaderView>
#include <QIcon>
#include <QMimeData>
#include <QString>
#include <QStyle>
#include <QThreadPool>
#include <QUrl>
#include <QUuid>

#include "Application.h"

#include "Json.h"
#include "minecraft/mod/MetadataHandler.h"
#include "minecraft/mod/Resource.h"
#include "minecraft/mod/tasks/LocalModParseTask.h"
#include "minecraft/mod/tasks/LocalModUpdateTask.h"
#include "minecraft/mod/tasks/ModFolderLoadTask.h"
#include "modplatform/ModIndex.h"
#include "modplatform/flame/FlameAPI.h"
#include "modplatform/flame/FlameModIndex.h"

ModFolderModel::ModFolderModel(const QString& dir, BaseInstance* instance, bool is_indexed, bool create_dir)
    : ResourceFolderModel(QDir(dir), instance, nullptr, create_dir), m_is_indexed(is_indexed)
{
    m_column_names = QStringList({ "Enable", "Image", "Name", "Version", "Last Modified", "Provider", "Size", "Side", "Loaders",
                                   "Minecraft Versions", "Release Type" });
    m_column_names_translated = QStringList({ tr("Enable"), tr("Image"), tr("Name"), tr("Version"), tr("Last Modified"), tr("Provider"),
                                              tr("Size"), tr("Side"), tr("Loaders"), tr("Minecraft Versions"), tr("Release Type") });
    m_column_sort_keys = { SortType::ENABLED, SortType::NAME,        SortType::NAME,        SortType::VERSION,
                           SortType::DATE,    SortType::PROVIDER,    SortType::SIZE,        SortType::SIDE,
                           SortType::LOADERS, SortType::MC_VERSIONS, SortType::RELEASE_TYPE };
    m_column_resize_modes = { QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Stretch,     QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive,
                              QHeaderView::Interactive, QHeaderView::Interactive, QHeaderView::Interactive };
    m_columnsHideable = { false, true, false, true, true, true, true, true, true, true, true };
    m_columnsHiddenByDefault = { false, false, false, false, false, false, false, true, true, true, true };
}

QVariant ModFolderModel::data(const QModelIndex& index, int role) const
{
    if (!validateIndex(index))
        return {};

    int row = index.row();
    int column = index.column();

    switch (role) {
        case Qt::DisplayRole:
            switch (column) {
                case NameColumn:
                    return m_resources[row]->name();
                case VersionColumn: {
                    switch (m_resources[row]->type()) {
                        case ResourceType::FOLDER:
                            return tr("Folder");
                        case ResourceType::SINGLEFILE:
                            return tr("File");
                        default:
                            break;
                    }
                    return at(row)->version();
                }
                case DateColumn:
                    return m_resources[row]->dateTimeChanged();
                case ProviderColumn: {
                    auto provider = at(row)->provider();
                    if (!provider.has_value()) {
                        //: Unknown mod provider (i.e. not Modrinth, CurseForge, etc...)
                        return tr("Unknown");
                    }

                    return provider.value();
                }
                case SideColumn: {
                    return Metadata::modSideToString(at(row)->side());
                }
                case LoadersColumn: {
                    QStringList loaders;
                    auto modLoaders = at(row)->loaders();
                    for (auto loader : { ModPlatform::NeoForge, ModPlatform::Forge, ModPlatform::Cauldron, ModPlatform::LiteLoader,
                                         ModPlatform::Fabric, ModPlatform::Quilt }) {
                        if (modLoaders & loader) {
                            loaders << getModLoaderAsString(loader);
                        }
                    }
                    return loaders.join(", ");
                }
                case McVersionsColumn: {
                    return at(row)->mcVersions().join(", ");
                }
                case ReleaseTypeColumn: {
                    return at(row)->releaseType().toString();
                }
                case SizeColumn:
                    return m_resources[row]->sizeStr();
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            if (column == NameColumn) {
                if (at(row)->isSymLinkUnder(instDirPath())) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is symbolically linked from elsewhere. Editing it will also change the original."
                              "\nCanonical Path: %1")
                               .arg(at(row)->fileinfo().canonicalFilePath());
                }
                if (at(row)->isMoreThanOneHardLink()) {
                    return m_resources[row]->internal_id() +
                           tr("\nWarning: This resource is hard linked elsewhere. Editing it will also change the original.");
                }
            }
            return m_resources[row]->internal_id();
        case Qt::DecorationRole: {
            if (column == NameColumn && (at(row)->isSymLinkUnder(instDirPath()) || at(row)->isMoreThanOneHardLink()))
                return APPLICATION->getThemedIcon("status-yellow");
            if (column == ImageColumn) {
                return at(row)->icon({ 32, 32 }, Qt::AspectRatioMode::KeepAspectRatioByExpanding);
            }
            return {};
        }
        case Qt::SizeHintRole:
            if (column == ImageColumn) {
                return QSize(32, 32);
            }
            return {};
        case Qt::CheckStateRole:
            if (column == ActiveColumn)
                return at(row)->enabled() ? Qt::Checked : Qt::Unchecked;
            return QVariant();
        default:
            return QVariant();
    }
}

QVariant ModFolderModel::headerData(int section, [[maybe_unused]] Qt::Orientation orientation, int role) const
{
    switch (role) {
        case Qt::DisplayRole:
            switch (section) {
                case ActiveColumn:
                case NameColumn:
                case VersionColumn:
                case DateColumn:
                case ProviderColumn:
                case ImageColumn:
                case SideColumn:
                case LoadersColumn:
                case McVersionsColumn:
                case ReleaseTypeColumn:
                case SizeColumn:
                    return columnNames().at(section);
                default:
                    return QVariant();
            }

        case Qt::ToolTipRole:
            switch (section) {
                case ActiveColumn:
                    return tr("Is the mod enabled?");
                case NameColumn:
                    return tr("The name of the mod.");
                case VersionColumn:
                    return tr("The version of the mod.");
                case DateColumn:
                    return tr("The date and time this mod was last changed (or added).");
                case ProviderColumn:
                    return tr("Where the mod was downloaded from.");
                case SideColumn:
                    return tr("On what environment the mod is running.");
                case LoadersColumn:
                    return tr("The mod loader.");
                case McVersionsColumn:
                    return tr("The supported minecraft versions.");
                case ReleaseTypeColumn:
                    return tr("The release type.");
                case SizeColumn:
                    return tr("The size of the mod.");
                default:
                    return QVariant();
            }
        default:
            return QVariant();
    }
    return QVariant();
}

int ModFolderModel::columnCount(const QModelIndex& parent) const
{
    return parent.isValid() ? 0 : NUM_COLUMNS;
}

Task* ModFolderModel::createUpdateTask()
{
    auto index_dir = indexDir();
    auto task = new ModFolderLoadTask(dir(), index_dir, m_is_indexed, m_first_folder_load);
    m_first_folder_load = false;
    return task;
}

Task* ModFolderModel::createParseTask(Resource& resource)
{
    return new LocalModParseTask(m_next_resolution_ticket, resource.type(), resource.fileinfo());
}

bool ModFolderModel::uninstallMod(const QString& filename, bool preserve_metadata)
{
    for (auto mod : allMods()) {
        if (mod->getOriginalFileName() == filename) {
            auto index_dir = indexDir();
            mod->destroy(index_dir, preserve_metadata, false);

            update();

            return true;
        }
    }

    return false;
}

bool ModFolderModel::deleteMods(const QModelIndexList& indexes)
{
    if (indexes.isEmpty())
        return true;

    for (auto i : indexes) {
        if (i.column() != 0) {
            continue;
        }
        auto m = at(i.row());
        auto index_dir = indexDir();
        m->destroy(index_dir);
    }

    update();

    return true;
}

bool ModFolderModel::deleteModsMetadata(const QModelIndexList& indexes)
{
    if (indexes.isEmpty())
        return true;

    for (auto i : indexes) {
        if (i.column() != 0) {
            continue;
        }
        auto m = at(i.row());
        auto index_dir = indexDir();
        m->destroyMetadata(index_dir);
    }

    update();

    return true;
}

bool ModFolderModel::isValid()
{
    return m_dir.exists() && m_dir.isReadable();
}

bool ModFolderModel::startWatching()
{
    // Remove orphaned metadata next time
    m_first_folder_load = true;
    return ResourceFolderModel::startWatching({ m_dir.absolutePath(), indexDir().absolutePath() });
}

bool ModFolderModel::stopWatching()
{
    return ResourceFolderModel::stopWatching({ m_dir.absolutePath(), indexDir().absolutePath() });
}

auto ModFolderModel::selectedMods(QModelIndexList& indexes) -> QList<Mod*>
{
    QList<Mod*> selected_resources;
    for (auto i : indexes) {
        if (i.column() != 0)
            continue;

        selected_resources.push_back(at(i.row()));
    }
    return selected_resources;
}

auto ModFolderModel::allMods() -> QList<Mod*>
{
    QList<Mod*> mods;

    for (auto& res : qAsConst(m_resources)) {
        mods.append(static_cast<Mod*>(res.get()));
    }

    return mods;
}

void ModFolderModel::onUpdateSucceeded()
{
    auto update_results = static_cast<ModFolderLoadTask*>(m_current_update_task.get())->result();

    auto& new_mods = update_results->mods;

#if QT_VERSION >= QT_VERSION_CHECK(5, 14, 0)
    auto current_list = m_resources_index.keys();
    QSet<QString> current_set(current_list.begin(), current_list.end());

    auto new_list = new_mods.keys();
    QSet<QString> new_set(new_list.begin(), new_list.end());
#else
    QSet<QString> current_set(m_resources_index.keys().toSet());
    QSet<QString> new_set(new_mods.keys().toSet());
#endif

    applyUpdates(current_set, new_set, new_mods);
}

void ModFolderModel::onParseSucceeded(int ticket, QString mod_id)
{
    auto iter = m_active_parse_tasks.constFind(ticket);
    if (iter == m_active_parse_tasks.constEnd())
        return;

    int row = m_resources_index[mod_id];

    auto parse_task = *iter;
    auto cast_task = static_cast<LocalModParseTask*>(parse_task.get());

    Q_ASSERT(cast_task->token() == ticket);

    auto resource = find(mod_id);

    auto result = cast_task->result();
    if (result && resource)
        resource->finishResolvingWithDetails(std::move(result->details));

    emit dataChanged(index(row), index(row, columnCount(QModelIndex()) - 1));
}

static const FlameAPI flameAPI;
bool ModFolderModel::installMod(QString file_path, ModPlatform::IndexedVersion& vers)
{
    if (vers.addonId.isValid()) {
        ModPlatform::IndexedPack pack{
            vers.addonId,
            ModPlatform::ResourceProvider::FLAME,
        };

        QEventLoop loop;

        auto response = std::make_shared<QByteArray>();
        auto job = flameAPI.getProject(vers.addonId.toString(), response);

        QObject::connect(job.get(), &Task::failed, [&loop] { loop.quit(); });
        QObject::connect(job.get(), &Task::aborted, &loop, &QEventLoop::quit);
        QObject::connect(job.get(), &Task::succeeded, [response, this, &vers, &loop, &pack] {
            QJsonParseError parse_error{};
            QJsonDocument doc = QJsonDocument::fromJson(*response, &parse_error);
            if (parse_error.error != QJsonParseError::NoError) {
                qWarning() << "Error while parsing JSON response for mod info at " << parse_error.offset
                           << " reason: " << parse_error.errorString();
                qDebug() << *response;
                return;
            }
            try {
                auto obj = Json::requireObject(Json::requireObject(doc), "data");
                FlameMod::loadIndexedPack(pack, obj);
            } catch (const JSONValidationError& e) {
                qDebug() << doc;
                qWarning() << "Error while reading mod info: " << e.cause();
            }
            LocalModUpdateTask update_metadata(indexDir(), pack, vers);
            QObject::connect(&update_metadata, &Task::finished, &loop, &QEventLoop::quit);
            update_metadata.start();
        });

        job->start();

        loop.exec();
    }
    return ResourceFolderModel::installResource(file_path);
}
