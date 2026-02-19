#include "MessageLevel.h"

MessageLevel MessageLevel::fromQtMsgType(const QtMsgType& type)
{
    switch (type) {
        case QtDebugMsg:
            return MessageLevel::Debug;
        case QtInfoMsg:
            return MessageLevel::Info;
        case QtWarningMsg:
            return MessageLevel::Warning;
        case QtCriticalMsg:
            return MessageLevel::Error;
        case QtFatalMsg:
            return MessageLevel::Fatal;
        default:
            return MessageLevel::Unknown;
    }
}

/* Get message level from a line. Line is modified if it was successful. */
MessageLevel MessageLevel::takeFromLine(QString& line)
{
    // Level prefix
    int endmark = line.indexOf("]!");
    if (line.startsWith("!![") && endmark != -1) {
        auto level = MessageLevel::fromName(line.left(endmark).mid(3));
        line = line.mid(endmark + 2);
        return level;
    }
    return MessageLevel::Unknown;
}

/* Get message level from a line from the launcher log. Line is modified if it was successful. */
MessageLevel MessageLevel::takeFromLauncherLine(QString& line)
{
    // Level prefix
    int startMark = 0;
    while (startMark < line.size() && (line[startMark].isDigit() || line[startMark].isSpace() || line[startMark] == '.'))
        ++startMark;
    int endmark = line.indexOf(":");
    if (startMark < line.size() && endmark != -1) {
        auto level = MessageLevel::fromName(line.left(endmark).mid(startMark));
        line = line.mid(endmark + 2);
        return level;
    }
    return MessageLevel::Unknown;
}
