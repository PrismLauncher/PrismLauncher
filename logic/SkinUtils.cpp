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

#include "MultiMC.h"
#include "logic/SkinUtils.h"
#include "net/HttpMetaCache.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace SkinUtils
{
QPixmap getFaceFromCache(QString username, int height, int width)
{
	bool gotFace = false;

	QByteArray data;
	{
		auto filename =
			MMC->metacache()->resolveEntry("skins", "skins.json")->getFullPath();
		QFile listFile(filename);
		if (!listFile.open(QIODevice::ReadOnly))
			return QPixmap();
		data = listFile.readAll();
	}

	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	QJsonObject root = jsonDoc.object();
	QJsonObject mappings = root.value("mappings").toObject();

	if (!mappings[username].isUndefined())
	{
		QJsonArray usernames = mappings.value(username).toArray();
		if (!usernames.isEmpty())
		{
			QString mapped_username = usernames[0].toString();

			if (!mapped_username.isEmpty())
			{
				QFile fskin(MMC->metacache()
								->resolveEntry("skins", mapped_username + ".png")
								->getFullPath());
				if (fskin.exists())
				{
					QPixmap skin(MMC->metacache()
									 ->resolveEntry("skins", mapped_username + ".png")
									 ->getFullPath());

					QPixmap face =
						skin.copy(8, 8, 8, 8).scaled(height, width, Qt::KeepAspectRatio);

					return face;
				}
			}
		}
	}

	if(!gotFace) return QPixmap();
}
}
