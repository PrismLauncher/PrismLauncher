/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "MinecraftUpdate.h"
#include "MinecraftInstance.h"

#include "minecraft/PackProfile.h"

#include "tasks/SequentialTask.h"
#include "update/AssetUpdateTask.h"
#include "update/FMLLibrariesTask.h"
#include "update/FoldersTask.h"
#include "update/LibrariesTask.h"

MinecraftUpdate::MinecraftUpdate(MinecraftInstance* inst, QObject* parent) : SequentialTask(parent), m_inst(inst) {}

void MinecraftUpdate::executeTask()
{
    m_queue.clear();
    // create folders
    {
        addTask(makeShared<FoldersTask>(m_inst));
    }

    // add metadata update task if necessary
    {
        auto components = m_inst->getPackProfile();
        components->reload(Net::Mode::Online);
        auto task = components->getCurrentTask();
        if (task) {
            addTask(task);
        }
    }

    // libraries download
    {
        addTask(makeShared<LibrariesTask>(m_inst));
    }

    // FML libraries download and copy into the instance
    {
        addTask(makeShared<FMLLibrariesTask>(m_inst));
    }

    // assets update
    {
        addTask(makeShared<AssetUpdateTask>(m_inst));
    }

    SequentialTask::executeTask();
}
