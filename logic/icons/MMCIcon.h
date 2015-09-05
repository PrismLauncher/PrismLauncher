/* Copyright 2013-2015 MultiMC Contributors
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
#include <QDateTime>
#include <QIcon>

#include "multimc_logic_export.h"

struct MULTIMC_LOGIC_EXPORT MMCImage
{
	QIcon icon;
	QString filename;
	QDateTime changed;
	bool present() const
	{
		return !icon.isNull();
	}
};

struct MULTIMC_LOGIC_EXPORT MMCIcon
{
	enum Type : unsigned
	{
		Builtin,
		Transient,
		FileBased,
		ICONS_TOTAL,
		ToBeDeleted
	};
	QString m_key;
	QString m_name;
	MMCImage m_images[ICONS_TOTAL];
	Type m_current_type = ToBeDeleted;

	Type type() const;
	QString name() const;
	bool has(Type _type) const;
	QIcon icon() const;
	void remove(Type rm_type);
	void replace(Type new_type, QIcon icon, QString path = QString());
};
