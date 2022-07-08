#pragma once

#include "InstanceCreationTask.h"

#include "modplatform/flame/FileResolvingTask.h"

#include "net/NetJob.h"

class FlameCreationTask final : public InstanceCreationTask {
    Q_OBJECT

   public:
    FlameCreationTask(QString staging_path, SettingsObjectPtr global_settings, QWidget* parent)
        : InstanceCreationTask(), m_parent(parent)
    {
        setStagingPath(staging_path);
        setParentSettings(global_settings);
    }

    bool abort() override;

    bool createInstance() override;

   private slots:
    void idResolverSucceeded(QEventLoop&);

   private:
    QWidget* m_parent = nullptr;

    shared_qobject_ptr<Flame::FileResolvingTask> m_mod_id_resolver;
    NetJob::Ptr m_files_job;
};
