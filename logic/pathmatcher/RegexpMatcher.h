#include "IPathMatcher.h"
#include <QRegularExpression>

class RegexpMatcher : public IPathMatcher
{
public:
	virtual ~RegexpMatcher() {};
	RegexpMatcher(QString regexp)
	{
		m_regexp.setPattern(regexp);
		m_onlyFilenamePart = !regexp.contains('/');
	}

	virtual bool matches(const QString &string) override
	{
		if(m_onlyFilenamePart)
		{
			auto slash = string.lastIndexOf('/');
			if(slash != -1)
			{
				auto part = string.mid(slash + 1);
				return m_regexp.match(part).hasMatch();
			}
		}
		return m_regexp.match(string).hasMatch();
	}
	QRegularExpression m_regexp;
	bool m_onlyFilenamePart = false;
};
