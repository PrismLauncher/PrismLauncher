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

struct MinecraftProfile {
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

    AccountType type = AccountType::MSA;
    bool legacy = false;
    bool canMigrateToMSA = false;

    Katabasis::Token msaToken;
    Katabasis::Token userToken;
    Katabasis::Token xboxApiToken;
    Katabasis::Token mojangservicesToken;

    Katabasis::Token yggdrasilToken;
    MinecraftProfile minecraftProfile;
    Katabasis::Validity validity_ = Katabasis::Validity::None;
};
