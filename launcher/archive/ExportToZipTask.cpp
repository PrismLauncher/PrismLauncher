// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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
#include "ExportToZipTask.h"

#include <QtConcurrent>

#include "FileSystem.h"

namespace MMCZip {
void ExportToZipTask::executeTask()
{
    setStatus("Adding files...");
    setProgress(0, m_files.length());
    m_buildZipFuture = QtConcurrent::run(QThreadPool::globalInstance(), [this]() { return exportZip(); });
    connect(&m_buildZipWatcher, &QFutureWatcher<ZipResult>::finished, this, &ExportToZipTask::finish);
    m_buildZipWatcher.setFuture(m_buildZipFuture);
}

auto ExportToZipTask::exportZip() -> ZipResult
{
    if (!m_dir.exists()) {
        return ZipResult(tr("Folder doesn't exist"));
    }
    if (!m_output.open()) {
        return ZipResult(tr("Could not create file"));
    }

    for (auto fileName : m_extraFiles.keys()) {
        if (m_buildZipFuture.isCanceled())
            return ZipResult();
        if (!m_output.addFile(fileName, m_extraFiles[fileName])) {
            return ZipResult(tr("Could not add:") + fileName);
        }
    }

    for (const QFileInfo& file : m_files) {
        if (m_buildZipFuture.isCanceled())
            return ZipResult();

        auto absolute = file.absoluteFilePath();
        auto relative = m_dir.relativeFilePath(absolute);
        setStatus("Compressing: " + relative);
        setProgress(m_progress + 1, m_progressTotal);
        if (m_followSymlinks) {
            if (file.isSymLink())
                absolute = file.symLinkTarget();
            else
                absolute = file.canonicalFilePath();
        }

        if (!m_excludeFiles.contains(relative) && !m_output.addFile(absolute, m_destinationPrefix + relative)) {
            return ZipResult(tr("Could not read and compress %1").arg(relative));
        }
    }

    if (!m_output.close()) {
        return ZipResult(tr("A zip error occurred"));
    }
    return ZipResult();
}

void ExportToZipTask::finish()
{
    if (m_buildZipFuture.isCanceled()) {
        FS::deletePath(m_outputPath);
        emitAborted();
    } else if (auto result = m_buildZipFuture.result(); result.has_value()) {
        FS::deletePath(m_outputPath);
        emitFailed(result.value());
    } else {
        emitSucceeded();
    }
}

bool ExportToZipTask::abort()
{
    if (m_buildZipFuture.isRunning()) {
        m_buildZipFuture.cancel();
        // NOTE: Here we don't do `emitAborted()` because it will be done when `m_build_zip_future` actually cancels, which may not occur
        // immediately.
        return true;
    }
    return false;
}
}  // namespace MMCZip
