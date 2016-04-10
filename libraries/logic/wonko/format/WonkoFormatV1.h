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

#include "WonkoFormat.h"

class WonkoFormatV1 : public WonkoFormat
{
public:
	BaseWonkoEntity::Ptr parseIndexInternal(const QJsonObject &obj) const override;
	BaseWonkoEntity::Ptr parseVersionInternal(const QJsonObject &obj) const override;
	BaseWonkoEntity::Ptr parseVersionListInternal(const QJsonObject &obj) const override;

	QJsonObject serializeIndexInternal(const WonkoIndex *ptr) const override;
	QJsonObject serializeVersionInternal(const WonkoVersion *ptr) const override;
	QJsonObject serializeVersionListInternal(const WonkoVersionList *ptr) const override;
};
