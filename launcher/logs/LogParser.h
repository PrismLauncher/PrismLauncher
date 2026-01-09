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
#pragma once
#include <QAnyStringView>
#include <QDateTime>
#include <QList>
#include <QString>
#include <QStringView>
#include <QXmlStreamReader>
#include <optional>
#include <variant>
#include "MessageLevel.h"

class LogParser {
   public:
    struct LogEntry {
        QString logger;
        MessageLevel level;
        QString levelText;
        QDateTime timestamp;
        QString thread;
        QString message;
    };
    struct Partial {
        QString data;
    };
    struct PlainText {
        QString message;
    };
    struct Error {
        QString errMessage;
        QXmlStreamReader::Error error;
    };

    using ParsedItem = std::variant<LogEntry, PlainText, Partial>;

   public:
    LogParser() = default;

    void appendLine(QAnyStringView data);
    std::optional<ParsedItem> parseNext();
    QList<ParsedItem> parseAvailable();
    std::optional<Error> getError();

    /// guess log level from a line of game log
    static MessageLevel guessLevel(const QString& line, MessageLevel previous);

   protected:
    std::optional<LogEntry> parseAttributes();
    void setError();
    void clearError();

    std::optional<ParsedItem> parseLog4J();

   private:
    QString m_buffer;
    QString m_partialData;
    QXmlStreamReader m_parser;
    std::optional<Error> m_error;
};
