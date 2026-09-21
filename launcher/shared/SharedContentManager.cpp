// SPDX-License-Identifier: GPL-3.0-only

#include "SharedContentManager.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QDir>
#include <QDirIterator>
#include <QEventLoop>
#include <QFile>
#include <QFileInfo>
#include <QLockFile>
#include <QRegularExpression>
#include <QSet>
#include <QTemporaryDir>
#include <QUuid>

#include "FileSystem.h"
#include "minecraft/MinecraftInstance.h"
#include "shared/SharedOptionsFile.h"

namespace SharedContent {
namespace {

constexpr auto GROUP_SETTING = "SharedContentGroup";
constexpr auto CATEGORY_SETTING = "SharedContentCategories";
constexpr auto CUSTOM_PATH_SETTING = "SharedContentCustomPaths";
constexpr auto EXCLUDED_OPTIONS_SETTING = "SharedContentExcludedOptions";
constexpr auto DATA_PACKS_PATH_SETTING = "SharedContentDataPacksPath";

const QList<Category> s_categories{ Category::Options,        Category::Screenshots,    Category::ResourcePacks, Category::TexturePacks,
                                    Category::ShaderPacks,    Category::Config,         Category::Servers,       Category::CommandHistory,
                                    Category::CreativeHotbar, Category::GlobalDataPacks };

bool pathsEqual(const QString& left, const QString& right)
{
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    const QFileInfo leftInfo(left);
    const QFileInfo rightInfo(right);
    const auto leftCanonical = leftInfo.canonicalFilePath();
    const auto rightCanonical = rightInfo.canonicalFilePath();
    if (!leftCanonical.isEmpty() && !rightCanonical.isEmpty()) {
        return leftCanonical.compare(rightCanonical, sensitivity) == 0;
    }
    return QDir::cleanPath(leftInfo.absoluteFilePath()).compare(QDir::cleanPath(rightInfo.absoluteFilePath()), sensitivity) == 0;
}

bool pathLocationsEqual(const QString& left, const QString& right)
{
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    return QDir::cleanPath(QFileInfo(left).absoluteFilePath())
               .compare(QDir::cleanPath(QFileInfo(right).absoluteFilePath()), sensitivity) == 0;
}


bool pathsOverlap(const QString& left, bool leftDirectory, const QString& right, bool rightDirectory)
{
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    const auto normalized = [](const QString& path) {
        return QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
    };
    const auto leftPath = normalized(left);
    const auto rightPath = normalized(right);
    return leftPath.compare(rightPath, sensitivity) == 0 || (leftDirectory && rightPath.startsWith(leftPath + '/', sensitivity)) ||
           (rightDirectory && leftPath.startsWith(rightPath + '/', sensitivity));
}

bool isInside(const QString& root, const QString& path)
{
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    QString rootPath = QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(root).absoluteFilePath()));
    const QString candidate = QDir::fromNativeSeparators(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
    if (!rootPath.endsWith('/')) {
        rootPath += '/';
    }
    if (!candidate.startsWith(rootPath, sensitivity) || QFileInfo(root).isSymLink()) {
        return false;
    }
    QDir parent(root);
    const auto components = QDir::fromNativeSeparators(parent.relativeFilePath(candidate)).split('/', Qt::SkipEmptyParts);
    for (qsizetype i = 0; i + 1 < components.size(); ++i) {
        parent.cd(components.at(i));
        if (QFileInfo(parent.absolutePath()).isSymLink()) {
            return false;
        }
    }
    return true;
}

bool resolveStoredDataPacksPath(MinecraftInstance* instance,
                                const QString& storedPath,
                                const QString& sharedPath,
                                QString* localPath,
                                QString* error)
{
    const QString gameRoot = instance->gameRoot();
    if (storedPath.isEmpty()) {
        const QString currentPath = QDir::cleanPath(instance->dataPacksDir());
        if (isInside(gameRoot, currentPath)) {
            *localPath = currentPath;
            return true;
        }
        if (error) {
            *error = QObject::tr("The global data packs path is outside this instance.");
        }
        return false;
    }

    const QString storedAbsolute = QDir::cleanPath(QDir::isRelativePath(storedPath) ? QDir(gameRoot).filePath(storedPath) : storedPath);
    if (isInside(gameRoot, storedAbsolute)) {
        *localPath = storedAbsolute;
        return true;
    }

    const QFileInfo oldPathInfo(storedAbsolute);
    const QString instancesRoot = QFileInfo(instance->instanceRoot()).dir().absolutePath();
    const QString relativeToInstances = QDir::fromNativeSeparators(QDir(instancesRoot).relativeFilePath(storedAbsolute));
    const QStringList parts = relativeToInstances.split('/', Qt::SkipEmptyParts);
#ifdef Q_OS_WIN
    constexpr auto sensitivity = Qt::CaseInsensitive;
#else
    constexpr auto sensitivity = Qt::CaseSensitive;
#endif
    if (!QDir::isRelativePath(storedPath) && parts.size() >= 3 && parts.at(0) != QStringLiteral("..") &&
        parts.at(1).compare(QFileInfo(gameRoot).fileName(), sensitivity) == 0) {
        const QString oldInstanceRoot = QDir(instancesRoot).filePath(parts.at(0));
        const QString relocated = FS::PathCombine(gameRoot, parts.mid(2).join('/'));
        const QFileInfo relocatedInfo(relocated);
        if (!oldPathInfo.exists() && !oldPathInfo.isSymLink() && !QFileInfo::exists(oldInstanceRoot) &&
            !QFileInfo(oldInstanceRoot).isSymLink() && isInside(gameRoot, relocated) && relocatedInfo.isSymLink() &&
            pathsEqual(relocatedInfo.symLinkTarget(), sharedPath)) {
            *localPath = relocated;
            return true;
        }
    }

    if (error) {
        *error = QObject::tr("The stored global data packs path is outside this instance and cannot be safely resolved.");
    }
    return false;
}

bool safeFilePair(const QString& gameRoot, const QString& sharedRoot, const QString& local, const QString& shared)
{
    return isInside(gameRoot, local) && isInside(sharedRoot, shared) && !QFileInfo(local).isSymLink() && !QFileInfo(shared).isSymLink();
}

bool copyDirectory(const QString& source, const QString& destination, bool overwrite)
{
    if (!QFileInfo(source).isDir() || QFileInfo(source).isSymLink() || QFileInfo(destination).isSymLink() ||
        (QFileInfo::exists(destination) && !QFileInfo(destination).isDir())) {
        return false;
    }
    for (const auto& path : { source, destination }) {
        if (!QFileInfo::exists(path)) {
            continue;
        }
        QDirIterator entries(path, QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
        while (entries.hasNext()) {
            entries.next();
            if (entries.fileInfo().isSymLink()) {
                return false;
            }
        }
    }
    if (!FS::ensureFolderPathExists(destination)) {
        return false;
    }
    FS::copy operation(source, destination);
    operation.followSymlinks(false).copyDirectories(true).overwrite(overwrite);
    if (!overwrite) {
        operation.matcher([destination](const QString& relative) { return !QFileInfo::exists(FS::PathCombine(destination, relative)); })
            .whitelist(true);
    }
    return operation();
}

bool copyFileAtomic(const QString& source, const QString& destination, QString* error)
{
    const auto contents = FS::read(source);
    if (!contents) {
        if (error) {
            *error = contents.error();
        }
        return false;
    }
    const auto result = FS::write(destination, contents.value());
    if (!result) {
        if (error) {
            *error = result.error();
        }
        return false;
    }
    return true;
}

bool makeDirectoryLink(const QString& source, const QString& destination, QString* error)
{
    FS::create_link linker(source, destination);
    linker.linkRecursively(false);
    if (linker()) {
        return true;
    }

#ifdef Q_OS_WIN
    QEventLoop loop;
    bool receivedResults = false;
    QObject::connect(&linker, &FS::create_link::finishedPrivileged, &loop, [&](bool gotResults) {
        receivedResults = gotResults;
        loop.quit();
    });
    linker.runPrivileged();
    loop.exec();
    if (receivedResults) {
        bool succeeded = true;
        for (const auto& result : linker.getResults()) {
            succeeded = succeeded && result.err_value == 0;
        }
        if (succeeded) {
            return true;
        }
    }
#endif

    if (error) {
        *error = QObject::tr("Could not link %1 to %2: %3").arg(destination, source, QString::fromStdString(linker.getOSError().message()));
    }
    return false;
}

bool detachDirectoryLink(const QString& local, const QString& shared, bool copyBack, QString* error)
{
    QTemporaryDir replacement(local + QStringLiteral(".prism-restore-XXXXXX"));
    if (!replacement.isValid() || (copyBack && QFileInfo::exists(shared) && !copyDirectory(shared, replacement.path(), true))) {
        if (error) {
            *error = QObject::tr("Could not stage a local copy of %1.").arg(shared);
        }
        return false;
    }
    if (!FS::deletePath(local)) {
        if (error) {
            *error = QObject::tr("Could not remove the link at %1.").arg(local);
        }
        return false;
    }
    if (!QDir().rename(replacement.path(), local)) {
        QString linkError;
        makeDirectoryLink(shared, local, &linkError);
        if (error) {
            *error = QObject::tr("Could not install the local folder at %1. %2").arg(local, linkError);
        }
        return false;
    }
    return true;
}

QString categoryRelativePath(Category category)
{
    switch (category) {
        case Category::Options:
            return QStringLiteral("options.txt");
        case Category::Screenshots:
            return QStringLiteral("screenshots");
        case Category::ResourcePacks:
            return QStringLiteral("resourcepacks");
        case Category::TexturePacks:
            return QStringLiteral("texturepacks");
        case Category::ShaderPacks:
            return QStringLiteral("shaderpacks");
        case Category::Config:
            return QStringLiteral("config");
        case Category::Servers:
            return QStringLiteral("servers.dat");
        case Category::CommandHistory:
            return QStringLiteral("command_history.txt");
        case Category::CreativeHotbar:
            return QStringLiteral("hotbar.nbt");
        case Category::GlobalDataPacks:
            return QStringLiteral("datapacks");
        case Category::None:
            break;
    }
    return {};
}

bool isDirectoryCategory(Category category)
{
    return category == Category::Screenshots || category == Category::ResourcePacks || category == Category::TexturePacks ||
           category == Category::ShaderPacks || category == Category::Config || category == Category::GlobalDataPacks;
}

bool isFileCategory(Category category)
{
    return category == Category::Servers || category == Category::CommandHistory || category == Category::CreativeHotbar;
}

}

Manager::Manager(QString rootPath)
{
    if (!rootPath.trimmed().isEmpty()) {
        m_rootPath = QDir::cleanPath(QFileInfo(rootPath).absoluteFilePath());
    }
}

Manager::~Manager()
{
    for (auto& lock : m_locks) {
        lock->unlock();
    }
}

QString Manager::rootPath() const
{
    return m_rootPath;
}

bool Manager::setRootPath(const QString& rootPath, QString* error)
{
    if (!m_locks.isEmpty()) {
        if (error) {
            *error = QObject::tr("The shared content folder cannot be changed while an instance is using it.");
        }
        return false;
    }
    if (rootPath.trimmed().isEmpty()) {
        if (error) {
            *error = QObject::tr("The shared content folder cannot be empty.");
        }
        return false;
    }
    const QString absolute = QDir::cleanPath(QFileInfo(rootPath).absoluteFilePath());
    if (QFileInfo::exists(absolute) && !QFileInfo(absolute).isDir()) {
        if (error) {
            *error = QObject::tr("The shared content path is not a directory: %1").arg(absolute);
        }
        return false;
    }
    if (!FS::ensureFolderPathExists(absolute)) {
        if (error) {
            *error = QObject::tr("Could not create the shared content folder: %1").arg(absolute);
        }
        return false;
    }
    m_rootPath = absolute;
    return true;
}

QStringList Manager::groups() const
{
    if (m_rootPath.isEmpty()) {
        return {};
    }
    QDir root(m_rootPath);
    auto result = root.entryList(QDir::Dirs | QDir::NoDotAndDotDot, QDir::Name | QDir::IgnoreCase);
    result.removeAll(QStringLiteral(".trash"));
    return result;
}

bool Manager::isValidGroupName(const QString& name, QString* error)
{
    const QString candidate = name.trimmed();
    static const QRegularExpression valid(QStringLiteral(R"(^[^/\\:*?"<>|\x00-\x1f]{1,64}$)"));
    static const QRegularExpression windowsReserved(QStringLiteral(R"(^(con|prn|aux|nul|com[1-9]|lpt[1-9])(\..*)?$)"),
                                                    QRegularExpression::CaseInsensitiveOption);
    const bool invalid = candidate != name || candidate.startsWith('.') || candidate.endsWith('.') || candidate.endsWith(' ') ||
                         !valid.match(candidate).hasMatch() || windowsReserved.match(candidate).hasMatch();
    if (invalid && error) {
        *error = QObject::tr("%1 is not a valid shared content group name.").arg(name);
    }
    return !invalid;
}

QString Manager::groupPath(const QString& name) const
{
    if (!isValidGroupName(name) || m_rootPath.isEmpty()) {
        return {};
    }
    return FS::PathCombine(m_rootPath, name);
}

bool Manager::createGroup(const QString& name, QString* error)
{
    if (!isValidGroupName(name, error)) {
        return false;
    }
    if (m_rootPath.isEmpty()) {
        if (error) {
            *error = QObject::tr("The shared content folder has not been configured.");
        }
        return false;
    }
    const QString path = groupPath(name);
    if (QFileInfo::exists(path)) {
        if (error) {
            *error = QObject::tr("A shared content group named %1 already exists.").arg(name);
        }
        return false;
    }
    if (!FS::ensureFolderPathExists(FS::PathCombine(path, QStringLiteral("minecraft")))) {
        if (error) {
            *error = QObject::tr("Could not create shared content group %1.").arg(name);
        }
        return false;
    }
    return true;
}

bool Manager::renameGroup(const QString& oldName, const QString& newName, QString* error)
{
    if (!isValidGroupName(oldName, error) || !isValidGroupName(newName, error)) {
        return false;
    }
    if (m_lockGroups.values().contains(oldName)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(oldName);
        }
        return false;
    }
    QLockFile groupLock(groupLockPath(oldName));
    if (!groupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(oldName);
        }
        return false;
    }
    const QString oldPath = groupPath(oldName);
    const QString newPath = groupPath(newName);
    if (!QFileInfo::exists(oldPath) || QFileInfo::exists(newPath) || !QDir().rename(oldPath, newPath)) {
        if (error) {
            *error = QObject::tr("Could not rename shared content group %1 to %2.").arg(oldName, newName);
        }
        return false;
    }
    return true;
}

