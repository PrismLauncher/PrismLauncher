// SPDX-License-Identifier: GPL-3.0-only
/*
 *  PolyMC - Minecraft Launcher
 *  Copyright (C) 2022 Sefa Eyeoglu <contact@scrumplex.net>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, version 3.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 * This file incorporates work covered by the following copyright and
 * permission notice:
 *
 *      Copyright 2013-2021 MultiMC Contributors
 *
 *      Licensed under the Apache License, Version 2.0 (the "License");
 *      you may not use this file except in compliance with the License.
 *      You may obtain a copy of the License at
 *
 *          http://www.apache.org/licenses/LICENSE-2.0
 *
 *      Unless required by applicable law or agreed to in writing, software
 *      distributed under the License is distributed on an "AS IS" BASIS,
 *      WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *      See the License for the specific language governing permissions and
 *      limitations under the License.
 */

#include "Library.h"
#include "MinecraftInstance.h"

#include <net/Download.h>
#include <net/ChecksumValidator.h>
#include <FileSystem.h>
#include <BuildConfig.h>


void Library::getApplicableFiles(const RuntimeContext & runtimeContext, QStringList& jar, QStringList& native, QStringList& native32,
                                 QStringList& native64, const QString &overridePath) const
{
    bool local = isLocal();
    auto actualPath = [&](QString relPath)
    {
        QFileInfo out(FS::PathCombine(storagePrefix(), relPath));
        if(local && !overridePath.isEmpty())
        {
            QString fileName = out.fileName();
            return QFileInfo(FS::PathCombine(overridePath, fileName)).absoluteFilePath();
        }
        return out.absoluteFilePath();
    };
    QString raw_storage = storageSuffix(runtimeContext);
    if(isNative())
    {
        if (raw_storage.contains("${arch}"))
        {
            auto nat32Storage = raw_storage;
            nat32Storage.replace("${arch}", "32");
            auto nat64Storage = raw_storage;
            nat64Storage.replace("${arch}", "64");
            native32 += actualPath(nat32Storage);
            native64 += actualPath(nat64Storage);
        }
        else
        {
            native += actualPath(raw_storage);
        }
    }
    else
    {
        jar += actualPath(raw_storage);
    }
}

QList<NetAction::Ptr> Library::getDownloads(
    const RuntimeContext & runtimeContext,
    class HttpMetaCache* cache,
    QStringList& failedLocalFiles,
    const QString & overridePath
) const
{
    QList<NetAction::Ptr> out;
    bool stale = isAlwaysStale();
    bool local = isLocal();

    auto check_local_file = [&](QString storage)
    {
        QFileInfo fileinfo(storage);
        QString fileName = fileinfo.fileName();
        auto fullPath = FS::PathCombine(overridePath, fileName);
        QFileInfo localFileInfo(fullPath);
        if(!localFileInfo.exists())
        {
            failedLocalFiles.append(localFileInfo.filePath());
            return false;
        }
        return true;
    };

    auto add_download = [&](QString storage, QString url, QString sha1)
    {
        if(local)
        {
            return check_local_file(storage);
        }
        auto entry = cache->resolveEntry("libraries", storage);
        if(stale)
        {
            entry->setStale(true);
        }
        if (!entry->isStale())
            return true;
        Net::Download::Options options;
        if(stale)
        {
            options |= Net::Download::Option::AcceptLocalFiles;
        }

        // Don't add a time limit for the libraries cache entry validity
        options |= Net::Download::Option::MakeEternal;

        if(sha1.size())
        {
            auto rawSha1 = QByteArray::fromHex(sha1.toLatin1());
            auto dl = Net::Download::makeCached(url, entry, options);
            dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
            qCDebug(LAUNCHER_LOG) << "Checksummed Download for:" << rawName().serialize() << "storage:" << storage << "url:" << url;
            out.append(dl);
        }
        else
        {
            out.append(Net::Download::makeCached(url, entry, options));
            qCDebug(LAUNCHER_LOG) << "Download for:" << rawName().serialize() << "storage:" << storage << "url:" << url;
        }
        return true;
    };

    QString raw_storage = storageSuffix(runtimeContext);
    if(m_mojangDownloads)
    {
        if(isNative())
        {
            auto nativeClassifier = getCompatibleNative(runtimeContext);
            if(!nativeClassifier.isNull())
            {
                if(nativeClassifier.contains("${arch}"))
                {
                    auto nat32Classifier = nativeClassifier;
                    nat32Classifier.replace("${arch}", "32");
                    auto nat64Classifier = nativeClassifier;
                    nat64Classifier.replace("${arch}", "64");
                    auto nat32info = m_mojangDownloads->getDownloadInfo(nat32Classifier);
                    if(nat32info)
                    {
                        auto cooked_storage = raw_storage;
                        cooked_storage.replace("${arch}", "32");
                        add_download(cooked_storage, nat32info->url, nat32info->sha1);
                    }
                    auto nat64info = m_mojangDownloads->getDownloadInfo(nat64Classifier);
                    if(nat64info)
                    {
                        auto cooked_storage = raw_storage;
                        cooked_storage.replace("${arch}", "64");
                        add_download(cooked_storage, nat64info->url, nat64info->sha1);
                    }
                }
                else
                {
                    auto info = m_mojangDownloads->getDownloadInfo(nativeClassifier);
                    if(info)
                    {
                        add_download(raw_storage, info->url, info->sha1);
                    }
                }
            }
            else
            {
                qCDebug(LAUNCHER_LOG) << "Ignoring native library" << m_name.serialize() << "because it has no classifier for current OS";
            }
        }
        else
        {
            if(m_mojangDownloads->artifact)
            {
                auto artifact = m_mojangDownloads->artifact;
                add_download(raw_storage, artifact->url, artifact->sha1);
            }
            else
            {
                qCDebug(LAUNCHER_LOG) << "Ignoring java library" << m_name.serialize() << "because it has no artifact";
            }
        }
    }
    else
    {
        auto raw_dl = [&]()
        {
            if (!m_absoluteURL.isEmpty())
            {
                return m_absoluteURL;
            }

            if (m_repositoryURL.isEmpty())
            {
                return BuildConfig.LIBRARY_BASE + raw_storage;
            }

            if(m_repositoryURL.endsWith('/'))
            {
                return m_repositoryURL + raw_storage;
            }
            else
            {
                return m_repositoryURL + QChar('/') + raw_storage;
            }
        }();
        if (raw_storage.contains("${arch}"))
        {
            QString cooked_storage = raw_storage;
            QString cooked_dl = raw_dl;
            add_download(cooked_storage.replace("${arch}", "32"), cooked_dl.replace("${arch}", "32"), QString());
            cooked_storage = raw_storage;
            cooked_dl = raw_dl;
            add_download(cooked_storage.replace("${arch}", "64"), cooked_dl.replace("${arch}", "64"), QString());
        }
        else
        {
            add_download(raw_storage, raw_dl, QString());
        }
    }
    return out;
}

