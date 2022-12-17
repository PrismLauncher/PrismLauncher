#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include "tasks/Task.h"
#include "QObjectPtr.h"

class CapeChange : public Task
{
    Q_OBJECT
public:
    CapeChange(QObject *parent, QString token, QString capeId);
    virtual ~CapeChange() {}

private:
    void setCape(QString & cape);
    void clearCape();

private:
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_capeId;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;
    shared_qobject_ptr<QNetworkReply> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};

