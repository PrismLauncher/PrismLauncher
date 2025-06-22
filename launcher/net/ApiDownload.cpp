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

#include "net/ApiDownload.h"
#include "net/ApiHeaderProxy.h"

namespace Net {

Download::Ptr ApiDownload::makeCached(QUrl url, MetaEntryPtr entry, Download::Options options)
{
    auto dl = Download::makeCached(url, entry, options);
    dl->addHeaderProxy(new ApiHeaderProxy());
    return dl;
}

Download::Ptr ApiDownload::makeByteArray(QUrl url, std::shared_ptr<QByteArray> output, Download::Options options)
{
    auto dl = Download::makeByteArray(url, output, options);
    dl->addHeaderProxy(new ApiHeaderProxy());
    return dl;
}

Download::Ptr ApiDownload::makeFile(QUrl url, QString path, Download::Options options)
{
    auto dl = Download::makeFile(url, path, options);
    dl->addHeaderProxy(new ApiHeaderProxy());
    return dl;
}

}  // namespace Net
