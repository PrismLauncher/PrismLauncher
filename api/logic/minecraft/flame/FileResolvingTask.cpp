#include "FileResolvingTask.h"
#include "Json.h"

const char * metabase = "https://cursemeta.dries007.net";

Flame::FileResolvingTask::FileResolvingTask(Flame::Manifest& toProcess)
	: m_toProcess(toProcess)
{
}

void Flame::FileResolvingTask::executeTask()
{
	setStatus(tr("Resolving mod IDs..."));
	setProgress(0, m_toProcess.files.size());
	m_dljob.reset(new NetJob("Mod id resolver"));
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
	connect(m_dljob.get(), &NetJob::finished, this, &Flame::FileResolvingTask::netJobFinished);
	m_dljob->start();
}

void Flame::FileResolvingTask::netJobFinished()
{
	bool failed = false;
	int index = 0;
	for(auto & bytes: results)
	{
		try
		{
			auto doc = Json::requireDocument(bytes);
			auto obj = Json::requireObject(doc);
			auto & out = m_toProcess.files[index];
			// result code signifies true failure.
			if(obj.contains("code"))
			{
				qCritical() << "Resolving of" << out.projectId << out.fileId << "failed because of a negative result:";
				qCritical() << bytes;
				failed = true;
				continue;
			}
			out.fileName = Json::requireString(obj, "FileNameOnDisk");
			out.url = Json::requireString(obj, "DownloadURL");
			// This is a piece of a Flame project JSON pulled out into the file metadata (here) for convenience
			// It is also optional
			QJsonObject projObj = Json::ensureObject(obj, "_Project", {});
			if(!projObj.isEmpty())
			{
				QString strType = Json::ensureString(projObj, "PackageType", "mod").toLower();
				if(strType == "singlefile")
				{
					out.type = File::Type::SingleFile;
				}
				else if(strType == "ctoc")
				{
					out.type = File::Type::Ctoc;
				}
				else if(strType == "cmod2")
				{
					out.type = File::Type::Cmod2;
				}
				else if(strType == "mod")
				{
					out.type = File::Type::Mod;
				}
				else if(strType == "folder")
				{
					out.type = File::Type::Folder;
				}
				else if(strType == "modpack")
				{
					out.type = File::Type::Modpack;
				}
				else
				{
					qCritical() << "Resolving of" << out.projectId << out.fileId << "failed because of unknown file type:" << strType;
					out.type = File::Type::Unknown;
					failed = true;
					continue;
				}
				out.targetFolder = Json::ensureString(projObj, "Path", "mods");
			}
			out.resolved = true;
		}
		catch(JSONValidationError & e)
		{
			auto & out = m_toProcess.files[index];
			qCritical() << "Resolving of" << out.projectId << out.fileId << "failed because of a parsing error:";
			qCritical() << e.cause();
			qCritical() << "JSON:";
			qCritical() << bytes;
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
		emitFailed(tr("Some mod ID resolving tasks failed."));
	}
}
