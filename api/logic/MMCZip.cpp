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

#include <quazip.h>
#include <quazipdir.h>
#include <quazipfile.h>
#include <JlCompress.h>
#include "MMCZip.h"
#include "FileSystem.h"

#include <QDebug>

// ours
bool MMCZip::mergeZipFiles(QuaZip *into, QFileInfo from, QSet<QString> &contained, const JlCompress::FilterFunction filter)
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
bool MMCZip::createModdedJar(QString sourceJarPath, QString targetJarPath, const QList<Mod>& mods)
{
    QuaZip zipOut(targetJarPath);
    if (!zipOut.open(QuaZip::mdCreate))
    {
        QFile::remove(targetJarPath);
        qCritical() << "Failed to open the minecraft.jar for modding";
        return false;
    }
    // Files already added to the jar.
    // These files will be skipped.
    QSet<QString> addedFiles;

    // Modify the jar
    QListIterator<Mod> i(mods);
    i.toBack();
    while (i.hasPrevious())
    {
        const Mod &mod = i.previous();
        // do not merge disabled mods.
        if (!mod.enabled())
            continue;
        if (mod.type() == Mod::MOD_ZIPFILE)
        {
            if (!mergeZipFiles(&zipOut, mod.filename(), addedFiles))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
        }
        else if (mod.type() == Mod::MOD_SINGLEFILE)
        {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod.filename();
            if (!JlCompress::compressFile(&zipOut, filename.absoluteFilePath(), filename.fileName()))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
            addedFiles.insert(filename.fileName());
        }
        else if (mod.type() == Mod::MOD_FOLDER)
        {
            // FIXME: buggy - does not work with addedFiles
            auto filename = mod.filename();
            QString what_to_zip = filename.absoluteFilePath();
            QDir dir(what_to_zip);
            dir.cdUp();
            QString parent_dir = dir.absolutePath();
            if (!JlCompress::compressSubDir(&zipOut, what_to_zip, parent_dir, addedFiles))
            {
                zipOut.close();
                QFile::remove(targetJarPath);
                qCritical() << "Failed to add" << mod.filename().fileName() << "to the jar.";
                return false;
            }
            qDebug() << "Adding folder " << filename.fileName() << " from "
                        << filename.absoluteFilePath();
        }
        else
        {
            // Make sure we do not continue launching when something is missing or undefined...
            zipOut.close();
            QFile::remove(targetJarPath);
            qCritical() << "Failed to add unknown mod type" << mod.filename().fileName() << "to the jar.";
            return false;
        }
    }

    if (!mergeZipFiles(&zipOut, QFileInfo(sourceJarPath), addedFiles, [](const QString key){return !key.contains("META-INF");}))
    {
        zipOut.close();
        QFile::remove(targetJarPath);
        qCritical() << "Failed to insert minecraft.jar contents.";
        return false;
    }

    // Recompress the jar
    zipOut.close();
    if (zipOut.getZipError() != 0)
    {
        QFile::remove(targetJarPath);
        qCritical() << "Failed to finalize minecraft.jar!";
        return false;
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
QStringList MMCZip::extractSubDir(QuaZip *zip, const QString & subdir, const QString &target)
{
    QDir directory(target);
    QStringList extracted;
    qDebug() << "Extracting subdir" << subdir << "from" << zip->getZipName() << "to" << target;
    if (!zip->goToFirstFile())
    {
        qWarning() << "Failed to seek to first file in zip";
        return QStringList();
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
            return QStringList();
        }
        extracted.append(absFilePath);
        qDebug() << "Extracted file" << name;
    } while (zip->goToNextFile());
    return extracted;
}

// ours
QStringList MMCZip::extractDir(QString fileCompressed, QString dir)
{
    QuaZip zip(fileCompressed);
    if (!zip.open(QuaZip::mdUnzip))
    {
        return {};
    }
    return MMCZip::extractSubDir(&zip, "", dir);
}
