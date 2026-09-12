// SPDX-FileCopyrightText: 2025 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
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
 */

#include <QTest>

#include <QList>
#include <QObject>
#include <QRegularExpression>
#include <QString>

#include <algorithm>
#include <iterator>

#include <FileSystem.h>
#include <MessageLevel.h>
#include <logs/LogParser.h>

class XmlLogParseTest : public QObject {
    Q_OBJECT

   private slots:

    void guessLevel_timestampFormats()
    {
        QCOMPARE(LogParser::guessLevel("[21:16:07] [Server thread/WARN]: short timestamp", MessageLevel::Unknown), MessageLevel::Warning);
        QCOMPARE(LogParser::guessLevel("[23Jul2026 18:12:07.877] [main/WARN] [Sodium-Workarounds/]: date and millis timestamp",
                                       MessageLevel::Unknown),
                 MessageLevel::Warning);
        QCOMPARE(LogParser::guessLevel(
                     "[25Jul2026 14:10:58.723] [main/ERROR] [net.minecraftforge.fml.loading.moddiscovery.ModFileParser/LOADING]: error",
                     MessageLevel::Unknown),
                 MessageLevel::Error);
    }

    void parseXml_data()
    {
        QString source = QFINDTESTDATA("testdata/TestLogs");

        QString shortXml = QString::fromUtf8(FS::read(FS::PathCombine(source, "vanilla-1.21.5.xml.log")));
        QString shortText = QString::fromUtf8(FS::read(FS::PathCombine(source, "vanilla-1.21.5.text.log")));
        QStringList shortTextLevels_s = QString::fromUtf8(FS::read(FS::PathCombine(source, "vanilla-1.21.5-levels.txt")))
                                            .split(QRegularExpression("\n|\r\n|\r"), Qt::SkipEmptyParts);

        QList<MessageLevel> shortTextLevels;
        shortTextLevels.reserve(24);
        std::transform(shortTextLevels_s.cbegin(), shortTextLevels_s.cend(), std::back_inserter(shortTextLevels),
                       [](const QString& line) { return MessageLevel::fromName(line.trimmed()); });

        QString longXml = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-forge.xml.log")));
        QString longText = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-forge.text.log")));
        QStringList longTextLevels_s = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-levels.txt")))
                                           .split(QRegularExpression("\n|\r\n|\r"), Qt::SkipEmptyParts);
        QStringList longTextLevelsXml_s = QString::fromUtf8(FS::read(FS::PathCombine(source, "TerraFirmaGreg-Modern-xml-levels.txt")))
                                              .split(QRegularExpression("\n|\r\n|\r"), Qt::SkipEmptyParts);

        QList<MessageLevel> longTextLevelsPlain;
        longTextLevelsPlain.reserve(974);
        std::transform(longTextLevels_s.cbegin(), longTextLevels_s.cend(), std::back_inserter(longTextLevelsPlain),
                       [](const QString& line) { return MessageLevel::fromName(line.trimmed()); });
        QList<MessageLevel> longTextLevelsXml;
        longTextLevelsXml.reserve(896);
        std::transform(longTextLevelsXml_s.cbegin(), longTextLevelsXml_s.cend(), std::back_inserter(longTextLevelsXml),
                       [](const QString& line) { return MessageLevel::fromName(line.trimmed()); });

        QTest::addColumn<QString>("log");
        QTest::addColumn<int>("num_entries");
        QTest::addColumn<QList<MessageLevel>>("entry_levels");

        QTest::newRow("short-vanilla-plain") << shortText << 25 << shortTextLevels;
        QTest::newRow("short-vanilla-xml") << shortXml << 25 << shortTextLevels;
        QTest::newRow("long-forge-plain") << longText << 945 << longTextLevelsPlain;
        QTest::newRow("long-forge-xml") << longXml << 869 << longTextLevelsXml;
    }

    void parseXml()
    {
        QFETCH(QString, log);
        QFETCH(int, num_entries);
        QFETCH(QList<MessageLevel>, entry_levels);

        QList<std::pair<MessageLevel, QString>> entries = {};

        QBENCHMARK
        {
            entries = parseLines(log.split(QRegularExpression("\n|\r\n|\r")));
        }

        QCOMPARE(entries.length(), num_entries);

        QList<MessageLevel> levels = {};

        std::transform(entries.cbegin(), entries.cend(), std::back_inserter(levels),
                       [](std::pair<MessageLevel, QString> entry) { return entry.first; });

        QCOMPARE(levels, entry_levels);
    }

