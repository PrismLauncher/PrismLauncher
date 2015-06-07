#include "GoUpdate.h"
#include <pathutils.h>
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <Env.h>

namespace GoUpdate
{

bool parseVersionInfo(const QByteArray &data, VersionFileList &list, QString &error)
{
	QJsonParseError jsonError;
	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError)
	{
		error = QString("Failed to parse version info JSON: %1 at %2")
					.arg(jsonError.errorString())
					.arg(jsonError.offset);
		qCritical() << error;
		return false;
	}

	QJsonObject json = jsonDoc.object();

	qDebug() << data;
	qDebug() << "Loading version info from JSON.";
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
		fixPathForOSX(file_path);
#endif
		VersionFileEntry file{file_path,		fileObj.value("Perms").toVariant().toInt(),
							  FileSourceList(), fileObj.value("MD5").toString(), };
		qDebug() << "File" << file.path << "with perms" << file.mode;

		QJsonArray sourceArray = fileObj.value("Sources").toArray();
		for (QJsonValue val : sourceArray)
		{
			QJsonObject sourceObj = val.toObject();

			QString type = sourceObj.value("SourceType").toString();
			if (type == "http")
			{
				file.sources.append(FileSource("http", sourceObj.value("Url").toString()));
			}
			else
			{
				qWarning() << "Unknown source type" << type << "ignored.";
			}
		}

		qDebug() << "Loaded info for" << file.path;

		list.append(file);
	}

	return true;
}

bool processFileLists
(
	const VersionFileList &currentVersion,
	const VersionFileList &newVersion,
	const QString &rootPath,
	const QString &tempPath,
	NetJobPtr job,
	OperationList &ops
)
{
	// First, if we've loaded the current version's file list, we need to iterate through it and
	// delete anything in the current one version's list that isn't in the new version's list.
	for (VersionFileEntry entry : currentVersion)
	{
		QFileInfo toDelete(PathCombine(rootPath, entry.path));
		if (!toDelete.exists())
		{
			qCritical() << "Expected file " << toDelete.absoluteFilePath()
						 << " doesn't exist!";
		}
		bool keep = false;

		//
		for (VersionFileEntry newEntry : newVersion)
		{
			if (newEntry.path == entry.path)
			{
				qDebug() << "Not deleting" << entry.path
							 << "because it is still present in the new version.";
				keep = true;
				break;
			}
		}

		// If the loop reaches the end and we didn't find a match, delete the file.
		if (!keep)
		{
			if (toDelete.exists())
				ops.append(Operation::DeleteOp(entry.path));
		}
	}

	// Next, check each file in MultiMC's folder and see if we need to update them.
	for (VersionFileEntry entry : newVersion)
	{
		// TODO: Let's not MD5sum a ton of files on the GUI thread. We should probably find a
		// way to do this in the background.
		QString fileMD5;
		QString realEntryPath = PathCombine(rootPath, entry.path);
		QFile entryFile(realEntryPath);
		QFileInfo entryInfo(realEntryPath);

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
				qCritical() << "File " << realEntryPath << " is not readable.";
				pass = false;
			}
			if (!entryInfo.isWritable())
			{
				qCritical() << "File " << realEntryPath << " is not writable.";
				pass = false;
			}
			if (!entryFile.open(QFile::ReadOnly))
			{
				qCritical() << "File " << realEntryPath << " cannot be opened for reading.";
				pass = false;
			}
			if (!pass)
			{
				ops.clear();
				return false;
			}
		}

		if(!needs_upgrade)
		{
			QCryptographicHash hash(QCryptographicHash::Md5);
			auto foo = entryFile.readAll();

			hash.addData(foo);
			fileMD5 = hash.result().toHex();
			if ((fileMD5 != entry.md5))
			{
				qDebug() << "MD5Sum does not match!";
				qDebug() << "Expected:'" << entry.md5 << "'";
				qDebug() << "Got:     '" << fileMD5 << "'";
				needs_upgrade = true;
			}
		}

		// skip file. it doesn't need an upgrade.
		if (!needs_upgrade)
		{
			qDebug() << "File" << realEntryPath << " does not need updating.";
			continue;
		}

		// yep. this file actually needs an upgrade. PROCEED.
		qDebug() << "Found file" << realEntryPath << " that needs updating.";

		// Go through the sources list and find one to use.
		// TODO: Make a NetAction that takes a source list and tries each of them until one
		// works. For now, we'll just use the first http one.
		for (FileSource source : entry.sources)
		{
			if (source.type != "http")
				continue;

			qDebug() << "Will download" << entry.path << "from" << source.url;

			// Download it to updatedir/<filepath>-<md5> where filepath is the file's
			// path with slashes replaced by underscores.
			QString dlPath = PathCombine(tempPath, QString(entry.path).replace("/", "_"));

			// We need to download the file to the updatefiles folder and add a task
			// to copy it to its install path.
			auto download = MD5EtagDownload::make(source.url, dlPath);
			download->m_expected_md5 = entry.md5;
			job->addNetAction(download);
			ops.append(Operation::CopyOp(dlPath, entry.path, entry.mode));
		}
	}
	return true;
}

bool fixPathForOSX(QString &path)
{
	if (path.startsWith("MultiMC.app/"))
	{
		// remove the prefix and add a new, more appropriate one.
		path.remove(0, 12);
		return true;
	}
	else
	{
		qCritical() << "Update path not within .app: " << path;
		return false;
	}
}

bool writeInstallScript(OperationList &opsList, QString scriptFile)
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
	for (Operation op : opsList)
	{
		QDomElement file = doc.createElement("file");

		switch (op.type)
		{
		case Operation::OP_COPY:
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
			qDebug() << "Will install file " << op.file << " to " << op.dest;
		}
		break;

		case Operation::OP_DELETE:
		{
			// Delete the file.
			file.appendChild(doc.createTextNode(op.file));
			removeFiles.appendChild(file);
			qDebug() << "Will remove file" << op.file;
		}
		break;

		default:
			qWarning() << "Can't write update operation of type" << op.type
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
		return false;
	}

	return true;
}


}