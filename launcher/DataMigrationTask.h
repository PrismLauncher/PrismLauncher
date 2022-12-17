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

class DataMigrationTask : public Task {
    Q_OBJECT
   public:
    explicit DataMigrationTask(QObject* parent, const QString& sourcePath, const QString& targetPath, const IPathMatcher::Ptr pathmatcher);
    ~DataMigrationTask() override = default;

   protected:
    virtual void executeTask() override;

   protected slots:
    void dryRunFinished();
    void dryRunAborted();
    void copyFinished();
    void copyAborted();

   private:
    const QString& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourcePath;
    const QString& hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_targetPath;
    const IPathMatcher::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pathMatcher;

    FS::copy hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toCopy = 0;
    QFuture<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture;
    QFutureWatcher<bool> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher;
};
