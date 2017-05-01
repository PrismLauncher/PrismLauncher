#include <QFile>
#include <QMessageBox>
#include <FileSystem.h>
#include <updater/GoUpdate.h>
#include "UpdateController.h"
#include <QApplication>
#include <thread>
#include <chrono>
#include <LocalPeer.h>

// from <sys/stat.h>
#ifndef S_IRUSR
#define __S_IREAD 0400         /* Read by owner.  */
#define __S_IWRITE 0200        /* Write by owner.  */
#define __S_IEXEC 0100         /* Execute by owner.  */
#define S_IRUSR __S_IREAD      /* Read by owner.  */
#define S_IWUSR __S_IWRITE     /* Write by owner.  */
#define S_IXUSR __S_IEXEC      /* Execute by owner.  */

#define S_IRGRP (S_IRUSR >> 3) /* Read by group.  */
#define S_IWGRP (S_IWUSR >> 3) /* Write by group.  */
#define S_IXGRP (S_IXUSR >> 3) /* Execute by group.  */

#define S_IROTH (S_IRGRP >> 3) /* Read by others.  */
#define S_IWOTH (S_IWGRP >> 3) /* Write by others.  */
#define S_IXOTH (S_IXGRP >> 3) /* Execute by others.  */
#endif
static QFile::Permissions unixModeToPermissions(const int mode)
{
	QFile::Permissions perms;

	if (mode & S_IRUSR)
	{
		perms |= QFile::ReadUser;
	}
	if (mode & S_IWUSR)
	{
		perms |= QFile::WriteUser;
	}
	if (mode & S_IXUSR)
	{
		perms |= QFile::ExeUser;
	}

	if (mode & S_IRGRP)
	{
		perms |= QFile::ReadGroup;
	}
	if (mode & S_IWGRP)
	{
		perms |= QFile::WriteGroup;
	}
	if (mode & S_IXGRP)
	{
		perms |= QFile::ExeGroup;
	}

	if (mode & S_IROTH)
	{
		perms |= QFile::ReadOther;
	}
	if (mode & S_IWOTH)
	{
		perms |= QFile::WriteOther;
	}
	if (mode & S_IXOTH)
	{
		perms |= QFile::ExeOther;
	}
	return perms;
}

static const QLatin1String liveCheckFile("live.check");

UpdateController::UpdateController(QWidget * parent, const QString& root, const QString updateFilesDir, GoUpdate::OperationList operations)
{
	m_parent = parent;
	m_root = root;
	m_updateFilesDir = updateFilesDir;
	m_operations = operations;
}


