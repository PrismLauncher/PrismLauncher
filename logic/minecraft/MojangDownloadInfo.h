#pragma once
#include <QString>
#include <QMap>
#include <memory>

struct MojangDownloadInfo
{
	// types
	typedef std::shared_ptr<MojangDownloadInfo> Ptr;

	// data
	/// Local filesystem path. WARNING: not used, only here so we can pass through mojang files unmolested!
	QString path;
	/// absolute URL of this file
	QString url;
	/// sha-1 checksum of the file
	QString sha1;
	/// size of the file in bytes
	int size;
};



struct MojangLibraryDownloadInfo
{
	MojangLibraryDownloadInfo(MojangDownloadInfo::Ptr artifact): artifact(artifact) {};
	MojangLibraryDownloadInfo() {};

	// types
	typedef std::shared_ptr<MojangLibraryDownloadInfo> Ptr;

	// methods
	MojangDownloadInfo *getDownloadInfo(QString classifier)
	{
		if (classifier.isNull())
		{
			return artifact.get();
		}
		
		return classifiers[classifier].get();
	}

	// data
	MojangDownloadInfo::Ptr artifact;
	QMap<QString, MojangDownloadInfo::Ptr> classifiers;
};



struct MojangAssetIndexInfo : public MojangDownloadInfo
{
	// types
	typedef std::shared_ptr<MojangAssetIndexInfo> Ptr;

	// methods
	MojangAssetIndexInfo()
	{
	}

	MojangAssetIndexInfo(QString id)
	{
		this->id = id;
		url = "https://s3.amazonaws.com/Minecraft.Download/indexes/" + id + ".json";
		known = false;
	}

	// data
	int totalSize;
	QString id;
	bool known = true;
};
