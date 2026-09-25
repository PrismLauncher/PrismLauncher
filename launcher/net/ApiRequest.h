// SPDX-License-Identifier: GPL-3.0-only
/*
 *  Prism Launcher - Minecraft Launcher
 *  Copyright (C) 2023 Rachel Powers <508861+Ryex@users.noreply.github.com>
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
 */

#pragma once

#include <QByteArray>
#include <QUrl>
#include <memory>
#include <utility>

#include "net/ApiHeaderProxy.h"
#include "net/Request.h"

namespace Net {

namespace ApiRequest {
inline Request::Ptr makeCached(const QUrl& url, MetaEntryPtr entry, Request::Options options = Request::Option::NoOptions)
{
    return Request::makeCached(url, std::move(entry), options | Request::Option::AddAPIHeaders);
}

inline std::pair<Request::Ptr, QByteArray*> makeByteArray(const QUrl& url, Request::Options options = Request::Option::NoOptions)
{
    return Request::makeByteArray(url, options | Request::Option::AddAPIHeaders);
}

inline std::pair<Request::Ptr, QByteArray*> makeByteArray(const QUrl& url,
                                                             QByteArray postData,
                                                             Request::Options options = Request::Option::NoOptions)
{
    return Request::makeByteArray(url, std::move(postData), options | Request::Option::AddAPIHeaders);
}

inline Request::Ptr makeFile(const QUrl& url,
                                const QString& path,
                                Request::Options options = Request::Option::NoOptions,
                                ModrinthDownloadMeta meta = ModrinthDownloadMeta())
{
    auto req = Request::makeFile(url, path, options);
    req->addHeaderProxy(std::make_unique<ApiHeaderProxy>(std::move(meta)));
    return req;
}
};  // namespace ApiRequest

}  // namespace Net
