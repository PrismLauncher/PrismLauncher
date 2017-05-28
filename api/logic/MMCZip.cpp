/*
Copyright (C) 2010 Roberto Pompermaier
Copyright (C) 2005-2014 Sergey A. Tachenov

Parts of this file were part of QuaZIP.

QuaZIP is free software: you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as published by
the Free Software Foundation, either version 2.1 of the License, or
(at your option) any later version.

QuaZIP is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public License
along with QuaZIP.  If not, see <http://www.gnu.org/licenses/>.

See COPYING file for the full LGPL text.

Original ZIP package is copyrighted by Gilles Vollant and contributors,
see quazip/(un)MMCZip.h files for details. Basically it's the zlib license.
*/

#include <quazip.h>
#include <quazipdir.h>
#include <quazipfile.h>
#include <JlCompress.h>
#include "MMCZip.h"
#include "FileSystem.h"

#include <QDebug>

bool MMCZip::compressSubDir(QuaZip* zip, QString dir, QString origDir, QSet<QString>& added,  QString prefix, const SeparatorPrefixTree <'/'> * blacklist)
{
	if (!zip) return false;
	if (zip->getMode()!=QuaZip::mdCreate && zip->getMode()!=QuaZip::mdAppend && zip->getMode()!=QuaZip::mdAdd)
	{
		return false;
	}

	QDir directory(dir);
	if (!directory.exists())
	{
		return false;
	}

	QDir origDirectory(origDir);
	if (dir != origDir)
	{
		QString internalDirName = origDirectory.relativeFilePath(dir);
		if(!blacklist || !blacklist->covers(internalDirName))
		{
			QuaZipFile dirZipFile(zip);
			auto dirPrefix = FS::PathCombine(prefix, origDirectory.relativeFilePath(dir)) + "/";
			if (!dirZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(dirPrefix, dir), 0, 0, 0))
			{
				return false;
			}
			dirZipFile.close();
		}
	}

	QFileInfoList files = directory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
	for (auto file: files)
	{
		if(!file.isDir())
		{
			continue;
		}
		if(!compressSubDir(zip,file.absoluteFilePath(),origDir, added, prefix, blacklist))
		{
			return false;
		}
	}

	files = directory.entryInfoList(QDir::Files);
	for (auto file: files)
	{
		if(!file.isFile())
		{
			continue;
		}

		if(file.absoluteFilePath()==zip->getZipName())
		{
			continue;
		}

		QString filename = origDirectory.relativeFilePath(file.absoluteFilePath());
		if(blacklist && blacklist->covers(filename))
		{
			continue;
		}
		if(prefix.size())
		{
			filename = FS::PathCombine(prefix, filename);
		}
		added.insert(filename);
		if (!JlCompress::compressFile(zip,file.absoluteFilePath(),filename))
		{
			return false;
		}
	}

	return true;
}

bool MMCZip::mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, std::function<bool(QString)> filter)
{
	QuaZip modZip(from.filePath());
	modZip.open(QuaZip::mdUnzip);

	QuaZipFile fileInsideMod(&modZip);
	QuaZipFile zipOutFile(into);
	for (bool more = modZip.goToFirstFile(); more; more = modZip.goToNextFile())
	{
		QString filename = modZip.getCurrentFileName();
		if (!filter(filename))
		{
			qDebug() << "Skipping file " << filename << " from "
						<< from.fileName() << " - filtered";
			continue;
		}
		if (contained.contains(filename))
		{
			qDebug() << "Skipping already contained file " << filename << " from "
						<< from.fileName();
			continue;
		}
		contained.insert(filename);

		if (!fileInsideMod.open(QIODevice::ReadOnly))
		{
			qCritical() << "Failed to open " << filename << " from " << from.fileName();
			return false;
		}

		QuaZipNewInfo info_out(fileInsideMod.getActualFileName());

		if (!zipOutFile.open(QIODevice::WriteOnly, info_out))
		{
			qCritical() << "Failed to open " << filename << " in the jar";
			fileInsideMod.close();
			return false;
		}
		if (!JlCompress::copyData(fileInsideMod, zipOutFile))
		{
			zipOutFile.close();
			fileInsideMod.close();
			qCritical() << "Failed to copy data of " << filename << " into the jar";
			return false;
		}
		zipOutFile.close();
		fileInsideMod.close();
	}
	return true;
}

