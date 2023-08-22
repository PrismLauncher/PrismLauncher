// Licensed under the Apache-2.0 license. See README.md for details.

#include "ProgressWidget.h"
#include <QEventLoop>
#include <QLabel>
#include <QProgressBar>
#include <QVBoxLayout>

#include "tasks/Task.h"

ProgressWidget::ProgressWidget(QWidget* parent, bool show_label) : QWidget(parent)
{
    auto* layout = new QVBoxLayout(this);

    if (show_label) {
        m_label = new QLabel(this);
        m_label->setWordWrap(true);
        layout->addWidget(m_label);
    }

    m_bar = new QProgressBar(this);
    m_bar->setMinimum(0);
    m_bar->setMaximum(100);
    layout->addWidget(m_bar);

    setLayout(layout);
}

void ProgressWidget::reset()
{
    m_bar->reset();
}

void ProgressWidget::progressFormat(QString format)
{
    if (format.isEmpty())
        m_bar->setTextVisible(false);
    else
        m_bar->setFormat(format);
}

void ProgressWidget::watch(const Task* task)
{
    if (!task)
        return;

    if (m_task)
        disconnect(m_task, nullptr, this, nullptr);

    m_task = task;

    connect(m_task, &Task::finished, this, &ProgressWidget::handleTaskFinish);
    connect(m_task, &Task::status, this, &ProgressWidget::handleTaskStatus);
    // TODO: should we connect &Task::details
    connect(m_task, &Task::progress, this, &ProgressWidget::handleTaskProgress);
    connect(m_task, &Task::destroyed, this, &ProgressWidget::taskDestroyed);

    if (m_task->isRunning())
        show();
    else
        connect(m_task, &Task::started, this, &ProgressWidget::show);
}

void ProgressWidget::start(const Task* task)
{
    watch(task);
    if (!m_task->isRunning())
        QMetaObject::invokeMethod(const_cast<Task*>(m_task), "start", Qt::QueuedConnection);
}

bool ProgressWidget::exec(std::shared_ptr<Task> task)
{
    QEventLoop loop;

    connect(task.get(), &Task::finished, &loop, &QEventLoop::quit);

    start(task.get());

    if (task->isRunning())
        loop.exec();

    return task->wasSuccessful();
}

void ProgressWidget::show()
{
    setHidden(false);
}
void ProgressWidget::hide()
{
    setHidden(true);
}

void ProgressWidget::handleTaskFinish()
{
    if (!m_task->wasSuccessful() && m_label)
        m_label->setText(m_task->failReason());

    if (m_hide_if_inactive)
        hide();
}
void ProgressWidget::handleTaskStatus(const QString& status)
{
    if (m_label)
        m_label->setText(status);
}
void ProgressWidget::handleTaskProgress(qint64 current, qint64 total)
{
    m_bar->setMaximum(total);
    m_bar->setValue(current);
}
void ProgressWidget::taskDestroyed()
{
    m_task = nullptr;
}
