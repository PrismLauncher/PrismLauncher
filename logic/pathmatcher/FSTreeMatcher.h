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

	virtual bool matches(const QString &string)  override
	{
		return m_fsTree.covers(string);
	}

	SeparatorPrefixTree<'/'> & m_fsTree;
};
