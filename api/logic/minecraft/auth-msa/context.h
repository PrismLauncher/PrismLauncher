#pragma once

#include <QObject>
#include <QList>
#include <QVector>
#include <QNetworkReply>
#include <QImage>

#include <katabasis/OAuth2.h>

struct Skin {
    QString id;
    QString url;
    QString variant;

    QByteArray data;
};

struct Cape {
    QString id;
    QString url;
    QString alias;

    QByteArray data;
};

struct MinecraftProfile  {
    QString id;
    QString name;
    Skin skin;
    int currentCape = -1;
    QVector<Cape> capes;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

enum class AccountType {
    MSA,
    Mojang
};

struct AccountData {
    AccountType type = AccountType::MSA;

    Katabasis::Token msaToken;
    Katabasis::Token userToken;
    Katabasis::Token xboxApiToken;
    Katabasis::Token mojangservicesToken;
    Katabasis::Token minecraftToken;

    MinecraftProfile minecraftProfile;
    Katabasis::Validity validity_ = Katabasis::Validity::None;
};

class Context : public QObject
{
    Q_OBJECT

public:
    explicit Context(QObject *parent = 0);

    QByteArray saveState();
    bool resumeFromState(QByteArray data);

    bool isBusy() {
        return activity_ != Katabasis::Activity::Idle;
    };
    Katabasis::Validity validity() {
        return m_account.validity_;
    };

    bool signIn();
    bool silentSignIn();
    bool signOut();

    QString userName();
    QString userId();
    QString gameToken();
signals:
    void succeeded();
    void failed();
    void activityChanged(Katabasis::Activity activity);

private slots:
    void onLinkingSucceeded();
    void onLinkingFailed();
    void onOpenBrowser(const QUrl &url);
    void onCloseBrowser();
    void onOAuthActivityChanged(Katabasis::Activity activity);

private:
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

private:
    void beginActivity(Katabasis::Activity activity);
    void finishActivity();
    void clearTokens();

private:
    Katabasis::OAuth2 *oauth2 = nullptr;

    int requestsDone = 0;
    bool xboxProfileSucceeded = false;
    bool mcAuthSucceeded = false;
    Katabasis::Activity activity_ = Katabasis::Activity::Idle;

    AccountData m_account;

    QNetworkAccessManager *mgr = nullptr;
};
