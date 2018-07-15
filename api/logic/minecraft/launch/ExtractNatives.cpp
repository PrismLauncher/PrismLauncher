/* Copyright 2013-2018 MultiMC Contributors
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

#include "ExtractNatives.h"
#include <minecraft/MinecraftInstance.h>
#include <launch/LaunchTask.h>

#include <quazip.h>
#include <quazipdir.h>
#include "MMCZip.h"
#include "FileSystem.h"
#include <QDir>

static QString replaceSuffix (QString target, const QString &suffix, const QString &replacement)
{
    if (!target.endsWith(suffix))
    {
        return target;
    }
    target.resize(target.length() - suffix.length());
    return target + replacement;
}

static bool unzipNatives(QString source, QString targetFolder, bool applyJnilibHack)
{
    QuaZip zip(source);
    if(!zip.open(QuaZip::mdUnzip))
    {
        return false;
    }
    QDir directory(targetFolder);
    if (!zip.goToFirstFile())
    {
        return false;
    }
    do
    {
        QString name = zip.getCurrentFileName();
        if(applyJnilibHack)
        {
            name = replaceSuffix(name, ".jnilib", ".dylib");
        }
        QString absFilePath = directory.absoluteFilePath(name);
        if (!JlCompress::extractFile(&zip, "", absFilePath))
        {
            return false;
        }
    } while (zip.goToNextFile());
    zip.close();
    if(zip.getZipError()!=0)
    {
        return false;
    }
    return true;
}

void ExtractNatives::executeTask()
{
    auto instance = m_parent->instance();
    std::shared_ptr<MinecraftInstance> minecraftInstance = std::dynamic_pointer_cast<MinecraftInstance>(instance);
    auto toExtract = minecraftInstance->getNativeJars();
    if(toExtract.isEmpty())
    {
        emitSucceeded();
        return;
    }
    auto outputPath  = minecraftInstance->getNativePath();
    auto javaVersion = minecraftInstance->getJavaVersion();
    bool jniHackEnabled = javaVersion.major() >= 8;
    for(const auto &source: toExtract)
    {
        if(!unzipNatives(source, outputPath, jniHackEnabled))
        {
            auto reason = tr("Couldn't extract native jar '%1' to destination '%2'").arg(source, outputPath);
            emit logLine(reason, MessageLevel::Fatal);
            emitFailed(reason);
        }
    }
    emitSucceeded();
}

void ExtractNatives::finalize()
{
    auto instance = m_parent->instance();
    QString target_dir = FS::PathCombine(instance->instanceRoot(), "natives/");
    QDir dir(target_dir);
    dir.removeRecursively();
}
