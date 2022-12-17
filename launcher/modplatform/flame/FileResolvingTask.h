#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

namespace Flame
{
class FileResolvingTask : public Task
{
    Q_OBJECT
public:
    explicit FileResolvingTask(const shared_qobject_ptr<QNetworkAccessManager>& network, Flame::Manifest &toProcess);
    virtual ~FileResolvingTask() {};

    bool canAbort() const override { return true; }
    bool abort() override;

    const Flame::Manifest &getResults() const
    {
        return hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess;
    }

protected:
    virtual void executeTask() override;

protected slots:
    void netJobFinished();

private: /* data */
    shared_qobject_ptr<QNetworkAccessManager> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_network;
    Flame::Manifest hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_toProcess;
	std::shared_ptr<QByteArray> result;
    NetJob::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_dljob;
	NetJob::Ptr hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_checkJob;

    void modrinthCheckFinished();

    QMap<File *, QByteArray *> blockedProjects;
};
}
