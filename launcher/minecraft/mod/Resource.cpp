#include "Resource.h"

#include <QDirIterator>
#include <QFileInfo>
#include <QRegularExpression>
#include <tuple>
#include <utility>

#include "FileSystem.h"
#include "StringUtils.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"

Resource::Resource(QObject* parent) : QObject(parent), m_size_info(0) {}

Resource::Resource(QFileInfo fileInfo) : m_size_info(0)
{
    setFile(fileInfo);
}

void Resource::setFile(QFileInfo fileInfo)
{
    m_file_info = std::move(fileInfo);
    parseFile();
}

namespace {
std::tuple<QString, qint64> calculateFileSize(const QFileInfo& file)
{
    if (file.isDir()) {
        auto dir = QDir(file.absoluteFilePath());
        dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
        auto count = dir.count();
        auto str = QObject::tr("item");
        if (count != 1) {
            str = QObject::tr("items");
        }
        return { QString("%1 %2").arg(QString::number(count), str), count };
    }
    return { StringUtils::humanReadableFileSize(file.size(), true), file.size() };
}
}  // namespace

void Resource::parseFile()
{
    QString fileName{ m_file_info.fileName() };

    m_type = ResourceType::UNKNOWN;

    m_internal_id = fileName;

    std::tie(m_size_str, m_size_info) = calculateFileSize(m_file_info);
    if (m_file_info.isDir()) {
        m_type = ResourceType::FOLDER;
        m_name = fileName;
    } else if (m_file_info.isFile()) {
        if (fileName.endsWith(".disabled")) {
            fileName.chop(9);
            m_enabled = false;
        }

        if (fileName.endsWith(".zip") || fileName.endsWith(".jar")) {
            m_type = ResourceType::ZIPFILE;
            fileName.chop(4);
        } else if (fileName.endsWith(".nilmod")) {
            m_type = ResourceType::ZIPFILE;
            fileName.chop(7);
        } else if (fileName.endsWith(".litemod")) {
            m_type = ResourceType::LITEMOD;
            fileName.chop(8);
        } else {
            m_type = ResourceType::SINGLEFILE;
        }

        m_name = fileName;
    }

    m_changed_date_time = m_file_info.lastModified();
}

auto Resource::name() const -> QString
{
    if (metadata()) {
        return metadata()->name;
    }

    return m_name;
}

namespace {
void removeThePrefix(QString& string)
{
    static const QRegularExpression s_regex(QStringLiteral("^(?:the|teh) +"), QRegularExpression::CaseInsensitiveOption);
    string.remove(s_regex);
    string = string.trimmed();
}
}  // namespace

auto Resource::provider() const -> QString
{
    if (metadata()) {
        return ModPlatform::ProviderCapabilities::readableName(metadata()->provider);
    }

    return tr("Unknown");
}

auto Resource::homepage() const -> QString
{
    if (metadata()) {
        return ModPlatform::getMetaURL(metadata()->provider, metadata()->project_id);
    }

    return {};
}

void Resource::setMetadata(std::shared_ptr<Metadata::ModStruct>&& metadata)
{
    if (status() == ResourceStatus::NoMetadata) {
        setStatus(ResourceStatus::Installed);
    }

    m_metadata = metadata;
}

QStringList Resource::issues() const
{
    QStringList result;
    result.reserve(m_issues.length());

    for (const char* issue : m_issues) {
        result.append(tr(issue));
    }

    return result;
}

void Resource::updateIssues(const BaseInstance* inst)
{
    m_issues.clear();

    if (m_metadata == nullptr) {
        return;
    }

    const auto* mcInst = dynamic_cast<const MinecraftInstance*>(inst);
    if (mcInst == nullptr) {
        return;
    }

    auto* profile = mcInst->getPackProfile();
    QString mcVersion = profile->getComponentVersion("net.minecraft");

    if (!m_metadata->mcVersions.empty() && !m_metadata->mcVersions.contains(mcVersion)) {
        // delay translation until issues() is called
        m_issues.append(QT_TR_NOOP("Not marked as compatible with the instance's game version."));
    }
}

