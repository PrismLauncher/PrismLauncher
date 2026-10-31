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
#include "net/RawHeaderProxy.h"

namespace Realms {

NetJob::Ptr fetch(MinecraftInstance* instance, QObject* context, std::function<void(const QList<Realm>&)> onSucceeded)
{
    auto account = instance->accountToUse();
    if (!account || !account->ownsMinecraft()) {
        return nullptr;
    }

    NetJob::Ptr job{ new NetJob("Fetch Realms", APPLICATION->network()) };
    job->setAskRetry(false);
    auto startJob = [job = job.get(), account, context, onSucceeded = std::move(onSucceeded),
                     version = instance->getPackProfile()->getComponentVersion("net.minecraft")] {
        auto cookie = QString("sid=token:%1:%2;user=%3;version=%4")
                          .arg(account->accessToken(), account->profileId(), account->profileName(), version);
        auto [request, response] = Net::Request::makeByteArray(QUrl("https://pc.realms.minecraft.net/worlds"));
        request->addHeaderProxy(
            std::make_unique<Net::RawHeaderProxy>(QList<Net::HeaderPair>{ { .headerName = "Cookie", .headerValue = cookie.toUtf8() } }));
        job->addNetAction(request);
        QObject::connect(job, &Task::succeeded, context, [response, onSucceeded] {
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
    };

    auto state = account->accountState();
    if (!account->isInUse() && (state == AccountState::Unchecked || state == AccountState::Errored || state == AccountState::Offline ||
                                account->shouldRefresh())) {
        account->refresh();
    }
    if (auto refresh = account->currentTask()) {
        QObject::connect(refresh.get(), &Task::finished, job.get(), startJob);
        if (!refresh->isRunning()) {
            refresh->start();
        }
    } else {
        startJob();
    }
    return job;
}

}  // namespace Realms
