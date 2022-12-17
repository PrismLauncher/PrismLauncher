// SPDX-FileCopyrightText: 2022 Sefa Eyeoglu <contact@scrumplex.net>
//
// SPDX-License-Identifier: GPL-3.0-only

#include <QRegularExpression>
#include "IPathMatcher.h"

class SimplePrefixMatcher : public IPathMatcher {
   public:
    virtual ~SimplePrefixMatcher(){};
    SimplePrefixMatcher(const QString& prefix)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prefix = prefix;
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isPrefix = prefix.endsWith('/');
    }

    virtual bool matches(const QString& string) const override
    {
        if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isPrefix)
            return string.startsWith(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prefix);
        return string == hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prefix;
    }
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_prefix;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_isPrefix = false;
};
