// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include <expected>
#include <tuple>
#include <utility>
#include "net/ByteArraySink.h"
#include "net/NetRequest.h"

namespace Net {

template <typename T>
class RpcSink : public ByteArraySink {
   public:
    using ParseResult = std::expected<T, QString>;
    using ParseFunc = std::function<ParseResult(const QByteArray&)>;

    explicit RpcSink(ParseFunc parseFunc) : m_parseFunc(parseFunc) {}
    ~RpcSink() override = default;

   public:
    auto finalize(QNetworkReply& reply) -> Task::State override
    {
        if (finalizeAllValidators(reply)) {
            try {
                auto result = m_parseFunc(m_output);
                if (!result.has_value()) {
                    m_fail_reason = result.error();
                    return Task::State::Failed;
                }
                m_result = *result;
            } catch (const std::exception& e) {
                m_fail_reason = QString::fromUtf8(e.what());
                return Task::State::Failed;
                // ToDo: make this suppport QJsonException
            } catch (...) {
                m_fail_reason = QObject::tr("Unknown error while parsing RPC response");
                return Task::State::Failed;
            }
            return Task::State::Succeeded;
        }
        m_fail_reason = "Failed to finalize validators";
        return Task::State::Failed;
    }

    T* result() { return &m_result; }

   private:
    T m_result;
    ParseFunc m_parseFunc;
};

template <typename T>
struct Spec {
    NetRequest::HttpMethod method = NetRequest::HttpMethod::Get;
    QUrl url{};
    NetRequest::PostData data = nullptr;
    RpcSink<T>::ParseFunc parse = nullptr;

    QString name{};

    bool isValid() { return url.isValid() && parse; }
    std::pair<NetRequest::Ptr, T*> make()
    {
        if (!isValid()) {
            return { nullptr, nullptr };
        }
        auto req =
            NetRequest::makeCustomRequest(url, data, method, NetRequest::Option::AutoRetry | NetRequest::Option::AddAPIHeaders, name);
        auto sink = std::make_unique<RpcSink<T>>(parse);
        auto output = sink->result();
        req->setSink(std::move(sink));
        return { std::move(req), output };
    }
};
}  // namespace Net