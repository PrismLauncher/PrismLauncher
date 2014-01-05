/* Copyright 2013 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "DownloadUpdateTask.h"

#include "MultiMC.h"
#include "logic/updater/UpdateChecker.h"
#include "logic/net/NetJob.h"
#include "pathutils.h"

#include <QFile>
#include <QTemporaryDir>
#include <QCryptographicHash>

#include <QDomDocument>

DownloadUpdateTask::DownloadUpdateTask(QString repoUrl, int versionId, QObject *parent)
	: Task(parent)
{
	m_cVersionId = MMC->version().build;

	m_nRepoUrl = repoUrl;
	m_nVersionId = versionId;

	m_updateFilesDir.setAutoRemove(false);
}

void DownloadUpdateTask::executeTask()
{
	// GO!
	// This will call the next step when it's done.
	findCurrentVersionInfo();
}

void DownloadUpdateTask::processChannels()
{
	auto checker = MMC->updateChecker();

	// Now, check the channel list again.
	if (!checker->hasChannels())
	{
		// We still couldn't load the channel list. Give up. Call loadVersionInfo and return.
		QLOG_INFO() << "Reloading the channel list didn't work. Giving up.";
		loadVersionInfo();
		return;
	}

	QList<UpdateChecker::ChannelListEntry> channels = checker->getChannelList();
	QString channelId = MMC->version().channel;

	// Search through the channel list for a channel with the correct ID.
	for (auto channel : channels)
	{
		if (channel.id == channelId)
		{
			QLOG_INFO() << "Found matching channel.";
			m_cRepoUrl = fixPathForTests(channel.url);
			break;
		}
	}

	// Now that we've done that, load version info.
	loadVersionInfo();
}

void DownloadUpdateTask::findCurrentVersionInfo()
{
	setStatus(tr("Finding information about the current version..."));

	auto checker = MMC->updateChecker();

	if (!checker->hasChannels())
	{
		// Load the channel list and wait for it to finish loading.
		QLOG_INFO() << "No channel list entries found. Will try reloading it.";

		QObject::connect(checker.get(), &UpdateChecker::channelListLoaded, this,
						 &DownloadUpdateTask::processChannels);
		checker->updateChanList();
	}
	else
	{
		processChannels();
	}
}

void DownloadUpdateTask::loadVersionInfo()
{
	setStatus(tr("Loading version information..."));

	// Create the net job for loading version info.
	NetJob *netJob = new NetJob("Version Info");

	// Find the index URL.
	QUrl newIndexUrl = QUrl(m_nRepoUrl).resolved(QString::number(m_nVersionId) + ".json");
	QLOG_DEBUG() << m_nRepoUrl << " turns into " << newIndexUrl;

	// Add a net action to download the version info for the version we're updating to.
	netJob->addNetAction(ByteArrayDownload::make(newIndexUrl));

	// If we have a current version URL, get that one too.
	if (!m_cRepoUrl.isEmpty())
	{
		QUrl cIndexUrl = QUrl(m_cRepoUrl).resolved(QString::number(m_cVersionId) + ".json");
		netJob->addNetAction(ByteArrayDownload::make(cIndexUrl));
		QLOG_DEBUG() << m_cRepoUrl << " turns into " << cIndexUrl;
	}

	// Connect slots so we know when it's done.
	QObject::connect(netJob, &NetJob::succeeded, this,
					 &DownloadUpdateTask::vinfoDownloadFinished);
	QObject::connect(netJob, &NetJob::failed, this, &DownloadUpdateTask::vinfoDownloadFailed);

	// Store the NetJob in a class member. We don't want to lose it!
	m_vinfoNetJob.reset(netJob);

	// Finally, we start the network job and the thread's event loop to wait for it to finish.
	netJob->start();
}

void DownloadUpdateTask::vinfoDownloadFinished()
{
	// Both downloads succeeded. OK. Parse stuff.
	parseDownloadedVersionInfo();
}

void DownloadUpdateTask::vinfoDownloadFailed()
{
	// Something failed. We really need the second download (current version info), so parse
	// downloads anyways as long as the first one succeeded.
	if (m_vinfoNetJob->first()->m_status != Job_Failed)
	{
		parseDownloadedVersionInfo();
		return;
	}

	// TODO: Give a more detailed error message.
	QLOG_ERROR() << "Failed to download version info files.";
	emitFailed(tr("Failed to download version info files."));
}

void DownloadUpdateTask::parseDownloadedVersionInfo()
{
	setStatus(tr("Reading file list for new version..."));
	QLOG_DEBUG() << "Reading file list for new version...";
	QString error;
	if (!parseVersionInfo(
			 std::dynamic_pointer_cast<ByteArrayDownload>(m_vinfoNetJob->first())->m_data,
			 &m_nVersionFileList, &error))
	{
		emitFailed(error);
		return;
	}

	// If there is a second entry in the network job's list, load it as the current version's
	// info.
	if (m_vinfoNetJob->size() >= 2 && m_vinfoNetJob->operator[](1)->m_status != Job_Failed)
	{
		setStatus(tr("Reading file list for current version..."));
		QLOG_DEBUG() << "Reading file list for current version...";
		QString error;
		parseVersionInfo(
			std::dynamic_pointer_cast<ByteArrayDownload>(m_vinfoNetJob->operator[](1))->m_data,
			&m_cVersionFileList, &error);
	}

	// We don't need this any more.
	m_vinfoNetJob.reset();

	// Now that we're done loading version info, we can move on to the next step. Process file
	// lists and download files.
	processFileLists();
}

bool DownloadUpdateTask::parseVersionInfo(const QByteArray &data, VersionFileList *list,
										  QString *error)
{
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		*error = QString("Failed to parse version info JSON: %1 at %2")
					 .arg(jsonError.errorString())
					 .arg(jsonError.offset);
		QLOG_ERROR() << error;
		return false;
	}

	QJsonObject json = jsonDoc.object();

	QLOG_DEBUG() << data;
	QLOG_DEBUG() << "Loading version info from JSON.";
	QJsonArray filesArray = json.value("Files").toArray();
	for (QJsonValue fileValue : filesArray)
	{
		QJsonObject fileObj = fileValue.toObject();

		QString file_path = fileObj.value("Path").toString();
#ifdef Q_OS_MAC
		// On OSX, the paths for the updater need to be fixed.
		// basically, anything that isn't in the .app folder is ignored.
		// everything else is changed so the code that processes the files actually finds
		// them and puts the replacements in the right spots.
		if (!fixPathForOSX(file_path))
			continue;
#endif
		VersionFileEntry file{file_path,		fileObj.value("Perms").toVariant().toInt(),
							  FileSourceList(), fileObj.value("MD5").toString(), };
		QLOG_DEBUG() << "File" << file.path << "with perms" << file.mode;

		QJsonArray sourceArray = fileObj.value("Sources").toArray();
		for (QJsonValue val : sourceArray)
		{
			QJsonObject sourceObj = val.toObject();

			QString type = sourceObj.value("SourceType").toString();
			if (type == "http")
			{
				file.sources.append(
					FileSource("http", fixPathForTests(sourceObj.value("Url").toString())));
			}
			else if (type == "httpc")
			{
				file.sources.append(
					FileSource("httpc", fixPathForTests(sourceObj.value("Url").toString()),
							   sourceObj.value("CompressionType").toString()));
			}
			else
			{
				QLOG_WARN() << "Unknown source type" << type << "ignored.";
			}
		}

		QLOG_DEBUG() << "Loaded info for" << file.path;

		list->append(file);
	}

	return true;
}

void DownloadUpdateTask::processFileLists()
{
	// Create a network job for downloading files.
	NetJob *netJob = new NetJob("Update Files");

	if (!processFileLists(netJob, m_cVersionFileList, m_nVersionFileList, m_operationList))
	{
		emitFailed(tr("Failed to process update lists..."));
		return;
	}

	// Add listeners to wait for the downloads to finish.
	QObject::connect(netJob, &NetJob::succeeded, this,
					 &DownloadUpdateTask::fileDownloadFinished);
	QObject::connect(netJob, &NetJob::progress, this,
					 &DownloadUpdateTask::fileDownloadProgressChanged);
	QObject::connect(netJob, &NetJob::failed, this, &DownloadUpdateTask::fileDownloadFailed);

	// Now start the download.
	setStatus(tr("Downloading %1 update files.").arg(QString::number(netJob->size())));
	QLOG_DEBUG() << "Begin downloading update files to" << m_updateFilesDir.path();
	m_filesNetJob.reset(netJob);
	netJob->start();

	writeInstallScript(m_operationList, PathCombine(m_updateFilesDir.path(), "file_list.xml"));
}

bool
DownloadUpdateTask::processFileLists(NetJob *job,
									 const DownloadUpdateTask::VersionFileList &currentVersion,
									 const DownloadUpdateTask::VersionFileList &newVersion,
									 DownloadUpdateTask::UpdateOperationList &ops)
{
	setStatus(tr("Processing file lists - figuring out how to install the update..."));

	// First, if we've loaded the current version's file list, we need to iterate through it and
	// delete anything in the current one version's list that isn't in the new version's list.
	for (VersionFileEntry entry : currentVersion)
	{
		QFileInfo toDelete(entry.path);
		if (!toDelete.exists())
		{
			QLOG_ERROR() << "Expected file " << toDelete.absoluteFilePath()
						 << " doesn't exist!";
			QLOG_ERROR() << "CWD: " << QDir::currentPath();
		}
		bool keep = false;

		//
		for (VersionFileEntry newEntry : newVersion)
		{
			if (newEntry.path == entry.path)
			{
				QLOG_DEBUG() << "Not deleting" << entry.path
							 << "because it is still present in the new version.";
				keep = true;
				break;
			}
		}

		// If the loop reaches the end and we didn't find a match, delete the file.
		if (!keep)
		{
			QFileInfo toDelete(entry.path);
			if (toDelete.exists())
				ops.append(UpdateOperation::DeleteOp(entry.path));
		}
	}

	// Next, check each file in MultiMC's folder and see if we need to update them.
	for (VersionFileEntry entry : newVersion)
	{
		// TODO: Let's not MD5sum a ton of files on the GUI thread. We should probably find a
		// way to do this in the background.
		QString fileMD5;
		QFile entryFile(entry.path);
		QFileInfo entryInfo(entry.path);

		bool needs_upgrade = false;
		if (!entryFile.exists())
		{
			needs_upgrade = true;
		}
		else
		{
			bool pass = true;
			if (!entryInfo.isReadable())
			{
				QLOG_ERROR() << "File " << entry.path << " is not readable.";
				pass = false;
			}
			if (!entryInfo.isWritable())
			{
				QLOG_ERROR() << "File " << entry.path << " is not writable.";
				pass = false;
			}
			if (!entryFile.open(QFile::ReadOnly))
			{
				QLOG_ERROR() << "File " << entry.path << " cannot be opened for reading.";
				pass = false;
			}
			if (!pass)
			{
				QLOG_ERROR() << "CWD: " << QDir::currentPath();
				ops.clear();
				return false;
			}
		}

		QCryptographicHash hash(QCryptographicHash::Md5);
		auto foo = entryFile.readAll();

		hash.addData(foo);
		fileMD5 = hash.result().toHex();
		if ((fileMD5 != entry.md5))
		{
			QLOG_DEBUG() << "MD5Sum does not match!";
			QLOG_DEBUG() << "Expected:'" << entry.md5 << "'";
			QLOG_DEBUG() << "Got:     '" << fileMD5 << "'";
			needs_upgrade = true;
		}

		// skip file. it doesn't need an upgrade.
		if (!needs_upgrade)
		{
			QLOG_DEBUG() << "File" << entry.path << " does not need updating.";
			continue;
		}

		// yep. this file actually needs an upgrade. PROCEED.
		QLOG_DEBUG() << "Found file" << entry.path << " that needs updating.";

		// if it's the updater we want to treat it separately
		bool isUpdater = entry.path.endsWith("updater") || entry.path.endsWith("updater.exe");

		// Go through the sources list and find one to use.
		// TODO: Make a NetAction that takes a source list and tries each of them until one
		// works. For now, we'll just use the first http one.
		for (FileSource source : entry.sources)
		{
			if (source.type == "http")
			{
				QLOG_DEBUG() << "Will download" << entry.path << "from" << source.url;

				// Download it to updatedir/<filepath>-<md5> where filepath is the file's
				// path with slashes replaced by underscores.
				QString dlPath =
					PathCombine(m_updateFilesDir.path(), QString(entry.path).replace("/", "_"));

				if (isUpdater)
				{
					auto cache_entry = MMC->metacache()->resolveEntry("root", entry.path);
					QLOG_DEBUG() << "Updater will be in " << cache_entry->getFullPath();
					// force check.
					cache_entry->stale = true;
					auto download = CacheDownload::make(QUrl(source.url), cache_entry);
					job->addNetAction(download);
				}
				else
				{
					// We need to download the file to the updatefiles folder and add a task
					// to copy it to its install path.
					auto download = MD5EtagDownload::make(source.url, dlPath);
					download->m_expected_md5 = entry.md5;
					job->addNetAction(download);
					ops.append(UpdateOperation::CopyOp(dlPath, entry.path, entry.mode));
				}
			}
		}
	}
	return true;
}

bool DownloadUpdateTask::writeInstallScript(UpdateOperationList &opsList, QString scriptFile)
{
	// Build the base structure of the XML document.
	QDomDocument doc;

	QDomElement root = doc.createElement("update");
	root.setAttribute("version", "3");
	doc.appendChild(root);

	QDomElement installFiles = doc.createElement("install");
	root.appendChild(installFiles);

	QDomElement removeFiles = doc.createElement("uninstall");
	root.appendChild(removeFiles);

	// Write the operation list to the XML document.
	for (UpdateOperation op : opsList)
	{
		QDomElement file = doc.createElement("file");

		switch (op.type)
		{
		case UpdateOperation::OP_COPY:
		{
			// Install the file.
			QDomElement name = doc.createElement("source");
			QDomElement path = doc.createElement("dest");
			QDomElement mode = doc.createElement("mode");
			name.appendChild(doc.createTextNode(op.file));
			path.appendChild(doc.createTextNode(op.dest));
			// We need to add a 0 at the beginning here, because Qt doesn't convert to octal
			// correctly.
			mode.appendChild(doc.createTextNode("0" + QString::number(op.mode, 8)));
			file.appendChild(name);
			file.appendChild(path);
			file.appendChild(mode);
			installFiles.appendChild(file);
			QLOG_DEBUG() << "Will install file " << op.file << " to " << op.dest;
		}
		break;

		case UpdateOperation::OP_DELETE:
		{
			// Delete the file.
			file.appendChild(doc.createTextNode(op.file));
			removeFiles.appendChild(file);
			QLOG_DEBUG() << "Will remove file" << op.file;
		}
		break;

		default:
			QLOG_WARN() << "Can't write update operation of type" << op.type
						<< "to file. Not implemented.";
			continue;
		}
	}

	// Write the XML document to the file.
	QFile outFile(scriptFile);

	if (outFile.open(QIODevice::WriteOnly))
	{
		outFile.write(doc.toByteArray());
	}
	else
	{
		emitFailed(tr("Failed to write update script file."));
		return false;
	}

	return true;
}

QString DownloadUpdateTask::fixPathForTests(const QString &path)
{
	if (path.startsWith("$PWD"))
	{
		QString foo = path;
		foo.replace("$PWD", qApp->applicationDirPath());
		return QUrl::fromLocalFile(foo).toString(QUrl::FullyEncoded);
	}
	return path;
}

bool DownloadUpdateTask::fixPathForOSX(QString &path)
{
	if (path.startsWith("MultiMC.app/"))
	{
		// remove the prefix and add a new, more appropriate one.
		path.remove(0, 12);
		path = QString("../../") + path;
		return true;
	}
	else
	{
		QLOG_ERROR() << "Update path not within .app: " << path;
		return false;
	}
}

void DownloadUpdateTask::fileDownloadFinished()
{
	emitSucceeded();
}

void DownloadUpdateTask::fileDownloadFailed()
{
	// TODO: Give more info about the failure.
	QLOG_ERROR() << "Failed to download update files.";
	emitFailed(tr("Failed to download update files."));
}

void DownloadUpdateTask::fileDownloadProgressChanged(qint64 current, qint64 total)
{
	setProgress((int)(((float)current / (float)total) * 100));
}

QString DownloadUpdateTask::updateFilesDir()
{
	return m_updateFilesDir.path();
}
