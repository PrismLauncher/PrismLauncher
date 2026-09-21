// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QFlags>
#include <QHash>
#include <QSharedPointer>
#include <QString>
#include <QStringList>

class MinecraftInstance;
class QLockFile;

namespace SharedContent {

enum class Category : quint32 {
    None = 0,
    Options = 1U << 0,
    Screenshots = 1U << 1,
    ResourcePacks = 1U << 2,
    TexturePacks = 1U << 3,
    ShaderPacks = 1U << 4,
    Config = 1U << 5,
    Servers = 1U << 6,
    CommandHistory = 1U << 7,
    CreativeHotbar = 1U << 8,
    GlobalDataPacks = 1U << 9,
};
Q_DECLARE_FLAGS(Categories, Category)

enum class MigrationPolicy {
    PreferShared,
    PreferInstance,
};

struct CustomPath {
    QString relativePath;
    bool directory = true;
};

class Manager {
   public:
    explicit Manager(QString rootPath = {});
    ~Manager();

    Manager(const Manager&) = delete;
    Manager& operator=(const Manager&) = delete;

    QString rootPath() const;
    bool setRootPath(const QString& rootPath, QString* error = nullptr);

    QStringList groups() const;
    bool createGroup(const QString& name, QString* error = nullptr);
    bool renameGroup(const QString& oldName, const QString& newName, QString* error = nullptr);
    bool deleteGroup(const QString& name, bool deleteContent, QString* error = nullptr);
    QString groupPath(const QString& name) const;

    static bool isValidGroupName(const QString& name, QString* error = nullptr);
    static bool validateCustomPath(const QString& relativePath, QString* error = nullptr);
    static QString categoryId(Category category);
    static Category categoryFromId(const QString& id);
    static QStringList serializeCategories(Categories categories);
    static Categories deserializeCategories(const QStringList& ids);
    static QString serializeCustomPath(const CustomPath& path);
    static CustomPath deserializeCustomPath(const QString& value);

    static void registerInstanceSettings(MinecraftInstance* instance);
    QString instanceGroup(MinecraftInstance* instance) const;
    Categories instanceCategories(MinecraftInstance* instance) const;
    QList<CustomPath> instanceCustomPaths(MinecraftInstance* instance) const;
    QStringList instanceExcludedOptions(MinecraftInstance* instance) const;

    bool configureInstance(MinecraftInstance* instance,
                           const QString& group,
                           Categories categories,
                           const QList<CustomPath>& customPaths,
                           const QStringList& excludedOptions,
                           MigrationPolicy policy,
                           QString* error = nullptr);
    bool disconnectInstance(MinecraftInstance* instance, bool copySharedContentBack, QString* error = nullptr);

    bool repairInstance(MinecraftInstance* instance, QString* error = nullptr);

    bool prepare(MinecraftInstance* instance, QString* error = nullptr);
    bool finish(MinecraftInstance* instance, QString* error = nullptr);
    void cancel(MinecraftInstance* instance);

    QString effectivePath(MinecraftInstance* instance, Category category) const;
    QString effectiveCustomPath(MinecraftInstance* instance, const CustomPath& path) const;

   private:
    bool ensureGroup(const QString& name, QString* error) const;
    bool reconcileDirectories(MinecraftInstance* instance, MigrationPolicy policy, QString* error);
    bool validatePathLayout(MinecraftInstance* instance,
                            const QString& group,
                            Categories categories,
                            const QList<CustomPath>& customPaths,
                            QString* error) const;
    bool reconcileDirectory(const QString& localPath,
                            const QString& sharedPath,
                            const QString& backupRoot,
                            MigrationPolicy policy,
                            QString* error) const;
    bool prepareFile(const QString& localPath, const QString& sharedPath, QString* error) const;
    bool finishFile(const QString& localPath, const QString& sharedPath, QString* error) const;
    bool updateAllowedSymlinks(MinecraftInstance* instance, const QString& path, bool add, QString* error) const;
    QString localPath(MinecraftInstance* instance, Category category) const;
    QString sharedPath(const QString& group, Category category) const;
    QString groupLockPath(const QString& group) const;
    QString lockKey(MinecraftInstance* instance) const;

   private:
    QString m_rootPath;
    QHash<QString, QSharedPointer<QLockFile>> m_locks;
    QHash<QString, QString> m_lockGroups;
};

}

Q_DECLARE_OPERATORS_FOR_FLAGS(SharedContent::Categories)
