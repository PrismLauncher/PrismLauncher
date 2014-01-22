/* Copyright 2013 Andrew Okin
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

#include "BaseVersion.h"
#include <QStringList>

struct MinecraftVersion : public BaseVersion
{
	/*!
	 * Gets the version's timestamp.
	 * This is primarily used for sorting versions in a list.
	 */
	qint64 timestamp;

	/// The URL that this version will be downloaded from. maybe.
	QString download_url;

	/// This version's type. Used internally to identify what kind of version this is.
	enum VersionType
	{
		Derp,
		Legacy,
		Nostalgia
	} type;

	/// is this the latest version?
	bool is_latest = false;

	/// is this a snapshot?
	bool is_snapshot = false;

	QString m_name;

	QString m_descriptor;

	virtual QString descriptor()
	{
		return m_descriptor;
	}

	virtual QString name()
	{
		return m_name;
	}

	virtual QString typeString() const
	{
		QStringList pre_final;
		if (is_latest == true)
		{
			pre_final.append("Latest");
		}
		switch (type)
		{
		case Derp:
			pre_final.append("Derp");
			break;
		case Legacy:
			pre_final.append("Legacy");
			break;
		case Nostalgia:
			pre_final.append("Nostalgia");
			break;

		default:
			pre_final.append(QString("Type(%1)").arg(type));
			break;
		}
		if (is_snapshot == true)
		{
			pre_final.append("Snapshot");
		}
		return pre_final.join(' ');
	}
};