bool Manager::deleteGroup(const QString& name, bool deleteContent, QString* error)
{
    if (!isValidGroupName(name, error)) {
        return false;
    }
    if (m_lockGroups.values().contains(name)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(name);
        }
        return false;
    }
    QLockFile groupLock(groupLockPath(name));
    if (!groupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(name);
        }
        return false;
    }
    const QString path = groupPath(name);
    if (!QFileInfo::exists(path)) {
        return true;
    }
    QDir content(FS::PathCombine(path, QStringLiteral("minecraft")));
    if (!deleteContent && !content.entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is not empty.").arg(name);
        }
        return false;
    }
    if (!FS::deletePath(path)) {
        if (error) {
            *error = QObject::tr("Could not delete shared content group %1.").arg(name);
        }
        return false;
    }
    return true;
}

QString Manager::categoryId(Category category)
{
    switch (category) {
        case Category::Options:
            return QStringLiteral("options");
        case Category::Screenshots:
            return QStringLiteral("screenshots");
        case Category::ResourcePacks:
            return QStringLiteral("resourcepacks");
        case Category::TexturePacks:
            return QStringLiteral("texturepacks");
        case Category::ShaderPacks:
            return QStringLiteral("shaderpacks");
        case Category::Config:
            return QStringLiteral("config");
        case Category::Servers:
            return QStringLiteral("servers");
        case Category::CommandHistory:
            return QStringLiteral("command_history");
        case Category::CreativeHotbar:
            return QStringLiteral("creative_hotbar");
        case Category::GlobalDataPacks:
            return QStringLiteral("global_datapacks");
        case Category::None:
            return {};
    }
    return {};
}

