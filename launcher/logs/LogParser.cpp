// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2025 Rachel Powers <508861+Ryex@users.noreply.github.com>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include "LogParser.h"

#include <QRegularExpression>
#include "MessageLevel.h"

using namespace Qt::Literals::StringLiterals;

void LogParser::appendLine(QAnyStringView data)
{
    if (!m_partialData.isEmpty()) {
        m_buffer = QString(m_partialData);
        m_buffer.append("\n");
        m_partialData.clear();
    }
    m_buffer.append(data.toString());
}

std::optional<LogParser::Error> LogParser::getError()
{
    return m_error;
}

std::optional<LogParser::LogEntry> LogParser::parseAttributes()
{
    LogParser::LogEntry entry{
        "",
        MessageLevel::Info,
    };
    auto attributes = m_parser.attributes();

    for (const auto& attr : attributes) {
        auto name = attr.name();
        auto value = attr.value();
        if (name == "logger"_L1) {
            entry.logger = value.trimmed().toString();
        } else if (name == "timestamp"_L1) {
            if (value.trimmed().isEmpty()) {
                m_parser.raiseError("log4j:Event Missing required attribute: timestamp");
                return {};
            }
            entry.timestamp = QDateTime::fromSecsSinceEpoch(value.trimmed().toLongLong());
        } else if (name == "level"_L1) {
            entry.levelText = value.trimmed().toString();
            entry.level = MessageLevel::fromName(entry.levelText);
        } else if (name == "thread"_L1) {
            entry.thread = value.trimmed().toString();
        }
    }
    if (entry.logger.isEmpty()) {
        m_parser.raiseError("log4j:Event Missing required attribute: logger");
        return {};
    }

    return entry;
}

void LogParser::setError()
{
    m_error = {
        m_parser.errorString(),
        m_parser.error(),
    };
}

void LogParser::clearError()
{
    m_error = {};  // clear previous error
}

bool isPotentialLog4JStart(QStringView buffer)
{
    static QString target = QStringLiteral("<log4j:event");
    if (buffer.isEmpty() || buffer[0] != '<') {
        return false;
    }
    auto bufLower = buffer.toString().toLower();
    return target.startsWith(bufLower) || bufLower.startsWith(target);
}

std::optional<LogParser::ParsedItem> LogParser::parseNext()
{
    clearError();

    if (m_buffer.isEmpty()) {
        return {};
    }

    if (m_buffer.trimmed().isEmpty()) {
        auto text = QString(m_buffer);
        m_buffer.clear();
        return LogParser::PlainText{ text };
    }

    // check if we have a full xml log4j event
    bool isCompleteLog4j = false;
    m_parser.clear();
    m_parser.setNamespaceProcessing(false);
    m_parser.addData(m_buffer);
    if (m_parser.readNextStartElement()) {
        if (m_parser.qualifiedName().compare("log4j:Event"_L1, Qt::CaseInsensitive) == 0) {
            int depth = 1;
            bool eod = false;
            while (depth > 0 && !eod) {
                auto tok = m_parser.readNext();
                switch (tok) {
                    case QXmlStreamReader::TokenType::StartElement: {
                        depth += 1;
                    } break;
                    case QXmlStreamReader::TokenType::EndElement: {
                        depth -= 1;
                    } break;
                    case QXmlStreamReader::TokenType::EndDocument: {
                        eod = true;  // break outer while loop
                    } break;
                    default: {
                        // no op
                    }
                }
                if (m_parser.hasError()) {
                    break;
                }
            }

            isCompleteLog4j = depth == 0;
        }
    }

    if (isCompleteLog4j) {
        return parseLog4J();
    } else {
        if (isPotentialLog4JStart(m_buffer)) {
            m_partialData = QString(m_buffer);
            return LogParser::Partial{ QString(m_buffer) };
        }

        int start = 0;
        auto bufView = QStringView(m_buffer);
        while (start < bufView.length()) {
            if (qsizetype pos = bufView.right(bufView.length() - start).indexOf('<'); pos != -1) {
                auto slicestart = start + pos;
                auto slice = bufView.right(bufView.length() - slicestart);
                if (isPotentialLog4JStart(slice)) {
                    if (slicestart > 0) {
                        auto text = m_buffer.left(slicestart);
                        m_buffer = m_buffer.right(m_buffer.length() - slicestart);
                        if (!text.trimmed().isEmpty()) {
                            return LogParser::PlainText{ text };
                        }
                    }
                    m_partialData = QString(m_buffer);
                    return LogParser::Partial{ QString(m_buffer) };
                }
                start = slicestart + 1;
            } else {
                break;
            }
        }

        // no log4j found, all plain text
        auto text = QString(m_buffer);
        m_buffer.clear();
        return LogParser::PlainText{ text };
    }
}

