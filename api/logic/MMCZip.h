#pragma once

#include <QString>
#include <QFileInfo>
#include <QSet>
#include "minecraft/Mod.h"
#include "SeparatorPrefixTree.h"
#include <functional>

#include "multimc_logic_export.h"

#include <JlCompress.h>

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
	bool MULTIMC_LOGIC_EXPORT compressSubDir(QuaZip *zip, QString dir, QString origDir, QSet<QString> &added,
					QString prefix = QString(), const SeparatorPrefixTree <'/'> * blacklist = nullptr);

	/**
	 * Compress a whole directory.
	 * \param fileCompressed The name of the archive.
	 * \param dir The directory to compress.
	 * \param recursive Whether to pack the subdirectories as well, or just regular files.
	 * \return true if success, false otherwise.
	 */
	bool MULTIMC_LOGIC_EXPORT compressDir(QString zipFile, QString dir, QString prefix = QString(), const SeparatorPrefixTree <'/'> * blacklist = nullptr);

	/// filter function for @mergeZipFiles - passthrough
	bool MULTIMC_LOGIC_EXPORT noFilter(QString key);

	/// filter function for @mergeZipFiles - ignores METAINF
	bool MULTIMC_LOGIC_EXPORT metaInfFilter(QString key);

	/**
	 * Merge two zip files, using a filter function
	 */
	bool MULTIMC_LOGIC_EXPORT mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, std::function<bool(QString)> filter);

	/**
	 * take a source jar, add mods to it, resulting in target jar
	 */
	bool MULTIMC_LOGIC_EXPORT createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods);

	/**
	 * Find a single file in archive by file name (not path)
	 *
	 * \return the path prefix where the file is
	 */
	QString MULTIMC_LOGIC_EXPORT findFileInZip(QuaZip * zip, const QString & what, const QString &root = QString());

	/**
	 * Find a multiple files of the same name in archive by file name
	 * If a file is found in a path, no deeper paths are searched
	 *
	 * \return true if anything was found
	 */
	bool MULTIMC_LOGIC_EXPORT findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root = QString());

	/**
	 * Extract a subdirectory from an archive
	 */
	QStringList MULTIMC_LOGIC_EXPORT extractSubDir(QuaZip *zip, const QString & subdir, const QString &target);

	/**
	 * Extract a whole archive.
	 *
	 * \param fileCompressed The name of the archive.
	 * \param dir The directory to extract to, the current directory if left empty.
	 * \param opts Extra options.
	 * \return The list of the full paths of the files extracted, empty on failure.
	 */
	QStringList MULTIMC_LOGIC_EXPORT extractDir(QString fileCompressed, QString dir);

}
