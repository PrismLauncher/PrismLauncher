#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <minecraft/auth/AuthSession.h>
#include "tasks/Task.h"

typedef shared_qobject_ptr<class SkinDelete> SkinDeletePtr;

class SkinDelete : public Task
{
    Q_OBJECT
public:
    SkinDelete(QObject *parent, AuthSessionPtr session);
    virtual ~SkinDelete() = default;

private:
    AuthSessionPtr m_session;
    shared_qobject_ptr<QNetworkReply> m_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};

