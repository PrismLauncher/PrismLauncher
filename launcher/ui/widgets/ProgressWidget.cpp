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
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label = new QLabel(this);
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label->setWordWrap(true);
        layout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label);
    }

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar = new QProgressBar(this);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setMinimum(0);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setMaximum(100);
    layout->addWidget(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar);

    setLayout(layout);
}

void ProgressWidget::reset()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->reset();
}

void ProgressWidget::progressFormat(QString format)
{
    if (format.isEmpty())
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setTextVisible(false);
    else
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setFormat(format);
}

void ProgressWidget::watch(Task* task)
{
    if (!task)
        return;

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task)
        disconnect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, nullptr, this, nullptr);

    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task = task;

    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, &Task::finished, this, &ProgressWidget::handleTaskFinish);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, &Task::status, this, &ProgressWidget::handleTaskStatus);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, &Task::progress, this, &ProgressWidget::handleTaskProgress);
    connect(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, &Task::destroyed, this, &ProgressWidget::taskDestroyed);

    show();
}

void ProgressWidget::start(Task* task)
{
    watch(task);
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task->isRunning())
        QMetaObject::invokeMethod(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task, "start", Qt::QueuedConnection);
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
    if (!hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task->wasSuccessful() && hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label->setText(hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task->failReason());

    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_hide_if_inactive)
        hide();
}
void ProgressWidget::handleTaskStatus(const QString& status)
{
    if (hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label)
        hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_label->setText(status);
}
void ProgressWidget::handleTaskProgress(qint64 current, qint64 total)
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setMaximum(total);
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_bar->setValue(current);
}
void ProgressWidget::taskDestroyed()
{
    hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_task = nullptr;
}
