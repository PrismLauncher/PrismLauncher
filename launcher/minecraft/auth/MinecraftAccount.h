/* Copyright 2013-2021 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <QObject>
#include <QString>
#include <QList>
#include <QJsonObject>
#include <QPair>
#include <QMap>
#include <QPixmap>

#include <memory>
#include "AuthSession.h"
#include "Usable.h"
#include "AccountData.h"
#include "QObjectPtr.h"

class Task;
class AccountTask;
class MinecraftAccount;

typedef shared_qobject_ptr<MinecraftAccount> MinecraftAccountPtr;
Q_DECLARE_METATYPE(MinecraftAccountPtr)

/**
 * A profile within someone's Mojang account.
 *
 * Currently, the profile system has not been implemented by Mojang yet,
 * but we might as well add some things for it in MultiMC right now so
 * we don't have to rip the code to pieces to add it later.
 */
struct AccountProfile
{
    QString id;
    QString name;
    bool legacy;
};

enum AccountStatus
{
    NotVerified,
    Verified
};

/**
 * Object that stores information about a certain Mojang account.
 *
 * Said information may include things such as that account's username, client token, and access
 * token if the user chose to stay logged in.
 */
class MinecraftAccount :
    public QObject,
    public Usable
{
    Q_OBJECT
public: /* construction */
    //! Do not copy accounts. ever.
    explicit MinecraftAccount(const MinecraftAccount &other, QObject *parent) = delete;

    //! Default constructor
    explicit MinecraftAccount(QObject *parent = 0) : QObject(parent) {};

    static MinecraftAccountPtr createFromUsername(const QString &username);

    static MinecraftAccountPtr createBlankMSA();

    static MinecraftAccountPtr loadFromJsonV2(const QJsonObject &json);
    static MinecraftAccountPtr loadFromJsonV3(const QJsonObject &json);

    //! Saves a MinecraftAccount to a JSON object and returns it.
    QJsonObject saveToJson() const;

public: /* manipulation */

    /**
     * Attempt to login. Empty password means we use the token.
     * If the attempt fails because we already are performing some task, it returns false.
     */
    shared_qobject_ptr<AccountTask> login(AuthSessionPtr session, QString password);

    shared_qobject_ptr<AccountTask> loginMSA(AuthSessionPtr session);

    shared_qobject_ptr<AccountTask> refresh(AuthSessionPtr session);

public: /* queries */
    QString accountDisplayString() const {
        return data.accountDisplayString();
    }

    QString mojangUserName() const {
        return data.userName();
    }

    QString accessToken() const {
        return data.accessToken();
    }

    QString profileId() const {
        return data.profileId();
    }

    QString profileName() const {
        return data.profileName();
    }

    bool isActive() const;

    bool canMigrate() const {
        return data.canMigrateToMSA;
    }

    bool isMSA() const {
        return data.type == AccountType::MSA;
    }

    QString typeString() const {
        switch(data.type) {
            case AccountType::Mojang: {
                if(data.legacy) {
                    return "legacy";
                }
                return "mojang";
            }
            break;
            case AccountType::MSA: {
                return "msa";
            }
            break;
            default: {
                return "unknown";
            }
        }
    }

    QPixmap getFace() const;

    //! Returns whether the account is NotVerified, Verified or Online
    AccountStatus accountStatus() const;

    AccountData * accountData() {
        return &data;
    }

    bool shouldRefresh() const;

signals:
    /**
     * This signal is emitted when the account changes
     */
    void changed();

    void activityChanged(bool active);

    // TODO: better signalling for the various possible state changes - especially errors

protected: /* variables */
    AccountData data;

    // current task we are executing here
    shared_qobject_ptr<AccountTask> m_currentTask;

protected: /* methods */

    void incrementUses() override;
    void decrementUses() override;

private
slots:
    void authSucceeded();
    void authFailed(QString reason);

private:
    void fillSession(AuthSessionPtr session);
};
