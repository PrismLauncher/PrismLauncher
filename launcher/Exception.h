// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QString>
#include <QDebug>
#include <exception>

class Exception : public std::exception
{
public:
    Exception(const QString &message) : std::exception(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_message(message)
    {
        qCritical() << "Exception:" << message;
    }
    Exception(const Exception &other)
        : std::exception(), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_message(other.cause())
    {
    }
    virtual ~Exception() noexcept {}
    const char *what() const noexcept
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_message.toLatin1().constData();
    }
    QString cause() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_message;
    }

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_message;
};
