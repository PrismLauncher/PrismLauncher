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

#pragma once

#include <QDialog>
#include <QList>
#include <QString>
#include <QTextEdit>
#include <QTextFormat>

#include <cstdint>
#include <filesystem>

#include <crash_handler/CrashHandler.h>

class QPushButton;

QT_BEGIN_NAMESPACE
namespace Ui {
// NOLINTNEXTLINE(readability-identifier-naming)
class CrashHandlerDialog;
}  // namespace Ui
QT_END_NAMESPACE

namespace CrashHandler {
class CrashHandlerDialog : public QDialog {
    Q_OBJECT

   public:
    CrashHandlerDialog(QWidget* parent,
                       const QString& title,
                       const QString& message,
                       CrashTrace&& trace,
                       std::filesystem::path dataPath,
                       std::string traceFileBase,
                       std::string traceFileExt);

   private:
    Ui::CrashHandlerDialog* m_ui;
    CrashTrace m_trace;
    std::string m_formattedTrace;
    QTextCharFormat m_defaultFormat;

    std::filesystem::path m_dataPath;
    std::string m_traceFileBase;
    std::string m_traceFileExt;

    QString traceHeader() const;
    void formatTrace();
    void reflowTrace();

    void copyTraceToClipboard() const;
    static void openGithubIssue();

    void parseEscapeSequence(std::uint32_t attribute,
                             QListIterator<QString>& i,
                             QTextCharFormat& textFormat,
                             const QTextCharFormat& defaultFormat);
    void setTextWithTermFormatting(QTextEdit* textEdit, const QString& text);

    void saveTrace();
};

}  // namespace CrashHandler
