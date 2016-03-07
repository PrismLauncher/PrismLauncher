#pragma once
#include <QString>
#include <memory>

class MojangDownloadInfo
{
	friend class MojangVersionFormat;
public:
	QString getUrl()
	{
		return m_url;
	}

	QString getSha1()
	{
		return m_sha1;
	}

	int getSize()
	{
		return m_size;
	}

protected:
	/// Local filesystem path. WARNING: not used, only here so we can pass through mojang files unmolested!
	QString m_path;
	/// absolute URL of this file
	QString m_url;
	/// sha-1 checksum of the file
	QString m_sha1;
	/// size of the file in bytes
	int m_size;
};

typedef std::shared_ptr<MojangDownloadInfo> MojangDownloadInfoPtr;

class MojangLibraryDownloadInfo
{
	friend class MojangVersionFormat;
public:
	MojangDownloadInfo *getDownloadInfo(QString classifier)
	{
		if (classifier.isNull())
		{
			return artifact.get();
		}
		
		return classifiers[classifier].get();
	}
private:
	MojangDownloadInfoPtr artifact;
	QMap<QString, MojangDownloadInfoPtr> classifiers;
};

typedef std::shared_ptr<MojangLibraryDownloadInfo> MojangLibraryDownloadInfoPtr;

class MojangAssetIndexInfo : public MojangDownloadInfo
{
	friend class MojangVersionFormat;
public:
	MojangAssetIndexInfo()
	{
	}

	MojangAssetIndexInfo(QString id)
	{
		m_id = id;
		m_url = "https://s3.amazonaws.com/Minecraft.Download/indexes/" + id + ".json";
		m_known = false;
	}

	int getTotalSize()
	{
		return m_totalSize;
	}

	QString getId()
	{
		return m_id;
	}

	bool sizeAndHashKnown()
	{
		return m_known;
	}

protected:
	int m_totalSize;
	QString m_id;
	bool m_known = true;
};
typedef std::shared_ptr<MojangAssetIndexInfo> MojangAssetIndexInfoPtr;
