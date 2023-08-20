#include "MessageLevel.h"

#include <QRegularExpression>

namespace MessageLevel {

MessageLevel::Enum getLevel(const QString& levelName)
{
    if (levelName == QLatin1String("Launcher"))
        return MessageLevel::Launcher;
    else if (levelName == QLatin1String("Debug"))
        return MessageLevel::Debug;
    else if (levelName == QLatin1String("Info"))
        return MessageLevel::Info;
    else if (levelName == QLatin1String("Message"))
        return MessageLevel::Message;
    else if (levelName == QLatin1String("Warning"))
        return MessageLevel::Warning;
    else if (levelName == QLatin1String("Error"))
        return MessageLevel::Error;
    else if (levelName == QLatin1String("Fatal"))
        return MessageLevel::Fatal;
    // Skip PrePost, it's not exposed to !![]!
    // Also skip StdErr and StdOut
    else
        return MessageLevel::Unknown;
}

MessageLevel::Enum fromLine(QString& line)
{
    // Level prefix
    int endmark = line.indexOf("]!");
    if (line.startsWith("!![") && endmark != -1) {
        auto level = MessageLevel::getLevel(line.left(endmark).mid(3));
        line = line.mid(endmark + 2);
        return level;
    }
    return MessageLevel::Unknown;
}

static const QRegularExpression s_guess_level_regex(QStringLiteral("\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]"));

static const QString javaSymbol = QStringLiteral("([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$][a-zA-Z\\d_$]*");
static const QString javaSymbolOpt = QStringLiteral("([a-zA-Z_$][a-zA-Z\\d_$]*\\.)+[a-zA-Z_$]?[a-zA-Z\\d_$]*");

static const QRegularExpression s_at(QStringLiteral("\\s+at "));
static const QRegularExpression s_caused_by(QStringLiteral("Caused by: ") + javaSymbol);
static const QRegularExpression s_java_problem(javaSymbolOpt + QStringLiteral("(Exception|Error|Throwable)"));
static const QRegularExpression s_more(QStringLiteral("... \\d+ more$"));

MessageLevel::Enum guessLevel(const QString& line, MessageLevel::Enum level)
{
    auto match = s_guess_level_regex.match(line);
    if (match.hasMatch()) {
        // New style logs from log4j
        QString timestamp = match.captured(QLatin1String("timestamp"));
        QString levelStr = match.captured(QLatin1String("level"));
        if (levelStr == QLatin1String("INFO"))
            level = MessageLevel::Message;
        else if (levelStr == QLatin1String("WARN"))
            level = MessageLevel::Warning;
        else if (levelStr == QLatin1String("ERROR"))
            level = MessageLevel::Error;
        else if (levelStr == QLatin1String("FATAL"))
            level = MessageLevel::Fatal;
        else if (levelStr == QLatin1String("TRACE") || levelStr == QLatin1String("DEBUG"))
            level = MessageLevel::Debug;
    } else {
        // Old style forge logs
        if (line.contains(QLatin1String("[INFO]")) || line.contains(QLatin1String("[CONFIG]")) || line.contains(QLatin1String("[FINE]")) ||
            line.contains(QLatin1String("[FINER]")) || line.contains(QLatin1String("[FINEST]")))
            level = MessageLevel::Message;
        else if (line.contains(QLatin1String("[SEVERE]")) || line.contains(QLatin1String("[STDERR]")))
            level = MessageLevel::Error;
        else if (line.contains(QLatin1String("[WARNING]")))
            level = MessageLevel::Warning;
        else if (line.contains(QLatin1String("[DEBUG]")))
            level = MessageLevel::Debug;
    }

    if (line.contains(QLatin1String("overwriting existing")))
        return MessageLevel::Fatal;

    // NOTE: this diverges from the real regexp. no unicode, the first section is + instead of *
    if (line.contains(QLatin1String("Exception in thread")) || line.contains(s_at) || line.contains(s_caused_by) ||
        line.contains(s_java_problem) || line.contains(s_more))
        return MessageLevel::Error;
    return level;
}

}  // namespace MessageLevel
