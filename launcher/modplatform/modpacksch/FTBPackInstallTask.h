// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 flowln <flowlnlnln@gmail.com>
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *      Copyright 2020-2021 Jamie Mansfield <jmansfield@cadixdev.org>
 *      Copyright 2020-2021 Petr Mrazek <peterix@gmail.com>
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

#include "FTBPackManifest.h"

#include "InstanceTask.h"
#include "QObjectPtr.h"
#include "modplatform/flame/FileResolvingTask.h"
#include "net/NetJob.h"

#include <QWidget>

namespace ModpacksCH {

class PackInstallTask final : public InstanceTask
{
    Q_OBJECT

public:
    explicit PackInstallTask(Modpack pack, QString version, QWidget* parent = nullptr);
    ~PackInstallTask() override = default;

    bool abort() override;

protected:
    void executeTask() override;

private slots:
    void onManifestDownloadSucceeded();
    void onResolveModsSucceeded();
    void onCreateInstanceSucceeded();
    void onModDownloadSucceeded();

    void onManifestDownloadFailed(QString reason);
    void onResolveModsFailed(QString reason);
    void onCreateInstanceFailed(QString reason);
    void onModDownloadFailed(QString reason);

private:
    void resolveMods();
    void createInstance();
    void downloadPack();

private:
    NetJob::Ptr m_net_job = nullptr;
    shared_qobject_ptr<Flame::FileResolvingTask> m_mod_id_resolver_task = nullptr;

    QList<int> m_file_id_map;

    QByteArray m_response;

    Modpack m_pack;
    QString m_version_name;
    Version m_version;

    QMap<QString, QString> m_files_to_copy;

    //FIXME: nuke
    QWidget* m_parent;
};

}
