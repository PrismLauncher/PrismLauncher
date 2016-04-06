/* Copyright 2015 MultiMC Contributors
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

#include "WonkoUtil.h"

#include <QUrl>
#include <QDir>

#include "Env.h"

namespace Wonko
{
QUrl rootUrl()
{
	return ENV.wonkoRootUrl();
}
QUrl indexUrl()
{
	return rootUrl().resolved(QStringLiteral("index.json"));
}
QUrl versionListUrl(const QString &uid)
{
	return rootUrl().resolved(uid + ".json");
}
QUrl versionUrl(const QString &uid, const QString &version)
{
	return rootUrl().resolved(uid + "/" + version + ".json");
}

QDir localWonkoDir()
{
	return QDir("wonko");
}

}
