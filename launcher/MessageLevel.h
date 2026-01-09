#pragma once

#include <QString>
#include <compare>

/**
 * @brief the MessageLevel Enum
 * defines what level a log message is
 */
struct MessageLevel {
    enum class Enum {
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
    using enum Enum;
    constexpr MessageLevel(Enum e = Unknown) : m_type(e) {}
    static MessageLevel fromName(const QString& type);
    static MessageLevel fromQtMsgType(const QtMsgType& type);
    static MessageLevel fromLine(const QString& line);
    inline bool isValid() const { return m_type != Unknown; }
    std::strong_ordering operator<=>(const MessageLevel& other) const = default;
    std::strong_ordering operator<=>(const MessageLevel::Enum& other) const { return m_type <=> other; }
    explicit operator int() const { return static_cast<int>(m_type); }
    explicit operator MessageLevel::Enum() { return m_type; }

    /* Get message level from a line. Line is modified if it was successful. */
    static MessageLevel takeFromLine(QString& line);

    /* Get message level from a line from the launcher log. Line is modified if it was successful. */
    static MessageLevel takeFromLauncherLine(QString& line);

   private:
    Enum m_type;
};
