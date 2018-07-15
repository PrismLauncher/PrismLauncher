#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

#include "multimc_logic_export.h"

namespace Flame
{
class MULTIMC_LOGIC_EXPORT FileResolvingTask : public Task
{
    Q_OBJECT
public:
    explicit FileResolvingTask(Flame::Manifest &toProcess);
    virtual ~FileResolvingTask() {};

    const Flame::Manifest &getResults() const
    {
        return m_toProcess;
    }

protected:
    virtual void executeTask() override;

protected slots:
    void netJobFinished();

private: /* data */
    Flame::Manifest m_toProcess;
    QVector<QByteArray> results;
    NetJobPtr m_dljob;
};
}
