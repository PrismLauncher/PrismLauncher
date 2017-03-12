/* Copyright 2015-2017 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "Util.h"

#include <QUrl>
#include <QDir>

#include "Env.h"

namespace Meta
{
QUrl rootUrl()
{
	return QUrl("https://meta.multimc.org");
}

QUrl indexUrl()
{
	return rootUrl().resolved(QStringLiteral("index.json"));
}

QUrl versionListUrl(const QString &uid)
{
	return rootUrl().resolved(uid + "/index.json");
}

QUrl versionUrl(const QString &uid, const QString &version)
{
	return rootUrl().resolved(uid + "/" + version + ".json");
}

QDir localDir()
{
	return QDir("meta");
}

}
