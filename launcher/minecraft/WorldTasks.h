#pragma once

#include <memory>

#include <QFileInfo>
#include <QPointer>
#include <QString>

#include "tasks/Task.h"

class WorldList;

class InstallWorldTask : public Task {
    Q_OBJECT
   public:
    struct Args {
        QPointer<WorldList> worlds;
        QFileInfo sourceFile;
        QString targetDir;
    };

    explicit InstallWorldTask(Args args);

   protected:
    void executeTask() override;

   private:
    Args m_args;
    Task::Ptr m_extractTask;
};

class CopyWorldTask : public Task {
    Q_OBJECT
   public:
    struct Args {
        QPointer<WorldList> worlds;
        QFileInfo sourceFile;
        QString targetDir;
        QString targetName;
    };

    explicit CopyWorldTask(Args args);

   protected:
    void executeTask() override;

   private:
    Args m_args;
};

class DeleteWorldTask : public Task {
    Q_OBJECT
   public:
    struct Args {
        QPointer<WorldList> worlds;
        QFileInfo sourceFile;
        QString displayName;
    };

    explicit DeleteWorldTask(Args args);

   protected:
    void executeTask() override;

   private:
    Args m_args;
};