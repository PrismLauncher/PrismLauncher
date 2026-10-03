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

#include "crash_handler/CrashHandlerDialog.h"
#include "ui_CrashHandlerDialog.h"

#include "BuildConfig.h"
#include "rainbow.h"

#include <cstdint>

#include <QDebug>
#include <QDesktopServices>
#include <QFont>
#include <QFontDatabase>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QtVersion>

#include <QClipboard>

#include <QScrollBar>

#include <cpptrace/formatting.hpp>

namespace CrashHandler {

CrashHandlerDialog::CrashHandlerDialog(QWidget* parent, const QString& title, const QString& message, CrashTrace&& trace)
    : QDialog(parent), m_ui(new Ui::CrashHandlerDialog), m_trace(std::move(trace))
{
    m_ui->setupUi(this);
    m_ui->messageLabel->setText(message);

    setWindowTitle(title);

    QFont monospace = QFontDatabase::systemFont(QFontDatabase::FixedFont);
    monospace.setStyleHint(QFont::Monospace);
    m_ui->traceText->setCurrentFont(monospace);
    m_defaultFormat = m_ui->traceText->currentCharFormat();

    auto defaultPalette = QGuiApplication::palette();
    auto defaultTextBg = defaultPalette.color(QPalette::Disabled, QPalette::Base);
    auto defaultTextFg = defaultPalette.color(QPalette::Disabled, QPalette::Text);
    m_ui->traceText->setTextBackgroundColor(defaultTextBg);
    m_ui->traceText->setTextColor(defaultTextFg);

    connect(m_ui->copyButton, &QPushButton::clicked, this, [this]() { copyTraceToClipboard(); });

    connect(m_ui->issueButton, &QPushButton::clicked, this, []() { CrashHandlerDialog::openGithubIssue(); });

    formatTrace();
}

namespace {

cpptrace::formatter defaultTraceFormatter()
{
    return cpptrace::formatter{}
        .break_before_filename()
        .hide_exception_machinery(true)
        .colors(cpptrace::formatter::color_mode::always)
        .symbols(cpptrace::formatter::symbol_mode::pretty)
        .paths(cpptrace::formatter::path_mode::full)
        .snippets(true)
        .snippet_context(2);
}

}  // namespace

void CrashHandlerDialog::copyTraceToClipboard() const
{
    auto trace = defaultTraceFormatter().snippets(false).colors(cpptrace::formatter::color_mode::none).format(m_trace.stacktrace);
    QGuiApplication::clipboard()->setText(QString::fromStdString(trace), QClipboard::Clipboard);
}

void CrashHandlerDialog::openGithubIssue()
{
    QDesktopServices::openUrl(QUrl(QStringLiteral("%1/issues/new?template=bug_report.yml").arg(BuildConfig.LAUNCHER_GIT)));
}

void CrashHandlerDialog::formatTrace()
{
    m_formattedTrace = defaultTraceFormatter().format(m_trace.stacktrace);
    reflowTrace();
}

void CrashHandlerDialog::reflowTrace()
{
    auto programInfo = QStringLiteral("%1 : %2\nBuild Platform: %3\nBuild Date: %4\nQt Version: %5")
                           .arg(BuildConfig.LAUNCHER_DISPLAYNAME, BuildConfig.printableVersionString(), BuildConfig.BUILD_PLATFORM,
                                BuildConfig.BUILD_DATE, QString(qVersion()));
    auto trace = QStringLiteral("%1\n\nMessage: %2\n\nProcess Id: %3\nThread Id: %4\n\n%5")
                     .arg(programInfo, QString::fromStdString(m_trace.header.message), QString::number(m_trace.header.processId),
                          QString::number(m_trace.header.threadId), QString::fromStdString(m_formattedTrace));

    setTextWithTermFormatting(m_ui->traceText, trace);

    auto cur = m_ui->traceText->textCursor();
    cur.movePosition(QTextCursor::Start);
    m_ui->traceText->setTextCursor(cur);
    m_ui->traceText->verticalScrollBar()->setValue(0);
    m_ui->traceText->ensureCursorVisible();
}

// based on information:
// http://en.m.wikipedia.org/wiki/ANSI_escape_code
// http://misc.flogisoft.com/bash/tip_colors_and_formatting
// http://invisible-island.net/xterm/ctlseqs/ctlseqs.html
//
// apply ANSI escaped formatting codes to text format
void CrashHandlerDialog::parseEscapeSequence(std::uint32_t attribute,
                                             QListIterator<QString>& i,
                                             QTextCharFormat& textFormat,
                                             const QTextCharFormat& defaultFormat)
{
    switch (attribute) {
        case 0: {  // Normal/Default (reset all attributes)
            textFormat = defaultFormat;
            break;
        }
        case 1: {  // Bold/Bright (bold or increased intensity)
            textFormat.setFontWeight(QFont::Bold);
            break;
        }
        case 2: {  // Dim/Faint (decreased intensity)
            textFormat.setFontWeight(QFont::Light);
            break;
        }
        case 3: {  // Italicized (italic on)
            textFormat.setFontItalic(true);
            break;
        }
        case 4: {  // Underscore (single underlined)
            textFormat.setUnderlineStyle(QTextCharFormat::SingleUnderline);
            textFormat.setFontUnderline(true);
            break;
        }
        case 5: {  // Blink (slow, appears as Bold)
            textFormat.setFontWeight(QFont::Bold);
            break;
        }
        case 6: {  // Blink (rapid, appears as very Bold)
            textFormat.setFontWeight(QFont::Black);
            break;
        }
        case 7: {  // Reverse/Inverse (swap foreground and background)
            QBrush foregroundBrush = textFormat.foreground();
            textFormat.setForeground(textFormat.background());
            textFormat.setBackground(foregroundBrush);
            break;
        }
        case 8: {  // Concealed/Hidden/Invisible (usefull for passwords)
            textFormat.setForeground(textFormat.background());
            break;
        }
        case 9: {  // Crossed-out characters
            textFormat.setFontStrikeOut(true);
            break;
        }
        case 10: {  // Primary (default) font
            textFormat.setFont(defaultFormat.font());
            break;
        }
        case 11:
        case 12:
        case 13:
        case 14:
        case 15:
        case 16:
        case 17:
        case 18:
        case 19: {
            QFont font = textFormat.font();
            QStringList fontFamilies = textFormat.fontFamilies().toStringList();
            QStringList fontStyles = QFontDatabase::styles(fontFamilies.first());
            std::uint32_t fontStyleIndex = attribute - 11;
            if (fontStyleIndex < fontStyles.length()) {
                textFormat.setFont(QFontDatabase::font(fontFamilies.first(), fontStyles.at(fontStyleIndex), textFormat.font().pointSize()));
            }
            break;
        }
        case 20: {  // Fraktur (unsupported)
            break;
        }
        case 21:    // Set Bold off
        case 22: {  // Set Dim off
            textFormat.setFontWeight(QFont::Normal);
            break;
        }
        case 23: {  // Unset italic and unset fraktur
            textFormat.setFontItalic(false);
            break;
        }
        case 24: {  // Unset underlining
            textFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
            textFormat.setFontUnderline(false);
            break;
        }
        case 25: {  // Unset Blink/Bold
            textFormat.setFontWeight(QFont::Normal);
            break;
        }
        case 26: {  // Reserved
            break;
        }
        case 27: {  // Positive (non-inverted)
            QBrush backgroundBrush = textFormat.background();
            textFormat.setBackground(textFormat.foreground());
            textFormat.setForeground(backgroundBrush);
            break;
        }
        case 28: {
            textFormat.setForeground(defaultFormat.foreground());
            textFormat.setBackground(defaultFormat.background());
            break;
        }
        case 29: {
            textFormat.setUnderlineStyle(QTextCharFormat::NoUnderline);
            textFormat.setFontUnderline(false);
            break;
        }
        case 30:
        case 31:
        case 32:
        case 33:
        case 34:
        case 35:
        case 36:
        case 37: {
            std::uint32_t colorIndex = attribute - 30;
            QColor color;
            if (QFont::Normal < textFormat.fontWeight()) {
                switch (colorIndex) {
                    case 0: {
                        color = Qt::darkGray;
                        break;
                    }
                    case 1: {
                        color = Qt::red;
                        break;
                    }
                    case 2: {
                        color = Qt::green;
                        break;
                    }
                    case 3: {
                        color = Qt::yellow;
                        break;
                    }
                    case 4: {
                        color = Qt::blue;
                        break;
                    }
                    case 5: {
                        color = Qt::magenta;
                        break;
                    }
                    case 6: {
                        color = Qt::cyan;
                        break;
                    }
                    case 7: {
                        color = Qt::white;
                        break;
                    }
                    default: {
                        Q_ASSERT(false);
                    }
                }
            } else {
                switch (colorIndex) {
                    case 0: {
                        color = Qt::black;
                        break;
                    }
                    case 1: {
                        color = Qt::darkRed;
                        break;
                    }
                    case 2: {
                        color = Qt::darkGreen;
                        break;
                    }
                    case 3: {
                        color = Qt::darkYellow;
                        break;
                    }
                    case 4: {
                        color = Qt::darkBlue;
                        break;
                    }
                    case 5: {
                        color = Qt::darkMagenta;
                        break;
                    }
                    case 6: {
                        color = Qt::darkCyan;
                        break;
                    }
                    case 7: {
                        color = Qt::lightGray;
                        break;
                    }
                    default: {
                        Q_ASSERT(false);
                    }
                }
            }
            textFormat.setForeground(color);
            break;
        }
        case 38: {
            if (i.hasNext()) {
                bool ok = false;
                int selector = i.next().toInt(&ok);
                Q_ASSERT(ok);
                QColor color;
                switch (selector) {
                    case 2: {
                        if (!i.hasNext()) {
                            break;
                        }
                        int red = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (!i.hasNext()) {
                            break;
                        }
                        int green = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (!i.hasNext()) {
                            break;
                        }
                        int blue = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        color.setRgb(red, green, blue);
                        break;
                    }
                    case 5: {
                        if (!i.hasNext()) {
                            break;
                        }
                        int index = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (index >= 0x00 && index <= 0x07) {  // 0x00-0x07:  standard colors (as in ESC [ 30..37 m)
                            parseEscapeSequence(index - 0x00 + 30, i, textFormat, defaultFormat);
                            return;
                        }
                        if (index >= 0x08 && index <= 0x0F) {  // 0x08-0x0F:  high intensity colors (as in ESC [ 90..97 m)
                            parseEscapeSequence(index - 0x08 + 90, i, textFormat, defaultFormat);
                            return;
                        }
                        if (index >= 0x10 && index <= 0xE7) {  // 0x10-0xE7:  6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
                            index -= 0x10;
                            int red = index % 6;
                            index /= 6;
                            int green = index % 6;
                            index /= 6;
                            int blue = index % 6;
                            index /= 6;
                            Q_ASSERT(index == 0);
                            color.setRgb(red, green, blue);
                            break;
                        }
                        if (index >= 0xE8 && index <= 0xFF) {  // 0xE8-0xFF:  grayscale from black to white in 24 steps
                            float intensity = static_cast<float>(index - 0xE8) / (0xFF - 0xE8);
                            color.setRgbF(intensity, intensity, intensity);
                            break;
                        }

                        textFormat.setForeground(color);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            break;
        }
        case 39: {
            textFormat.setForeground(defaultFormat.foreground());
            break;
        }
        case 40:
        case 41:
        case 42:
        case 43:
        case 44:
        case 45:
        case 46:
        case 47: {
            std::uint32_t colorIndex = attribute - 40;
            QColor color;
            switch (colorIndex) {
                case 0: {
                    color = Qt::darkGray;
                    break;
                }
                case 1: {
                    color = Qt::red;
                    break;
                }
                case 2: {
                    color = Qt::green;
                    break;
                }
                case 3: {
                    color = Qt::yellow;
                    break;
                }
                case 4: {
                    color = Qt::blue;
                    break;
                }
                case 5: {
                    color = Qt::magenta;
                    break;
                }
                case 6: {
                    color = Qt::cyan;
                    break;
                }
                case 7: {
                    color = Qt::white;
                    break;
                }
                default: {
                    Q_ASSERT(false);
                }
            }
            textFormat.setBackground(color);
            break;
        }
        case 48: {
            if (i.hasNext()) {
                bool ok = false;
                int selector = i.next().toInt(&ok);
                Q_ASSERT(ok);
                QColor color;
                switch (selector) {
                    case 2: {
                        if (!i.hasNext()) {
                            break;
                        }
                        int red = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (!i.hasNext()) {
                            break;
                        }
                        int green = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (!i.hasNext()) {
                            break;
                        }
                        int blue = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        color.setRgb(red, green, blue);
                        break;
                    }
                    case 5: {
                        if (!i.hasNext()) {
                            break;
                        }
                        int index = i.next().toInt(&ok);
                        Q_ASSERT(ok);
                        if (index >= 0x00 && index <= 0x07) {  // 0x00-0x07:  standard colors (as in ESC [ 40..47 m)
                            parseEscapeSequence(index - 0x00 + 40, i, textFormat, defaultFormat);
                            return;
                        }
                        if (index >= 0x08 && index <= 0x0F) {  // 0x08-0x0F:  high intensity colors (as in ESC [ 100..107 m)

                            parseEscapeSequence(index - 0x08 + 100, i, textFormat, defaultFormat);
                            return;
                        }
                        if (index >= 0x10 && index <= 0xE7) {  // 0x10-0xE7:  6*6*6=216 colors: 16 + 36*r + 6*g + b (0≤r,g,b≤5)
                            index -= 0x10;
                            int red = index % 6;
                            index /= 6;
                            int green = index % 6;
                            index /= 6;
                            int blue = index % 6;
                            index /= 6;
                            Q_ASSERT(index == 0);
                            color.setRgb(red, green, blue);
                            break;
                        }
                        if (index >= 0xE8 && index <= 0xFF) {  // 0xE8-0xFF:  grayscale from black to white in 24 steps
                            float intensity = static_cast<float>(index - 0xE8) / (0xFF - 0xE8);
                            color.setRgbF(intensity, intensity, intensity);
                            break;
                        }
                        textFormat.setBackground(color);
                        break;
                    }
                    default: {
                        break;
                    }
                }
            }
            break;
        }
        case 49: {
            textFormat.setBackground(defaultFormat.background());
            break;
        }
        case 90:
        case 91:
        case 92:
        case 93:
        case 94:
        case 95:
        case 96:
        case 97: {
            std::uint32_t colorIndex = attribute - 90;
            QColor color;
            switch (colorIndex) {
                case 0: {
                    color = Qt::darkGray;
                    break;
                }
                case 1: {
                    color = Qt::red;
                    break;
                }
                case 2: {
                    color = Qt::green;
                    break;
                }
                case 3: {
                    color = Qt::yellow;
                    break;
                }
                case 4: {
                    color = Qt::blue;
                    break;
                }
                case 5: {
                    color = Qt::magenta;
                    break;
                }
                case 6: {
                    color = Qt::cyan;
                    break;
                }
                case 7: {
                    color = Qt::white;
                    break;
                }
                default: {
                    Q_ASSERT(false);
                }
            }
            color.setRedF(color.redF() * 0.8F);
            color.setGreenF(color.greenF() * 0.8F);
            color.setBlueF(color.blueF() * 0.8F);
            textFormat.setForeground(color);
            break;
        }
        case 100:
        case 101:
        case 102:
        case 103:
        case 104:
        case 105:
        case 106:
        case 107: {
            std::uint32_t colorIndex = attribute - 100;
            QColor color;
            switch (colorIndex) {
                case 0: {
                    color = Qt::darkGray;
                    break;
                }
                case 1: {
                    color = Qt::red;
                    break;
                }
                case 2: {
                    color = Qt::green;
                    break;
                }
                case 3: {
                    color = Qt::yellow;
                    break;
                }
                case 4: {
                    color = Qt::blue;
                    break;
                }
                case 5: {
                    color = Qt::magenta;
                    break;
                }
                case 6: {
                    color = Qt::cyan;
                    break;
                }
                case 7: {
                    color = Qt::white;
                    break;
                }
                default: {
                    Q_ASSERT(false);
                }
            }
            color.setRedF(color.redF() * 0.8F);
            color.setGreenF(color.greenF() * 0.8F);
            color.setBlueF(color.blueF() * 0.8F);
            textFormat.setBackground(color);
            break;
        }
        default: {
            break;
        }
    }
}

namespace {

void ensureTextContrast(QTextCharFormat& textFormat)
{
    auto bgColor = textFormat.background().color();
    auto fgColor = textFormat.foreground().color();

    auto bgLuma = Rainbow::luma(bgColor);
    auto fgLuma = Rainbow::luma(fgColor);

    auto contrast = Rainbow::contrastRatio(fgColor, bgColor);
    // if contrast isn't readable
    if (contrast < 8.0) {
        // invert strait black / white and darken / lighten others
        if (bgLuma < 0.5) {
            if (fgColor == Qt::black) {
                fgColor = Qt::white;
            } else {
                // background is dark, lighten (the darker the color the more lightening it needs)
                fgColor = Rainbow::lighten(fgColor, (1.0 - fgLuma) - ((1.0 - bgLuma) / 2));
            }
        } else {
            if (fgColor == Qt::white) {
                fgColor = Qt::black;
            } else {
                // background is light, darken (the brighter the color the more darkening it needs)
                fgColor = Rainbow::darken(fgColor, fgLuma + (bgLuma / 2));
            }
        }
        textFormat.setForeground(fgColor);
    }
}

}  // namespace

void CrashHandlerDialog::setTextWithTermFormatting(QTextEdit* textEdit, const QString& text)
{
    QTextDocument* document = textEdit->document();
    const QRegularExpression escapeSeq(R"(\x1B\[([\d;]+)m)");
    QTextCursor cursor(document);
    auto bg = textEdit->textBackgroundColor();
    qDebug() << "textedit bg color" << bg;
    QTextCharFormat defaultFormat = textEdit->currentCharFormat();
    defaultFormat.setBackground(bg);

    cursor.beginEditBlock();

    // find ANSI escape sequences
    auto match = escapeSeq.match(text);
    auto offset = match.capturedStart();

    // insert text to the first offset
    cursor.insertText(text.mid(0, offset));
    QTextCharFormat textFormat = defaultFormat;

    // while there are ANSI Codes...
    while (!(offset < 0)) {
        // update offset
        auto lastOffset = offset + match.capturedLength();

        // split multi codes from one sequence
        QStringList escapeSequences = match.capturedTexts().back().split(';');
        QListIterator<QString> iter(escapeSequences);
        // apply codes to text format
        while (iter.hasNext()) {
            bool ok = false;
            std::uint32_t attribute = iter.next().toUInt(&ok);
            Q_ASSERT(ok);
            parseEscapeSequence(attribute, iter, textFormat, defaultFormat);
        }

        // find next escape sequence
        match = escapeSeq.match(text, lastOffset);
        offset = match.capturedStart();

        // ensure applied format is readable with current text background
        ensureTextContrast(textFormat);

        // insert text under current formating
        if (offset < 0) {
            cursor.insertText(text.mid(lastOffset), textFormat);
        } else {
            cursor.insertText(text.mid(lastOffset, offset - lastOffset), textFormat);
        }
    }

    // reset formatting to default
    cursor.setCharFormat(defaultFormat);
    cursor.endEditBlock();
    textEdit->setTextCursor(cursor);
}

}  // namespace CrashHandler
