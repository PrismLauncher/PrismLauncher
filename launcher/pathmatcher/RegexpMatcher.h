#include "IPathMatcher.h"
#include <QRegularExpression>

class RegexpMatcher : public IPathMatcher
{
public:
    virtual ~RegexpMatcher() {};
    RegexpMatcher(const QString &regexp)
    {
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp.setPattern(regexp);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_onlyFilenamePart = !regexp.contains('/');
    }

    RegexpMatcher &caseSensitive(bool cs = true)
    {
        if(cs)
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp.setPatternOptions(QRegularExpression::CaseInsensitiveOption);
        }
        else
        {
            hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp.setPatternOptions(QRegularExpression::NoPatternOption);
        }
        return *this;
    }

    virtual bool matches(const QString &string) const override
    {
        if(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_onlyFilenamePart)
        {
            auto slash = string.lastIndexOf('/');
            if(slash != -1)
            {
                auto part = string.mid(slash + 1);
                return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp.match(part).hasMatch();
            }
        }
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp.match(string).hasMatch();
    }
    QRegularExpression hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_regexp;
    bool hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_onlyFilenamePart = false;
};
