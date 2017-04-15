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
	bool isLocal = (hint() == "local");
	auto actualPath = [&](QString relPath)
	{
		QFileInfo out(FS::PathCombine(storagePrefix(), relPath));
		if(isLocal && !overridePath.isEmpty())
		{
			QString fileName = out.fileName();
			auto fullPath = FS::PathCombine(overridePath, fileName);
			qDebug() << fullPath;
			QFileInfo fileinfo(fullPath);
			if(fileinfo.exists())
			{
				return fileinfo.absoluteFilePath();
			}
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

QList< std::shared_ptr< NetAction > > Library::getDownloads(OpSys system, class HttpMetaCache* cache,
															QStringList& failedFiles, const QString & overridePath) const
{
	QList<NetActionPtr> out;
	bool isAlwaysStale = (hint() == "always-stale");
	bool isLocal = (hint() == "local");
	bool isForge = (hint() == "forge-pack-xz");

	auto add_download = [&](QString storage, QString url, QString sha1 = QString())
	{
		auto entry = cache->resolveEntry("libraries", storage);
		if(isAlwaysStale)
		{
			entry->setStale(true);
		}
		if (!entry->isStale())
			return true;
		if(isLocal)
		{
			if(!overridePath.isEmpty())
			{
				QString fileName;
				int position = storage.lastIndexOf('/');
				if(position == -1)
				{
					fileName = storage;
				}
				else
				{
					fileName = storage.mid(position);
				}
				auto fullPath = FS::PathCombine(overridePath, fileName);
				QFileInfo fileinfo(fullPath);
				if(fileinfo.exists())
				{
					return true;
				}
			}
			QFileInfo fileinfo(entry->getFullPath());
			if(!fileinfo.exists())
			{
				failedFiles.append(entry->getFullPath());
				return false;
			}
			return true;
		}
		Net::Download::Options options;
		if(isAlwaysStale)
		{
			options |= Net::Download::Option::AcceptLocalFiles;
		}
		if (isForge)
		{
			out.append(ForgeXzDownload::make(storage, entry));
		}
		else
		{
			if(sha1.size())
			{
				auto rawSha1 = QByteArray::fromHex(sha1.toLatin1());
				auto dl = Net::Download::makeCached(url, entry, options);
				dl->addValidator(new Net::ChecksumValidator(QCryptographicHash::Sha1, rawSha1));
				out.append(dl);
			}

			else
				out.append(Net::Download::makeCached(url, entry, options));
		}
		return true;
	};

	QString raw_storage = storageSuffix(system);
	if(m_mojangDownloads)
	{
		if(m_mojangDownloads->artifact)
		{
			auto artifact = m_mojangDownloads->artifact;
			add_download(raw_storage, artifact->url, artifact->sha1);
		}
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
	}
	else
	{
		auto raw_dl = [&](){
			if (!m_absoluteURL.isEmpty())
			{
				return m_absoluteURL;
			}

			if (m_repositoryURL.isEmpty())
			{
				return QString("https://" + URLConstants::LIBRARY_BASE) + raw_storage;
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
			add_download(cooked_storage.replace("${arch}", "32"), cooked_dl.replace("${arch}", "32"));
			cooked_storage = raw_storage;
			cooked_dl = raw_dl;
			add_download(cooked_storage.replace("${arch}", "64"), cooked_dl.replace("${arch}", "64"));
		}
		else
		{
			add_download(raw_storage, raw_dl);
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

QString Library::storageSuffix(OpSys system) const
{
	// non-native? use only the gradle specifier
	if (!isNative())
	{
		return m_name.toPath();
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
	return nativeSpec.toPath();
}
