#include "Resource.h"


#include <QRegularExpression>
#include <QFileInfo>

#include "FileSystem.h"

Resource::Resource(QObject* parent) : QObject(parent) {}

Resource::Resource(QFileInfo file_info) : QObject()
{
    setFile(file_info);
}

void Resource::setFile(QFileInfo file_info)
{
    m_file_info = file_info;
    parseFile();
}

void Resource::parseFile()
{
    QString file_name{ m_file_info.fileName() };

    m_type = ResourceType::UNKNOWN;

    m_internal_id = file_name;

    if (m_file_info.isDir()) {
        m_type = ResourceType::FOLDER;
        m_name = file_name;
    } else if (m_file_info.isFile()) {
        if (file_name.endsWith(".disabled")) {
            file_name.chop(9);
            m_enabled = false;
        }

        if (file_name.endsWith(".zip") || file_name.endsWith(".jar")) {
            m_type = ResourceType::ZIPFILE;
            file_name.chop(4);
        } else if (file_name.endsWith(".nilmod")) {
            m_type = ResourceType::ZIPFILE;
            file_name.chop(7);
        } else if (file_name.endsWith(".litemod")) {
            m_type = ResourceType::LITEMOD;
            file_name.chop(8);
        } else {
            m_type = ResourceType::SINGLEFILE;
        }

        m_name = file_name;
    }

    m_changed_date_time = m_file_info.lastModified();
}

static void removeThePrefix(QString& string)
{
    QRegularExpression regex(QStringLiteral("^(?:the|teh) +"), QRegularExpression::CaseInsensitiveOption);
    string.remove(regex);
    string = string.trimmed();
}

std::pair<int, bool> Resource::compare(const Resource& other, SortType type) const
{
    switch (type) {
        default:
        case SortType::ENABLED:
            if (enabled() && !other.enabled())
                return { 1, type == SortType::ENABLED };
            if (!enabled() && other.enabled())
                return { -1, type == SortType::ENABLED };
        case SortType::NAME: {
            QString this_name{ name() };
            QString other_name{ other.name() };

            removeThePrefix(this_name);
            removeThePrefix(other_name);

            auto compare_result = QString::compare(this_name, other_name, Qt::CaseInsensitive);
            if (compare_result != 0)
                return { compare_result, type == SortType::NAME };
        }
        case SortType::DATE:
            if (dateTimeChanged() > other.dateTimeChanged())
                return { 1, type == SortType::DATE };
            if (dateTimeChanged() < other.dateTimeChanged())
                return { -1, type == SortType::DATE };
    }

    return { 0, false };
}

bool Resource::applyFilter(QRegularExpression filter) const
{
    return filter.match(name()).hasMatch();
}

bool Resource::enable(EnableAction action)
{
    if (m_type == ResourceType::UNKNOWN || m_type == ResourceType::FOLDER)
        return false;


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

    if (m_enabled == enable)
        return false;

    if (enable) {
        // m_enabled is false, but there's no '.disabled' suffix.
        // TODO: Report error?
        if (!path.endsWith(".disabled"))
            return false;
        path.chop(9);

        if (!file.rename(path))
            return false;
    } else {
        path += ".disabled";

        if (!file.rename(path))
            return false;
    }

    setFile(QFileInfo(path));

    m_enabled = enable;
    return true;
}

bool Resource::destroy()
{
    m_type = ResourceType::UNKNOWN;

    if (FS::trash(m_file_info.filePath()))
        return true;

    return FS::deletePath(m_file_info.filePath());
}

bool Resource::isSymLinkUnder(const QString& instPath) const 
{
    if (isSymLink())
        return true;

    auto instDir = QDir(instPath);

    auto relAbsPath = instDir.relativeFilePath(m_file_info.absoluteFilePath());
    auto relCanonPath = instDir.relativeFilePath(m_file_info.canonicalFilePath());

    return relAbsPath != relCanonPath;
}

bool Resource::isMoreThanOneHardLink() const 
{
    return FS::hardLinkCount(m_file_info.absoluteFilePath()) > 1;
}
