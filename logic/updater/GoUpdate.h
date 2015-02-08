#pragma once
#include <QByteArray>
#include <logic/net/NetJob.h>

namespace GoUpdate
{

/**
 * A temporary object exchanged between updated checker and the actual update task
 */
struct Status
{
	bool updateAvailable = false;

	int newVersionId = -1;
	QString newRepoUrl;

	int currentVersionId = -1;
	QString currentRepoUrl;

	// path to the root of the application
	QString rootPath;
};

/**
 * Struct that describes an entry in a VersionFileEntry's `Sources` list.
 */
struct FileSource
{
	FileSource(QString type, QString url, QString compression="")
	{
		this->type = type;
		this->url = url;
		this->compressionType = compression;
	}

	bool operator==(const FileSource &f2) const
	{
		return type == f2.type && url == f2.url && compressionType == f2.compressionType;
	}

	QString type;
	QString url;
	QString compressionType;
};
typedef QList<FileSource> FileSourceList;

/**
 * Structure that describes an entry in a GoUpdate version's `Files` list.
 */
struct VersionFileEntry
{
	QString path;
	int mode;
	FileSourceList sources;
	QString md5;
	bool operator==(const VersionFileEntry &v2) const
	{
		return path == v2.path && mode == v2.mode && sources == v2.sources && md5 == v2.md5;
	}
};
typedef QList<VersionFileEntry> VersionFileList;

/**
 * Structure that describes an operation to perform when installing updates.
 */
struct Operation
{
	static Operation CopyOp(QString fsource, QString fdest, int fmode=0644) { return Operation{OP_COPY, fsource, fdest, fmode}; }
	static Operation MoveOp(QString fsource, QString fdest, int fmode=0644) { return Operation{OP_MOVE, fsource, fdest, fmode}; }
	static Operation DeleteOp(QString file) { return Operation{OP_DELETE, file, "", 0644}; }
	static Operation ChmodOp(QString file, int fmode) { return Operation{OP_CHMOD, file, "", fmode}; }

	// FIXME: for some types, some of the other fields are irrelevant!
	bool operator==(const Operation &u2) const
	{
		return type == u2.type && file == u2.file && dest == u2.dest && mode == u2.mode;
	}

	//! Specifies the type of operation that this is.
	enum Type
	{
		OP_COPY,
		OP_DELETE,
		OP_MOVE,
		OP_CHMOD,
	} type;

	//! The file to operate on. If this is a DELETE or CHMOD operation, this is the file that will be modified.
	QString file;

	//! The destination file. If this is a DELETE or CHMOD operation, this field will be ignored.
	QString dest;

	//! The mode to change the source file to. Ignored if this isn't a CHMOD operation.
	int mode;
};
typedef QList<Operation> OperationList;

/**
 * Takes the @OperationList list and writes an install script for the updater to the update files directory.
 */
bool writeInstallScript(OperationList& opsList, QString scriptFile);

/**
 * Loads the file list from the given version info JSON object into the given list.
 */
bool parseVersionInfo(const QByteArray &data, VersionFileList& list, QString &error);

/*!
 * Takes a list of file entries for the current version's files and the new version's files
 * and populates the downloadList and operationList with information about how to download and install the update.
 */
bool processFileLists
(
	const VersionFileList &currentVersion,
	const VersionFileList &newVersion,
	const QString &rootPath,
	const QString &tempPath,
	NetJobPtr job,
	OperationList &ops,
	bool useLocalUpdater
);

/*!
 * This fixes destination paths for OSX - removes 'MultiMC.app' prefix
 * The updater runs in MultiMC.app/Contents/MacOs by default
 * The destination paths are such as this: MultiMC.app/blah/blah
 *
 * @return false if the path couldn't be fixed (is invalid)
 */
bool fixPathForOSX(QString &path);

}
Q_DECLARE_METATYPE(GoUpdate::Status);