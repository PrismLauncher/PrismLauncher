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
#pragma once

#include <QDir>
#include <QFuture>
#include <QFutureWatcher>

#include "Result.h"
#include "archive/ArchiveReader.h"
#include "tasks/Task.h"

namespace MMCZip {

class ExtractZipTask : public Task {
    Q_OBJECT
   public:
    ExtractZipTask(QString input, QDir outputDir, QString subdirectory = "")
        : m_input(input), m_outputDir(outputDir), m_subdirectory(subdirectory)
    {}
    virtual ~ExtractZipTask() = default;

   protected:
    virtual void executeTask() override;
    bool abort() override;

    Result<> extractZip();
    void finish();

   private:
    ArchiveReader m_input;
    QDir m_outputDir;
    QString m_subdirectory;

    QFuture<Result<>> m_zipFuture;
    QFutureWatcher<Result<>> m_zipWatcher;
};
}  // namespace MMCZip