int Resource::compare(const Resource& other, SortType type) const
{
    switch (type) {
        default:
        case SortType::Enabled:
            if (enabled() && !other.enabled()) {
                return 1;
            }
            if (!enabled() && other.enabled()) {
                return -1;
            }
            break;
        case SortType::Name: {
            QString thisName{ name() };
            QString otherName{ other.name() };

            // TODO do we need this? it could result in 0 being returned
            removeThePrefix(thisName);
            removeThePrefix(otherName);

            return QString::compare(thisName, otherName, Qt::CaseInsensitive);
        }
        case SortType::Date:
            if (dateTimeChanged() > other.dateTimeChanged()) {
                return 1;
            }
            if (dateTimeChanged() < other.dateTimeChanged()) {
                return -1;
            }
            break;
        case SortType::Filename:
            return fileinfo().fileName().localeAwareCompare(other.fileinfo().fileName());

        case SortType::Size: {
            if (this->type() != other.type()) {
                if (this->type() == ResourceType::FOLDER) {
                    return -1;
                }
                if (other.type() == ResourceType::FOLDER) {
                    return 1;
                }
            }

            if (sizeInfo() > other.sizeInfo()) {
                return 1;
            }
            if (sizeInfo() < other.sizeInfo()) {
                return -1;
            }
            break;
        }

        case SortType::Provider: {
            auto compareResult = QString::compare(provider(), other.provider(), Qt::CaseInsensitive);
            if (compareResult != 0) {
                return compareResult;
            }
            break;
        }
    }

    return 0;
}

bool Resource::applyFilter(QRegularExpression filter) const
{
    if (filter.match(name()).hasMatch()) {
        return true;
    }
    if (filter.match(fileinfo().fileName()).hasMatch()) {
        return true;
    }
    return false;
}

bool Resource::enable(EnableAction action)
{
    if (m_type == ResourceType::UNKNOWN || m_type == ResourceType::FOLDER) {
        return false;
    }

    QString path = m_file_info.absoluteFilePath();
    QFile file(path);

    bool enable = true;
    switch (action) {
        case EnableAction::ENABLE:
            enable = true;
            break;
        case EnableAction::DISABLE:
            enable = false;
            break;
        case EnableAction::TOGGLE:
        default:
            enable = !enabled();
            break;
    }

    if (m_enabled == enable) {
        return false;
    }

    if (enable) {
        // m_enabled is false, but there's no '.disabled' suffix.
        // TODO: Report error?
        if (!path.endsWith(".disabled")) {
            return false;
        }
        path.chop(9);
    } else {
        path += ".disabled";
        if (QFile::exists(path)) {
            path = FS::getUniqueResourceName(path);
        }
    }
    if (!file.rename(path)) {
        return false;
    }

    setFile(QFileInfo(path));

    m_enabled = enable;
    return true;
}

auto Resource::destroy(const QDir& indexDir, bool preserveMetadata, bool attemptTrash) -> bool
{
    m_type = ResourceType::UNKNOWN;

    if (!preserveMetadata) {
        qDebug() << QString("Destroying metadata for '%1' on purpose").arg(name());
        destroyMetadata(indexDir);
    }

    return (attemptTrash && FS::trash(m_file_info.filePath())) || FS::deletePath(m_file_info.filePath());
}

auto Resource::destroyMetadata(const QDir& indexDir) -> void
{
    if (metadata()) {
        Metadata::remove(indexDir, metadata()->slug);
    } else {
        auto n = name();
        Metadata::remove(indexDir, n);
    }
    m_metadata = nullptr;
}

bool Resource::isSymLinkUnder(const QString& instPath) const
{
    if (isSymLink()) {
        return true;
    }

    auto instDir = QDir(instPath);

    auto relAbsPath = instDir.relativeFilePath(m_file_info.absoluteFilePath());
    auto relCanonPath = instDir.relativeFilePath(m_file_info.canonicalFilePath());

    return relAbsPath != relCanonPath;
}

bool Resource::isMoreThanOneHardLink() const
{
    return FS::hardLinkCount(m_file_info.absoluteFilePath()) > 1;
}

auto Resource::getOriginalFileName() const -> QString
{
    auto fileName = m_file_info.fileName();
    if (!m_enabled) {
        fileName.chop(9);
    }
    return fileName;
}

QDebug operator<<(QDebug debug, ResourceType type)
{
    switch (type) {
        case ResourceType::ZIPFILE:
            debug << "ZIPFILE";
            break;
        case ResourceType::SINGLEFILE:
            debug << "SINGLEFILE";
            break;
        case ResourceType::FOLDER:
            debug << "FOLDER";
            break;
        case ResourceType::LITEMOD:
            debug << "LITEMOD";
            break;
        case ResourceType::UNKNOWN:
        default:
            debug << "UNKNOWN";
            break;
    };
    return debug;
}

QDebug operator<<(QDebug debug, ResourceStatus status)
{
    switch (status) {
        case ResourceStatus::Installed:
            debug << "INSTALLED";
            break;
        case ResourceStatus::NotInstalled:
            debug << "NOT_INSTALLED";
            break;
        case ResourceStatus::NoMetadata:
            debug << "NO_METADATA";
            break;
        case ResourceStatus::Unknown:
        default:
            debug << "UNKNOWN";
            break;
    };
    return debug;
}
