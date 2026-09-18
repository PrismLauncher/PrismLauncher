// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2026 Trial97 <alexandru.tripon97@gmail.com>
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

#include <expected>
#include <utility>

#include "Result.h"
#include "net/ByteArraySink.h"
#include "net/Request.h"

namespace Net::RPC {

template <typename T>
class Sink : public ByteArraySink {
   public:
    using ParseResult = Result<T>;
    using ParseFunc = std::function<ParseResult(const QByteArray&)>;

    explicit Sink(ParseFunc parseFunc) : m_parseFunc(parseFunc) {}
    ~Sink() override = default;

   public:
    Result<> finalize(QNetworkReply& /*reply*/) override
    {
        TRY(finalizeAllValidators())
        TRY_INTO(m_result, m_parseFunc(m_output))
        return {};
    }

    T* result() { return &m_result; }

   private:
    T m_result;
    ParseFunc m_parseFunc;
};

template <typename T>
using Spec = std::pair<Request::Spec, typename Sink<T>::ParseFunc>;

template <typename T>
std::pair<Request::Ptr, T*> make(const Spec<T>& specPair)
{
    auto [spec, parse] = specPair;
    if (!spec.url.isValid() && parse) {
        return { nullptr, nullptr };
    }
    spec.options |= Request::Option::AutoRetry | Request::Option::AddAPIHeaders;
    auto req = Request::makeCustomRequest(spec);
    auto sink = std::make_unique<Sink<T>>(parse);
    auto output = sink->result();
    req->setSink(std::move(sink));
    return { std::move(req), output };
}

}  // namespace Net::RPC
