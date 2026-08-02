#pragma once

#include <QDir>
#include <cstdint>
#include "settings/SettingsObject.h"
#include "tasks/Task.h"

class MinecraftInstance;

/* Helpers */
enum class InstanceNameChange : std::uint8_t { ShouldChange, ShouldKeep };
[[nodiscard]] InstanceNameChange askForChangingInstanceName(QWidget* parent, const QString& oldName, const QString& newName);
enum class ShouldUpdate : std::uint8_t { Update, SkipUpdating, Cancel };
[[nodiscard]] ShouldUpdate askIfShouldUpdate(QWidget* parent, const QString& originalVersionName);
enum class ShouldDeleteSaves : std::uint8_t { NotAsked, Yes, No };
[[nodiscard]] ShouldDeleteSaves askIfShouldDeleteSaves(QWidget* parent);

class InstanceTask : public Task {
    Q_OBJECT
   public:
    InstanceTask() = default;
    ~InstanceTask() override = default;

    void setParentSettings(SettingsObject* settings) { m_globalSettings = settings; }

    void setStagingPath(const QString& stagingPath) { m_stagingPath = stagingPath; }

    void setIcon(const QString& icon) { m_instIcon = icon; }

    void setGroup(const QString& group) { m_instGroup = group; }
    QString group() const { return m_instGroup; }

    void setTargetDir(const QString& dir) { m_targetDir = dir; }
    QString targetDir() const { return m_targetDir; }

    bool shouldConfirmUpdate() const { return m_confirmUpdate; }
    void setConfirmUpdate(bool confirm) { m_confirmUpdate = confirm; }

    bool shouldOverride() const { return m_overrideExisting; }

    QString originalInstanceID() const { return m_originalInstanceId; };

    QString modifiedName() const;
    QString originalName() const;
    QString name() const;
    QString version() const;

    void setName(const QString& name) { m_modifiedName = name; }
    void setOriginalName(const QString& name, const QString& version);

   protected:
    void setOverride(bool override, const QString& instanceIdToOverride = {});

    void scheduleToDelete(QWidget* parent, const QDir& dir, const QString& path, bool checkDisabled = false);
    void downloadFiles(MinecraftInstance* inst);

   public slots:
    bool abort() override;

   protected: /* data */
    SettingsObject* m_globalSettings{};
    QString m_instIcon;
    QString m_instGroup;
    QString m_targetDir;
    QString m_stagingPath;

    bool m_overrideExisting = false;
    bool m_confirmUpdate = true;

    QString m_originalInstanceId;

    QString m_originalName;
    QString m_originalVersion;

    QString m_modifiedName;

    QStringList m_filesToRemove;
    ShouldDeleteSaves m_shouldDeleteSaves{};

    Task::Ptr m_gameFilesTask;
};
