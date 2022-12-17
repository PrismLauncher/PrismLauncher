#include "JavaVersion.h"

#include "StringUtils.h"

#include <QRegularExpression>
#include <QString>

JavaVersion & JavaVersion::operator=(const QString & javaVersionString)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string = javaVersionString;

    auto getCapturedInteger = [](const QRegularExpressionMatch & match, const QString &what) -> int
    {
        auto str = match.captured(what);
        if(str.isEmpty())
        {
            return 0;
        }
        return str.toInt();
    };

    QRegularExpression pattern;
    if(javaVersionString.startsWith("1."))
    {
        pattern = QRegularExpression ("1[.](?<major>[0-9]+)([.](?<minor>[0-9]+))?(_(?<security>[0-9]+)?)?(-(?<prerelease>[a-zA-Z0-9]+))?");
    }
    else
    {
        pattern = QRegularExpression("(?<major>[0-9]+)([.](?<minor>[0-9]+))?([.](?<security>[0-9]+))?(-(?<prerelease>[a-zA-Z0-9]+))?");
    }

    auto match = pattern.match(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable = match.hasMatch();
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major = getCapturedInteger(match, "major");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor = getCapturedInteger(match, "minor");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security = getCapturedInteger(match, "security");
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease = match.captured("prerelease");
    return *this;
}

JavaVersion::JavaVersion(const QString &rhs)
{
    operator=(rhs);
}

QString JavaVersion::toString()
{
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string;
}

bool JavaVersion::requiresPermGen()
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major < 8;
    }
    return true;
}

bool JavaVersion::operator<(const JavaVersion &rhs)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable && rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable)
    {
        auto major = hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major;
        auto rmajor = rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major;

        // HACK: discourage using java 9
        if(major > 8)
            major = -major;
        if(rmajor > 8)
            rmajor = -rmajor;

        if(major < rmajor)
            return true;
        if(major > rmajor)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor < rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor)
            return true;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor > rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor)
            return false;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security < rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security)
            return true;
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security > rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security)
            return false;

        // everything else being equal, consider prerelease status
        bool thisPre = !hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease.isEmpty();
        bool rhsPre = !rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease.isEmpty();
        if(thisPre && !rhsPre)
        {
            // this is a prerelease and the other one isn't -> lesser
            return true;
        }
        else if(!thisPre && rhsPre)
        {
            // this isn't a prerelease and the other one is -> greater
            return false;
        }
        else if(thisPre && rhsPre)
        {
            // both are prereleases - use natural compare...
            return StringUtils::naturalCompare(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease, rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease, Qt::CaseSensitive) < 0;
        }
        // neither is prerelease, so they are the same -> this cannot be less than rhs
        return false;
    }
    else return StringUtils::naturalCompare(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string, rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string, Qt::CaseSensitive) < 0;
}

bool JavaVersion::operator==(const JavaVersion &rhs)
{
    if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable && rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable)
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major == rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor == rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security == rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease == rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease;
    }
    return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string == rhs.hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string;
}

bool JavaVersion::operator>(const JavaVersion &rhs)
{
    return (!operator<(rhs)) && (!operator==(rhs));
}
