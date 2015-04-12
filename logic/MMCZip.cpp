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

#include <pathutils.h>
#include <quazip.h>
#include <JlCompress.h>
#include "MMCZip.h"

#include <QDebug>

bool copyData(QIODevice &inFile, QIODevice &outFile)
{
	while (!inFile.atEnd())
	{
		char buf[4096];
		qint64 readLen = inFile.read(buf, 4096);
		if (readLen <= 0)
			return false;
		if (outFile.write(buf, readLen) != readLen)
			return false;
	}
	return true;
}

QStringList MMCZip::extractDir(QString fileCompressed, QString dir)
{
	return JlCompress::extractDir(fileCompressed, dir);
}

bool compressFile(QuaZip *zip, QString fileName, QString fileDest)
{
	if (!zip)
	{
		return false;
	}
	if (zip->getMode() != QuaZip::mdCreate && zip->getMode() != QuaZip::mdAppend &&
		zip->getMode() != QuaZip::mdAdd)
	{
		return false;
	}

	QFile inFile;
	inFile.setFileName(fileName);
	if (!inFile.open(QIODevice::ReadOnly))
	{
		return false;
	}

	QuaZipFile outFile(zip);
	if (!outFile.open(QIODevice::WriteOnly, QuaZipNewInfo(fileDest, inFile.fileName())))
	{
		return false;
	}

	if (!copyData(inFile, outFile) || outFile.getZipError() != UNZ_OK)
	{
		return false;
	}

	outFile.close();
	if (outFile.getZipError() != UNZ_OK)
	{
		return false;
	}
	inFile.close();

	return true;
}

bool MMCZip::compressSubDir(QuaZip* zip, QString dir, QString origDir, QSet<QString>& added,  QString prefix)
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
		QuaZipFile dirZipFile(zip);
		auto dirPrefix = PathCombine(prefix, origDirectory.relativeFilePath(dir)) + "/";
		if (!dirZipFile.open(QIODevice::WriteOnly, QuaZipNewInfo(dirPrefix, dir), 0, 0, 0))
		{
			return false;
		}
		dirZipFile.close();
	}

	QFileInfoList files = directory.entryInfoList(QDir::AllDirs | QDir::NoDotAndDotDot | QDir::Hidden);
	for (auto file: files)
	{
		if(!file.isDir())
		{
			continue;
		}
		if(!compressSubDir(zip,file.absoluteFilePath(),origDir, added, prefix))
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
		if(prefix.size())
		{
			filename = PathCombine(prefix, filename);
		}
		added.insert(filename);
		if (!compressFile(zip,file.absoluteFilePath(),filename))
		{
			return false;
		}
	}

	return true;
}

bool MMCZip::mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
				   std::function<bool(QString)> filter)
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
		if (!copyData(fileInsideMod, zipOutFile))
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
			if (!compressFile(&zipOut, filename.absoluteFilePath(),
										  filename.fileName()))
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
			if (!compressSubDir(&zipOut, what_to_zip, parent_dir, addedFiles))
			{
				zipOut.close();
				QFile::remove(targetJarPath);
				qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
				return false;
			}
			qDebug() << "Adding folder " << filename.fileName() << " from "
						<< filename.absoluteFilePath();
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

bool MMCZip::compressDir(QString zipFile, QString dir, QString prefix)
{
	QuaZip zip(zipFile);
	QDir().mkpath(QFileInfo(zipFile).absolutePath());
	if(!zip.open(QuaZip::mdCreate))
	{
		QFile::remove(zipFile);
		return false;
	}

	QSet<QString> added;
	if (!compressSubDir(&zip, dir, dir, added, prefix))
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

