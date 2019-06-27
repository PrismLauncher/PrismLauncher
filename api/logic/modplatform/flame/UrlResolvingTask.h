#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

#include "multimc_logic_export.h"

namespace Flame
{
class MULTIMC_LOGIC_EXPORT UrlResolvingTask : public Task
{
    Q_OBJECT
public:
    explicit UrlResolvingTask(const QString &toProcess);
    virtual ~UrlResolvingTask() {};

    const Flame::File &getResults() const
    {
        return m_result;
    }

protected:
    virtual void executeTask() override;

protected slots:
    void processCCIP();
    void processHTML();
    void netJobFinished();

private: /* data */
    QString m_url;
    QString needle;
    Flame::File m_result;
    QByteArray results;
    NetJobPtr m_dljob;
    bool weAreDigging = false;
};
}

