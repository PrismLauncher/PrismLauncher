// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#include "LuaCodeEditor.h"

#include <QAbstractItemView>
#include <QCompleter>
#include <QFontDatabase>
#include <QHelpEvent>
#include <QKeyEvent>
#include <QPainter>
#include <QRegularExpression>
#include <QScrollBar>
#include <QStandardItemModel>
#include <QSyntaxHighlighter>
#include <QTextBlock>
#include <QToolTip>
#include <algorithm>

namespace {

class LineNumberArea : public QWidget {
   public:
    explicit LineNumberArea(LuaCodeEditor* editor) : QWidget(editor), m_editor(editor) {}
    QSize sizeHint() const override { return { m_editor->lineNumberAreaWidth(), 0 }; }

   protected:
    void paintEvent(QPaintEvent* event) override { m_editor->paintLineNumberArea(event); }

   private:
    LuaCodeEditor* m_editor;
};

class LuaHighlighter : public QSyntaxHighlighter {
   public:
    explicit LuaHighlighter(QTextDocument* document) : QSyntaxHighlighter(document)
    {
        const QPalette palette;
        const bool dark = palette.color(QPalette::Base).lightness() < 128;
        m_keyword.setForeground(dark ? QColor("#c792ea") : QColor("#7a3e9d"));
        m_keyword.setFontWeight(QFont::Bold);
        m_string.setForeground(dark ? QColor("#c3e88d") : QColor("#2e7d32"));
        m_number.setForeground(dark ? QColor("#f78c6c") : QColor("#b5520a"));
        m_comment.setForeground(dark ? QColor("#80868b") : QColor("#6a737d"));
        m_comment.setFontItalic(true);
        m_api.setForeground(dark ? QColor("#82aaff") : QColor("#1f5fbf"));
    }

   protected:
    // Block states: 0 normal, 1 inside a long comment, 2 inside a long string
    void highlightBlock(const QString& text) override
    {
        static const QRegularExpression keywords(
            "\\b(and|break|do|else|elseif|end|false|for|function|goto|if|in|local|nil|not|or|repeat|return|then|true|until|while)\\b");
        static const QRegularExpression numbers("\\b(0[xX][0-9a-fA-F]+|\\d+(\\.\\d*)?([eE][+-]?\\d+)?)\\b");
        static const QRegularExpression api("\\bprism(\\.[A-Za-z_]\\w*)*");

        for (const auto& regex : { std::pair{ &keywords, &m_keyword }, std::pair{ &numbers, &m_number }, std::pair{ &api, &m_api } }) {
            auto it = regex.first->globalMatch(text);
            while (it.hasNext()) {
                const auto match = it.next();
                setFormat(static_cast<int>(match.capturedStart()), static_cast<int>(match.capturedLength()), *regex.second);
            }
        }

        // Strings and comments win over the above, so scan them last, left to right
        int i = 0;
        setCurrentBlockState(0);
        if (previousBlockState() == 1 || previousBlockState() == 2) {
            i = closeLong(text, 0, previousBlockState());
        }
        while (i < text.size()) {
            const QChar c = text.at(i);
            if (text.mid(i, 2) == "--") {
                if (text.mid(i + 2, 2) == "[[") {
                    i = closeLong(text, i, 1);
                } else {
                    setFormat(i, static_cast<int>(text.size()) - i, m_comment);
                    return;
                }
            } else if (text.mid(i, 2) == "[[") {
                i = closeLong(text, i, 2);
            } else if (c == '"' || c == '\'') {
                int end = i + 1;
                while (end < text.size() && text.at(end) != c) {
                    end += text.at(end) == '\\' ? 2 : 1;
                }
                end = std::min(end + 1, static_cast<int>(text.size()));
                setFormat(i, end - i, m_string);
                i = end;
            } else {
                i++;
            }
        }
    }

   private:
    // Formats a long comment or string starting at start (or continuing from the previous line) and returns where it
    // ends, or the end of the line if it goes on
    int closeLong(const QString& text, int start, int state)
    {
        const qsizetype end = text.indexOf("]]", start + (previousBlockState() == state && start == 0 ? 0 : 2));
        const int stop = end < 0 ? static_cast<int>(text.size()) : static_cast<int>(end) + 2;
        setFormat(start, stop - start, state == 1 ? m_comment : m_string);
        if (end < 0) {
            setCurrentBlockState(state);
        }
        return stop;
    }

    QTextCharFormat m_keyword;
    QTextCharFormat m_string;
    QTextCharFormat m_number;
    QTextCharFormat m_comment;
    QTextCharFormat m_api;
};

}  // namespace

namespace {

const QRegularExpression s_namePattern("[A-Za-z_][A-Za-z0-9_]*(\\.[A-Za-z_][A-Za-z0-9_]*)*");
const QStringList s_keywords = { "and", "break", "do",  "else", "elseif", "end",    "false",  "for",  "function", "goto",  "if",
                                 "in",  "local", "nil", "not",  "or",     "repeat", "return", "then", "true",     "until", "while" };

bool isNameChar(QChar c)
{
    return c.isLetterOrNumber() || c == '_' || c == '.';
}

}  // namespace

