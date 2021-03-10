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

#include "ScanModFolders.h"
#include "launch/LaunchTask.h"
#include "MMCZip.h"
#include "minecraft/OpSys.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/mod/ModFolderModel.h"

void ScanModFolders::executeTask()
{
    auto m_inst = std::dynamic_pointer_cast<MinecraftInstance>(m_parent->instance());

    auto loaders = m_inst->loaderModList();
    connect(loaders.get(), &ModFolderModel::updateFinished, this, &ScanModFolders::modsDone);
    if(!loaders->update()) {
        m_modsDone = true;
    }

    auto cores = m_inst->coreModList();
    connect(cores.get(), &ModFolderModel::updateFinished, this, &ScanModFolders::coreModsDone);
    if(!cores->update()) {
        m_coreModsDone = true;
    }
    checkDone();
}

void ScanModFolders::modsDone()
{
    m_modsDone = true;
    checkDone();
}

void ScanModFolders::coreModsDone()
{
    m_coreModsDone = true;
    checkDone();
}

void ScanModFolders::checkDone()
{
    if(m_modsDone && m_coreModsDone) {
        emitSucceeded();
    }
}
