// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2024 Prism Launcher Contributors
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

#include <optional>

#include <QString>
#include <QTemporaryDir>
#include <QUrl>
#include <QWidget>

#include "BaseInstance.h"
#include "InstanceCreationTask.h"
#include "TechnicPackManifest.h"
#include "net/NetJob.h"

namespace Technic {

class InstanceCreationTask final : public ::InstanceCreationTask {
    Q_OBJECT

   public:
    /** Constructor for Solder packs. */
    InstanceCreationTask(QWidget* parent,
                         const QString& slug,
                         const QString& solderUrl,
                         const QString& version,
                         const QString& minecraftVersion,
                         const QString& original_instance_id = {})
        : ::InstanceCreationTask()
        , m_parent(parent)
        , m_slug(slug)
        , m_solderUrl(solderUrl)
        , m_version(version)
        , m_minecraftVersion(minecraftVersion)
        , m_isSolder(true)
    {
        m_original_instance_id = original_instance_id;
    }

    /** Constructor for non-Solder (single zip) packs. */
    InstanceCreationTask(QWidget* parent,
                         const QString& slug,
                         const QUrl& downloadUrl,
                         const QString& version,
                         const QString& minecraftVersion,
                         const QString& original_instance_id = {})
        : ::InstanceCreationTask()
        , m_parent(parent)
        , m_slug(slug)
        , m_downloadUrl(downloadUrl)
        , m_version(version)
        , m_minecraftVersion(minecraftVersion)
        , m_isSolder(false)
    {
        m_original_instance_id = original_instance_id;
    }

    bool abort() override;

    bool updateInstance() override;
    bool createInstance() override;

   private:
    bool extractMods();
    bool processInstance();

   private:
    QWidget* m_parent = nullptr;

    QString m_slug;
    QString m_solderUrl;
    QUrl m_downloadUrl;
    QString m_version;
    QString m_minecraftVersion;
    bool m_isSolder = false;

    NetJob::Ptr m_filesNetJob;
    std::shared_ptr<QByteArray> m_response = std::make_shared<QByteArray>();
    QTemporaryDir m_outputDir;
    int m_modCount = 0;
    bool m_abortable = false;

    std::optional<BaseInstance*> m_instance;
};

}  // namespace Technic
