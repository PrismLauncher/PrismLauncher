#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include "tasks/Task.h"

typedef shared_qobject_ptr<class SkinDelete> SkinDeletePtr;

class SkinDelete : public Task
{
    Q_OBJECT
public:
    SkinDelete(QObject *parent, QString token);
    virtual ~SkinDelete() = default;

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;
    shared_qobject_ptr<QNetworkReply> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};
