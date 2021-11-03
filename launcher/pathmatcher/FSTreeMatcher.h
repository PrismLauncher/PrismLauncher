#pragma once

#include "IPathMatcher.h"
#include <SeparatorPrefixTree.h>
#include <QRegularExpression>

class FSTreeMatcher : public IPathMatcher
{
public:
    virtual ~FSTreeMatcher() {};
    FSTreeMatcher(SeparatorPrefixTree<'/'> & tree) : m_fsTree(tree)
    {
    }

    bool matches(const QString &string) const override
    {
        return m_fsTree.covers(string);
    }

    SeparatorPrefixTree<'/'> & m_fsTree;
};
