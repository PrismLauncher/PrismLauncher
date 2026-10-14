// SPDX-License-Identifier: GPL-3.0-only
#pragma once

#include <QFrame>
#include <QList>

class Task;
class QLabel;
class QProgressBar;
class QToolButton;
class QVBoxLayout;

// Lists running tasks with progress bars, keeps failures around as copyable rows, and
// shows completed actions that need attention. Lives in the activity drawer above the
// activity strip; replaces modal progress dialogs in the new layout.
// addTask takes ownership of the task and deletes it when its row is dismissed.
class ActivityDrawer : public QFrame {
    Q_OBJECT

   public:
    explicit ActivityDrawer(QWidget* parent = nullptr);
    ~ActivityDrawer() override = default;

    void addTask(Task* task);
    // A completed action that needs attention, such as a pack update that is ready
    void addNotice(const QString& title, const QString& body);

    int activeTaskCount() const { return m_activeTasks; }

   public slots:
    void retranslate();

   signals:
    void activeTaskCountChanged(int count);

   private:
    struct Row {
        QWidget* widget = nullptr;
        QLabel* title = nullptr;
        QLabel* status = nullptr;
        QProgressBar* bar = nullptr;
        QToolButton* action = nullptr;   // Abort while running, Copy once failed
        QToolButton* dismiss = nullptr;  // Failed rows only
        Task* task = nullptr;            // owned by the drawer
        bool counted = false;            // contributes to activeTaskCount
    };
    Row* makeRow();
    void finishRow(Row* row, bool failed, const QString& reason);
    void removeRow(Row* row);
    void updateHeader();

    QWidget* m_rowsWidget = nullptr;
    QVBoxLayout* m_rowsLayout = nullptr;
    QLabel* m_placeholder = nullptr;
    QList<Row*> m_rows;
    int m_activeTasks = 0;
};
