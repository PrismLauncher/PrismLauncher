// SPDX-License-Identifier: GPL-3.0-only
#include "ActivityDrawer.h"

#include <QApplication>
#include <QClipboard>
#include <QHBoxLayout>
#include <QLabel>
#include <QProgressBar>
#include <QScrollArea>
#include <QTimer>
#include <QToolButton>
#include <QVBoxLayout>

#include "tasks/Task.h"

ActivityDrawer::ActivityDrawer(QWidget* parent) : QFrame(parent)
{
    setFrameShape(QFrame::StyledPanel);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    m_placeholder = new QLabel(tr("Nothing is running."), this);
    m_placeholder->setAlignment(Qt::AlignCenter);
    m_placeholder->setEnabled(false);
    layout->addWidget(m_placeholder);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    scroll->setMaximumHeight(150);
    m_rowsWidget = new QWidget(scroll);
    m_rowsLayout = new QVBoxLayout(m_rowsWidget);
    m_rowsLayout->setContentsMargins(0, 0, 0, 0);
    m_rowsLayout->setSpacing(4);
    m_rowsLayout->addStretch(1);
    scroll->setWidget(m_rowsWidget);
    layout->addWidget(scroll, 1);

    updateHeader();
}

ActivityDrawer::Row* ActivityDrawer::makeRow()
{
    auto* row = new Row;
    row->widget = new QWidget(m_rowsWidget);
    auto* layout = new QHBoxLayout(row->widget);
    layout->setContentsMargins(8, 4, 8, 4);
    layout->setSpacing(8);

    auto* textColumn = new QVBoxLayout;
    textColumn->setSpacing(2);
    row->title = new QLabel(row->widget);
    row->title->setTextFormat(Qt::PlainText);
    QFont titleFont = row->title->font();
    titleFont.setBold(true);
    row->title->setFont(titleFont);
    textColumn->addWidget(row->title);
    row->status = new QLabel(row->widget);
    row->status->setTextFormat(Qt::PlainText);
    row->status->setWordWrap(true);
    textColumn->addWidget(row->status);

    row->bar = new QProgressBar(row->widget);
    row->bar->setRange(0, 0);  // indeterminate until the task reports progress
    row->bar->setTextVisible(false);
    row->bar->setFixedWidth(140);

    row->action = new QToolButton(row->widget);
    row->action->setAutoRaise(true);
    row->action->setToolButtonStyle(Qt::ToolButtonTextOnly);
    row->action->setFocusPolicy(Qt::NoFocus);
    row->action->setVisible(false);

    row->dismiss = new QToolButton(row->widget);
    row->dismiss->setAutoRaise(true);
    row->dismiss->setFocusPolicy(Qt::NoFocus);
    row->dismiss->setText(QStringLiteral("✕"));
    row->dismiss->setVisible(false);

    layout->addLayout(textColumn, 1);
    layout->addWidget(row->bar);
    layout->addWidget(row->action);
    layout->addWidget(row->dismiss);

    m_rowsLayout->insertWidget(m_rowsLayout->count() - 1, row->widget);
    m_rows.append(row);
    return row;
}

void ActivityDrawer::addTask(Task* task)
{
    auto* row = makeRow();
    row->task = task;

    row->status->setText(task->getStatus());
    row->title->setText(task->getDetails().isEmpty() ? task->getStatus() : task->getDetails());

    connect(task, &Task::started, this, [this, row] {
        row->counted = true;
        ++m_activeTasks;
        emit activeTaskCountChanged(m_activeTasks);
    });
    connect(task, &Task::status, this, [row](const QString& text) {
        row->status->setText(text);
        if (row->title->text().isEmpty())
            row->title->setText(text);
    });
    connect(task, &Task::details, this, [row](const QString& text) { row->title->setText(text); });
    connect(task, &Task::progress, this, [row](qint64 current, qint64 total) {
        if (total > 0) {
            row->bar->setRange(0, int(total));
            row->bar->setValue(int(current));
        } else {
            row->bar->setRange(0, 0);
        }
    });
    connect(task, &Task::abortStatusChanged, this, [row](bool canAbort) {
        row->action->setText(tr("Abort"));
        row->action->setToolTip(tr("Cancel this task"));
        row->action->setVisible(canAbort);
    });
    connect(task, &Task::abortButtonTextChanged, this, [row](const QString& text) { row->action->setText(text); });
    connect(row->action, &QToolButton::clicked, this, [row] {
        if (row->task)
            row->task->abort();
    });
    connect(task, &Task::succeeded, this, [this, row] { finishRow(row, false, QString()); });
    connect(task, &Task::failed, this, [this, row](const QString& reason) { finishRow(row, true, reason); });
    connect(task, &Task::aborted, this, [this, row] { removeRow(row); });

    if (task->isRunning()) {
        row->counted = true;
        ++m_activeTasks;
        emit activeTaskCountChanged(m_activeTasks);
    }
    updateHeader();
}

void ActivityDrawer::addNotice(const QString& title, const QString& body)
{
    auto* row = makeRow();
    row->title->setText(title);
    row->status->setText(body);
    row->bar->setVisible(false);
    row->action->setText(tr("Copy"));
    row->action->setToolTip(tr("Copy the message"));
    row->action->setVisible(true);
    connect(row->action, &QToolButton::clicked, this, [body] { QApplication::clipboard()->setText(body); });
    row->dismiss->setToolTip(tr("Dismiss"));
    row->dismiss->setVisible(true);
    connect(row->dismiss, &QToolButton::clicked, this, [this, row] { removeRow(row); });
    updateHeader();
}

void ActivityDrawer::finishRow(Row* row, bool failed, const QString& reason)
{
    if (row->task) {
        disconnect(row->task, nullptr, this, nullptr);
        row->task = nullptr;
    }
    if (row->counted) {
        row->counted = false;
        --m_activeTasks;
        emit activeTaskCountChanged(m_activeTasks);
    }
    row->bar->setVisible(false);

    if (failed) {
        row->status->setText(reason);
        row->action->setText(tr("Copy"));
        row->action->setToolTip(tr("Copy the error message"));
        row->action->setVisible(true);
        connect(row->action, &QToolButton::clicked, this, [reason] { QApplication::clipboard()->setText(reason); });
        row->dismiss->setToolTip(tr("Dismiss"));
        row->dismiss->setVisible(true);
        connect(row->dismiss, &QToolButton::clicked, this, [this, row] { removeRow(row); });
    } else {
        row->status->setText(tr("Finished"));
        QTimer::singleShot(2000, this, [this, row] { removeRow(row); });
    }
}

void ActivityDrawer::removeRow(Row* row)
{
    if (row->task) {
        disconnect(row->task, nullptr, this, nullptr);
        row->task->deleteLater();
        row->task = nullptr;
    }
    if (row->counted) {
        row->counted = false;
        --m_activeTasks;
        emit activeTaskCountChanged(m_activeTasks);
    }
    m_rows.removeOne(row);
    row->widget->deleteLater();
    delete row;
    updateHeader();
}

void ActivityDrawer::updateHeader()
{
    m_placeholder->setVisible(m_rows.isEmpty());
}

void ActivityDrawer::retranslate()
{
    m_placeholder->setText(tr("Nothing is running."));
    for (auto* row : m_rows) {
        if (row->task) {
            row->action->setText(tr("Abort"));
            row->action->setToolTip(tr("Cancel this task"));
        } else if (row->action->isVisible()) {
            row->action->setText(tr("Copy"));
        }
        if (row->dismiss->isVisible()) {
            row->dismiss->setToolTip(tr("Dismiss"));
        }
    }
}
