/* Copyright 2013-2021 MultiMC Contributors
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
#ifdef QUAZIP_USE_SUBMODULE
#include <quazip/quazip.h>
#include <quazip/quazipdir.h>
#include <quazip/quazipfile.h>
#else
#include <QuaZip-Qt5-1.2/quazip/quazip.h>
#include <QuaZip-Qt5-1.2/quazip/quazipdir.h>
#include <QuaZip-Qt5-1.2/quazip/quazipfile.h>
#endif
#include "MMCZip.h"
#include "FileSystem.h"

#include <QDebug>

// ours
bool MMCZip::mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, const FilterFunction filter)
{
    QuaZip modZip(from.filePath());
    modZip.open(QuaZip::mdUnzip);

    QuaZipFile fileInsideMod(&modZip);
    QuaZipFile zipOutFile(into);
    for (bool more = modZip.goToFirstFile(); more; more = modZip.goToNextFile())
    {
        QString filename = modZip.getCurrentFileName();
        if (filter && !filter(filename))
        {
            qDebug() << "Skipping file " << filename << " from "
                        << from.fileName() << " - filtered";
            continue;
        }
        if (contained.contains(filename))
        {
            qDebug() << "Skipping already contained file " << filename << " from "
                        << from.fileName();
            continue;
        }
        contained.insert(filename);

        if (!fileInsideMod.open(QIODevice::ReadOnly))
        {
            qCritical() << "Failed to open " << filename << " from " << from.fileName();
            return false;
        }

        QuaZipNewInfo info_out(fileInsideMod.getActualFileName());

        if (!zipOutFile.open(QIODevice::WriteOnly, info_out))
        {
            qCritical() << "Failed to open " << filename << " in the jar";
            fileInsideMod.close();
            return false;
        }
        if (!JlCompress::copyData(fileInsideMod, zipOutFile))
        {
            zipOutFile.close();
            fileInsideMod.close();
            qCritical() << "Failed to copy data of " << filename << " into the jar";
            return false;
        }
        zipOutFile.close();
        fileInsideMod.close();
    }
    return true;
}

// ours
QString MMCZip::findFolderOfFileInZip(QuaZip * zip, const QString & what, const QString &root)
{
    QuaZipDir rootDir(zip, root);
    for(auto fileName: rootDir.entryList(QDir::Files))
    {
        if(fileName == what)
            return root;
    }
    for(auto fileName: rootDir.entryList(QDir::Dirs))
    {
        QString result = findFolderOfFileInZip(zip, what, root + fileName);
        if(!result.isEmpty())
        {
            return result;
        }
    }
    return QString();
}

// ours
bool MMCZip::findFilesInZip(QuaZip * zip, const QString & what, QStringList & result, const QString &root)
{
    QuaZipDir rootDir(zip, root);
    for(auto fileName: rootDir.entryList(QDir::Files))
    {
        if(fileName == what)
        {
            result.append(root);
            return true;
        }
    }
    for(auto fileName: rootDir.entryList(QDir::Dirs))
    {
        findFilesInZip(zip, what, result, root + fileName);
    }
    return !result.isEmpty();
}


// ours
nonstd::optional<QStringList> MMCZip::extractSubDir(QuaZip *zip, const QString & subdir, const QString &target)
{
    QDir directory(target);
    QStringList extracted;

    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    auto numEntries = zip->getEntriesCount();
    if(numEntries < 0) {
        qWarning() << "Failed to enumerate files in archive";
        return nonstd::nullopt;
    }
    else if(numEntries == 0) {
        qDebug() << "Extracting empty archives seems odd...";
        return extracted;
    }
    else if (!zip->goToFirstFile())
    {
        qWarning() << "Failed to seek to first file in zip";
        return nonstd::nullopt;
    }

    do
    {
        QString name = zip->getCurrentFileName();
        if(!name.startsWith(subdir))
        {
            continue;
        }
        name.remove(0, subdir.size());
        QString absFilePath = directory.absoluteFilePath(name);
        if(name.isEmpty())
        {
            absFilePath += "/";
        }
        if (!JlCompress::extractFile(zip, "", absFilePath))
        {
            qWarning() << "Failed to extract file" << name << "to" << absFilePath;
            JlCompress::removeFile(extracted);
            return nonstd::nullopt;
        }
        extracted.append(absFilePath);
        qDebug() << "Extracted file" << name;
    } while (zip->goToNextFile());
    return extracted;
}

// ours
bool MMCZip::extractRelFile(QuaZip *zip, const QString &file, const QString &target)
{
    return JlCompress::extractFile(zip, file, target);
}

// ours
nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();;
        return nonstd::nullopt;
    }
    return MMCZip::extractSubDir(&zip, "", dir);
}

// ours
nonstd::optional<QStringList> MMCZip::extractDir(QString fileCompressed, QString subdir, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return QStringList();
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();;
        return nonstd::nullopt;
    }
    return MMCZip::extractSubDir(&zip, subdir, dir);
}

// ours
bool MMCZip::extractFile(QString fileCompressed, QString file, QString target)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        // check if this is a minimum size empty zip file...
        QFileInfo fileInfo(fileCompressed);
        if(fileInfo.size() == 22) {
            return true;
        }
        qWarning() << "Could not open archive for unzipping:" << fileCompressed << "Error:" << zip.getZipError();
        return false;
    }
    return MMCZip::extractRelFile(&zip, file, target);
}
