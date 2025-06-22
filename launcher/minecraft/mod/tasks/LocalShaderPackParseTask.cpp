// SPDX-FileCopyrightText: 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "LocalShaderPackParseTask.h"

#include "FileSystem.h"

#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>

namespace ShaderPackUtils {

bool process(ShaderPack& pack, ProcessingLevel level)
{
    switch (pack.type()) {
        case ResourceType::FOLDER:
            return ShaderPackUtils::processFolder(pack, level);
        case ResourceType::ZIPFILE:
            return ShaderPackUtils::processZIP(pack, level);
        default:
            qWarning() << "Invalid type for shader pack parse task!";
            return false;
    }
}

bool processFolder(ShaderPack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::FOLDER);

    QFileInfo shaders_dir_info(FS::PathCombine(pack.fileinfo().filePath(), "shaders"));
    if (!shaders_dir_info.exists() || !shaders_dir_info.isDir()) {
        return false;  // assets dir does not exists or isn't valid
    }
    pack.setPackFormat(ShaderPackFormat::VALID);

    if (level == ProcessingLevel::BasicInfoOnly) {
        return true;  // only need basic info already checked
    }

    return true;  // all tests passed
}

bool processZIP(ShaderPack& pack, ProcessingLevel level)
{
    Q_ASSERT(pack.type() == ResourceType::ZIPFILE);

    QuaZip zip(pack.fileinfo().filePath());
    if (!zip.open(QuaZip::mdUnzip))
        return false;  // can't open zip file

    QuaZipFile file(&zip);

    QuaZipDir zipDir(&zip);
    if (!zipDir.exists("/shaders")) {
        return false;  // assets dir does not exists at zip root
    }
    pack.setPackFormat(ShaderPackFormat::VALID);

    if (level == ProcessingLevel::BasicInfoOnly) {
        zip.close();
        return true;  // only need basic info already checked
    }

    zip.close();

    return true;
}

bool validate(QFileInfo file)
{
    ShaderPack sp{ file };
    return ShaderPackUtils::process(sp, ProcessingLevel::BasicInfoOnly) && sp.valid();
}

}  // namespace ShaderPackUtils

LocalShaderPackParseTask::LocalShaderPackParseTask(int token, ShaderPack& sp) : Task(false), m_token(token), m_shader_pack(sp) {}

bool LocalShaderPackParseTask::abort()
{
    m_aborted = true;
    return true;
}

void LocalShaderPackParseTask::executeTask()
{
    if (!ShaderPackUtils::process(m_shader_pack)) {
        emitFailed("this is not a shader pack");
        return;
    }

    if (m_aborted)
        emitAborted();
    else
        emitSucceeded();
}
