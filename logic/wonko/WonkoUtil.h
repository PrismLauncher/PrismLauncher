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

#pragma once

#include "multimc_logic_export.h"

class QUrl;
class QString;
class QDir;

namespace Wonko
{
MULTIMC_LOGIC_EXPORT QUrl rootUrl();
MULTIMC_LOGIC_EXPORT QUrl indexUrl();
MULTIMC_LOGIC_EXPORT QUrl versionListUrl(const QString &uid);
MULTIMC_LOGIC_EXPORT QUrl versionUrl(const QString &uid, const QString &version);
MULTIMC_LOGIC_EXPORT QDir localWonkoDir();
}
