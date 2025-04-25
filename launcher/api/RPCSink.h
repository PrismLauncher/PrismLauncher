// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (c) 2025 Trial97 <alexandru.tripon97@gmail.com>
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

#pragma once

#include "Json.h"
#include "net/ByteArraySink.h"

namespace API {
template <class T>
class RPCSink : public Net::ByteArraySink {
   public:
    using Handler = std::function<bool(const QJsonDocument& doc, T& rsp)>;
    RPCSink(Handler handle, std::shared_ptr<T> respone)
        : Net::ByteArraySink(std::make_shared<QByteArray>()), m_handle(std::move(handle)), m_response(respone)
    {}
    virtual ~RPCSink() = default;
    Task::State finalize(QNetworkReply& reply) override
    {
        auto status = Net::ByteArraySink::finalize(reply);
        if (status != Task::State::Succeeded) {
            return status;
        }
        QJsonParseError parse_error{};
        QJsonDocument doc = QJsonDocument::fromJson(*m_output, &parse_error);
        if (parse_error.error != QJsonParseError::NoError) {
            qWarning() << "Error while parsing JSON response at " << parse_error.offset << " reason: " << parse_error.errorString();
            qWarning() << *m_output;
            return Task::State::Failed;
        }
        try {
            if (!m_handle(doc, *m_response)) {
                return Task::State::Failed;
            }
        } catch (Json::JsonException& e) {
            qCritical() << "Failed to parse response request.";
            qCritical() << e.what();
            qDebug() << e.cause();
            qDebug() << doc;
            return Task::State::Failed;
        }
        return Task::State::Succeeded;
    }

   private:
    Handler m_handle;
    std::shared_ptr<T> m_response;
};

}  // namespace API