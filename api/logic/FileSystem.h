// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Exception.h"
#include "pathmatcher/IPathMatcher.h"

#include "multimc_logic_export.h"
#include <QDir>
#include <QFlags>

namespace FS
{

class MULTIMC_LOGIC_EXPORT FileSystemException : public ::Exception
{
public:
	FileSystemException(const QString &message) : Exception(message) {}
};

/**
 * write data to a file safely
 */
MULTIMC_LOGIC_EXPORT void write(const QString &filename, const QByteArray &data);

/**
 * read data from a file safely\
 */
MULTIMC_LOGIC_EXPORT QByteArray read(const QString &filename);

/**
 * Update the last changed timestamp of an existing file
 */
MULTIMC_LOGIC_EXPORT bool updateTimestamp(const QString & filename);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a file name and is ignored!
 */
MULTIMC_LOGIC_EXPORT bool ensureFilePathExists(QString filenamepath);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a folder name and is created!
 */
MULTIMC_LOGIC_EXPORT bool ensureFolderPathExists(QString filenamepath);

class MULTIMC_LOGIC_EXPORT copy
{
public:
	copy(const QString & src, const QString & dst)
	{
		m_src = src;
		m_dst = dst;
	}
	copy & followSymlinks(const bool follow)
	{
		m_followSymlinks = follow;
		return *this;
	}
	copy & blacklist(const IPathMatcher * filter)
	{
		m_blacklist = filter;
		return *this;
	}
	bool operator()()
	{
		return operator()(QString());
	}

private:
	bool operator()(const QString &offset);

private:
	bool m_followSymlinks = true;
	const IPathMatcher * m_blacklist = nullptr;
	QDir m_src;
	QDir m_dst;
};

/**
 * Delete a folder recursively
 */
MULTIMC_LOGIC_EXPORT bool deletePath(QString path);

MULTIMC_LOGIC_EXPORT QString PathCombine(const QString &path1, const QString &path2);
MULTIMC_LOGIC_EXPORT QString PathCombine(const QString &path1, const QString &path2, const QString &path3);
MULTIMC_LOGIC_EXPORT QString PathCombine(const QString &path1, const QString &path2, const QString &path3, const QString &path4);

MULTIMC_LOGIC_EXPORT QString AbsolutePath(QString path);

/**
 * Resolve an executable
 *
 * Will resolve:
 *   single executable (by name)
 *   relative path
 *   absolute path
 *
 * @return absolute path to executable or null string
 */
MULTIMC_LOGIC_EXPORT QString ResolveExecutable(QString path);

/**
 * Normalize path
 *
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 *
 * Returns false if the path logic somehow filed (and normalizedPath in invalid)
 */
MULTIMC_LOGIC_EXPORT QString NormalizePath(QString path);

MULTIMC_LOGIC_EXPORT QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

MULTIMC_LOGIC_EXPORT QString DirNameFromString(QString string, QString inDir = ".");

/// Checks if the a given Path contains "!"
MULTIMC_LOGIC_EXPORT bool checkProblemticPathJava(QDir folder);

// Get the Directory representing the User's Desktop
MULTIMC_LOGIC_EXPORT QString getDesktopDir();

// Create a shortcut at *location*, pointing to *dest* called with the arguments *args*
// call it *name* and assign it the icon *icon*
// return true if operation succeeded
MULTIMC_LOGIC_EXPORT bool createShortCut(QString location, QString dest, QStringList args, QString name, QString iconLocation);
}
