#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include <minecraft/auth/AuthSession.h>
#include "tasks/Task.h"

class CapeChange : public Task
{
    Q_OBJECT
public:
    CapeChange(QObject *parent, AuthSessionPtr session, QString capeId);
    virtual ~CapeChange() {}

private:
    void setCape(QString & cape);
    void clearCape();

private:
    QString m_capeId;
    AuthSessionPtr m_session;
    std::shared_ptr<QNetworkReply> m_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};

