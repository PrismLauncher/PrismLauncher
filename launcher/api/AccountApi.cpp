// SPDX-License-Identifier: GPL-3.0-only
/*
 *  MaterialMC - Minecraft Launcher
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "AccountApi.h"

#include <QJsonArray>
#include <QRegularExpression>

#include "Application.h"
#include "minecraft/auth/AccountList.h"

#include "ApiRouter.h"
#include "ApiUtils.h"
#include "TaskTracker.h"

namespace api {

namespace {

QString stateName(AccountState state)
{
    switch (state) {
        case AccountState::Unchecked:
            return "unchecked";
        case AccountState::Offline:
            return "offline";
        case AccountState::Working:
            return "working";
        case AccountState::Online:
            return "online";
        case AccountState::Disabled:
            return "disabled";
        case AccountState::Errored:
            return "errored";
        case AccountState::Expired:
            return "expired";
        case AccountState::Gone:
            return "gone";
    }
    return "unchecked";
}

QJsonObject serializeAccount(const MinecraftAccountPtr& account, const MinecraftAccountPtr& defaultAccount)
{
    return {
        { "id", account->internalId() },
        { "profileId", account->profileId() },
        { "profileName", account->profileName() },
        { "type", account->accountType() == AccountType::MSA ? "msa" : "offline" },
        { "state", stateName(account->accountState()) },
        { "isDefault", defaultAccount == account },
        { "ownsMinecraft", account->ownsMinecraft() },
        { "hasProfile", account->hasProfile() },
        { "faceUrl", hostResourceUrl({ "_face", account->internalId() }) },
        { "lastError", account->lastError() },
    };
}

int rowOf(const MinecraftAccountPtr& account)
{
    auto* accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); i++) {
        if (accounts->at(i) == account) {
            return i;
        }
    }
    return -1;
}

}  // namespace

AccountApi::AccountApi(ApiRouter* router, TaskTracker* tasks, QObject* parent) : QObject(parent), m_router(router), m_tasks(tasks)
{
    m_changedTimer.setSingleShot(true);
    m_changedTimer.setInterval(100);
    connect(&m_changedTimer, &QTimer::timeout, this, [this] { m_router->emitEvent("account.changed"); });
    auto schedule = [this] { m_changedTimer.start(); };
    auto* accounts = APPLICATION->accounts();
    connect(accounts, &AccountList::listChanged, this, schedule);
    connect(accounts, &AccountList::defaultAccountChanged, this, schedule);
    connect(accounts, &AccountList::listActivityChanged, this, schedule);
    connect(accounts, &QAbstractItemModel::dataChanged, this, schedule);
    connect(accounts, &QAbstractItemModel::modelReset, this, schedule);
    registerMethods();
}

MinecraftAccountPtr AccountApi::findAccount(const QString& id)
{
    auto* accounts = APPLICATION->accounts();
    for (int i = 0; i < accounts->count(); i++) {
        if (accounts->at(i)->internalId() == id) {
            return accounts->at(i);
        }
    }
    return nullptr;
}

QString AccountApi::startLogin(bool useDeviceCode)
{
    const auto flowId = QStringLiteral("f%1").arg(m_nextFlow++);
    auto account = MinecraftAccount::createBlankMSA();
    auto task = account->login(useDeviceCode);
    m_flows.insert(flowId, { account, task });

    auto* raw = task.get();
    connect(raw, &Task::status, this, [this, flowId](const QString& status) {
        m_router->emitEvent("account.login.status", QJsonObject{ { "flowId", flowId }, { "status", status } });
    });
    connect(raw, &AuthFlow::authorizeWithBrowser, this, [this, flowId](const QUrl& url) {
        m_router->emitEvent("account.login.prompt",
                            QJsonObject{ { "flowId", flowId }, { "url", url.toString() }, { "code", QJsonValue() }, { "expiresIn", QJsonValue() } });
    });
    connect(raw, &AuthFlow::authorizeWithBrowserWithExtra, this, [this, flowId](QString url, const QString& code, int expiresIn) {
        if (url == QLatin1String("https://www.microsoft.com/link") && !code.isEmpty()) {
            url += QStringLiteral("?otc=%1").arg(code);
        }
        m_router->emitEvent("account.login.prompt",
                            QJsonObject{ { "flowId", flowId }, { "url", url }, { "code", code }, { "expiresIn", expiresIn } });
    });
    connect(raw, &Task::succeeded, this, [this, flowId] {
        const auto flow = m_flows.take(flowId);
        auto* accounts = APPLICATION->accounts();
        // Signing in again with an existing profile replaces the old entry.
        const auto existing = accounts->findAccountByProfileId(flow.account->profileId());
        if (existing >= 0 && !flow.account->profileId().isEmpty()) {
            const bool wasDefault = accounts->defaultAccount() == accounts->at(existing);
            accounts->removeAccount(accounts->index(existing));
            accounts->addAccount(flow.account);
            if (wasDefault) {
                accounts->setDefaultAccount(flow.account);
            }
        } else {
            accounts->addAccount(flow.account);
        }
        if (!accounts->defaultAccount()) {
            accounts->setDefaultAccount(flow.account);
        }
        m_router->emitEvent("account.login.finished", QJsonObject{ { "flowId", flowId }, { "accountId", flow.account->internalId() } });
    });
    connect(raw, &Task::failed, this, [this, flowId](const QString& reason) {
        m_flows.remove(flowId);
        m_router->emitEvent("account.login.failed", QJsonObject{ { "flowId", flowId }, { "error", ApiError("TASK_FAILED", reason).toJson() } });
    });
    connect(raw, &Task::aborted, this, [this, flowId] {
        m_flows.remove(flowId);
        m_router->emitEvent("account.login.failed", QJsonObject{ { "flowId", flowId }, { "error", ApiError::cancelled().toJson() } });
    });

    m_tasks->start(task, "account.login", tr("Signing in with Microsoft"));
    return flowId;
}

void AccountApi::registerMethods()
{
    m_router->addSync("accounts.list", [](const QJsonObject&) {
        auto* accounts = APPLICATION->accounts();
        const auto defaultAccount = accounts->defaultAccount();
        QJsonArray out;
        for (int i = 0; i < accounts->count(); i++) {
            out.append(serializeAccount(accounts->at(i), defaultAccount));
        }
        return out;
    });

    m_router->addSync("accounts.loginMsa", [this](const QJsonObject& p) {
        if (!APPLICATION->capabilities().testFlag(Application::SupportsMSA)) {
            throw ApiError::unsupported(tr("This build has no Microsoft client ID, Microsoft accounts are unavailable"));
        }
        return QJsonObject{ { "flowId", startLogin(params::optionalBool(p, "useDeviceCode", false)) } };
    });

    m_router->addSync("accounts.cancelLogin", [this](const QJsonObject& p) {
        const auto flowId = params::requireNonEmpty(p, "flowId", 64);
        const auto it = m_flows.find(flowId);
        if (it != m_flows.end() && it->task) {
            it->task->abort();
        }
        return ok();
    });

    m_router->addSync("accounts.addOffline", [](const QJsonObject& p) {
        const auto name = params::requireNonEmpty(p, "name", 16);
        static const QRegularExpression s_validName(QStringLiteral("^[A-Za-z0-9_]{1,16}$"));
        if (!s_validName.match(name).hasMatch()) {
            throw ApiError::invalidParams(tr("Player names may only contain letters, digits and underscores (1-16 characters)"));
        }
        auto* accounts = APPLICATION->accounts();
        auto account = MinecraftAccount::createOffline(name);
        accounts->addAccount(account);
        if (accounts->count() == 1) {
            accounts->setDefaultAccount(account);
        }
        return serializeAccount(account, accounts->defaultAccount());
    });

    m_router->addSync("accounts.remove", [](const QJsonObject& p) {
        const auto id = params::requireNonEmpty(p, "id", 256);
        const auto account = findAccount(id);
        if (!account) {
            throw ApiError::accountNotFound(id);
        }
        auto* accounts = APPLICATION->accounts();
        accounts->removeAccount(accounts->index(rowOf(account)));
        return ok();
    });

    m_router->addSync("accounts.setDefault", [](const QJsonObject& p) {
        auto* accounts = APPLICATION->accounts();
        const auto id = params::optionalString(p, "id", 256);
        if (!id) {
            accounts->setDefaultAccount(nullptr);
            return ok();
        }
        const auto account = findAccount(*id);
        if (!account) {
            throw ApiError::accountNotFound(*id);
        }
        accounts->setDefaultAccount(account);
        return ok();
    });

    m_router->addSync("accounts.refresh", [](const QJsonObject& p) {
        const auto id = params::requireNonEmpty(p, "id", 256);
        const auto account = findAccount(id);
        if (!account) {
            throw ApiError::accountNotFound(id);
        }
        if (account->accountType() != AccountType::MSA) {
            throw ApiError::unsupported(tr("Offline accounts do not need to be refreshed"));
        }
        // Goes through AccountList's refresh queue, like the Qt account page.
        APPLICATION->accounts()->requestRefresh(account->internalId());
        return ok();
    });
}

}  // namespace api
