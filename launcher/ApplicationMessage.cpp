// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
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
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "ApplicationMessage.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <variant>
#include "Json.h"
#include "Result.h"

Result<> ApplicationMessage::parse(const QByteArray& input)
{
    TRY_INTO(const auto& root, Json::requireObject(input, "ApplicationMessage"))

    command = root.value("command").toString();
    args.clear();

    auto parsedArgs = root.value("args").toObject();
    for (auto iter = parsedArgs.constBegin(); iter != parsedArgs.constEnd(); iter++) {
        args.insert(iter.key(), iter.value().toString());
    }
    return {};
}

QByteArray ApplicationMessage::serialize() const
{
    QJsonObject root;
    root.insert("command", command);
    QJsonObject outArgs;
    for (auto iter = args.constBegin(); iter != args.constEnd(); iter++) {
        outArgs.insert(iter.key(), iter.value());
    }
    root.insert("args", outArgs);

    return Json::toText(root);
}

Result<Cli::Command> ApplicationMessage::toCliCommand() const
{
    const QString notFoundMsg = "missing `%1` for Launch command";
    // Launch
    if (command == "launch") {
        TRY_INTO(auto targetType, Try::findByMapKey(args, "targetType", notFoundMsg));
        TRY_INTO(auto target, Try::findByMapKey(args, "target", notFoundMsg));
        TRY_INTO(auto accountType, Try::findByMapKey(args, "accountType", notFoundMsg));
        TRY_INTO(auto account, Try::findByMapKey(args, "account", notFoundMsg));
        TRY_INTO(auto id, Try::findByMapKey(args, "id", notFoundMsg));

        Cli::Cmd::Launch launch{
            .id = id.toStdString(),
        };

        if (targetType == "no_target") {
            launch.target = Cli::Cmd::Launch::NoTarget{};
        } else if (targetType == "server") {
            launch.target = Cli::Cmd::Launch::ServerTarget{
                .target = target.toStdString(),
            };
        } else if (targetType == "world") {
            launch.target = Cli::Cmd::Launch::WorldTarget{
                .target = target.toStdString(),
            };
        } else {
            return std::unexpected{ QString("unknown launch target type `%1`").arg(targetType) };
        }

        if (accountType == "default") {
            launch.account = Cli::Cmd::Launch::AccountDefault{};
        } else if (accountType == "profile") {
            launch.account = Cli::Cmd::Launch::AccountProfile{
                .name = account.toStdString(),
            };

        } else if (accountType == "offline") {
            launch.account = Cli::Cmd::Launch::AccountOffline{
                .name = account.toStdString(),
            };
        } else {
            return std::unexpected{ QString("unknown account type `%1`").arg(accountType) };
        }

        return launch;
    }

    // ProcessUri
    if (command == "processUri") {
        TRY_INTO(auto uri, Try::findByMapKey(args, "uri", notFoundMsg));

        return Cli::Cmd::ProcessURI{ .uri = uri.toStdString() };
    }

    // ShowMainWindow
    if (command == "showMainWindow") {
        return Cli::Cmd::ShowMainWindow{};
    }

    // ShowInstanceWindow
    if (command == "showInstanceWindow") {
        TRY_INTO(auto id, Try::findByMapKey(args, "id", notFoundMsg));

        return Cli::Cmd::ShowInstanceWindow{ .id = id.toStdString() };
    }

    // Alive
    if (command == "alive") {
        TRY_INTO(auto path, Try::findByMapKey(args, "path", notFoundMsg));
        return Cli::Cmd::Alive{ .path = path.toStdU16String() };
    }

    return std::unexpected{ QString("unknown cli command `%1`").arg(command) };
}

ApplicationMessage ApplicationMessage::fromCliCommand(const Cli::Command& cmd)
{
    struct CmdVisitorSerializer {
        ApplicationMessage message;

        void operator()(const Cli::Cmd::Launch& launch)
        {
            message.command = "launch";
            struct LaunchTargetSerializer {
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::NoTarget& /*unused*/) { return { "no_target", "" }; }
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::ServerTarget& server)
                {
                    return { "server", QString::fromStdString(server.target) };
                }
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::WorldTarget& world)
                {
                    return { "world", QString::fromStdString(world.target) };
                }
            } targetSerializer{};

            struct LaunchAccountSerializer {
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::AccountDefault& /*unused*/) { return { "default", "" }; }
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::AccountProfile& profile)
                {
                    return { "profile", QString::fromStdString(profile.name) };
                }
                std::pair<QString, QString> operator()(const Cli::Cmd::Launch::AccountOffline& offline)
                {
                    return { "offline", QString::fromStdString(offline.name) };
                }
            } accountSerializer{};

            auto [targetType, target] = std::visit(targetSerializer, launch.target);
            auto [accountType, account] = std::visit(accountSerializer, launch.account);

            message.args.insert("targetType", targetType);
            message.args.insert("target", target);
            message.args.insert("accountType", accountType);
            message.args.insert("account", account);
            message.args.insert("id", QString::fromStdString(launch.id));
        }
        void operator()(const Cli::Cmd::ProcessURI& processUri)
        {
            message.command = "processUri";
            message.args.insert("uri", QString::fromStdString(processUri.uri));
        }
        void operator()(const Cli::Cmd::ShowMainWindow& /*unused*/) { message.command = "showMainWindow"; }
        void operator()(const Cli::Cmd::ShowInstanceWindow& showInstanceWindow)
        {
            message.command = "showInstanceWindow";
            message.args.insert("id", QString::fromStdString(showInstanceWindow.id));
        }
        void operator()(const Cli::Cmd::Alive& alive)
        {
            message.command = "alive";
            message.args.insert("path", QString::fromStdU16String(alive.path.u16string()));
        }
    } serializer{};

    std::visit(serializer, cmd);
    return serializer.message;
}
