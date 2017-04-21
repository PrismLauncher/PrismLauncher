#include "FileResolvingTask.h"
#include "Json.h"

const char * metabase = "https://cursemeta.dries007.net";

Curse::FileResolvingTask::FileResolvingTask(Curse::Manifest& toProcess)
	: m_toProcess(toProcess)
{
}

void Curse::FileResolvingTask::executeTask()
{
	setStatus(tr("Resolving curse mod IDs..."));
	setProgress(0, m_toProcess.files.size());
	m_dljob.reset(new NetJob("Curse file resolver"));
	results.resize(m_toProcess.files.size());
	int index = 0;
	for(auto & file: m_toProcess.files)
	{
		auto projectIdStr = QString::number(file.projectId);
		auto fileIdStr = QString::number(file.fileId);
		QString metaurl = QString("%1/%2/%3.json").arg(metabase, projectIdStr, fileIdStr);
		auto dl = Net::Download::makeByteArray(QUrl(metaurl), &results[index]);
		m_dljob->addNetAction(dl);
		index ++;
	}
	connect(m_dljob.get(), &NetJob::finished, this, &Curse::FileResolvingTask::netJobFinished);
	m_dljob->start();
}

void Curse::FileResolvingTask::netJobFinished()
{
	bool failed = false;
	int index = 0;
	for(auto & bytes: results)
	{
		try
		{
			auto doc = Json::requireDocument(bytes);
			auto obj = Json::requireObject(doc);
			// result code signifies true failure.
			if(obj.contains("code"))
			{
				failed = true;
				continue;
			}
			auto & out = m_toProcess.files[index];
			out.fileName = Json::requireString(obj, "FileNameOnDisk");
			out.url = Json::requireString(obj, "DownloadURL");
			out.resolved = true;
		}
		catch(JSONValidationError & e)
		{
			failed = true;
		}
		index++;
	}
	if(!failed)
	{
		emitSucceeded();
	}
	else
	{
		emitFailed(tr("Some curse ID resolving tasks failed."));
	}
}
