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

#include <QByteArray>
#include <QUrl>

namespace API {
// we only handle GET and POST
enum class HttpMethod {
    GET,
    HEAD,
    OPTIONS,
    TRACE,
    PUT,
    DELETE,
    POST,
    PATCH,
    CONNECT,
};

struct HttpRequest {
    HttpMethod method;
    QUrl url;
    QByteArray data;

    HttpRequest(HttpMethod m, const QUrl& u, const QByteArray& d = {}) : method(m), url(std::move(u)), data(std::move(d)) {}
    static std::unique_ptr<HttpRequest> GET(const QUrl& u) { return std::make_unique<HttpRequest>(HttpMethod::GET, std::move(u)); }
    static std::unique_ptr<HttpRequest> POST(const QUrl& u, const QByteArray& d)
    {
        return std::make_unique<HttpRequest>(HttpMethod::POST, std::move(u), std::move(d));
    }
};

}  // namespace API