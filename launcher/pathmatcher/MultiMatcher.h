#include "IPathMatcher.h"
#include <SeparatorPrefixTree.h>
#include <QRegularExpression>

class MultiMatcher : public IPathMatcher
{
public:
    virtual ~MultiMatcher() {};
    MultiMatcher()
    {
    }
    MultiMatcher &add(Ptr add)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matchers.append(add);
        return *this;
    }

    virtual bool matches(const QString &string) const override
    {
        for(auto iter: hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matchers)
        {
            if(iter->matches(string))
            {
                return true;
            }
        }
        return false;
    }

    QList<Ptr> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_matchers;
};