void UpdateController::installUpdates()
{
	qint64 pid = -1;
	QStringList args;
	bool started = false;

	qDebug() << "Installing updates.";
#ifdef Q_OS_WIN
	QString finishCmd = QApplication::applicationFilePath();
#elif defined Q_OS_LINUX
	QString finishCmd = FS::PathCombine(m_root, "MultiMC");
#elif defined Q_OS_MAC
	QString finishCmd = QApplication::applicationFilePath();
#else
#error Unsupported operating system.
#endif

	QString backupPath = FS::PathCombine(m_root, "update", "backup");
	QDir origin(m_root);

	// clean up the backup folder. it should be empty before we start
	if(!FS::deletePath(backupPath))
	{
		qWarning() << "couldn't remove previous backup folder" << backupPath;
	}
	// and it should exist.
	if(!FS::ensureFolderPathExists(backupPath))
	{
		qWarning() << "couldn't create folder" << backupPath;
		return;
	}

	bool useXPHack = false;
	QString exePath;
	QString exeOrigin;
	QString exeBackup;

	// perform the update operations
	for(auto op: m_operations)
	{
		switch(op.type)
		{
			// replace = move original out to backup, if it exists, move the new file in its place
			case GoUpdate::Operation::OP_REPLACE:
			{
#ifdef Q_OS_WIN32
				// hack for people renaming the .exe because ... reasons :)
				if(op.destination == "MultiMC.exe")
				{
					op.destination = QFileInfo(QApplication::applicationFilePath()).fileName();
				}
#endif
				QFileInfo destination (FS::PathCombine(m_root, op.destination));
#ifdef Q_OS_WIN32
				if(QSysInfo::windowsVersion() < QSysInfo::WV_VISTA)
				{
					if(destination.fileName() == "MultiMC.exe")
					{
						QDir rootDir(m_root);
						exeOrigin = rootDir.relativeFilePath(op.source);
						exePath = rootDir.relativeFilePath(op.destination);
						exeBackup = rootDir.relativeFilePath(FS::PathCombine(backupPath, destination.fileName()));
						useXPHack = true;
						continue;
					}
				}
#endif
				if(destination.exists())
				{
					QString backupName = op.destination;
					backupName.replace('/', '_');
					QString backupFilePath = FS::PathCombine(backupPath, backupName);
					if(!QFile::rename(destination.absoluteFilePath(), backupFilePath))
					{
						qWarning() << "Couldn't move:" << destination.absoluteFilePath() << "to" << backupFilePath;
						m_failedOperationType = Replace;
						m_failedFile = op.destination;
						fail();
						return;
					}
					BackupEntry be;
					be.original = destination.absoluteFilePath();
					be.backup = backupFilePath;
					be.update = op.source;
					m_replace_backups.append(be);
				}
				// make sure the folder we are putting this into exists
				if(!FS::ensureFilePathExists(destination.absoluteFilePath()))
				{
					qWarning() << "REPLACE: Couldn't create folder:" << destination.absoluteFilePath();
					m_failedOperationType = Replace;
					m_failedFile = op.destination;
					fail();
					return;
				}
				// now move the new file in
				if(!QFile::rename(op.source, destination.absoluteFilePath()))
				{
					qWarning() << "REPLACE: Couldn't move:" << op.source << "to" << destination.absoluteFilePath();
					m_failedOperationType = Replace;
					m_failedFile = op.destination;
					fail();
					return;
				}
				QFile::setPermissions(destination.absoluteFilePath(), unixModeToPermissions(op.destinationMode));
			}
			break;
			// delete = move original to backup
			case GoUpdate::Operation::OP_DELETE:
			{
				QString destFilePath = FS::PathCombine(m_root, op.destination);
				if(QFile::exists(destFilePath))
				{
					QString backupName = op.destination;
					backupName.replace('/', '_');
					QString trashFilePath = FS::PathCombine(backupPath, backupName);

					if(!QFile::rename(destFilePath, trashFilePath))
					{
						qWarning() << "DELETE: Couldn't move:" << op.destination << "to" << trashFilePath;
						m_failedFile = op.destination;
						m_failedOperationType = Delete;
						fail();
						return;
					}
					BackupEntry be;
					be.original = destFilePath;
					be.backup = trashFilePath;
					m_delete_backups.append(be);
				}
			}
			break;
		}
	}

	// try to start the new binary
	args = qApp->arguments();
	args.removeFirst();

	// on old Windows, do insane things... no error checking here, this is just to have something.
	if(useXPHack)
	{
		QString script;
		auto nativePath = QDir::toNativeSeparators(exePath);
		auto nativeOriginPath = QDir::toNativeSeparators(exeOrigin);
		auto nativeBackupPath = QDir::toNativeSeparators(exeBackup);

		// so we write this vbscript thing...
		QTextStream out(&script);
		out << "WScript.Sleep 1000\n";
		out << "Set fso=CreateObject(\"Scripting.FileSystemObject\")\n";
		out << "Set shell=CreateObject(\"WScript.Shell\")\n";
		out << "fso.MoveFile \"" << nativePath << "\", \"" << nativeBackupPath << "\"\n";
		out << "fso.MoveFile \"" << nativeOriginPath << "\", \"" << nativePath << "\"\n";
		out << "shell.Run \"" << nativePath << "\"\n";

		QString scriptPath = FS::PathCombine(m_root, "update", "update.vbs");

		// we save it
		QFile scriptFile(scriptPath);
		scriptFile.open(QIODevice::WriteOnly);
		scriptFile.write(script.toLocal8Bit().replace("\n", "\r\n"));
		scriptFile.close();

		// we run it
		started = QProcess::startDetached("wscript", {scriptPath}, m_root);

		// and we quit. conscious thought.
		qApp->quit();
		return;
	}
	bool doLiveCheck = true;
	bool startFailed = false;

	// remove live check file, if any
	if(QFile::exists(liveCheckFile))
	{
		if(!QFile::remove(liveCheckFile))
		{
			qWarning() << "Couldn't remove the" << liveCheckFile << "file! We will proceed without :(";
			doLiveCheck = false;
		}
	}

	if(doLiveCheck)
	{
		if(!args.contains("--alive"))
		{
			args.append("--alive");
		}
	}

	// FIXME: reparse args and construct a safe variant from scratch. This is a workaround for GH-1874:
	QStringList realargs;
	int skip = 0;
	for(auto & arg: args)
	{
		if(skip)
		{
			skip--;
			continue;
		}
		if(arg == "-l")
		{
			skip = 1;
			continue;
		}
		realargs.append(arg);
	}

	// start the updated application
	started = QProcess::startDetached(finishCmd, realargs, QDir::currentPath(), &pid);
	// much dumber check - just find out if the call
	if(!started || pid == -1)
	{
		qWarning() << "Couldn't start new process properly!";
		startFailed = true;
	}
	if(!startFailed && doLiveCheck)
	{
		int attempts = 0;
		while(attempts < 10)
		{
			attempts++;
			QString key;
			std::this_thread::sleep_for(std::chrono::milliseconds(250));
			if(!QFile::exists(liveCheckFile))
			{
				qWarning() << "Couldn't find the" << liveCheckFile << "file!";
				startFailed = true;
				continue;
			}
			try
			{
				key = QString::fromUtf8(FS::read(liveCheckFile));
				auto id = ApplicationId::fromRawString(key);
				LocalPeer peer(nullptr, id);
				if(peer.isClient())
				{
					startFailed = false;
					qDebug() << "Found process started with key " << key;
					break;
				}
				else
				{
					startFailed = true;
					qDebug() << "Process started with key " << key << "apparently died or is not reponding...";
					break;
				}
			}
			catch(Exception e)
			{
				qWarning() << "Couldn't read the" << liveCheckFile << "file!";
				startFailed = true;
				continue;
			}
		}
	}
	if(startFailed)
	{
		m_failedOperationType = Start;
		fail();
		return;
	}
	else
	{
		origin.rmdir(m_updateFilesDir);
		qApp->quit();
		return;
	}
}

