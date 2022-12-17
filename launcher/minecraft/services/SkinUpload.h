#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include "tasks/Task.h"

typedef shared_qobject_ptr<class SkinUpload> SkinUploadPtr;

class SkinUpload : public Task
{
    Q_OBJECT
public:
    enum Model
    {
        STEVE,
        ALEX
    };

    // Note this class takes ownership of the file.
    SkinUpload(QObject *parent, QString token, QByteArray skin, Model model = STEVE);
    virtual ~SkinUpload() {}

private:
    Model hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_model;
    QByteArray hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_skin;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;
    shared_qobject_ptr<QNetworkReply> hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_reply;
protected:
    virtual void executeTask();

public slots:

    void downloadError(QNetworkReply::NetworkError);

    void downloadFinished();
};
