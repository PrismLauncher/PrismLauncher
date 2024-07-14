// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include <QWidget>
#include "tasks/Task.h"

class QProgressBar;
class QLabel;

class ProgressWidget : public QWidget {
    Q_OBJECT
   public:
    explicit ProgressWidget(QWidget* parent = nullptr, bool show_label = true);

    /** Whether to hide the widget automatically if it's watching no running task. */
    void hideIfInactive(bool hide) { m_hide_if_inactive = hide; }

    /** Reset the displayed progress to 0 */
    void reset();

    /** The text that shows up in the middle of the progress bar.
     *  By default it's '%p%', with '%p' being the total progress in percentage.
     */
    void progressFormat(QString);

   public slots:
    /** Watch the progress of a task. */
    void watch(TaskV2* taskV2);

    /** Watch the progress of a task, and start it if needed */
    void start(TaskV2* task);

    /** Blocking way of waiting for a task to finish. */
    bool exec(TaskV2::Ptr task);

    /** Un-hide the widget if needed. */
    void show();

    /** Make the widget invisible. */
    void hide();

   private slots:
    void handleTaskFinish();
    void handleTaskStatus(TaskV2* t);
    void taskDestroyed();

   private:
    QLabel* m_label = nullptr;
    QProgressBar* m_bar = nullptr;
    TaskV2* m_task = nullptr;

    bool m_hide_if_inactive = false;
};
