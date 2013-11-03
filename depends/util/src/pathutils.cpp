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

#include "include/pathutils.h"

#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

QString PathCombine(QString path1, QString path2)
{
	if (!path1.endsWith('/'))
		return path1.append('/').append(path2);
	else
		return path1.append(path2);
}

QString PathCombine(QString path1, QString path2, QString path3)
{
	return PathCombine(PathCombine(path1, path2), path3);
}

QString AbsolutePath(QString path)
{
	return QFileInfo(path).absolutePath();
}

/**
 * Normalize path
 * 
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 */
QString NormalizePath(QString path)
{
	QDir a = QDir::currentPath();
	QString currentAbsolute = a.absolutePath();

	QDir b(path);
	QString newAbsolute = b.absolutePath();

	if (newAbsolute.startsWith(currentAbsolute))
	{
		return a.relativeFilePath(newAbsolute);
	}
	else
	{
		return newAbsolute;
	}
}

QString badFilenameChars = "\"\\/?<>:*|!";

QString RemoveInvalidFilenameChars(QString string, QChar replaceWith)
{
	for (int i = 0; i < string.length(); i++)
	{
		if (badFilenameChars.contains(string[i]))
		{
			string[i] = replaceWith;
		}
	}
	return string;
}

QString DirNameFromString(QString string, QString inDir)
{
	int num = 0;
	QString dirName = RemoveInvalidFilenameChars(string, '-');
	while (QFileInfo(PathCombine(inDir, dirName)).exists())
	{
		num++;
		dirName = RemoveInvalidFilenameChars(dirName, '-') + QString::number(num);
		
		// If it's over 9000
		if (num > 9000)
			return "";
	}
	return dirName;
}

bool ensureFilePathExists(QString filenamepath)
{
	QFileInfo a ( filenamepath );
	QDir dir;
	QString ensuredPath = a.path();
	bool success = dir.mkpath ( ensuredPath );
	return success;
}

bool ensureFolderPathExists(QString foldernamepath)
{
	QFileInfo a ( foldernamepath );
	QDir dir;
	QString ensuredPath = a.filePath();
	bool success = dir.mkpath ( ensuredPath );
	return success;
}


bool copyPath(QString src, QString dst)
{
	QDir dir(src);
	if (!dir.exists())
		return false;
	if(!ensureFolderPathExists(dst))
		return false;

	foreach (QString d, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
	{
		QString inner_src = src + QDir::separator() + d;
		QString inner_dst = dst + QDir::separator() + d;
		copyPath(inner_src, inner_dst);
	}

	foreach (QString f, dir.entryList(QDir::Files))
	{
		QFile::copy(src + QDir::separator() + f, dst + QDir::separator() + f);
	}
	return true;
}

void openDirInDefaultProgram ( QString path, bool ensureExists )
{
	QDir parentPath;
	QDir dir( path );
	if(!dir.exists())
	{
		parentPath.mkpath(dir.absolutePath());
	}
	QDesktopServices::openUrl ( "file:///" + dir.absolutePath() );
}

void openFileInDefaultProgram ( QString filename )
{
	QDesktopServices::openUrl ( "file:///" + QFileInfo ( filename ).absolutePath() );
}


