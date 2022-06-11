// Licensed under the Apache-2.0 license. See README.md for details.

#pragma once

#include "Exception.h"
#include "pathmatcher/IPathMatcher.h"

#include <QDir>
#include <QFlags>

namespace FS
{

class FileSystemException : public ::Exception
{
public:
    FileSystemException(const QString &message) : Exception(message) {}
};

/**
 * write data to a file safely
 */
void write(const QString &filename, const QByteArray &data);

/**
 * read data from a file safely\
 */
QByteArray read(const QString &filename);

/**
 * Update the last changed timestamp of an existing file
 */
bool updateTimestamp(const QString & filename);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a file name and is ignored!
 */
bool ensureFilePathExists(QString filenamepath);

/**
 * Creates all the folders in a path for the specified path
 * last segment of the path is treated as a folder name and is created!
 */
bool ensureFolderPathExists(QString filenamepath);

class copy
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
bool deletePath(QString path);

QString PathCombine(const QString &path1, const QString &path2);
QString PathCombine(const QString &path1, const QString &path2, const QString &path3);
QString PathCombine(const QString &path1, const QString &path2, const QString &path3, const QString &path4);

QString AbsolutePath(QString path);

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
QString ResolveExecutable(QString path);

/**
 * Normalize path
 *
 * Any paths inside the current directory will be normalized to relative paths (to current)
 * Other paths will be made absolute
 *
 * Returns false if the path logic somehow filed (and normalizedPath in invalid)
 */
QString NormalizePath(QString path);

QString RemoveInvalidFilenameChars(QString string, QChar replaceWith = '-');

QString DirNameFromString(QString string, QString inDir = ".");

/// Checks if the a given Path contains "!"
bool checkProblemticPathJava(QDir folder);

// Get the Directory representing the User's Desktop
QString getDesktopDir();

// Create a shortcut at *location*, pointing to *dest* called with the arguments *args*
// call it *name* and assign it the icon *icon*
// return true if operation succeeded
bool createShortCut(QString location, QString dest, QStringList args, QString name, QString iconLocation);

// Overrides one folder with the contents of another, preserving items exclusive to the first folder
// Equivalent to doing QDir::rename, but allowing for overrides
bool overrideFolder(QString overwritten_path, QString override_path);
}
