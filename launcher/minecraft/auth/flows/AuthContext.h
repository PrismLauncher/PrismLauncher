#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QNetworkReply>
#include <QImage>

#include <katabasis/OAuth2.h>
#include "Yggdrasil.h"
#include "../AccountData.h"
#include "../AccountTask.h"

class AuthContext : public AccountTask
{
    Q_OBJECT

public:
    explicit AuthContext(AccountData * data, QObject *parent = 0);

    bool isBusy() {
        return m_activity != Katabasis::Activity::Idle;
    };
    Katabasis::Validity validity() {
        return m_data->validity_;
    };

    //bool signOut();

    QString getStateMessage() const override;

signals:
    void activityChanged(Katabasis::Activity activity);

private slots:
// OAuth-specific callbacks
    void onOAuthLinkingSucceeded();
    void onOAuthLinkingFailed();

    void onOAuthActivityChanged(Katabasis::Activity activity);

// Yggdrasil specific callbacks
    void onMojangSucceeded();
    void onMojangFailed();

protected:
    void initMSA();
    void initMojang();

    void doUserAuth();
    Q_SLOT void onUserAuthDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doSTSAuthMinecraft();
    Q_SLOT void onSTSAuthMinecraftDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
    void doMinecraftAuth();
    Q_SLOT void onMinecraftAuthDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doSTSAuthGeneric();
    Q_SLOT void onSTSAuthGenericDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
    void doXBoxProfile();
    Q_SLOT void onXBoxProfileDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doMinecraftProfile();
    Q_SLOT void onMinecraftProfileDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doGetSkin();
    Q_SLOT void onSkinDone(int, QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void checkResult();

protected:
    void beginActivity(Katabasis::Activity activity);
    void finishActivity();
    void clearTokens();

protected:
    Katabasis::OAuth2 *m_oauth2 = nullptr;
    Yggdrasil *m_yggdrasil = nullptr;

    int m_requestsDone = 0;
    bool m_xboxProfileSucceeded = false;
    bool m_mcAuthSucceeded = false;
    Katabasis::Activity m_activity = Katabasis::Activity::Idle;
    enum class AuthStage {
        Initial,
        UserAuth,
        XboxAuth,
        MinecraftProfile,
        Skin,
        Complete
    } m_stage = AuthStage::Initial;

    void setStage(AuthStage stage);

    QNetworkAccessManager *mgr = nullptr;
};
