#include "logic/minecraft/JarUtils.h"
#include <quazip.h>
#include <quazipfile.h>
#include <JlCompress.h>
#include <QDebug>

namespace JarUtils {

bool mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained,
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

bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods)
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
			if (!JlCompress::compressFile(&zipOut, filename.absoluteFilePath(),
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
			if (!JlCompress::compressSubDir(&zipOut, what_to_zip, parent_dir, true, addedFiles))
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

bool noFilter(QString)
{
	return true;
}

bool metaInfFilter(QString key)
{
	if(key.contains("META-INF"))
	{
		return false;
	}
	return true;
}

}