bool MMCZip::createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods)
{
	QuaZip zipOut(targetJarPath);
	if (!zipOut.open(QuaZip::mdCreate))
	{
		QFile::remove(targetJarPath);
		qCritical() << "Failed to open the minecraft.jar for modding";
		return false;
	}
	// Files already added to the jar.
	// These files will be skipped.
	QSet<QString> addedFiles;

	// Modify the jar
	QListIterator<Mod> i(mods);
    i.toBack();
    while (i.hasPrevious())
	{
		const Mod &mod = i.previous();
		// do not merge disabled mods.
		if (!mod.enabled())
			continue;
		if (mod.type() == Mod::MOD_ZIPFILE)
		{
			if (!mergeZipFiles(&zipOut, mod.filename(), addedFiles, noFilter))
			{
				zipOut.close();
				QFile::remove(targetJarPath);
				qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
				return false;
			}
		}
		else if (mod.type() == Mod::MOD_SINGLEFILE)
		{
			auto filename = mod.filename();
			if (!JlCompress::compressFile(&zipOut, filename.absoluteFilePath(), filename.fileName()))
			{
				zipOut.close();
				QFile::remove(targetJarPath);
				qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
				return false;
			}
			addedFiles.insert(filename.fileName());
		}
		else if (mod.type() == Mod::MOD_FOLDER)
		{
			auto filename = mod.filename();
			QString what_to_zip = filename.absoluteFilePath();
			QDir dir(what_to_zip);
			dir.cdUp();
			QString parent_dir = dir.absolutePath();
			if (!MMCZip::compressSubDir(&zipOut, what_to_zip, parent_dir, addedFiles))
			{
				zipOut.close();
				QFile::remove(targetJarPath);
				qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
				return false;
			}
			qDebug() << "Adding folder " << filename.fileName() << " from "
						<< filename.absoluteFilePath();
		}
		else
		{
			// Make sure we do not continue launching when something is missing or undefined...
			zipOut.close();
			QFile::remove(targetJarPath);
			qCritical() << "Failed to add unknown mod type" << mod.filename().fileName() << "to the jar.";
			return false;
		}
	}

	if (!mergeZipFiles(&zipOut, QFileInfo(sourceJarPath), addedFiles, metaInfFilter))
	{
		zipOut.close();
		QFile::remove(targetJarPath);
		qCritical() << "Failed to insert minecraft.jar contents.";
		return false;
	}

	// Recompress the jar
	zipOut.close();
	if (zipOut.getZipError() != 0)
	{
		QFile::remove(targetJarPath);
		qCritical() << "Failed to finalize minecraft.jar!";
		return false;
	}
	return true;
}

bool MMCZip::noFilter(QString)
{
	return true;
}

bool MMCZip::metaInfFilter(QString key)
{
	if(key.contains("META-INF"))
	{
		return false;
	}
	return true;
}

bool MMCZip::compressDir(QString zipFile, QString dir, QString prefix, const SeparatorPrefixTree <'/'> * blacklist)
{
	QuaZip zip(zipFile);
	QDir().mkpath(QFileInfo(zipFile).absolutePath());
	if(!zip.open(QuaZip::mdCreate))
	{
		QFile::remove(zipFile);
		return false;
	}

	QSet<QString> added;
	if (!MMCZip::compressSubDir(&zip, dir, dir, added, prefix, blacklist))
	{
		QFile::remove(zipFile);
		return false;
	}
	zip.close();
	if(zip.getZipError()!=0)
	{
		QFile::remove(zipFile);
		return false;
	}
	return true;
}

QString MMCZip::findFileInZip(QuaZip * zip, const QString & what, const QString &root)
{
	QuaZipDir rootDir(zip, root);
	for(auto fileName: rootDir.entryList(QDir::Files))
	{
		if(fileName == what)
			return root;
	}
	for(auto fileName: rootDir.entryList(QDir::Dirs))
	{
		QString result = findFileInZip(zip, what, root + fileName);
		if(!result.isEmpty())
		{
			return result;
		}
	}
	return QString();
}

bool MMCZip::findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root)
{
	QuaZipDir rootDir(zip, root);
	for(auto fileName: rootDir.entryList(QDir::Files))
	{
		if(fileName == what)
		{
			result.append(root);
			return true;
		}
	}
	for(auto fileName: rootDir.entryList(QDir::Dirs))
	{
		findFilesInZip(zip, what, result, root + fileName);
	}
	return !result.isEmpty();
}

QStringList MMCZip::extractSubDir(QuaZip *zip, const QString & subdir, const QString &target)
{
	QDir directory(target);
	QStringList extracted;
	if (!zip->goToFirstFile())
	{
		return QStringList();
	}
	do
	{
		QString name = zip->getCurrentFileName();
		if(!name.startsWith(subdir))
		{
			continue;
		}
		name.remove(0, subdir.size());
		QString absFilePath = directory.absoluteFilePath(name);
		if(name.isEmpty())
		{
			absFilePath += "/";
		}
		if (!JlCompress::extractFile(zip, "", absFilePath))
		{
			JlCompress::removeFile(extracted);
			return QStringList();
		}
		extracted.append(absFilePath);
	} while (zip->goToNextFile());
	return extracted;
}

QStringList MMCZip::extractDir(QString fileCompressed, QString dir)
{
	QuaZip zip(fileCompressed);
	if (!zip.open(QuaZip::mdUnzip))
	{
		return {};
	}
	return MMCZip::extractSubDir(&zip, "", dir);
}
