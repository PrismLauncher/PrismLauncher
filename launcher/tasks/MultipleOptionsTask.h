#pragma once

#include "SequentialTask.h"

/* This task type will attempt to do run each of it's subtasks in sequence,
 * until one of them succeeds. When that happens, the remaining tasks will not run.
 * */
class MultipleOptionsTask : public SequentialTask
{
    Q_OBJECT
public:
    explicit MultipleOptionsTask(QObject *parent = nullptr, const QString& task_name = "");
    virtual ~MultipleOptionsTask() = default;

private
slots:
    void startNext() override;
    void subTaskFailed(const QString &msg) override;
};
