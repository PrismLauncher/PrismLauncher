#pragma once

#include <QString>

// NOTE: apparently the GNU C library pollutes the global namespace with these... undef them.
#ifdef major
    #undef major
#endif
#ifdef minor
    #undef minor
#endif

class JavaVersion
{
    friend class JavaVersionTest;
public:
    JavaVersion() {};
    JavaVersion(const QString & rhs);

    JavaVersion & operator=(const QString & rhs);

    bool operator<(const JavaVersion & rhs);
    bool operator==(const JavaVersion & rhs);
    bool operator>(const JavaVersion & rhs);

    bool requiresPermGen();

    QString toString();

    int major()
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major;
    }
    int minor()
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor;
    }
    int security()
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security;
    }
private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_string;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_major = 0;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_minor = 0;
    int hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_security = 0;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_parseable = false;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prerelease;
};
