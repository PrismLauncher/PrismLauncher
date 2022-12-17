// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
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

#include "InstanceCreationTask.h"

#include <optional>

#include "minecraft/MinecraftInstance.h"

#include "modplatform/flame/FileResolvingTask.h"

#include "net/NetJob.h"

#include "ui/dialogs/BlockedModsDialog.h"

class FlameCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    FlameCreationTask(const QString& staging_path,
                      SettingsObjectPtr global_settings,
                      QWidget* parent,
                      QString id,
                      QString version_id,
                      QString original_instance_id = {})
        : InstanceCreationTask()
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent(parent)
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_id(std::move(id))
        , hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_version_id(std::move(version_id))
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_original_instance_id = std::move(original_instance_id);
    }

    bool abort() override;

    bool updateInstance() override;
    bool createInstance() override;

   private slots:
    void idResolverSucceeded(QEventLoop&);
    void setupDownloadJob(QEventLoop&);
    void copyBlockedMods(QList<BlockedMod> const& blocked_mods);

   private:
    QWidget* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent = nullptr;

    shared_qobject_ptr<Flame::FileResolvingTask> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_mod_id_resolver;
    Flame::Manifest hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pack;

    // Handle to allow aborting
    NetJob* hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_process_update_file_info_job = nullptr;
    NetJob::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_files_job = nullptr;

    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_id, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_managed_version_id;

    std::optional<InstancePtr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance;
};