LuaCodeEditor::LuaCodeEditor(QWidget* parent)
    : QPlainTextEdit(parent)
    , m_lineNumberArea(new LineNumberArea(this))
    , m_completer(new QCompleter(this))
    , m_completionModel(new QStandardItemModel(this))
{
    m_completer->setModel(m_completionModel);
    m_completer->setWidget(this);
    m_completer->setCompletionMode(QCompleter::PopupCompletion);
    m_completer->setCaseSensitivity(Qt::CaseInsensitive);
    m_completer->setFilterMode(Qt::MatchStartsWith);
    m_completer->setMaxVisibleItems(12);
    connect(m_completer, qOverload<const QString&>(&QCompleter::activated), this, &LuaCodeEditor::insertCompletion);
    connect(m_completer, qOverload<const QString&>(&QCompleter::highlighted), this, &LuaCodeEditor::setFocusedName);
    connect(this, &QPlainTextEdit::textChanged, this, [this] { m_completionModelDirty = true; });
    connect(this, &QPlainTextEdit::cursorPositionChanged, this, [this] {
        if (!m_completer->popup()->isVisible()) {
            setFocusedName(nameAtCursor());
        }
    });

    setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    setTabStopDistance(fontMetrics().horizontalAdvance(' ') * 4);
    setLineWrapMode(QPlainTextEdit::NoWrap);
    new LuaHighlighter(document());

    connect(this, &QPlainTextEdit::blockCountChanged, this, [this] { setViewportMargins(lineNumberAreaWidth(), 0, 0, 0); });
    connect(this, &QPlainTextEdit::updateRequest, this, &LuaCodeEditor::updateLineNumberArea);
    setViewportMargins(lineNumberAreaWidth(), 0, 0, 0);
}

void LuaCodeEditor::setProblems(const QList<Problem>& problems)
{
    m_problems = problems;
    updateMarkers();
    m_lineNumberArea->update();
}

void LuaCodeEditor::setCompletions(const QList<Completion>& completions)
{
    m_completions = completions;
    m_completionModelDirty = true;
}

QStringList LuaCodeEditor::completionsFor(const QString& prefix)
{
    rebuildCompletionModel();
    m_completer->setCompletionPrefix(prefix);
    QStringList names;
    for (int i = 0; m_completer->setCurrentRow(i); i++) {
        names.append(m_completer->currentCompletion());
    }
    return names;
}

void LuaCodeEditor::rebuildCompletionModel()
{
    if (!m_completionModelDirty) {
        return;
    }
    m_completionModelDirty = false;
    m_completionModel->clear();

    QSet<QString> seen;
    auto add = [this, &seen](const QString& name, const QString& tooltip) {
        if (seen.contains(name)) {
            return;
        }
        seen.insert(name);
        auto* item = new QStandardItem(name);
        if (!tooltip.isEmpty()) {
            item->setToolTip(tooltip);
        }
        m_completionModel->appendRow(item);
    };
    for (const Completion& completion : m_completions) {
        add(completion.name, completion.doc.isEmpty() ? completion.signature : completion.signature + "\n\n" + completion.doc);
    }
    for (const QString& keyword : s_keywords) {
        add(keyword, {});
    }
    // Names already used in the text, e.g. local variables, except the one being typed
    const int cursor = textCursor().position();
    auto matches = s_namePattern.globalMatch(toPlainText());
    while (matches.hasNext()) {
        const auto match = matches.next();
        const bool typing = match.capturedStart() <= cursor && cursor <= match.capturedEnd();
        if (match.capturedLength() >= 3 && !typing) {
            add(match.captured(), {});
        }
    }
    m_completionModel->sort(0);
}

void LuaCodeEditor::keyPressEvent(QKeyEvent* event)
{
    QAbstractItemView* popup = m_completer->popup();
    if (popup->isVisible()) {
        switch (event->key()) {
            case Qt::Key_Enter:
            case Qt::Key_Return:
            case Qt::Key_Escape:
            case Qt::Key_Tab:
            case Qt::Key_Backtab:
                // The completer handles these
                event->ignore();
                return;
            default:
                break;
        }
    }

    const bool force = (event->modifiers() & Qt::ControlModifier) && event->key() == Qt::Key_Space;
    if (!force) {
        QPlainTextEdit::keyPressEvent(event);
    }

    const QString prefix = prefixAtCursor();
    const bool typedName =
        !event->text().isEmpty() && !(event->modifiers() & (Qt::ControlModifier | Qt::AltModifier)) && isNameChar(event->text().back());
    if (!force && (!typedName || prefix.size() < 2)) {
        popup->hide();
        return;
    }

    rebuildCompletionModel();
    m_completer->setCompletionPrefix(prefix);
    if (m_completer->completionCount() == 0 || (m_completer->completionCount() == 1 && m_completer->currentCompletion() == prefix)) {
        popup->hide();
        return;
    }
    popup->setCurrentIndex(m_completer->completionModel()->index(0, 0));
    QRect rect = cursorRect();
    rect.setWidth(popup->sizeHintForColumn(0) + popup->verticalScrollBar()->sizeHint().width() + 8);
    m_completer->complete(rect);
}

