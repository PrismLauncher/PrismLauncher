/* Copyright 2013 MultiMC Contributors
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

#include "MMCIcon.h"
#include <QFileInfo>

MMCIcon::Type operator--(MMCIcon::Type &t, int)
{
	MMCIcon::Type temp = t;
	switch (t)
	{
	case MMCIcon::Type::Builtin:
		t = MMCIcon::Type::ToBeDeleted;
		break;
	case MMCIcon::Type::Transient:
		t = MMCIcon::Type::Builtin;
		break;
	case MMCIcon::Type::FileBased:
		t = MMCIcon::Type::Transient;
		break;
	default:
	{
	}
	}
	return temp;
}

MMCIcon::Type MMCIcon::type() const
{
	return m_current_type;
}

QString MMCIcon::name() const
{
	if (m_name.size())
		return m_name;
	return m_key;
}

bool MMCIcon::has(MMCIcon::Type _type) const
{
	return m_images[_type].present();
}

QIcon MMCIcon::icon() const
{
	if (m_current_type == Type::ToBeDeleted)
		return QIcon();
	return m_images[m_current_type].icon;
}

void MMCIcon::remove(Type rm_type)
{
	m_images[rm_type].filename = QString();
	m_images[rm_type].icon = QIcon();
	for (auto iter = rm_type; iter != Type::ToBeDeleted; iter--)
	{
		if (m_images[iter].present())
		{
			m_current_type = iter;
			return;
		}
	}
	m_current_type = Type::ToBeDeleted;
}

void MMCIcon::replace(MMCIcon::Type new_type, QIcon icon, QString path)
{
	QFileInfo foo(path);
	if (new_type > m_current_type || m_current_type == MMCIcon::ToBeDeleted)
	{
		m_current_type = new_type;
	}
	m_images[new_type].icon = icon;
	m_images[new_type].changed = foo.lastModified();
	m_images[new_type].filename = path;
}
