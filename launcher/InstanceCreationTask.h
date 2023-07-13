#pragma once

#include "BaseVersion.h"
#include "InstanceTask.h"

class InstanceCreationTask : public InstanceTask {
    Q_OBJECT
   public:
    InstanceCreationTask() = default;
    virtual ~InstanceCreationTask() = default;

   protected:
    void executeTask() final override { checkUpdate(); };

    /**
     * Tries to update an already existing instance.
     *
     * This can be implemented by subclasses to provide a way of updating an already existing
     * instance, according to that implementation's concept of 'identity' (i.e. instances that
     * are updates / downgrades of one another).
     *
     * This is also responsible to call createInstance() if needed.
     */
    virtual void checkUpdate() { createInstance(); }

    /**
     * Creates a new instance.
     *
     * When succesfully done this should call finishTask.
     */
    virtual void createInstance() { finishTask(); }

    /**
     * Finishes the instance creation.
     *
     * Removes the overides and emmits success.
     */
    void finishTask();

   protected slots:
    virtual void emitFailed(QString reason = "") override;

   protected:
    QStringList m_files_to_remove;
};
