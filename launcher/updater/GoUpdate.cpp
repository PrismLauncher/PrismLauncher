#include "GoUpdate.h"
#include <QDebug>
#include <QDomDocument>
#include <QFile>
#include <FileSystem.h>

#include "net/Download.h"
#include "net/ChecksumValidator.h"

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

        VersionFileEntry file{file_path,        fileObj.value("Perms").toVariant().toInt(),
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
    NetJob::Ptr job,
    OperationList &ops
)
{
    // First, if we've loaded the current version's file list, we need to iterate through it and
    // delete anything in the current one version's list that isn't in the new version's list.
    for (VersionFileEntry entry : currentVersion)
    {
        QFileInfo toDelete(FS::PathCombine(rootPath, entry.path));
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
        QString realEntryPath = FS::PathCombine(rootPath, entry.path);
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
            QString dlPath = FS::PathCombine(tempPath, QString(entry.path).replace("/", "_"));

            // We need to download the file to the updatefiles folder and add a task
            // to copy it to its install path.
            auto download = Net::Download::makeFile(source.url, dlPath);
            auto rawMd5 = QByteArray::fromHex(entry.md5.toLatin1());
            download->addValidator(new Net::ChecksumValidator(QCryptographicHash::Md5, rawMd5));
            job->addNetAction(download);
            ops.append(Operation::CopyOp(dlPath, entry.path, entry.mode));
        }
    }
    return true;
}
}
