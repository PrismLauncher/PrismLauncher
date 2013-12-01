
#include "GoUpdate.h"

#include "config.h"
#include "logger/QsLog.h"

GoUpdate::GoUpdate()
{
	currentBuildIndex = VERSION_BUILD;
	builderName = VERSION_BUILD_TYPE;
	repoUrlBase = VERSION_REPO;
}

void GoUpdate::updateCheckFailed()
{
	// TODO: log errors better
	QLOG_ERROR() << "Update check failed for reasons unknown.";
}

void GoUpdate::updateCheckFinished()
{
	QJsonParseError jsonError;
	QByteArray data;
	{
		ByteArrayDownloadPtr dl =
			std::dynamic_pointer_cast<ByteArrayDownload>(index_job->first());
		data = dl->m_data;
		index_job.reset();
	}

	QJsonDocument jsonDoc = QJsonDocument::fromJson(data, &jsonError);
	if (jsonError.error != QJsonParseError::NoError || !jsonDoc.isObject())
	{
		return;
	}

	QVariant doc = jsonDoc.toVariant();
	auto stuff = doc.toMap();

	// check api version (or later, branch?)
	int ApiVersion = stuff["ApiVersion"].toInt();
	if (ApiVersion != 0)
		return;

	// parse and store the channel list
	auto parsedChannels = stuff["Channels"].toList();
	for (auto channel : parsedChannels)
	{
		auto chanMap = channel.toMap();
		channels.append({chanMap["Id"].toString(), chanMap["Name"].toString(),
						 chanMap["CurrentVersion"].toInt()});
	}

	// parse and store the version list
	auto parsedVersions = stuff["Versions"].toList();
	for (auto version : parsedVersions)
	{
		auto verMap = version.toMap();
		int versionId = verMap["Id"].toInt();
		versions.append({versionId, verMap["Name"].toString()});
		if (currentBuildIndex < versionId)
		{
			newBuildIndex = versionId;
		}
	}

	if (newBuildIndex != -1)
	{
		QLOG_INFO() << "Update is available.";
		emit updateAvailable();
	}
	else
	{
		QLOG_INFO() << "Update check finished.";
	}
}

void GoUpdate::checkForUpdate()
{
	if (repoUrlBase == "invalid")
	{
		return;
	}

	auto job = new NetJob("Assets index");
	job->addNetAction(
		ByteArrayDownload::make(QUrl(repoUrlBase + "/" + VERSION_BRANCH + "/index.json")));
	connect(job, SIGNAL(succeeded()), SLOT(updateCheckFinished()));
	connect(job, SIGNAL(failed()), SLOT(updateCheckFailed()));
	index_job.reset(job);
	job->start();
}

/*
<Forkk> files.multimc.org/lin64/
<manmaed> Hi Forkkie
<Forkk> files.multimc.org/win32/
<Forkk> files.multimc.org/lin32/
*/
