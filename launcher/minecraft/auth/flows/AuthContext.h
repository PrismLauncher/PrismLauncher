#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QSet>
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
    Q_SLOT void onUserAuthDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void processSTSError(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doSTSAuthMinecraft();
    Q_SLOT void onSTSAuthMinecraftDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
    void doMinecraftAuth();
    Q_SLOT void onMinecraftAuthDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doSTSAuthGeneric();
    Q_SLOT void onSTSAuthGenericDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);
    void doXBoxProfile();
    Q_SLOT void onXBoxProfileDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doMinecraftProfile();
    Q_SLOT void onMinecraftProfileDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doMigrationEligibilityCheck();
    Q_SLOT void onMigrationEligibilityCheckDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void doGetSkin();
    Q_SLOT void onSkinDone(QNetworkReply::NetworkError, QByteArray, QList<QNetworkReply::RawHeaderPair>);

    void failResult(bool & flag);
    void succeedResult(bool & flag);
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

    QSet<int64_t> stsErrors;
    bool stsFailed = false;

    Katabasis::Activity m_activity = Katabasis::Activity::Idle;
    enum class AuthStage {
        Initial,
        UserAuth,
        XboxAuth,
        MinecraftProfile,
        MigrationEligibility,
        Skin,
        Complete
    } m_stage = AuthStage::Initial;

    void setStage(AuthStage stage);
};
