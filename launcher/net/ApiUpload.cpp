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

#include "net/ApiUpload.h"
#include "ByteArraySink.h"
#include "ChecksumValidator.h"
#include "MetaCacheSink.h"
#include "net/NetAction.h"

namespace Net {

Upload::Ptr ApiUpload::makeByteArray(QUrl url, std::shared_ptr<QByteArray> output, QByteArray m_post_data)
{
    auto up = makeShared<ApiUpload>();
    up->m_url = std::move(url);
    up->m_sink.reset(new ByteArraySink(output));
    up->m_post_data = std::move(m_post_data);
    return up;
}

void ApiUpload::init()
{
    qDebug() << "Setting up api upload";
    auto api_headers = new ApiHeaderProxy();
    addHeaderProxy(api_headers);
}
}  // namespace Net
