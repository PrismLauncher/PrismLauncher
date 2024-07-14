// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include "FileSystem.h"
#include "pathmatcher/IPathMatcher.h"
#include "tasks/Task.h"

#include <QFuture>
#include <QFutureWatcher>

/*
 * Migrate existing data from other MMC-like launchers.
 */

class DataMigrationTask : public TaskV2 {
    Q_OBJECT
   public:
    explicit DataMigrationTask(QObject* parent, const QString& sourcePath, const QString& targetPath, IPathMatcher::Ptr pathmatcher);
    ~DataMigrationTask() = default;

   protected:
    virtual void executeTask() override;

   protected slots:
    void dryRunFinished();
    void dryRunAborted();
    void copyFinished();
    void copyAborted();

   private:
    const QString& m_sourcePath;
    const QString& m_targetPath;
    const IPathMatcher::Ptr m_pathMatcher;

    FS::copy m_copy;
    QFuture<bool> m_copyFuture;
    QFutureWatcher<bool> m_copyFutureWatcher;
};
