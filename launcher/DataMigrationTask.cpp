// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "DataMigrationTask.h"

#include "FileSystem.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QMap>

#include <QtConcurrent>

DataMigrationTask::DataMigrationTask(QObject* parent,
                                     const QString& sourcePath,
                                     const QString& targetPath,
                                     const IPathMatcher::Ptr pathMatcher)
    : Task(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sourcePath(sourcePath), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_targetPath(targetPath), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pathMatcher(pathMatcher), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy(sourcePath, targetPath)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy.matcher(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_pathMatcher.get()).whitelist(true);
}

void DataMigrationTask::executeTask()
{
    setStatus(tr("Scanning files..."));

    // 1. Scan
    // Check how many files we gotta copy
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), [&] {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy(true);  // dry run to collect amount of files
    });
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &DataMigrationTask::dryRunFinished);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &DataMigrationTask::dryRunAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture);
}

void DataMigrationTask::dryRunFinished()
{
    disconnect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &DataMigrationTask::dryRunFinished);
    disconnect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &DataMigrationTask::dryRunAborted);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.isValid() || !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.result()) {
#else
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.result()) {
#endif
        emitFailed(tr("Failed to scan source path."));
        return;
    }

    // 2. Copy
    // Actually copy all files now.
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toCopy = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy.totalCopied();
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy, &FS::copy::fileCopied, [&, this](const QString& relativeName) {
        QString shortenedName = relativeName;
        // shorten the filename to hopefully fit into one line
        if (shortenedName.length() > 50)
            shortenedName = relativeName.left(20) + "…" + relativeName.right(29);
        setProgress(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy.totalCopied(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toCopy);
        setStatus(tr("Copying %1…").arg(shortenedName));
    });
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture = QtConcurrent::run(QThreadPool::globalInstance(), [&] {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copy(false);  // actually copy now
    });
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &DataMigrationTask::copyFinished);
    connect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &DataMigrationTask::copyAborted);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher.setFuture(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture);
}

void DataMigrationTask::dryRunAborted()
{
    emitFailed(tr("Aborted"));
}

void DataMigrationTask::copyFinished()
{
    disconnect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::finished, this, &DataMigrationTask::copyFinished);
    disconnect(&hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFutureWatcher, &QFutureWatcher<bool>::canceled, this, &DataMigrationTask::copyAborted);

#if (QT_VERSION >= QT_VERSION_CHECK(6, 0, 0))
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.isValid() || !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.result()) {
#else
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_copyFuture.result()) {
#endif
        emitFailed(tr("Some paths could not be copied!"));
        return;
    }

    emitSucceeded();
}

void DataMigrationTask::copyAborted()
{
    emitFailed(tr("Aborted"));
}
