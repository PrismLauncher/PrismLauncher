#include "TextPrint.h"

TextPrint::TextPrint(LaunchTask * parent, const QStringList &lines, MessageLevel::Enum level) : LaunchStep(parent)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lines = lines;
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_level = level;
}
TextPrint::TextPrint(LaunchTask *parent, const QString &line, MessageLevel::Enum level) : LaunchStep(parent)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lines.append(line);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_level = level;
}

void TextPrint::executeTask()
{
    emit logLines(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_lines, hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_level);
    emitSucceeded();
}

bool TextPrint::canAbort() const
{
    return true;
}

bool TextPrint::abort()
{
    emitFailed("Aborted.");
    return true;
}