Category Manager::categoryFromId(const QString& id)
{
    for (const auto category : s_categories) {
        if (categoryId(category).compare(id, Qt::CaseInsensitive) == 0) {
            return category;
        }
    }
    return Category::None;
}

QStringList Manager::serializeCategories(Categories categories)
{
    QStringList result;
    for (const auto category : s_categories) {
        if (categories.testFlag(category)) {
            result.append(categoryId(category));
        }
    }
    return result;
}

Categories Manager::deserializeCategories(const QStringList& ids)
{
    Categories result;
    for (const auto& id : ids) {
        result.setFlag(categoryFromId(id));
    }
    result.setFlag(Category::None, false);
    return result;
}

bool Manager::validateCustomPath(const QString& relativePath, QString* error)
{
    QString normalized = relativePath.trimmed();
    normalized.replace('\\', '/');
    while (normalized.size() > 1 && normalized.endsWith('/')) {
        normalized.chop(1);
    }
    const QString clean = QDir::cleanPath(normalized);
    const QString first = clean.section('/', 0, 0).toLower();
    static const QSet<QString> blocked{ QStringLiteral("mods"),          QStringLiteral("coremods"),
                                        QStringLiteral("nilmods"),       QStringLiteral("saves"),
                                        QStringLiteral("libraries"),     QStringLiteral("assets"),
                                        QStringLiteral("versions"),      QStringLiteral("screenshots"),
                                        QStringLiteral("resourcepacks"), QStringLiteral("texturepacks"),
                                        QStringLiteral("shaderpacks"),   QStringLiteral("config"),
                                        QStringLiteral("datapacks"),     QStringLiteral("options.txt"),
                                        QStringLiteral("servers.dat"),   QStringLiteral("command_history.txt"),
                                        QStringLiteral("hotbar.nbt") };
    const auto segments = normalized.split('/');
    bool unsafeSegment = false;
    static const QRegularExpression windowsReserved(QStringLiteral(R"(^(con|prn|aux|nul|com[1-9]|lpt[1-9])(\..*)?$)"),
                                                    QRegularExpression::CaseInsensitiveOption);
    for (const auto& segment : segments) {
        unsafeSegment = unsafeSegment || segment.isEmpty() || segment.endsWith('.') || segment.endsWith(' ') ||
                        windowsReserved.match(segment).hasMatch();
    }
    const bool invalid = normalized != relativePath || normalized.isEmpty() || QDir::isAbsolutePath(normalized) ||
                         clean == QStringLiteral(".") || clean == QStringLiteral("..") || clean.startsWith(QStringLiteral("../")) ||
                         clean != normalized || blocked.contains(first) || normalized.contains(QChar::Null) || normalized.contains(':') ||
                         unsafeSegment;
    if (invalid && error) {
        *error = QObject::tr("%1 is not a safe shared path. Paths must stay inside the game folder; mods and saves cannot be shared.")
                     .arg(relativePath);
    }
    return !invalid;
}

QString Manager::serializeCustomPath(const CustomPath& path)
{
    return QStringLiteral("%1:%2").arg(path.directory ? QStringLiteral("dir") : QStringLiteral("file"), path.relativePath);
}

CustomPath Manager::deserializeCustomPath(const QString& value)
{
    auto withoutTrailingSeparators = [](QString path) {
        path.replace('\\', '/');
        while (path.size() > 1 && path.endsWith('/')) {
            path.chop(1);
        }
        return path;
    };
    if (value.startsWith(QStringLiteral("file:"), Qt::CaseInsensitive)) {
        return { withoutTrailingSeparators(value.mid(5)), false };
    }
    if (value.startsWith(QStringLiteral("dir:"), Qt::CaseInsensitive)) {
        return { withoutTrailingSeparators(value.mid(4)), true };
    }
    return { withoutTrailingSeparators(value), true };
}

void Manager::registerInstanceSettings(MinecraftInstance* instance)
{
    if (!instance) {
        return;
    }
    auto settings = instance->settings();
    settings->getOrRegisterSetting(GROUP_SETTING, QString());
    settings->getOrRegisterSetting(CATEGORY_SETTING, QStringList());
    settings->getOrRegisterSetting(CUSTOM_PATH_SETTING, QStringList());
    settings->getOrRegisterSetting(EXCLUDED_OPTIONS_SETTING, QStringList());
    settings->getOrRegisterSetting(DATA_PACKS_PATH_SETTING, QString());
}

QString Manager::instanceGroup(MinecraftInstance* instance) const
{
    registerInstanceSettings(instance);
    return instance ? instance->settings()->get(GROUP_SETTING).toString() : QString();
}

Categories Manager::instanceCategories(MinecraftInstance* instance) const
{
    registerInstanceSettings(instance);
    return instance ? deserializeCategories(instance->settings()->get(CATEGORY_SETTING).toStringList()) : Categories();
}

QList<CustomPath> Manager::instanceCustomPaths(MinecraftInstance* instance) const
{
    registerInstanceSettings(instance);
    QList<CustomPath> result;
    if (!instance) {
        return result;
    }
    for (const auto& value : instance->settings()->get(CUSTOM_PATH_SETTING).toStringList()) {
        auto path = deserializeCustomPath(value);
        if (validateCustomPath(path.relativePath)) {
            result.append(std::move(path));
        }
    }
    return result;
}

