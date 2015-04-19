#pragma once

#include <QString>
#include <QFileInfo>
#include <QSet>
#include "minecraft/Mod.h"
#include "SeparatorPrefixTree.h"
#include <functional>

class QuaZip;

namespace MMCZip
{
    /**
	 * Compress a subdirectory.
	 * \param parentZip Opened zip containing the parent directory.
	 * \param dir The full path to the directory to pack.
	 * \param parentDir The full path to the directory corresponding to the root of the ZIP.
	 * \param recursive Whether to pack sub-directories as well or only files.
	 * \return true if success, false otherwise.
     */
	bool compressSubDir(QuaZip *zip, QString dir, QString origDir, QSet<QString> &added,
					QString prefix = QString(), const SeparatorPrefixTree <'/'> * blacklist = nullptr);

	/**
	 * Compress a whole directory.
	 * \param fileCompressed The name of the archive.
	 * \param dir The directory to compress.
	 * \param recursive Whether to pack the subdirectories as well, or just regular files.
	 * \return true if success, false otherwise.
	 */
	bool compressDir(QString zipFile, QString dir, QString prefix = QString(), const SeparatorPrefixTree <'/'> * blacklist = nullptr);

	/// filter function for @mergeZipFiles - passthrough
	bool noFilter(QString key);

	/// filter function for @mergeZipFiles - ignores METAINF
	bool metaInfFilter(QString key);

	/**
	 * Merge two zip files, using a filter function
	 */
	bool mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, std::function<bool(QString)> filter);

	/**
	 * take a source jar, add mods to it, resulting in target jar
	 */
	bool createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods);

	/**
	 * Extract a whole archive.
	 *
	 * \param fileCompressed The name of the archive.
	 * \param dir The directory to extract to, the current directory if
	 * left empty.
	 * \return The list of the full paths of the files extracted, empty on failure.
	 */
    QStringList extractDir(QString fileCompressed, QString dir = QString());
}