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

#include "ReconstructAssets.h"
#include "launch/LaunchTask.h"
#include "minecraft/AssetsUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

void ReconstructAssets::executeTask()
{
    auto instance = m_parent->instance();
    auto components = instance->getPackProfile();
    auto profile = components->getProfile();
    auto assets = profile->getMinecraftAssets();

    if (!AssetsUtils::reconstructAssets(assets->id, instance->resourcesDir())) {
        emit logLine("Failed to reconstruct Minecraft assets.", MessageLevel::Error);
    }

    emitSucceeded();
}