QStringList Manager::instanceExcludedOptions(MinecraftInstance* instance) const
{
    registerInstanceSettings(instance);
    return instance ? instance->settings()->get(EXCLUDED_OPTIONS_SETTING).toStringList() : QStringList();
}

bool Manager::ensureGroup(const QString& name, QString* error) const
{
    if (!isValidGroupName(name, error)) {
        return false;
    }
    const auto path = groupPath(name);
    if (path.isEmpty() || !QFileInfo(path).isDir() || QFileInfo(path).isSymLink() ||
        QFileInfo(FS::PathCombine(path, QStringLiteral("minecraft"))).isSymLink()) {
        if (error) {
            *error = QObject::tr("Shared content group %1 does not exist.").arg(name);
        }
        return false;
    }
    return true;
}

bool Manager::validatePathLayout(MinecraftInstance* instance,
                                 const QString& group,
                                 Categories categories,
                                 const QList<CustomPath>& customPaths,
                                 QString* error) const
{
    const auto gameRoot = QFileInfo(instance->gameRoot()).canonicalFilePath();
    const auto sharedRoot = QFileInfo(groupPath(group)).canonicalFilePath();
    if (pathsOverlap(gameRoot.isEmpty() ? instance->gameRoot() : gameRoot, true, sharedRoot.isEmpty() ? groupPath(group) : sharedRoot,
                     true)) {
        if (error) {
            *error = QObject::tr("The shared content group cannot be inside an instance game folder, or contain one.");
        }
        return false;
    }

    struct PlannedPath {
        QString location;
        QString label;
        bool directory;
    };
    QList<PlannedPath> plannedPaths;
    for (const auto category : s_categories) {
        if (!categories.testFlag(category)) {
            continue;
        }
        const QString local = localPath(instance, category);
        if (category == Category::GlobalDataPacks) {
            const auto localRoot = QDir::fromNativeSeparators(QDir(instance->gameRoot()).relativeFilePath(QDir::cleanPath(local)))
                                       .section('/', 0, 0)
                                       .toLower();
            if (localRoot == QStringLiteral("saves") || localRoot == QStringLiteral("mods") || localRoot == QStringLiteral("coremods") ||
                localRoot == QStringLiteral("nilmods")) {
                if (error) {
                    *error = QObject::tr("Global data packs cannot use a mods or saves folder.");
                }
                return false;
            }
        }
        plannedPaths.append({ local, categoryId(category), isDirectoryCategory(category) });
    }
    for (const auto& path : customPaths) {
        plannedPaths.append({ FS::PathCombine(instance->gameRoot(), path.relativePath), path.relativePath, path.directory });
    }
    for (qsizetype i = 0; i < plannedPaths.size(); ++i) {
        for (qsizetype j = i + 1; j < plannedPaths.size(); ++j) {
            const auto& left = plannedPaths.at(i);
            const auto& right = plannedPaths.at(j);
            if (pathsOverlap(left.location, left.directory, right.location, right.directory)) {
                if (error) {
                    *error = QObject::tr("Shared paths %1 and %2 overlap. Choose separate locations.").arg(left.label, right.label);
                }
                return false;
            }
        }
    }
    return true;
}