    void parseAngleBrackets_data()
    {
        QTest::addColumn<QStringList>("lines");
        QTest::addColumn<QStringList>("messages");

        // Text that merely begins to look like a log4j event must not be held back: lines reach the
        // parser whole, so the rest of `<log4j:Event` can never turn up later on. See #5825.
        QTest::newRow("trailing left angle bracket")
            << QStringList{ "[21:16:07] [Render thread/INFO]: happy >w<", "[21:16:08] [Render thread/INFO]: unrelated" }
            << QStringList{ "[21:16:07] [Render thread/INFO]: happy >w<", "[21:16:08] [Render thread/INFO]: unrelated" };
        QTest::newRow("trailing partial tag") << QStringList{ "generics are <log", "unrelated" }
                                              << QStringList{ "generics are <log", "unrelated" };
        QTest::newRow("lone left angle bracket") << QStringList{ "<", "unrelated" } << QStringList{ "<", "unrelated" };
        QTest::newRow("unrelated markup") << QStringList{ "<html>", "unrelated" } << QStringList{ "<html>", "unrelated" };
        QTest::newRow("longer element name") << QStringList{ "talking about <log4j:eventually", "unrelated" }
                                             << QStringList{ "talking about <log4j:eventually", "unrelated" };

        // ... while real events, spread over several lines or not, still have to be recognised.
        QTest::newRow("event over several lines")
            << QStringList{ R"(  <log4j:Event logger="fqq" timestamp="1745005150596" level="INFO" thread="Render thread">)",
                            R"(    <log4j:Message><![CDATA[Setting user: Ryexandrite]]></log4j:Message>)", R"(  </log4j:Event>)" }
            << QStringList{ "Setting user: Ryexandrite" };
        QTest::newRow("event with attributes on the next line")
            << QStringList{ R"(  <log4j:Event)", R"(      logger="fqq" timestamp="1745005150596" level="INFO" thread="Render thread">)",
                            R"(    <log4j:Message><![CDATA[Setting user: Ryexandrite]]></log4j:Message>)", R"(  </log4j:Event>)" }
            << QStringList{ "Setting user: Ryexandrite" };
        QTest::newRow("event preceded by text")
            << QStringList{ R"(stray output <log4j:Event logger="fqq" timestamp="1745005150596" level="INFO" thread="Render thread">)"
                            R"(<log4j:Message><![CDATA[Setting user: Ryexandrite]]></log4j:Message></log4j:Event>)" }
            << QStringList{ "stray output ", "Setting user: Ryexandrite" };
    }

    void parseAngleBrackets()
    {
        QFETCH(QStringList, lines);
        QFETCH(QStringList, messages);

        QCOMPARE(parseMessages(lines), messages);
    }

   private:
    QList<std::pair<MessageLevel, QString>> parseLines(const QStringList& lines)
    {
        LogParser parser;
        QList<std::pair<MessageLevel, QString>> out;
        MessageLevel last = MessageLevel::Unknown;

        for (const auto& line : lines) {
            parser.appendLine(line);

            auto items = parser.parseAvailable();
            for (const auto& item : items) {
                if (std::holds_alternative<LogParser::LogEntry>(item)) {
                    auto entry = std::get<LogParser::LogEntry>(item);
                    auto msg = QString("[%1] [%2/%3] [%4]: %5")
                                   .arg(entry.timestamp.toString("HH:mm:ss"))
                                   .arg(entry.thread)
                                   .arg(entry.levelText)
                                   .arg(entry.logger)
                                   .arg(entry.message);
                    out.append(std::make_pair(entry.level, msg));
                    last = entry.level;
                } else if (std::holds_alternative<LogParser::PlainText>(item)) {
                    auto msg = std::get<LogParser::PlainText>(item).message;
                    auto level = LogParser::guessLevel(msg, last);

                    out.append(std::make_pair(level, msg));
                    last = level;
                }
            }
        }
        return out;
    }

    /// The messages the parser produces, without the timestamp formatting parseLines() applies - so that
    /// expectations do not depend on the time zone the test happens to run in.
    QStringList parseMessages(const QStringList& lines)
    {
        LogParser parser;
        QStringList out;

        for (const auto& line : lines) {
            parser.appendLine(line);

            for (const auto& item : parser.parseAvailable()) {
                if (std::holds_alternative<LogParser::LogEntry>(item)) {
                    out.append(std::get<LogParser::LogEntry>(item).message);
                } else if (std::holds_alternative<LogParser::PlainText>(item)) {
                    out.append(std::get<LogParser::PlainText>(item).message);
                }
            }
        }
        return out;
    }
};

QTEST_GUILESS_MAIN(XmlLogParseTest)

#include "XmlLogs_test.moc"
