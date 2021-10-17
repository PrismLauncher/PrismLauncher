#include "MessageLevel.h"

MessageLevel::Enum MessageLevel::getLevel(const QString& levelName)
{
    if (levelName == "Launcher")
        return MessageLevel::Launcher;
    else if (levelName == "Debug")
        return MessageLevel::Debug;
    else if (levelName == "Info")
        return MessageLevel::Info;
    else if (levelName == "Message")
        return MessageLevel::Message;
    else if (levelName == "Warning")
        return MessageLevel::Warning;
    else if (levelName == "Error")
        return MessageLevel::Error;
    else if (levelName == "Fatal")
        return MessageLevel::Fatal;
    // Skip PrePost, it's not exposed to !![]!
    // Also skip StdErr and StdOut
    else
        return MessageLevel::Unknown;
}

MessageLevel::Enum MessageLevel::fromLine(QString &line)
{
    // Level prefix
    int endmark = line.indexOf("]!");
    if (line.startsWith("!![") && endmark != -1)
    {
        auto level = MessageLevel::getLevel(line.left(endmark).mid(3));
        line = line.mid(endmark + 2);
        return level;
    }
    return MessageLevel::Unknown;
}
