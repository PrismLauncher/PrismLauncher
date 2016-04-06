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

#include <QString>
#include <QMetaType>

#include "multimc_logic_export.h"

class MULTIMC_LOGIC_EXPORT WonkoReference
{
public:
	WonkoReference() {}
	explicit WonkoReference(const QString &uid);

	QString uid() const;

	QString version() const;
	void setVersion(const QString &version);

	bool operator==(const WonkoReference &other) const;
	bool operator!=(const WonkoReference &other) const;

private:
	QString m_uid;
	QString m_version;
};
Q_DECLARE_METATYPE(WonkoReference)
