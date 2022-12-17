#include "Resource.h"

#include <QRegularExpression>

#include "FileSystem.h"

Resource::Resource(QObject* parent) : QObject(parent) {}

Resource::Resource(QFileInfo file_info) : QObject()
{
    setFile(file_info);
}

void Resource::setFile(QFileInfo file_info)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info = file_info;
    parseFile();
}

void Resource::parseFile()
{
    QString file_name{ hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.fileName() };

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::UNKNOWN;

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_internal_id = file_name;

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.isDir()) {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::FOLDER;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name = file_name;
    } else if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.isFile()) {
        if (file_name.endsWith(".disabled")) {
            file_name.chop(9);
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled = false;
        }

        if (file_name.endsWith(".zip") || file_name.endsWith(".jar")) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::ZIPFILE;
            file_name.chop(4);
        } else if (file_name.endsWith(".litemod")) {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::LITEMOD;
            file_name.chop(8);
        } else {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::SINGLEFILE;
        }

        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_name = file_name;
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_changed_date_time = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.lastModified();
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
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type == ResourceType::UNKNOWN || hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type == ResourceType::FOLDER)
        return false;


    QString path = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.absoluteFilePath();
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

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled == enable)
        return false;

    if (enable) {
        // hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled is false, but there's no '.disabled' suffix.
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

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_enabled = enable;
    return true;
}

bool Resource::destroy()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_type = ResourceType::UNKNOWN;
    return FS::deletePath(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_file_info.filePath());
}
