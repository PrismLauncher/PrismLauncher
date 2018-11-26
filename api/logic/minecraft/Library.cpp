#include "Library.h"
#include "MinecraftInstance.h"

#include <net/Download.h>
#include <net/ChecksumValidator.h>
#include <minecraft/forge/ForgeXzDownload.h>
#include <Env.h>
#include <FileSystem.h>


void Library::getApplicableFiles(OpSys system, QStringList& jar, QStringList& native, QStringList& native32,
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
    QString raw_storage = storageSuffix(system);
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

QList< std::shared_ptr< NetAction > > Library::getDownloads(
    OpSys system,
    class HttpMetaCache* cache,
    QStringList& failedLocalFiles,
    const QString & overridePath
) const
{
    QList<NetActionPtr> out;
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
        if (isForge())
        {
            qDebug() << "XzDownload for:" << rawName() << "storage:" << storage << "url:" << url;
            out.append(ForgeXzDownload::make(url, storage, entry));
        }
        else
        {
            if(sha1.size())
            {
                auto rawSha1 = QByteArray::fromHex(sha1.toLatin1());
                auto dl = Net::Download::makeCached(url, entry, options);
                dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
                qDebug() << "Checksummed Download for:" << rawName() << "storage:" << storage << "url:" << url;
                out.append(dl);
            }
            else
            {
                out.append(Net::Download::makeCached(url, entry, options));
                qDebug() << "Download for:" << rawName() << "storage:" << storage << "url:" << url;
            }
        }
        return true;
    };

    QString raw_storage = storageSuffix(system);
    if(m_mojangDownloads)
    {
        if(isNative())
        {
            if(m_nativeClassifiers.contains(system))
            {
                auto nativeClassifier = m_nativeClassifiers[system];
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
                qDebug() << "Ignoring native library" << m_name << "because it has no classifier for current OS";
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
                qDebug() << "Ignoring java library" << m_name << "because it has no artifact";
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
                return URLConstants::LIBRARY_BASE + raw_storage;
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

bool Library::isActive() const
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
            RuleAction temp = rule->apply(this);
            if (temp != Defer)
                ruleResult = temp;
        }
        result = result && (ruleResult == Allow);
    }
    if (isNative())
    {
        result = result && m_nativeClassifiers.contains(currentSystem);
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

bool Library::isForge() const
{
    return m_hint == "forge-pack-xz";
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

QString Library::filename(OpSys system) const
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
    if (m_nativeClassifiers.contains(system))
    {
        nativeSpec.setClassifier(m_nativeClassifiers[system]);
    }
    else
    {
        nativeSpec.setClassifier("INVALID");
    }
    return nativeSpec.getFileName();
}

QString Library::displayName(OpSys system) const
{
    if(!m_displayname.isEmpty())
        return m_displayname;
    return filename(system);
}

QString Library::storageSuffix(OpSys system) const
{
    // non-native? use only the gradle specifier
    if (!isNative())
    {
        return m_name.toPath(m_filename);
    }

    // otherwise native, override classifiers. Mojang HACK!
    GradleSpecifier nativeSpec = m_name;
    if (m_nativeClassifiers.contains(system))
    {
        nativeSpec.setClassifier(m_nativeClassifiers[system]);
    }
    else
    {
        nativeSpec.setClassifier("INVALID");
    }
    return nativeSpec.toPath(m_filename);
}
