#pragma once

#include <QString>

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
namespace MessageLevel {
enum Enum : uint8_t {
    Unknown,  /**< No idea what this is or where it came from */
    StdOut,   /**< Undetermined stderr messages */
    StdErr,   /**< Undetermined stdout messages */
    Launcher, /**< Launcher Messages */
    Debug,    /**< Debug Messages */
    Info,     /**< Info Messages */
    Message,  /**< Standard Messages */
    Warning,  /**< Warnings */
    Error,    /**< Errors */
    Fatal,    /**< Fatal Errors */
};
MessageLevel::Enum getLevel(const QString& levelName);

/* Get message level from a line. Line is modified if it was successful. */
MessageLevel::Enum fromLine(QString& line);

/** Tries to guess the log level from some heuristics in the line.
 *  Falls back to 'default_level' if no heuristic works out.
 */
MessageLevel::Enum guessLevel(const QString& line, MessageLevel::Enum default_level);
}  // namespace MessageLevel
