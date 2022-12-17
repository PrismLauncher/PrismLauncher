#include "Version.h"

#include <QStringList>
#include <QUrl>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

Version::Version(const QString &str) : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string(str)
{
    parse();
}

bool Version::operator<(const Version &other) const
{
    const int size = qMax(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size(), other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        const Section sec2 =
            (i >= other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        if (sec1 != sec2)
        {
            return sec1 < sec2;
        }
    }

    return false;
}
bool Version::operator<=(const Version &other) const
{
    return *this < other || *this == other;
}
bool Version::operator>(const Version &other) const
{
    const int size = qMax(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size(), other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        const Section sec2 =
            (i >= other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        if (sec1 != sec2)
        {
            return sec1 > sec2;
        }
    }

    return false;
}
bool Version::operator>=(const Version &other) const
{
    return *this > other || *this == other;
}
bool Version::operator==(const Version &other) const
{
    const int size = qMax(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size(), other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size());
    for (int i = 0; i < size; ++i)
    {
        const Section sec1 = (i >= hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        const Section sec2 =
            (i >= other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.size()) ? Section("0") : other.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.at(i);
        if (sec1 != sec2)
        {
            return false;
        }
    }

    return true;
}
bool Version::operator!=(const Version &other) const
{
    return !operator==(other);
}

void Version::parse()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.clear();

    // FIXME: this is bad. versions can contain a lot more separators...
    QStringList parts = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string.split('.');

    for (const auto& part : parts)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_sections.append(Section(part));
    }
}
