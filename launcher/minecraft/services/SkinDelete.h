#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include <minecraft/auth/AuthSession.h>
#include "tasks/Task.h"

typedef std::shared_ptr<class SkinDelete> SkinDeletePtr;

class SkinDelete : public Task
{
    Q_OBJECT
public:
    SkinDelete(QObject *parent, AuthSessionPtr session);
    virtual ~SkinDelete() = default;

private:
    AuthSessionPtr m_session;
    std::shared_ptr<QNetworkReply> m_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};