bool Manager::configureInstance(MinecraftInstance* instance,
                                const QString& group,
                                Categories categories,
                                const QList<CustomPath>& customPaths,
                                const QStringList& excludedOptions,
                                MigrationPolicy policy,
                                QString* error)
{
    if (!instance || !ensureGroup(group, error)) {
        return false;
    }
    const QString previousGroup = instanceGroup(instance);
    if (instance->isRunning() || m_lockGroups.values().contains(group) ||
        (!previousGroup.isEmpty() && m_lockGroups.values().contains(previousGroup))) {
        if (error) {
            *error = QObject::tr("Stop every instance using %1 before changing shared content.").arg(group);
        }
        return false;
    }
    QLockFile groupLock(groupLockPath(group));
    if (!groupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(group);
        }
        return false;
    }
    QLockFile previousGroupLock(groupLockPath(previousGroup));
    if (!previousGroup.isEmpty() && previousGroup != group && !previousGroupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(previousGroup);
        }
        return false;
    }
    for (const auto& path : customPaths) {
        if (!validateCustomPath(path.relativePath, error) ||
            !isInside(instance->gameRoot(), FS::PathCombine(instance->gameRoot(), path.relativePath))) {
            if (error && error->isEmpty()) {
                *error = QObject::tr("The custom path %1 goes through a link outside this instance.").arg(path.relativePath);
            }
            return false;
        }
        for (const auto& other : customPaths) {
            if (&path != &other &&
                (path.relativePath == other.relativePath || (other.directory && path.relativePath.startsWith(other.relativePath + '/')))) {
                if (error) {
                    *error = QObject::tr("Custom shared paths cannot overlap: %1 and %2.").arg(path.relativePath, other.relativePath);
                }
                return false;
            }
        }
    }
    if (!validatePathLayout(instance, group, categories, customPaths, error)) {
        return false;
    }
    registerInstanceSettings(instance);
    QStringList serializedPaths;
    for (const auto& path : customPaths) {
        serializedPaths.append(serializeCustomPath(path));
    }

    auto settings = instance->settings();
    const auto oldGroup = settings->get(GROUP_SETTING);
    const auto oldCategories = settings->get(CATEGORY_SETTING);
    const auto oldCustomPaths = settings->get(CUSTOM_PATH_SETTING);
    const auto oldExcludedOptions = settings->get(EXCLUDED_OPTIONS_SETTING);
    const auto oldDataPacksPath = settings->get(DATA_PACKS_PATH_SETTING);
    const QString oldGroupName = oldGroup.toString();
    const Categories oldCategoryFlags = deserializeCategories(oldCategories.toStringList());
    const QString currentDataPacksPath = QDir::cleanPath(localPath(instance, Category::GlobalDataPacks));
    QString previousDataPacksPath = currentDataPacksPath;
    if (oldCategoryFlags.testFlag(Category::GlobalDataPacks) &&
        !resolveStoredDataPacksPath(instance, oldDataPacksPath.toString(), sharedPath(oldGroupName, Category::GlobalDataPacks),
                                    &previousDataPacksPath, error)) {
        return false;
    }
    if (oldCategoryFlags.testFlag(Category::GlobalDataPacks) && categories.testFlag(Category::GlobalDataPacks) &&
        !pathLocationsEqual(previousDataPacksPath, currentDataPacksPath)) {
        if (error) {
            *error = QObject::tr("The global data packs path changed while it was shared. Disconnect shared content before changing it.");
        }
        return false;
    }
    QList<CustomPath> oldCustomPathList;
    for (const auto& value : oldCustomPaths.toStringList()) {
        const auto path = deserializeCustomPath(value);
        if (validateCustomPath(path.relativePath)) {
            oldCustomPathList.append(path);
        }
    }

    for (const auto category : s_categories) {
        if (!categories.testFlag(category)) {
            continue;
        }
        const QString local = localPath(instance, category);
        const QString shared = sharedPath(group, category);
        if (!isInside(instance->gameRoot(), local) || !isInside(FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), shared) ||
            QFileInfo(shared).isSymLink() ||
            (isDirectoryCategory(category) ? (QFileInfo(local).exists() && !QFileInfo(local).isDir() && !QFileInfo(local).isSymLink())
                                           : QFileInfo(local).isSymLink())) {
            if (error) {
                *error = QObject::tr("The location for %1 is not safe to share.").arg(categoryId(category));
            }
            return false;
        }
        if (isDirectoryCategory(category) && QFileInfo(local).isSymLink()) {
            const QString target = QFileInfo(local).symLinkTarget();
            if (!pathsEqual(target, shared) &&
                !(oldCategoryFlags.testFlag(category) && pathsEqual(target, sharedPath(oldGroupName, category)))) {
                if (error) {
                    *error = QObject::tr("The folder %1 is already linked elsewhere.").arg(local);
                }
                return false;
            }
        }
    }
    for (const auto& path : customPaths) {
        const auto local = FS::PathCombine(instance->gameRoot(), path.relativePath);
        const auto shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), path.relativePath);
        if (!isInside(FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), shared) || QFileInfo(shared).isSymLink() ||
            (!path.directory && QFileInfo(local).isSymLink())) {
            if (error) {
                *error = QObject::tr("The custom path %1 is not safe to share.").arg(path.relativePath);
            }
            return false;
        }
        if (path.directory && QFileInfo(local).isSymLink()) {
            const auto oldShared = FS::PathCombine(groupPath(oldGroupName), QStringLiteral("minecraft/custom"), path.relativePath);
            bool wasShared = false;
            for (const auto& oldPath : oldCustomPathList) {
                wasShared = wasShared || (oldPath.directory && oldPath.relativePath == path.relativePath);
            }
            if (!pathsEqual(QFileInfo(local).symLinkTarget(), shared) &&
                !(wasShared && pathsEqual(QFileInfo(local).symLinkTarget(), oldShared))) {
                if (error) {
                    *error = QObject::tr("The custom folder %1 is already linked elsewhere.").arg(local);
                }
                return false;
            }
        }
    }

    if (!oldGroupName.isEmpty()) {
        for (const auto category : s_categories) {
            if (!oldCategoryFlags.testFlag(category) || !isDirectoryCategory(category) ||
                (oldGroupName == group && categories.testFlag(category))) {
                continue;
            }
            const QString local = category == Category::GlobalDataPacks ? previousDataPacksPath : localPath(instance, category);
            const QString oldShared = sharedPath(oldGroupName, category);
            const QFileInfo localInfo(local);
            if (localInfo.isSymLink() && pathsEqual(localInfo.symLinkTarget(), oldShared)) {
                QString source = oldShared;
                const QString renamedSource = sharedPath(group, category);
                if (!QFileInfo::exists(source) && QFileInfo::exists(renamedSource)) {
                    source = renamedSource;
                }
                if (!detachDirectoryLink(local, source, true, error)) {
                    return false;
                }
            }
        }
        for (const auto& oldPath : oldCustomPathList) {
            if (!oldPath.directory) {
                continue;
            }
            bool retained = false;
            if (oldGroupName == group) {
                for (const auto& newPath : customPaths) {
                    retained = retained || (newPath.directory && newPath.relativePath == oldPath.relativePath);
                }
            }
            if (retained) {
                continue;
            }
            const QString local = FS::PathCombine(instance->gameRoot(), oldPath.relativePath);
            const QString oldShared = FS::PathCombine(groupPath(oldGroupName), QStringLiteral("minecraft/custom"), oldPath.relativePath);
            const QFileInfo localInfo(local);
            if (localInfo.isSymLink() && pathsEqual(localInfo.symLinkTarget(), oldShared)) {
                QString source = oldShared;
                const QString renamedSource = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), oldPath.relativePath);
                if (!QFileInfo::exists(source) && QFileInfo::exists(renamedSource)) {
                    source = renamedSource;
                }
                if (!detachDirectoryLink(local, source, true, error)) {
                    return false;
                }
            }
        }

        for (const auto category : s_categories) {
            if (!oldCategoryFlags.testFlag(category) || (!isFileCategory(category) && category != Category::Options) ||
                (oldGroupName == group && categories.testFlag(category))) {
                continue;
            }
            QString source = sharedPath(oldGroupName, category);
            const QString renamedSource = sharedPath(group, category);
            if (!QFileInfo::exists(source) && QFileInfo::exists(renamedSource)) {
                source = renamedSource;
            }
            if (!QFileInfo::exists(source)) {
                continue;
            }
            if (category == Category::Options) {
                const auto oldExcludedList = oldExcludedOptions.toStringList();
                const QSet<QString> oldExcluded(oldExcludedList.cbegin(), oldExcludedList.cend());
                if (!SharedOptionsFile::overlayFile(source, localPath(instance, category), oldExcluded, error)) {
                    return false;
                }
            } else if (!copyFileAtomic(source, localPath(instance, category), error)) {
                return false;
            }
        }
    }

    auto restoreSettings = [&]() {
        SettingsObject::Lock lock(settings);
        settings->set(GROUP_SETTING, oldGroup);
        settings->set(CATEGORY_SETTING, oldCategories);
        settings->set(CUSTOM_PATH_SETTING, oldCustomPaths);
        settings->set(EXCLUDED_OPTIONS_SETTING, oldExcludedOptions);
        settings->set(DATA_PACKS_PATH_SETTING, oldDataPacksPath);
    };
    bool settingsSaved = false;
    {
        SettingsObject::Lock lock(settings);
        settingsSaved =
            settings->set(GROUP_SETTING, group) && settings->set(CATEGORY_SETTING, serializeCategories(categories)) &&
            settings->set(CUSTOM_PATH_SETTING, serializedPaths) && settings->set(EXCLUDED_OPTIONS_SETTING, excludedOptions) &&
            settings->set(DATA_PACKS_PATH_SETTING,
                          categories.testFlag(Category::GlobalDataPacks)
                              ? QDir::fromNativeSeparators(QDir(instance->gameRoot()).relativeFilePath(currentDataPacksPath))
                              : QString());
    }
    if (!settingsSaved) {
        restoreSettings();
        if (error) {
            *error = QObject::tr("Could not save shared content settings for %1.").arg(instance->name());
        }
        return false;
    }
    auto incomplete = [&]() {
        if (error) {
            *error += QObject::tr(" Sharing settings were saved, but setup is incomplete. Fix the problem and apply again.");
        }
        return false;
    };
    if (!reconcileDirectories(instance, policy, error)) {
        return incomplete();
    }

    const auto excludedList = instanceExcludedOptions(instance);
    const QSet<QString> exclusions(excludedList.cbegin(), excludedList.cend());
    if (categories.testFlag(Category::Options)) {
        const QString local = localPath(instance, Category::Options);
        const QString shared = sharedPath(group, Category::Options);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local, shared)) {
            if (error) {
                *error = QObject::tr("Game options must be regular files, not symbolic links.");
            }
            return incomplete();
        }
        const bool ok = policy == MigrationPolicy::PreferInstance && QFileInfo::exists(local)
                            ? SharedOptionsFile::updateSharedFile(local, shared, exclusions, error)
                            : (QFileInfo::exists(shared)
                                   ? SharedOptionsFile::overlayFile(shared, local, exclusions, error)
                                   : (!QFileInfo::exists(local) || SharedOptionsFile::updateSharedFile(local, shared, exclusions, error)));
        if (!ok) {
            return incomplete();
        }
    }
    for (const auto category : s_categories) {
        if (!categories.testFlag(category) || !isFileCategory(category)) {
            continue;
        }
        const QString local = localPath(instance, category);
        const QString shared = sharedPath(group, category);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local, shared)) {
            if (error) {
                *error = QObject::tr("The shared file %1 is a symbolic link.").arg(categoryId(category));
            }
            return incomplete();
        }
        const bool ok = policy == MigrationPolicy::PreferInstance && QFileInfo::exists(local) ? finishFile(local, shared, error)
                                                                                              : prepareFile(local, shared, error);
        if (!ok) {
            return incomplete();
        }
    }
    for (const auto& path : customPaths) {
        if (path.directory) {
            continue;
        }
        const QString local = FS::PathCombine(instance->gameRoot(), path.relativePath);
        const QString shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), path.relativePath);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local, shared)) {
            if (error) {
                *error = QObject::tr("The custom file %1 is a symbolic link.").arg(path.relativePath);
            }
            return incomplete();
        }
        const bool ok = policy == MigrationPolicy::PreferInstance && QFileInfo::exists(local) ? finishFile(local, shared, error)
                                                                                              : prepareFile(local, shared, error);
        if (!ok) {
            return incomplete();
        }
    }
    if (!oldGroupName.isEmpty() && oldGroupName != group &&
        !updateAllowedSymlinks(instance, FS::PathCombine(groupPath(oldGroupName), QStringLiteral("minecraft")), false, error)) {
        return incomplete();
    }
    if (categories.testFlag(Category::GlobalDataPacks)) {
        settings->set("GlobalDataPacksEnabled", true);
    }
    return true;
}