void LuaCodeEditor::insertCompletion(const QString& completion)
{
    QTextCursor cursor = textCursor();
    // Matching ignores case, so the typed prefix is replaced as a whole
    cursor.movePosition(QTextCursor::Left, QTextCursor::KeepAnchor, static_cast<int>(m_completer->completionPrefix().size()));
    cursor.insertText(completion);
    setTextCursor(cursor);
    setFocusedName(completion);
}

void LuaCodeEditor::setFocusedName(const QString& name)
{
    if (name != m_focusedName) {
        m_focusedName = name;
        emit focusedNameChanged(name);
    }
}

QString LuaCodeEditor::prefixAtCursor() const
{
    const QTextCursor cursor = textCursor();
    const QString line = cursor.block().text().left(cursor.positionInBlock());
    int start = static_cast<int>(line.size());
    while (start > 0 && isNameChar(line.at(start - 1))) {
        start--;
    }
    return line.mid(start);
}

QString LuaCodeEditor::nameAtCursor() const
{
    const QTextCursor cursor = textCursor();
    const QString line = cursor.block().text();
    int start = cursor.positionInBlock();
    int end = start;
    while (start > 0 && isNameChar(line.at(start - 1))) {
        start--;
    }
    while (end < line.size() && isNameChar(line.at(end))) {
        end++;
    }
    // Without a trailing dot, e.g. "prism.log." while typing
    QString name = line.mid(start, end - start);
    while (name.endsWith('.')) {
        name.chop(1);
    }
    return name;
}

int LuaCodeEditor::lineNumberAreaWidth() const
{
    const int digits = static_cast<int>(QString::number(std::max(1, blockCount())).size());
    return 8 + fontMetrics().horizontalAdvance('9') * digits;
}

void LuaCodeEditor::paintLineNumberArea(QPaintEvent* event)
{
    QPainter painter(m_lineNumberArea);
    painter.fillRect(event->rect(), palette().color(QPalette::AlternateBase));

    QTextBlock block = firstVisibleBlock();
    int top = qRound(blockBoundingGeometry(block).translated(contentOffset()).top());
    while (block.isValid() && top <= event->rect().bottom()) {
        const int height = qRound(blockBoundingRect(block).height());
        if (block.isVisible()) {
            const int line = block.blockNumber() + 1;
            const bool problem = std::any_of(m_problems.begin(), m_problems.end(), [line](const Problem& p) { return p.line == line; });
            painter.setPen(problem ? QColor(Qt::red) : palette().color(QPalette::PlaceholderText));
            painter.drawText(0, top, m_lineNumberArea->width() - 4, height, Qt::AlignRight, QString::number(line));
        }
        block = block.next();
        top += height;
    }
}

void LuaCodeEditor::resizeEvent(QResizeEvent* event)
{
    QPlainTextEdit::resizeEvent(event);
    const QRect area = contentsRect();
    m_lineNumberArea->setGeometry(area.left(), area.top(), lineNumberAreaWidth(), area.height());
}

bool LuaCodeEditor::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip) {
        auto* help = static_cast<QHelpEvent*>(event);
        const int line = cursorForPosition(viewport()->mapFrom(this, help->pos())).blockNumber() + 1;
        QStringList messages;
        for (const Problem& problem : m_problems) {
            if (problem.line == line) {
                messages.append(problem.message);
            }
        }
        if (messages.isEmpty()) {
            QToolTip::hideText();
        } else {
            QToolTip::showText(help->globalPos(), messages.join('\n'), this);
        }
        return true;
    }
    return QPlainTextEdit::event(event);
}

void LuaCodeEditor::updateLineNumberArea(const QRect& rect, int dy)
{
    if (dy) {
        m_lineNumberArea->scroll(0, dy);
    } else {
        m_lineNumberArea->update(0, rect.y(), m_lineNumberArea->width(), rect.height());
    }
}

void LuaCodeEditor::updateMarkers()
{
    QList<QTextEdit::ExtraSelection> selections;
    QColor background(Qt::red);
    background.setAlpha(40);
    for (const Problem& problem : m_problems) {
        const QTextBlock block = document()->findBlockByNumber(problem.line - 1);
        if (!block.isValid()) {
            continue;
        }
        QTextEdit::ExtraSelection selection;
        selection.format.setBackground(background);
        selection.format.setProperty(QTextFormat::FullWidthSelection, true);
        selection.format.setToolTip(problem.message);
        selection.cursor = QTextCursor(block);
        selections.append(selection);
    }
    setExtraSelections(selections);
}
