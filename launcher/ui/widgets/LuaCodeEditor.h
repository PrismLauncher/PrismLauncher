// SPDX-FileCopyrightText: 2026 Alberto Valls Ortiz <24375270+albvalort@users.noreply.github.com>
//
// SPDX-License-Identifier: GPL-3.0-only

#pragma once

#include <QList>
#include <QPlainTextEdit>
#include <QString>

// A plain text editor for Lua code: monospace, line numbers, syntax highlighting and problem markers
class LuaCodeEditor : public QPlainTextEdit {
    Q_OBJECT

   public:
    struct Problem {
        // 1-based
        int line;
        QString message;
    };

    explicit LuaCodeEditor(QWidget* parent = nullptr);

    // Marks the lines with problems; hovering one shows its message
    void setProblems(const QList<Problem>& problems);
    QList<Problem> problems() const { return m_problems; }

    int lineNumberAreaWidth() const;
    void paintLineNumberArea(QPaintEvent* event);

   protected:
    void resizeEvent(QResizeEvent* event) override;
    bool event(QEvent* event) override;

   private:
    void updateLineNumberArea(const QRect& rect, int dy);
    void updateMarkers();

    QWidget* m_lineNumberArea;
    QList<Problem> m_problems;
};
