#pragma once

#include "IPathMatcher.h"
#include <SeparatorPrefixTree.h>
#include <QRegularExpression>

class FSTreeMatcher : public IPathMatcher
{
public:
    virtual ~FSTreeMatcher() {};
    FSTreeMatcher(SeparatorPrefixTree<'/'> & tree) : hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fsTree(tree)
    {
    }

    bool matches(const QString &string) const override
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fsTree.covers(string);
    }

    SeparatorPrefixTree<'/'> & hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_fsTree;
};
