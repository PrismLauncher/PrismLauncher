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

#include <QFile>

#include "BaseInstaller.h"
#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"

BaseInstaller::BaseInstaller() {}

bool BaseInstaller::isApplied(MinecraftInstance* on)
{
    return QFile::exists(filename(on->instanceRoot()));
}

bool BaseInstaller::add(MinecraftInstance* to)
{
    if (!patchesDir(to->instanceRoot()).exists()) {
        QDir(to->instanceRoot()).mkdir("patches");
    }

    if (isApplied(to)) {
        if (!remove(to)) {
            return false;
        }
    }

    return true;
}

bool BaseInstaller::remove(MinecraftInstance* from)
{
    return FS::deletePath(filename(from->instanceRoot()));
}

QString BaseInstaller::filename(const QString& root) const
{
    return patchesDir(root).absoluteFilePath(id() + ".json");
}
QDir BaseInstaller::patchesDir(const QString& root) const
{
    return QDir(root + "/patches/");
}