bool Library::isActive(const RuntimeContext & runtimeContext) const
{
    bool result = true;
    if (m_rules.empty())
    {
        result = true;
    }
    else
    {
        RuleAction ruleResult = Disallow;
        for (auto rule : m_rules)
        {
            RuleAction temp = rule->apply(this, runtimeContext);
            if (temp != Defer)
                ruleResult = temp;
        }
        result = result && (ruleResult == Allow);
    }
    if (isNative())
    {
        result = result && !getCompatibleNative(runtimeContext).isNull();
    }
    return result;
}

bool Library::isLocal() const
{
    return m_hint == "local";
}

bool Library::isAlwaysStale() const
{
    return m_hint == "always-stale";
}

QString Library::getCompatibleNative(const RuntimeContext & runtimeContext) const {
    // try to match precise classifier "[os]-[arch]"
    auto entry = m_nativeClassifiers.constFind(runtimeContext.getClassifier());
    // try to match imprecise classifier on legacy architectures "[os]"
    if (entry == m_nativeClassifiers.constEnd() && runtimeContext.isLegacyArch())
        entry = m_nativeClassifiers.constFind(runtimeContext.system);

    if (entry == m_nativeClassifiers.constEnd())
        return QString();

    return entry.value();
}

void Library::setStoragePrefix(QString prefix)
{
    m_storagePrefix = prefix;
}

QString Library::defaultStoragePrefix()
{
    return "libraries/";
}

QString Library::storagePrefix() const
{
    if(m_storagePrefix.isEmpty())
    {
        return defaultStoragePrefix();
    }
    return m_storagePrefix;
}

QString Library::filename(const RuntimeContext & runtimeContext) const
{
    if(!m_filename.isEmpty())
    {
        return m_filename;
    }
    // non-native? use only the gradle specifier
    if (!isNative())
    {
        return m_name.getFileName();
    }

    // otherwise native, override classifiers. Mojang HACK!
    GradleSpecifier nativeSpec = m_name;
    QString nativeClassifier = getCompatibleNative(runtimeContext);
    if (!nativeClassifier.isNull())
    {
        nativeSpec.setClassifier(nativeClassifier);
    }
    else
    {
        nativeSpec.setClassifier("INVALID");
    }
    return nativeSpec.getFileName();
}

QString Library::displayName(const RuntimeContext & runtimeContext) const
{
    if(!m_displayname.isEmpty())
        return m_displayname;
    return filename(runtimeContext);
}

QString Library::storageSuffix(const RuntimeContext & runtimeContext) const
{
    // non-native? use only the gradle specifier
    if (!isNative())
    {
        return m_name.toPath(m_filename);
    }

    // otherwise native, override classifiers. Mojang HACK!
    GradleSpecifier nativeSpec = m_name;
    QString nativeClassifier = getCompatibleNative(runtimeContext);
    if (!nativeClassifier.isNull())
    {
        nativeSpec.setClassifier(nativeClassifier);
    }
    else
    {
        nativeSpec.setClassifier("INVALID");
    }
    return nativeSpec.toPath(m_filename);
}
