// Licensed under the Apache-2.0 license. See README.md for details.

#include "FileSystem.h"

#include <QDir>
#include <QSaveFile>
#include <QFileInfo>

void ensureExists(const QDir &dir)
{
	if (!QDir().mkpath(dir.absolutePath()))
	{
		throw FS::FileSystemException("Unable to create directory " + dir.dirName() + " (" +
									  dir.absolutePath() + ")");
	}
}

void FS::write(const QString &filename, const QByteArray &data)
{
	ensureExists(QFileInfo(filename).dir());
	QSaveFile file(filename);
	if (!file.open(QSaveFile::WriteOnly))
	{
		throw FileSystemException("Couldn't open " + filename + " for writing: " +
								  file.errorString());
	}
	if (data.size() != file.write(data))
	{
		throw FileSystemException("Error writing data to " + filename + ": " +
								  file.errorString());
	}
	if (!file.commit())
	{
		throw FileSystemException("Error while committing data to " + filename + ": " +
								  file.errorString());
	}
}

QByteArray FS::read(const QString &filename)
{
	QFile file(filename);
	if (!file.open(QFile::ReadOnly))
	{
		throw FileSystemException("Unable to open " + filename + " for reading: " +
								  file.errorString());
	}
	const qint64 size = file.size();
	QByteArray data(int(size), 0);
	const qint64 ret = file.read(data.data(), size);
	if (ret == -1 || ret != size)
	{
		throw FileSystemException("Error reading data from " + filename + ": " +
								  file.errorString());
	}
	return data;
}
