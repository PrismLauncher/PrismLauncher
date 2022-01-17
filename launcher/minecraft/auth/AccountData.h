#pragma once
#include <QString>
#include <QByteArray>
#include <QVector>
#include <katabasis/Bits.h>
#include <QJsonObject>

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

struct MinecraftEntitlement {
    bool ownsMinecraft = false;
    bool canPlayMinecraft = false;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

struct MinecraftProfile {
    QString id;
    QString name;
    Skin skin;
    QString currentCape;
    QMap<QString, Cape> capes;
    Katabasis::Validity validity = Katabasis::Validity::None;
};

enum class AccountType {
    MSA,
    Mojang,
    Offline
};

enum class AccountState {
    Unchecked,
    Offline,
    Working,
    Online,
    Errored,
    Expired,
    Gone
};

struct AccountData {
    QJsonObject saveState() const;
    bool resumeStateFromV2(QJsonObject data);
    bool resumeStateFromV3(QJsonObject data);

    //! userName for Mojang accounts, gamertag for MSA
    QString accountDisplayString() const;

    //! Only valid for Mojang accounts. MSA does not preserve this information
    QString userName() const;

    //! Only valid for Mojang accounts.
    QString clientToken() const;
    void setClientToken(QString clientToken);
    void invalidateClientToken();
    void generateClientTokenIfMissing();

    //! Yggdrasil access token, as passed to the game.
    QString accessToken() const;

    QString profileId() const;
    QString profileName() const;

    QString lastError() const;

    AccountType type = AccountType::MSA;
    bool legacy = false;
    bool canMigrateToMSA = false;

    Katabasis::Token msaToken;
    Katabasis::Token userToken;
    Katabasis::Token xboxApiToken;
    Katabasis::Token mojangservicesToken;

    Katabasis::Token yggdrasilToken;
    MinecraftProfile minecraftProfile;
    MinecraftEntitlement minecraftEntitlement;
    Katabasis::Validity validity_ = Katabasis::Validity::None;

    // runtime only information (not saved with the account)
    QString internalId;
    QString errorString;
    AccountState accountState = AccountState::Unchecked;
};