QList<LogParser::ParsedItem> LogParser::parseAvailable()
{
    QList<LogParser::ParsedItem> items;
    bool doNext = true;
    while (doNext) {
        auto item_ = parseNext();
        if (m_error.has_value()) {
            return {};
        }
        if (item_.has_value()) {
            auto item = item_.value();
            if (std::holds_alternative<LogParser::Partial>(item)) {
                break;
            } else {
                items.push_back(item);
            }
        } else {
            doNext = false;
        }
    }
    return items;
}

std::optional<LogParser::ParsedItem> LogParser::parseLog4J()
{
    m_parser.clear();
    m_parser.setNamespaceProcessing(false);
    m_parser.addData(m_buffer);

    m_parser.readNextStartElement();
    if (m_parser.qualifiedName().compare("log4j:Event"_L1, Qt::CaseInsensitive) == 0) {
        auto entry_ = parseAttributes();
        if (!entry_.has_value()) {
            setError();
            return {};
        }
        auto entry = entry_.value();

        bool foundMessage = false;
        int depth = 1;

        enum parseOp { noOp, entryReady, parseError };

        auto foundStart = [&]() -> parseOp {
            depth += 1;
            if (m_parser.qualifiedName().compare("log4j:Message"_L1, Qt::CaseInsensitive) == 0) {
                QString message;
                bool messageComplete = false;

                while (!messageComplete) {
                    auto tok = m_parser.readNext();

                    switch (tok) {
                        case QXmlStreamReader::TokenType::Characters: {
                            message.append(m_parser.text());
                        } break;
                        case QXmlStreamReader::TokenType::EndElement: {
                            if (m_parser.qualifiedName().compare("log4j:Message"_L1, Qt::CaseInsensitive) == 0) {
                                messageComplete = true;
                            }
                        } break;
                        case QXmlStreamReader::TokenType::EndDocument: {
                            return parseError;  // parse fail
                        } break;
                        default: {
                            // no op
                        }
                    }

                    if (m_parser.hasError()) {
                        return parseError;
                    }
                }

                entry.message = message;
                foundMessage = true;
                depth -= 1;
            }
            return noOp;
        };

        auto foundEnd = [&]() -> parseOp {
            depth -= 1;
            if (depth == 0 && m_parser.qualifiedName().compare("log4j:Event"_L1, Qt::CaseInsensitive) == 0) {
                if (foundMessage) {
                    auto consumed = m_parser.characterOffset();
                    if (consumed > 0 && consumed <= m_buffer.length()) {
                        m_buffer = m_buffer.right(m_buffer.length() - consumed);
                        // potential whitespace preserved for next item
                    }
                    clearError();
                    return entryReady;
                }
                m_parser.raiseError("log4j:Event Missing required attribute: message");
                setError();
                return parseError;
            }
            return noOp;
        };

        while (!m_parser.atEnd()) {
            auto tok = m_parser.readNext();
            parseOp op = noOp;
            switch (tok) {
                case QXmlStreamReader::TokenType::StartElement: {
                    op = foundStart();
                } break;
                case QXmlStreamReader::TokenType::EndElement: {
                    op = foundEnd();
                } break;
                case QXmlStreamReader::TokenType::EndDocument: {
                    return {};
                } break;
                default: {
                    // no op
                }
            }

            switch (op) {
                case parseError:
                    return {};  // parse fail or error
                case entryReady:
                    return entry;
                case noOp:
                default: {
                    // no op
                }
            }

            if (m_parser.hasError()) {
                return {};
            }
        }
    }

    throw std::runtime_error("unreachable: already verified this was a complete log4j:Event");
}

MessageLevel LogParser::guessLevel(const QString& line, MessageLevel previous)
{
    static const QRegularExpression LINE_WITH_LEVEL("^\\[(?<timestamp>[0-9:]+)\\] \\[[^/]+/(?<level>[^\\]]+)\\]");
    auto match = LINE_WITH_LEVEL.match(line);
    if (match.hasMatch()) {
        // New style logs from log4j
        QString timestamp = match.captured("timestamp");
        QString levelStr = match.captured("level");

        return MessageLevel::fromName(levelStr);
    } else {
        // Old style forge logs
        if (line.contains("[INFO]") || line.contains("[CONFIG]") || line.contains("[FINE]") || line.contains("[FINER]") ||
            line.contains("[FINEST]"))
            return MessageLevel::Info;
        if (line.contains("[SEVERE]") || line.contains("[STDERR]"))
            return MessageLevel::Error;
        if (line.contains("[WARNING]"))
            return MessageLevel::Warning;
        if (line.contains("[DEBUG]"))
            return MessageLevel::Debug;
    }

    if (line.contains("Exception: ") || line.contains("Throwable: "))
        return MessageLevel::Error;

    if (line.startsWith("Caused by: ") || line.startsWith("Exception in thread"))
        return MessageLevel::Error;

    if (line.contains("overwriting existing"))
        return MessageLevel::Fatal;

    if (line.startsWith("\t") || line.startsWith(" "))
        return previous;

    return MessageLevel::Unknown;
}
