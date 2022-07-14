#pragma once

#include "settings/SettingsObject.h"
#include "tasks/Task.h"

struct InstanceName {
   public:
    InstanceName() = default;
    InstanceName(QString name, QString version) : m_original_name(std::move(name)), m_original_version(std::move(version)) {}

    [[nodiscard]] QString modifiedName() const;
    [[nodiscard]] QString originalName() const;
    [[nodiscard]] QString name() const;
    [[nodiscard]] QString version() const;

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

    void setParentSettings(SettingsObjectPtr settings) { m_globalSettings = settings; }

    void setStagingPath(const QString& stagingPath) { m_stagingPath = stagingPath; }

    void setIcon(const QString& icon) { m_instIcon = icon; }

    void setGroup(const QString& group) { m_instGroup = group; }
    QString group() const { return m_instGroup; }

    bool shouldOverride() const { return m_override_existing; }

   protected:
    void setOverride(bool override) { m_override_existing = override; }

   protected: /* data */
    SettingsObjectPtr m_globalSettings;
    QString m_instIcon;
    QString m_instGroup;
    QString m_stagingPath;

    bool m_override_existing = false;
};
