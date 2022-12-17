#pragma once
#include <QObject>

#include "QObjectPtr.h"
#include "minecraft/auth/AuthStep.h"


class XboxAuthorizationStep : public AuthStep {
    Q_OBJECT

public:
    explicit XboxAuthorizationStep(AccountData *data, Katabasis::Token *token, QString relyingParty, QString authorizationKind);
    virtual ~XboxAuthorizationStep() noexcept;

    void perform() override;
    void rehydrate() override;

    QString describe() override;

private:
    bool processSTSError(
        QNetworkReply::NetworkError error,
        QByteArray data,
        QList<QNetworkReply::RawHeaderPair> headers
    );

private slots:
    void onRequestDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

private:
    Katabasis::Token *hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_token;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_relyingParty;
    QString hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_authorizationKind;
};