QString Manager::localPath(MinecraftInstance* instance, Category category) const
{
    if (!instance) {
        return {};
    }
    if (category == Category::GlobalDataPacks) {
        return instance->dataPacksDir();
    }
    return FS::PathCombine(instance->gameRoot(), categoryRelativePath(category));
}

QString Manager::sharedPath(const QString& group, Category category) const
{
    return FS::PathCombine(groupPath(group), QStringLiteral("minecraft"), categoryRelativePath(category));
}

QString Manager::effectivePath(MinecraftInstance* instance, Category category) const
{
    const auto categories = instanceCategories(instance);
    const auto group = instanceGroup(instance);
    if (!group.isEmpty() && categories.testFlag(category)) {
        return sharedPath(group, category);
    }
    return localPath(instance, category);
}

QString Manager::effectiveCustomPath(MinecraftInstance* instance, const CustomPath& path) const
{
    if (!instance || !validateCustomPath(path.relativePath)) {
        return {};
    }
    const auto group = instanceGroup(instance);
    if (group.isEmpty()) {
        return FS::PathCombine(instance->gameRoot(), path.relativePath);
    }
    return FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), path.relativePath);
}

bool Manager::reconcileDirectory(const QString& local,
                                 const QString& shared,
                                 const QString& backupRoot,
                                 MigrationPolicy policy,
                                 QString* error) const
{
    if (QFileInfo(shared).isSymLink() || !FS::ensureFolderPathExists(shared)) {
        if (error) {
            *error = QObject::tr("Could not create shared folder %1.").arg(shared);
        }
        return false;
    }

    const QFileInfo localInfo(local);
    if (localInfo.isSymLink()) {
        if (pathsEqual(localInfo.symLinkTarget(), shared)) {
            return true;
        }
        if (error) {
            *error = QObject::tr("The folder %1 is already linked elsewhere. Remove that link manually before sharing it.").arg(local);
        }
        return false;
    } else if (localInfo.exists()) {
        if (!localInfo.isDir()) {
            if (error) {
                *error = QObject::tr("Expected a folder at %1.").arg(local);
            }
            return false;
        }

        const QString backup = FS::PathCombine(backupRoot, QFileInfo(local).fileName());
        if (!copyDirectory(local, backup, true)) {
            if (error) {
                *error = QObject::tr("Could not back up %1 before sharing it.").arg(local);
            }
            return false;
        }
        const bool overwrite = policy == MigrationPolicy::PreferInstance;
        if (!copyDirectory(local, shared, overwrite)) {
            if (error) {
                *error = QObject::tr("Could not merge %1 into %2.").arg(local, shared);
            }
            return false;
        }
        if (!FS::deletePath(local)) {
            if (error) {
                *error = QObject::tr("Could not replace %1 with a shared-folder link.").arg(local);
            }
            return false;
        }
    }

    if (!makeDirectoryLink(shared, local, error)) {
        copyDirectory(shared, local, true);
        return false;
    }
    return true;
}

bool Manager::updateAllowedSymlinks(MinecraftInstance* instance, const QString& path, bool add, QString* error) const
{
    const QString filePath = FS::PathCombine(instance->gameRoot(), QStringLiteral("allowed_symlinks.txt"));
    QStringList lines;
    if (QFileInfo::exists(filePath)) {
        const auto contents = FS::read(filePath);
        if (!contents) {
            if (error) {
                *error = contents.error();
            }
            return false;
        }
        lines = QString::fromUtf8(contents.value()).split('\n', Qt::SkipEmptyParts);
    }
    const QString cleanPath = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    lines.removeIf([&](const QString& line) { return pathsEqual(line.trimmed(), cleanPath); });
    if (add) {
        lines.append(cleanPath);
    }
    if (QFileInfo(filePath).isSymLink()) {
        if (error) {
            *error = QObject::tr("The existing allowed_symlinks.txt is linked elsewhere. Remove that link manually.");
        }
        return false;
    }
    QByteArray output;
    if (!lines.isEmpty()) {
        output = lines.join('\n').toUtf8() + '\n';
    }
    const auto result = FS::write(filePath, output);
    if (!result && error) {
        *error = result.error();
    }
    return result.has_value();
}

