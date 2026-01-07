#pragma once

#include "settings/SettingsObject.h"
#include "tasks/Task.h"

/* Helpers */
enum class InstanceNameChange { ShouldChange, ShouldKeep };
[[nodiscard]] InstanceNameChange askForChangingInstanceName(QWidget* parent, const QString& old_name, const QString& new_name);
enum class ShouldUpdate { Update, SkipUpdating, Cancel };
[[nodiscard]] ShouldUpdate askIfShouldUpdate(QWidget* parent, QString original_version_name);

struct InstanceName {
   public:
    InstanceName() = default;
    InstanceName(QString name, QString version) : m_original_name(std::move(name)), m_original_version(std::move(version)) {}

    QString modifiedName() const;
    QString originalName() const;
    QString name() const;
    QString version() const;

    void setName(QString name) { m_modified_name = name; }
    void setName(InstanceName& other);

   protected:
    QString m_original_name;
    QString m_original_version;

    QString m_modified_name;
};

class InstanceTask : public Task, public InstanceName {
    Q_OBJECT
   public:
    InstanceTask();
    ~InstanceTask() override = default;

    void setParentSettings(SettingsObject* settings) { m_globalSettings = settings; }

    void setStagingPath(const QString& stagingPath) { m_stagingPath = stagingPath; }

    void setIcon(const QString& icon) { m_instIcon = icon; }

    void setGroup(const QString& group) { m_instGroup = group; }
    QString group() const { return m_instGroup; }

    bool shouldConfirmUpdate() const { return m_confirm_update; }
    void setConfirmUpdate(bool confirm) { m_confirm_update = confirm; }

    bool shouldOverride() const { return m_override_existing; }

    QString originalInstanceID() const { return m_original_instance_id; };

   protected:
    void setOverride(bool override, QString instance_id_to_override = {})
    {
        m_override_existing = override;
        if (!instance_id_to_override.isEmpty())
            m_original_instance_id = instance_id_to_override;
    }

   protected: /* data */
    SettingsObject* m_globalSettings;
    QString m_instIcon;
    QString m_instGroup;
    QString m_stagingPath;

    bool m_override_existing = false;
    bool m_confirm_update = true;

    QString m_original_instance_id;
};
