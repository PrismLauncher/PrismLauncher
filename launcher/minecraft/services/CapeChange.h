#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include <memory>
#include "tasks/Task.h"
#include "QObjectPtr.h"
#include <minecraft/auth/MinecraftAccount.h>

class CapeChange : public Task
{
    Q_OBJECT
public:
    CapeChange(QObject *parent, MinecraftAccountPtr m_acct, QString capeId);
    virtual ~CapeChange() {}

private:
    void setCape(QString & cape);
    void clearCape();

private:
    QString m_capeId;
    MinecraftAccountPtr m_acct;
    shared_qobject_ptr<QNetworkReply> m_reply;

protected:
    virtual void executeTask();

public slots:
    void downloadError(QNetworkReply::NetworkError);
    void downloadFinished();
};

