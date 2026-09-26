// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Vishrut Sachan <vishrutsachan2004@gmail.com>
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

#include "Realms.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "Application.h"
#include "minecraft/MinecraftInstance.h"
#include "minecraft/PackProfile.h"
#include "minecraft/auth/AccountList.h"
#include "net/RawHeaderProxy.h"

namespace Realms {

NetJob::Ptr fetch(MinecraftInstance* instance, QObject* context, std::function<void(const QList<Realm>&)> onSucceeded)
{
    auto* accounts = APPLICATION->accounts();
    auto accountId = instance->settings()->get("InstanceAccountId").toString();
    auto accountIndex = accounts->findAccountByProfileId(accountId);
    auto account = accountIndex == -1 || accountId.isEmpty() ? accounts->defaultAccount() : accounts->at(accountIndex);
    if (!account || !account->ownsMinecraft()) {
        return nullptr;
    }

    auto cookie = QString("sid=token:%1:%2;user=%3;version=%4")
                      .arg(account->accessToken(), account->profileId(), account->profileName(),
                           instance->getPackProfile()->getComponentVersion("net.minecraft"));
    auto [request, response] = Net::Request::makeByteArray(QUrl("https://pc.realms.minecraft.net/worlds"));
    request->addHeaderProxy(
        std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{ { .headerName = "Cookie", .headerValue = cookie.toUtf8() } }));

    NetJob::Ptr job{ new NetJob("Fetch Realms", APPLICATION->network()) };
    job->setAskRetry(false);
    job->addNetAction(request);
    QObject::connect(job.get(), &Task::succeeded, context, [response, onSucceeded = std::move(onSucceeded)] {
        QList<Realm> realms;
        for (auto value : QJsonDocument::fromJson(*response).object().value("servers").toArray()) {
            auto obj = value.toObject();
            realms.append({ .id = QString::number(obj.value("id").toInteger()),
                            .name = obj.value("name").toString(),
                            .owner = obj.value("owner").toString(),
                            .motd = obj.value("motd").toString(),
                            .open = obj.value("state").toString() == "OPEN",
                            .expired = obj.value("expired").toBool() });
        }
        onSucceeded(realms);
    });
    job->start();
    return job;
}

}  // namespace Realms
