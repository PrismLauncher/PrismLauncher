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

#include <QDir>
#include <QDirIterator>
#include <QCryptographicHash>

#include "AssetsUtils.h"
#include "MultiMC.h"

namespace AssetsUtils
{
void migrateOldAssets()
{
	QDir assets_dir("assets");
	if(!assets_dir.exists()) return;
	assets_dir.setFilter(QDir::AllEntries | QDir::NoDotAndDotDot);
	int base_length = assets_dir.path().length();

	QList<QString> blacklist = {"indexes", "objects", "virtual"};

	if(!assets_dir.exists("objects")) assets_dir.mkdir("objects");
	QDir objects_dir("assets/objects");

	QDirIterator iterator(assets_dir, QDirIterator::Subdirectories);
	int successes = 0;
	int failures = 0;
	while (iterator.hasNext()) {
		QString currentDir = iterator.next();
		currentDir = currentDir.remove(0, base_length+1);

		bool ignore = false;
		for(QString blacklisted : blacklist)
		{
			if(currentDir.startsWith(blacklisted)) ignore = true;
		}

		if (!iterator.fileInfo().isDir() && !ignore) {
			QString filename = iterator.filePath();

			QFile input(filename);
			input.open(QIODevice::ReadOnly | QIODevice::WriteOnly);
			QString sha1sum = QCryptographicHash::hash(input.readAll(), QCryptographicHash::Sha1)
								 .toHex()
								 .constData();

			QString object_name = filename.remove(0, base_length+1);
			QLOG_DEBUG() << "Processing" << object_name << ":" << sha1sum << input.size();

			QString object_tlk = sha1sum.left(2);
			QString object_tlk_dir = objects_dir.path() + "/" + object_tlk;

			QDir tlk_dir(object_tlk_dir);
			if(!tlk_dir.exists()) objects_dir.mkdir(object_tlk);

			QString new_filename = tlk_dir.path() + "/" + sha1sum;
			QFile new_object(new_filename);
			if(!new_object.exists())
			{
				bool rename_success = input.rename(new_filename);
				QLOG_DEBUG() << " Doesn't exist, copying to" << new_filename << ":" << QString::number(rename_success);
				if(rename_success) successes++;
				else failures++;
			}
			else
			{
				input.remove();
				QLOG_DEBUG() << " Already exists, deleting original and not copying.";
			}
		}
	}

	if(successes + failures == 0) {
		QLOG_DEBUG() << "No legacy assets needed importing.";
	}
	else
	{
		QLOG_DEBUG() << "Finished copying legacy assets:" << successes << "successes and" << failures << "failures.";

		QDirIterator cleanup_iterator(assets_dir);

		while (cleanup_iterator.hasNext()) {
			QString currentDir = cleanup_iterator.next();
			currentDir = currentDir.remove(0, base_length+1);

			bool ignore = false;
			for(QString blacklisted : blacklist)
			{
				if(currentDir.startsWith(blacklisted)) ignore = true;
			}

			if (cleanup_iterator.fileInfo().isDir() && !ignore) {
				QString path = cleanup_iterator.filePath();
				QDir folder(path);

				QLOG_DEBUG() << "Cleaning up legacy assets folder:" << path;

				folder.removeRecursively();
			}
		}
	}
}
}
