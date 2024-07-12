#pragma once
#include <QObject>
#include <memory>

#include "minecraft/auth/AuthStep.h"
#include "net/NetJob.h"
#include "net/Upload.h"

class XboxAuthorizationStep : public AuthStep {
    Q_OBJECT

   public:
    explicit XboxAuthorizationStep(AccountData* data, Token* token, QString relyingParty, QString authorizationKind);
    virtual ~XboxAuthorizationStep() noexcept = default;

    void perform() override;

    QString describe() override;

   private:
    bool processSTSError();

   private slots:
    void onRequestDone();

   private:
    Token* m_token;
    QString m_relyingParty;
    QString m_authorizationKind;

    std::shared_ptr<QByteArray> m_response;
    Net::Upload::Ptr m_request;
    NetJob::Ptr m_task;
};