bool Manager::reconcileDirectories(MinecraftInstance* instance, MigrationPolicy policy, QString* error)
{
    if (!instance) {
        if (error) {
            *error = QObject::tr("No instance was supplied.");
        }
        return false;
    }
    const QString group = instanceGroup(instance);
    if (!ensureGroup(group, error)) {
        return false;
    }
    const Categories categories = instanceCategories(instance);
    if (!validatePathLayout(instance, group, categories, instanceCustomPaths(instance), error)) {
        return false;
    }
    if (categories.testFlag(Category::GlobalDataPacks)) {
        QString configuredPath;
        if (!resolveStoredDataPacksPath(instance, instance->settings()->get(DATA_PACKS_PATH_SETTING).toString(),
                                        sharedPath(group, Category::GlobalDataPacks), &configuredPath, error)) {
            return false;
        }
        if (!pathLocationsEqual(configuredPath, localPath(instance, Category::GlobalDataPacks))) {
            if (error) {
                *error =
                    QObject::tr("The global data packs path changed while it was shared. Disconnect shared content before changing it.");
            }
            return false;
        }
    }
    const QString backupRoot =
        FS::PathCombine(instance->instanceRoot(), QStringLiteral(".prism-shared-backup"),
                        QStringLiteral("%1-%2").arg(QDateTime::currentMSecsSinceEpoch()).arg(QUuid::createUuid().toString(QUuid::Id128)));

    for (const auto category : s_categories) {
        if (!categories.testFlag(category) || !isDirectoryCategory(category)) {
            continue;
        }
        const auto local = localPath(instance, category);
        if (!isInside(instance->gameRoot(), local) ||
            !isInside(FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), sharedPath(group, category))) {
            if (error) {
                *error = QObject::tr("The path for %1 is outside this instance and cannot be shared.").arg(categoryId(category));
            }
            return false;
        }
        if (!reconcileDirectory(local, sharedPath(group, category), FS::PathCombine(backupRoot, categoryId(category)), policy, error)) {
            return false;
        }
    }

    for (const auto& custom : instanceCustomPaths(instance)) {
        if (!custom.directory) {
            continue;
        }
        const auto local = FS::PathCombine(instance->gameRoot(), custom.relativePath);
        const auto shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath);
        if (!isInside(instance->gameRoot(), local) || !isInside(FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), shared)) {
            if (error) {
                *error = QObject::tr("The custom path %1 goes through a link outside this instance.").arg(custom.relativePath);
            }
            return false;
        }
        const QString customBackup = FS::PathCombine(backupRoot, QStringLiteral("custom"), QFileInfo(custom.relativePath).path());
        if (!reconcileDirectory(local, shared, customBackup, policy, error)) {
            return false;
        }
    }
    if (!updateAllowedSymlinks(instance, FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), true, error)) {
        return false;
    }
    if (categories.testFlag(Category::GlobalDataPacks)) {
        const QString relative = QDir::fromNativeSeparators(
            QDir(instance->gameRoot()).relativeFilePath(localPath(instance, Category::GlobalDataPacks)));
        if (!instance->settings()->set(DATA_PACKS_PATH_SETTING, relative)) {
            if (error) {
                *error = QObject::tr("Could not update the shared global data packs path for %1.").arg(instance->name());
            }
            return false;
        }
    }
    return true;
}

bool Manager::repairInstance(MinecraftInstance* instance, QString* error)
{
    if (!instance) {
        if (error) {
            *error = QObject::tr("No instance was supplied.");
        }
        return false;
    }
    const QString group = instanceGroup(instance);
    if (!ensureGroup(group, error)) {
        return false;
    }
    if (instance->isRunning() || m_lockGroups.values().contains(group)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(group);
        }
        return false;
    }
    QLockFile groupLock(groupLockPath(group));
    if (!groupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(group);
        }
        return false;
    }
    return reconcileDirectories(instance, MigrationPolicy::PreferShared, error);
}

bool Manager::prepareFile(const QString& local, const QString& shared, QString* error) const
{
    if (QFileInfo::exists(shared)) {
        return copyFileAtomic(shared, local, error);
    }
    if (QFileInfo::exists(local)) {
        return copyFileAtomic(local, shared, error);
    }
    return true;
}

bool Manager::finishFile(const QString& local, const QString& shared, QString* error) const
{
    if (!QFileInfo::exists(local)) {
        return true;
    }
    return copyFileAtomic(local, shared, error);
}

QString Manager::lockKey(MinecraftInstance* instance) const
{
    const auto uuid = instance ? instance->uuid() : QString();
    return uuid.isEmpty() && instance ? instance->id() : uuid;
}

QString Manager::groupLockPath(const QString& group) const
{
#ifdef Q_OS_WIN
    const auto key = group.toCaseFolded().toUtf8();
#else
    const auto key = group.toUtf8();
#endif
    const auto hash = QCryptographicHash::hash(key, QCryptographicHash::Sha256).toHex();
    return FS::PathCombine(m_rootPath, QStringLiteral(".launch-%1.lock").arg(QString::fromLatin1(hash)));
}

bool Manager::prepare(MinecraftInstance* instance, QString* error)
{
    if (!instance) {
        if (error) {
            *error = QObject::tr("No instance was supplied.");
        }
        return false;
    }
    const QString group = instanceGroup(instance);
    if (!ensureGroup(group, error)) {
        return false;
    }
    const Categories categories = instanceCategories(instance);
    const auto customPaths = instanceCustomPaths(instance);
    const bool needsLock = categories != Categories() || !customPaths.isEmpty();
    const QString key = lockKey(instance);
    if (needsLock && !m_locks.contains(key)) {
        auto lock = QSharedPointer<QLockFile>::create(groupLockPath(group));
        lock->setStaleLockTime(30000);
        if (!lock->tryLock()) {
            if (error) {
                qint64 pid = 0;
                QString host;
                QString application;
                lock->getLockInfo(&pid, &host, &application);
                *error = QObject::tr("Shared content group %1 is already in use by %2 (process %3 on %4).")
                             .arg(group, application, QString::number(pid), host);
            }
            return false;
        }
        m_locks.insert(key, lock);
        m_lockGroups.insert(key, group);
    }

    if (!reconcileDirectories(instance, MigrationPolicy::PreferShared, error)) {
        cancel(instance);
        return false;
    }

    if (categories.testFlag(Category::Options)) {
        const QString local = localPath(instance, Category::Options);
        const QString shared = sharedPath(group, Category::Options);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local, shared)) {
            if (error) {
                *error = QObject::tr("Game options must be regular files, not symbolic links.");
            }
            cancel(instance);
            return false;
        }
        const auto excludedList = instanceExcludedOptions(instance);
        const QSet<QString> excluded(excludedList.cbegin(), excludedList.cend());
        if (QFileInfo::exists(shared)) {
            if (!SharedOptionsFile::overlayFile(shared, local, excluded, error)) {
                cancel(instance);
                return false;
            }
        } else if (QFileInfo::exists(local) && !SharedOptionsFile::updateSharedFile(local, shared, excluded, error)) {
            cancel(instance);
            return false;
        }
    }

    for (const auto category : s_categories) {
        if (categories.testFlag(category) && isFileCategory(category) &&
            !safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")),
                          localPath(instance, category), sharedPath(group, category))) {
            if (error) {
                *error = QObject::tr("The shared file %1 is a symbolic link.").arg(categoryId(category));
            }
            cancel(instance);
            return false;
        }
        if (categories.testFlag(category) && isFileCategory(category) &&
            !prepareFile(localPath(instance, category), sharedPath(group, category), error)) {
            cancel(instance);
            return false;
        }
    }
    for (const auto& custom : customPaths) {
        if (custom.directory) {
            continue;
        }
        const auto local = FS::PathCombine(instance->gameRoot(), custom.relativePath);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local,
                          FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath))) {
            if (error) {
                *error = QObject::tr("The custom file %1 is linked outside this instance.").arg(custom.relativePath);
            }
            cancel(instance);
            return false;
        }
        const auto shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath);
        if (!prepareFile(local, shared, error)) {
            cancel(instance);
            return false;
        }
    }
    return true;
}

