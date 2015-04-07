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

#include "include/pathutils.h"

#include <QFileInfo>
#include <QDir>
#include <QDesktopServices>
#include <QUrl>
#include <QDebug>

QString PathCombine(QString path1, QString path2)
{
	if(!path1.size())
		return path2;
	if(!path2.size())
		return path1;
    return QDir::cleanPath(path1 + QDir::separator() + path2);
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
	QFileInfo a(filenamepath);
	QDir dir;
	QString ensuredPath = a.path();
	bool success = dir.mkpath(ensuredPath);
	return success;
}

bool ensureFolderPathExists(QString foldernamepath)
{
	QFileInfo a(foldernamepath);
	QDir dir;
	QString ensuredPath = a.filePath();
	bool success = dir.mkpath(ensuredPath);
	return success;
}

bool copyPath(QString src, QString dst, bool follow_symlinks)
{
	//NOTE always deep copy on windows. the alternatives are too messy.
	#if defined Q_OS_WIN32
	follow_symlinks = true;
	#endif

	QDir dir(src);
	if (!dir.exists())
		return false;
	if (!ensureFolderPathExists(dst))
		return false;

	bool OK = true;

	foreach(QString f, dir.entryList(QDir::Files | QDir::Dirs | QDir::NoDotAndDotDot | QDir::Hidden | QDir::System))
	{
		QString inner_src = src + QDir::separator() + f;
		QString inner_dst = dst + QDir::separator() + f;
		QFileInfo fileInfo(inner_src);
		if(!follow_symlinks && fileInfo.isSymLink())
		{
			OK &= QFile::link(fileInfo.symLinkTarget(),inner_dst);
		}
		else if (fileInfo.isDir())
		{
			OK &= copyPath(inner_src, inner_dst, follow_symlinks);
		}
		else if (fileInfo.isFile())
		{
			OK &= QFile::copy(inner_src, inner_dst);
		}
		else
		{
			OK = false;
			qCritical() << "Copy ERROR: Unknown filesystem object:" << inner_src;
		}
	}
	return OK;
}
#if defined Q_OS_WIN32
#include <windows.h>
#include <string>
#endif
bool deletePath(QString path)
{
	bool OK = true;
	QDir dir(path);

	if (!dir.exists())
	{
		return OK;
	}
	auto allEntries = dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden |
										QDir::AllDirs | QDir::Files,
										QDir::DirsFirst);

	for(QFileInfo info: allEntries)
	{
#if defined Q_OS_WIN32
		QString nativePath = QDir::toNativeSeparators(info.absoluteFilePath());
		auto wString = nativePath.toStdWString();
		DWORD dwAttrs = GetFileAttributesW(wString.c_str());
		// Windows: check for junctions, reparse points and other nasty things of that sort
		if(dwAttrs & FILE_ATTRIBUTE_REPARSE_POINT)
		{
			if (info.isFile())
			{
				OK &= QFile::remove(info.absoluteFilePath());
			}
			else if (info.isDir())
			{
				OK &= dir.rmdir(info.absoluteFilePath());
			}
		}
#else
		// We do not trust Qt with reparse points, but do trust it with unix symlinks.
		if(info.isSymLink())
		{
			OK &= QFile::remove(info.absoluteFilePath());
		}
#endif
		else if (info.isDir())
		{
			OK &= deletePath(info.absoluteFilePath());
		}
		else if (info.isFile())
		{
			OK &= QFile::remove(info.absoluteFilePath());
		}
		else
		{
			OK = false;
			qCritical() << "Delete ERROR: Unknown filesystem object:" << info.absoluteFilePath();
		}
	}
	OK &= dir.rmdir(dir.absolutePath());
	return OK;
}

void openDirInDefaultProgram(QString path, bool ensureExists)
{
	QDir parentPath;
	QDir dir(path);
	if (!dir.exists())
	{
		parentPath.mkpath(dir.absolutePath());
	}
	QDesktopServices::openUrl(QUrl::fromLocalFile(dir.absolutePath()));
}

void openFileInDefaultProgram(QString filename)
{
	QDesktopServices::openUrl(QUrl::fromLocalFile(filename));
}

// Does the directory path contain any '!'? If yes, return true, otherwise false.
// (This is a problem for Java)
bool checkProblemticPathJava(QDir folder)
{
	QString pathfoldername = folder.absolutePath();
	return pathfoldername.contains("!", Qt::CaseInsensitive);
}
