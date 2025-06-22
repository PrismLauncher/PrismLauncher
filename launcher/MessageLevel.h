#pragma once

#include <qlogging.h>
#include <QString>

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
namespace MessageLevel {
enum Enum {
    Unknown,  /**< No idea what this is or where it came from */
    StdOut,   /**< Undetermined stderr messages */
    StdErr,   /**< Undetermined stdout messages */
    Launcher, /**< Launcher Messages */
    Trace,    /**< Trace Messages */
    Debug,    /**< Debug Messages */
    Info,     /**< Info Messages */
    Message,  /**< Standard Messages */
    Warning,  /**< Warnings */
    Error,    /**< Errors */
    Fatal,    /**< Fatal Errors */
};
MessageLevel::Enum getLevel(const QString& levelName);
MessageLevel::Enum getLevel(QtMsgType type);

/* Get message level from a line. Line is modified if it was successful. */
MessageLevel::Enum fromLine(QString& line);

/* Get message level from a line from the launcher log. Line is modified if it was successful. */
MessageLevel::Enum fromLauncherLine(QString& line);
}  // namespace MessageLevel