bool Manager::finish(MinecraftInstance* instance, QString* error)
{
    if (!instance) {
        return false;
    }
    const QString group = instanceGroup(instance);
    if (!ensureGroup(group, error)) {
        cancel(instance);
        return false;
    }
    const Categories categories = instanceCategories(instance);
    bool succeeded = true;
    QString firstError;
    if (categories.testFlag(Category::Options) && QFileInfo::exists(localPath(instance, Category::Options))) {
        QString currentError;
        const auto excludedList = instanceExcludedOptions(instance);
        const QSet<QString> excluded(excludedList.cbegin(), excludedList.cend());
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")),
                          localPath(instance, Category::Options), sharedPath(group, Category::Options)) ||
            !SharedOptionsFile::updateSharedFile(localPath(instance, Category::Options), sharedPath(group, Category::Options), excluded,
                                                 &currentError)) {
            succeeded = false;
            firstError = currentError.isEmpty() ? QObject::tr("The game options path is linked outside this instance.") : currentError;
        }
    }
    for (const auto category : s_categories) {
        QString currentError;
        if (categories.testFlag(category) && isFileCategory(category) &&
            (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")),
                           localPath(instance, category), sharedPath(group, category)) ||
             !finishFile(localPath(instance, category), sharedPath(group, category), &currentError))) {
            succeeded = false;
            if (firstError.isEmpty()) {
                firstError =
                    currentError.isEmpty() ? QObject::tr("A shared game file path is linked outside this instance.") : currentError;
            }
        }
    }
    for (const auto& custom : instanceCustomPaths(instance)) {
        if (custom.directory) {
            continue;
        }
        QString currentError;
        const auto local = FS::PathCombine(instance->gameRoot(), custom.relativePath);
        const auto shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath);
        if (!safeFilePair(instance->gameRoot(), FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), local, shared) ||
            !finishFile(local, shared, &currentError)) {
            succeeded = false;
            if (firstError.isEmpty()) {
                firstError =
                    currentError.isEmpty() ? QObject::tr("A custom shared file path is linked outside this instance.") : currentError;
            }
        }
    }
    cancel(instance);
    if (!succeeded && error) {
        *error = firstError;
    }
    return succeeded;
}

void Manager::cancel(MinecraftInstance* instance)
{
    const auto key = lockKey(instance);
    auto lock = m_locks.take(key);
    m_lockGroups.remove(key);
    if (lock) {
        lock->unlock();
    }
}

bool Manager::disconnectInstance(MinecraftInstance* instance, bool copySharedContentBack, QString* error)
{
    if (!instance) {
        return false;
    }
    const QString group = instanceGroup(instance);
    if (instance->isRunning() || m_lockGroups.values().contains(group)) {
        if (error) {
            *error = QObject::tr("Stop every instance using %1 before disconnecting shared content.").arg(group);
        }
        return false;
    }
    QLockFile groupLock(groupLockPath(group));
    if (!groupLock.tryLock(0)) {
        if (error) {
            *error = QObject::tr("Shared content group %1 is currently in use.").arg(group);
        }
        return false;
    }
    const Categories categories = instanceCategories(instance);
    const auto sharedRoot = FS::PathCombine(groupPath(group), QStringLiteral("minecraft"));
    QString configuredDataPacksPath;
    if (categories.testFlag(Category::GlobalDataPacks) &&
        !resolveStoredDataPacksPath(instance, instance->settings()->get(DATA_PACKS_PATH_SETTING).toString(),
                                    sharedPath(group, Category::GlobalDataPacks), &configuredDataPacksPath, error)) {
        return false;
    }
    for (const auto category : s_categories) {
        if (categories.testFlag(category) && (category == Category::Options || isFileCategory(category)) &&
            !safeFilePair(instance->gameRoot(), sharedRoot, localPath(instance, category), sharedPath(group, category))) {
            if (error) {
                *error = QObject::tr("The file path for %1 is not safe to disconnect.").arg(categoryId(category));
            }
            return false;
        }
    }
    for (const auto& custom : instanceCustomPaths(instance)) {
        if (custom.directory) {
            continue;
        }
        if (!safeFilePair(instance->gameRoot(), sharedRoot, FS::PathCombine(instance->gameRoot(), custom.relativePath),
                          FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath))) {
            if (error) {
                *error = QObject::tr("The custom file %1 is not safe to disconnect.").arg(custom.relativePath);
            }
            return false;
        }
    }
    if (copySharedContentBack) {
        if (categories.testFlag(Category::Options) && QFileInfo::exists(sharedPath(group, Category::Options))) {
            const auto excludedList = instanceExcludedOptions(instance);
            const QSet<QString> excluded(excludedList.cbegin(), excludedList.cend());
            if (!SharedOptionsFile::overlayFile(sharedPath(group, Category::Options), localPath(instance, Category::Options), excluded,
                                                error)) {
                return false;
            }
        }
        for (const auto category : s_categories) {
            if (categories.testFlag(category) && isFileCategory(category) && QFileInfo::exists(sharedPath(group, category)) &&
                !copyFileAtomic(sharedPath(group, category), localPath(instance, category), error)) {
                return false;
            }
        }
        for (const auto& custom : instanceCustomPaths(instance)) {
            if (custom.directory) {
                continue;
            }
            const auto shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath);
            if (QFileInfo::exists(shared) && !copyFileAtomic(shared, FS::PathCombine(instance->gameRoot(), custom.relativePath), error)) {
                return false;
            }
        }
    }
    for (const auto category : s_categories) {
        if (!categories.testFlag(category) || !isDirectoryCategory(category)) {
            continue;
        }
        const QString local = category == Category::GlobalDataPacks ? configuredDataPacksPath : localPath(instance, category);
        const QString shared = sharedPath(group, category);
        const QFileInfo localInfo(local);
        if (localInfo.isSymLink() && pathsEqual(localInfo.symLinkTarget(), shared)) {
            if (!detachDirectoryLink(local, shared, copySharedContentBack, error)) {
                if (error) {
                    *error = QObject::tr("Could not restore the local folder %1.").arg(local);
                }
                return false;
            }
        }
    }
    for (const auto& custom : instanceCustomPaths(instance)) {
        if (!custom.directory) {
            continue;
        }
        const QString local = FS::PathCombine(instance->gameRoot(), custom.relativePath);
        const QString shared = FS::PathCombine(groupPath(group), QStringLiteral("minecraft/custom"), custom.relativePath);
        const QFileInfo localInfo(local);
        if (localInfo.isSymLink() && pathsEqual(localInfo.symLinkTarget(), shared)) {
            if (!detachDirectoryLink(local, shared, copySharedContentBack, error)) {
                if (error) {
                    *error = QObject::tr("Could not restore the local folder %1.").arg(local);
                }
                return false;
            }
        }
    }
    if (!updateAllowedSymlinks(instance, FS::PathCombine(groupPath(group), QStringLiteral("minecraft")), false, error)) {
        return false;
    }
    SettingsObject::Lock lock(instance->settings());
    instance->settings()->set(GROUP_SETTING, QString());
    instance->settings()->set(CATEGORY_SETTING, QStringList());
    instance->settings()->set(CUSTOM_PATH_SETTING, QStringList());
    instance->settings()->set(EXCLUDED_OPTIONS_SETTING, QStringList());
    instance->settings()->set(DATA_PACKS_PATH_SETTING, QString());
    return true;
}

}