void UpdateController::fail()
{
	qWarning() << "Update failed!";

	QString msg;
	bool doRollback = false;
	QString failTitle = QObject::tr("Update failed!");
	QString rollFailTitle = QObject::tr("Rollback failed!");
	switch (m_failedOperationType)
	{
		case Replace:
		{
			msg = QObject::tr("Couldn't replace file %1. Changes will be reverted.\n"
				"See the MultiMC log file for details.").arg(m_failedFile);
			doRollback = true;
			QMessageBox::critical(m_parent, failTitle, msg);
			break;
		}
		case Delete:
		{
			msg = QObject::tr("Couldn't remove file %1. Changes will be reverted.\n"
				"See the MultiMC log file for details.").arg(m_failedFile);
			doRollback = true;
			QMessageBox::critical(m_parent, failTitle, msg);
			break;
		}
		case Start:
		{
			msg = QObject::tr("The new version didn't start or is too old and doesn't respond to startup checks.\n"
				"\n"
				"Roll back to previous version?");
			auto result = QMessageBox::critical(
				m_parent,
				failTitle,
				msg,
				QMessageBox::Yes | QMessageBox::No,
				QMessageBox::Yes
			);
			doRollback = (result == QMessageBox::Yes);
			break;
		}
		case Nothing:
		default:
			return;
	}
	if(doRollback)
	{
		auto rollbackOK = rollback();
		if(!rollbackOK)
		{
			msg = QObject::tr("The rollback failed too.\n"
				"You will have to repair MultiMC manually.\n"
				"Please let us know why and how this happened.").arg(m_failedFile);
			QMessageBox::critical(m_parent, rollFailTitle, msg);
			qApp->quit();
		}
	}
	else
	{
		qApp->quit();
	}
}

bool UpdateController::rollback()
{
	bool revertOK = true;
	// if the above failed, roll back changes
	for(auto backup:m_replace_backups)
	{
		qWarning() << "restoring" << backup.original << "from" << backup.backup;
		if(!QFile::rename(backup.original, backup.update))
		{
			revertOK = false;
			qWarning() << "moving new" << backup.original << "back to" << backup.update << "failed!";
			continue;
		}

		if(!QFile::rename(backup.backup, backup.original))
		{
			revertOK = false;
			qWarning() << "restoring" << backup.original << "failed!";
		}
	}
	for(auto backup:m_delete_backups)
	{
		qWarning() << "restoring" << backup.original << "from" << backup.backup;
		if(!QFile::rename(backup.backup, backup.original))
		{
			revertOK = false;
			qWarning() << "restoring" << backup.original << "failed!";
		}
	}
	return revertOK;
}
