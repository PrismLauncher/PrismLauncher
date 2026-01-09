#include "MessageLevel.h"

MessageLevel MessageLevel::fromName(const QString& levelName)
{
    QString name = levelName.toUpper();
    if (name == "LAUNCHER")
        return MessageLevel::Launcher;
    else if (name == "TRACE")
        return MessageLevel::Trace;
    else if (name == "DEBUG")
        return MessageLevel::Debug;
    else if (name == "INFO")
        return MessageLevel::Info;
    else if (name == "MESSAGE")
        return MessageLevel::Message;
    else if (name == "WARNING" || name == "WARN")
        return MessageLevel::Warning;
    else if (name == "ERROR" || name == "CRITICAL")
        return MessageLevel::Error;
    else if (name == "FATAL")
        return MessageLevel::Fatal;
    // Skip PrePost, it's not exposed to !![]!
    // Also skip StdErr and StdOut
    else
        return MessageLevel::Unknown;
}

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
