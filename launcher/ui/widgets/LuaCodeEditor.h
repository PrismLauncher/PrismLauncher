// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QHash>
#include <QList>
#include <QPlainTextEdit>
#include <QString>

class QCompleter;
class QStandardItemModel;

// A plain text editor for Lua code: monospace, line numbers, syntax highlighting, problem markers and completion
class LuaCodeEditor : public QPlainTextEdit {
    Q_OBJECT

   public:
    struct Problem {
        // 1-based
        int line;
        QString message;
    };

    explicit LuaCodeEditor(QWidget* parent = nullptr);

    struct Completion {
        // May contain dots, e.g. "prism.log.info"
        QString name;
        QString signature;
        QString doc;
    };

    // Marks the lines with problems; hovering one shows its message
    void setProblems(const QList<Problem>& problems);
    QList<Problem> problems() const { return m_problems; }

    // Offered while typing (or with Ctrl+Space), together with the names already used in the text
    void setCompletions(const QList<Completion>& completions);
    QStringList completionsFor(const QString& prefix);
    QCompleter* completer() const { return m_completer; }

    int lineNumberAreaWidth() const;
    void paintLineNumberArea(QPaintEvent* event);

   signals:
    // The name the user is looking at: the one under the cursor, or the completion highlighted in the popup
    void focusedNameChanged(const QString& name);

   protected:
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

   private:
    void updateLineNumberArea(const QRect& rect, int dy);
    void updateMarkers();
    void rebuildCompletionModel();
    void insertCompletion(const QString& completion);
    // Without a selection, copy and cut take the whole line, and pasting a whole line puts it below the current one
    bool handleLineClipboard(QKeyEvent* event);
    void copyLine(bool cut);
    void pasteLineBelow();
    void setFocusedName(const QString& name);
    QString prefixAtCursor() const;
    QString nameAtCursor() const;

    QWidget* m_lineNumberArea;
    QList<Problem> m_problems;
    QList<Completion> m_completions;
    QCompleter* m_completer;
    QStandardItemModel* m_completionModel;
    bool m_completionModelDirty = true;
    QString m_focusedName;
};
