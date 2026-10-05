// SPDX-FileCopyrightText: 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Rachel Powers <508861+Ryex@users.noreply.github.com>
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

#include "logging/Debug.h"

#include <atomic>
#include <mutex>

#include <QDebug>
#include <QFile>

#include "Application.h"

#include "console/Console.h"
#include "launch/LogModel.h"

namespace Logging {

namespace {

// NOLINTNEXTLINE(cppcoreguidelines-avoid-non-const-global-variables)
constinit std::atomic_bool isANSIColorConsole{ false };
// constexpr data for log formats (no bad uncatchable exceptions)

// NOLINTNEXTLINE(bugprone-throwing-static-initialization
const QString g_defaultLogFormat = QStringLiteral(
    "%{time process}"
    " "
    "%{if-debug}Debug:%{endif}"
    "%{if-info}Info:%{endif}"
    "%{if-warning}Warning:%{endif}"
    "%{if-critical}Critical:%{endif}"
    "%{if-fatal}Fatal:%{endif}"
    " "
    "%{if-category}[%{category}] %{endif}"
    "%{message}"
    " "
    "(%{function}:%{line})");

// NOLINTBEGIN(cppcoreguidelines-macro-usage,readability-identifier-naming)
#define reset "\x1b[0m"
#define bold "\x1b[1m"
#define reset_bold "\x1b[22m"
#define faint "\x1b[2m"
#define red_fg "\x1b[31m"
#define green_fg "\x1b[32m"
#define yellow_fg "\x1b[33m"
#define blue_fg "\x1b[34m"
#define inverse "\x1b[7m"
#define italic "\x1b[3m"
#define purple_fg "\x1b[35m"
// NOLINTEND(cppcoreguidelines-macro-usage,readability-identifier-naming)

// clang-format off
// NOLINTNEXTLINE(bugprone-throwing-static-initialization)
const QString g_ansiLogFormat = QStringLiteral(
    faint "%{time process}" reset
    " "
    "%{if-debug}" bold green_fg "D:" reset "%{endif}"
    "%{if-info}" bold blue_fg "I:" reset "%{endif}"
    "%{if-warning}" bold yellow_fg "W:" reset_bold "%{endif}"
    "%{if-critical}" bold red_fg "C:" reset_bold "%{endif}"
    "%{if-fatal}" bold inverse red_fg "F:" reset_bold "%{endif}"
    " "
    "%{if-category}" bold "[%{category}]" reset_bold " %{endif}"
    "%{message}"
    " "
    reset faint "(%{function}:%{line})" reset
);

// clang-format on
#undef reset
#undef bold
#undef reset_bold
#undef faint
#undef red_fg
#undef green_fg
#undef yellow_fg
#undef blue_fg
#undef inverse
#undef italic
#undef purple_fg

/** This is used so that we can output to the log file in addition to the CLI. */
void appDebugOutput(QtMsgType type, const QMessageLogContext& context, const QString& msg)
{
    static std::mutex s_loggerMutex;
    const std::scoped_lock lock(s_loggerMutex);  // synchronized, QFile logFile is not thread-safe

    const bool isANSI = isANSIColorConsole.load();  // atomics are safer
    if (isANSI) {
        // ensure default is set for log file
        qSetMessagePattern(g_defaultLogFormat);
    }

    QString out = qFormatLogMessage(type, context, msg);
    if (APPLICATION->logModel) {
        APPLICATION->logModel->append(MessageLevel::fromQtMsgType(type), out);
    }

    out += QChar::LineFeed;
    APPLICATION->logFile->write(out.toUtf8());
    APPLICATION->logFile->flush();

    if (isANSI) {
        // format ansi for console;
        qSetMessagePattern(g_ansiLogFormat);
        out = qFormatLogMessage(type, context, msg);
        out += QChar::LineFeed;
    }

    QTextStream(stderr) << out.toLocal8Bit();
    fflush(stderr);
}

}  // namespace

void setupQtLogFunction()
{
    if (Console::isConsole()) {
        isANSIColorConsole.store(true);
    }

    qInstallMessageHandler(appDebugOutput);
    qSetMessagePattern(g_defaultLogFormat);
}

}  // namespace Logging
