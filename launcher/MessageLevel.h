#pragma once

#include <QString>
#include <cstdint>
#include "EnumWrapper.h"

enum class MessageLevelValue : std::uint8_t {
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

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
struct MessageLevel : EnumWrapper<MessageLevel, MessageLevelValue> {
    static constexpr auto invalid() { return Unknown; };
    static constexpr auto mapping()
    {
        return std::array{
            std::pair{ Unknown, "UNKNOWN" }, std::pair{ Launcher, "LAUNCHER" }, std::pair{ Trace, "TRACE" },
            std::pair{ Debug, "DEBUG" },     std::pair{ Info, "INFO" },         std::pair{ Message, "MESSAGE" },
            std::pair{ Warning, "WARNING" }, std::pair{ Warning, "WARN" },      std::pair{ Error, "ERROR" },
            std::pair{ Error, "CRITICAL" },  std::pair{ Fatal, "FATAL" },
        };
    };
    using enum MessageLevelValue;
    using Base = EnumWrapper<MessageLevel, MessageLevelValue>;
    using Base::Base; /* inherit ctor */

    static MessageLevel fromName(const QString& type) { return fromString(type.toUpper()); };
    static MessageLevel fromQtMsgType(const QtMsgType& type);
    static MessageLevel fromLine(const QString& line);

    /* Get message level from a line. Line is modified if it was successful. */
    static MessageLevel takeFromLine(QString& line);

    /* Get message level from a line from the launcher log. Line is modified if it was successful. */
    static MessageLevel takeFromLauncherLine(QString& line);
};
