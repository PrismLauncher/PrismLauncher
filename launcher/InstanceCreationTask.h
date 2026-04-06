#pragma once

#include "BaseVersion.h"
#include "InstanceTask.h"
#include "minecraft/MinecraftInstance.h"

class InstanceCreationTask : public InstanceTask {
    Q_OBJECT
   public:
    InstanceCreationTask() = default;
    virtual ~InstanceCreationTask() = default;

    bool abort() override;

   protected:
    void executeTask() final override;

    /**
     * Tries to update an already existing instance.
     *
     * This can be implemented by subclasses to provide a way of updating an already existing
     * instance, according to that implementation's concept of 'identity' (i.e. instances that
     * are updates / downgrades of one another).
     *
     * If this returns true, createInstance() will not run, so you should do all update steps in here.
     * Otherwise, createInstance() is run as normal.
     */
    virtual bool updateInstance() { return false; };

    /**
     * Creates a new instance.
     *
     * Returns the instance if it was created or nullptr otherwise.
     */
    virtual std::unique_ptr<MinecraftInstance> createInstance() { return nullptr; }

    QString getError() const { return m_error_message; }

   protected:
    void setError(const QString& message) { m_error_message = message; };
    void scheduleToDelete(QWidget* parent, QDir dir, QString path, bool checkDisabled = false);

   protected:
    bool m_abort = false;

    QStringList m_filesToRemove;
    ShouldDeleteSaves m_shouldDeleteSaves;

   private:
    QString m_error_message;
    std::unique_ptr<MinecraftInstance> m_instance;
    Task::Ptr m_gameFilesTask;
};
