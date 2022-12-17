// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
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

#include "ScanModFolders.h"
#include "launch/LaunchTask.h"
#include "MMCZip.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"

void ScanModFolders::executeTask()
{
    auto hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst = std::dynamic_pointer_cast<MinecraftInstance>(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parent->instance());

    auto loaders = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst->loaderModList();
    connect(loaders.get(), &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone);
    if(!loaders->update()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modsDone = true;
    }

    auto cores = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_inst->coreModList();
    connect(cores.get(), &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone);
    if(!cores->update()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_coreModsDone = true;
    }
    checkDone();
}

void ScanModFolders::modsDone()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modsDone = true;
    checkDone();
}

void ScanModFolders::coreModsDone()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_coreModsDone = true;
    checkDone();
}

void ScanModFolders::checkDone()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_modsDone && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_coreModsDone) {
        emitSucceeded();
    }
}
