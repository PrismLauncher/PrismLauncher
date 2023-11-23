#pragma once

#include <QFile>
#include <QtNetwork/QtNetwork>
#include "tasks/Task.h"

using SkinDeletePtr = shared_qobject_ptr<class SkinDelete>;

class SkinDelete : public Task {
    Q_OBJECT
   public:
    SkinDelete(QObject* parent, QString token);
    virtual ~SkinDelete() = default;

   private:
    QString m_token;
    shared_qobject_ptr<QNetworkReply> m_reply;

   protected:
    virtual void executeTask();

   public slots:
    void downloadError(QNetworkReply::NetworkError);
    void sslErrors(const QList<QSslError>& errors);
    void downloadFinished();
};
