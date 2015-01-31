#include "logic/Env.h"
#include "ForgeMirrors.h"
#include "logger/QsLog.h"
#include <algorithm>
#include <random>

ForgeMirrors::ForgeMirrors(QList<ForgeXzDownloadPtr> &libs, NetJobPtr parent_job,
						   QString mirrorlist)
{
	m_libs = libs;
	m_parent_job = parent_job;
	m_url = QUrl(mirrorlist);
	m_status = Job_NotStarted;
}

void ForgeMirrors::start()
{
	QLOG_INFO() << "Downloading " << m_url.toString();
	QNetworkRequest request(m_url);
	request.setHeader(QNetworkRequest::UserAgentHeader, "MultiMC/5.0 (Uncached)");
	auto worker = ENV.qnam();
	QNetworkReply *rep = worker->get(request);

	m_reply = std::shared_ptr<QNetworkReply>(rep);
	connect(rep, SIGNAL(downloadProgress(qint64, qint64)),
			SLOT(downloadProgress(qint64, qint64)));
	connect(rep, SIGNAL(finished()), SLOT(downloadFinished()));
	connect(rep, SIGNAL(error(QNetworkReply::NetworkError)),
			SLOT(downloadError(QNetworkReply::NetworkError)));
	connect(rep, SIGNAL(readyRead()), SLOT(downloadReadyRead()));
}

void ForgeMirrors::downloadError(QNetworkReply::NetworkError error)
{
	// error happened during download.
	QLOG_ERROR() << "Error getting URL:" << m_url.toString().toLocal8Bit()
				 << "Network error: " << error;
	m_status = Job_Failed;
}

void ForgeMirrors::downloadFinished()
{
	// if the download succeeded
	if (m_status != Job_Failed)
	{
		// nothing went wrong... ?
		parseMirrorList();
		return;
	}
	// else the download failed, we use a fixed list
	else
	{
		m_status = Job_Finished;
		m_reply.reset();
		deferToFixedList();
		return;
	}
}

void ForgeMirrors::deferToFixedList()
{
	m_mirrors.clear();
	m_mirrors.append(
		{"Minecraft Forge",				  "http://files.minecraftforge.net/forge_logo.png",
		 "http://files.minecraftforge.net/", "http://files.minecraftforge.net/maven/"});
	m_mirrors.append({"Creeper Host",
					  "http://files.minecraftforge.net/forge_logo.png",
					  "https://www.creeperhost.net/link.php?id=1",
					  "http://new.creeperrepo.net/forge/maven/"});
	injectDownloads();
	emit succeeded(m_index_within_job);
}

void ForgeMirrors::parseMirrorList()
{
	m_status = Job_Finished;
	auto data = m_reply->readAll();
	m_reply.reset();
	auto dataLines = data.split('\n');
	for(auto line: dataLines)
	{
		auto elements = line.split('!');
		if (elements.size() == 4)
		{
			m_mirrors.append({elements[0],elements[1],elements[2],elements[3]});
		}
	}
	if(!m_mirrors.size())
		deferToFixedList();
	injectDownloads();
	emit succeeded(m_index_within_job);
}

void ForgeMirrors::injectDownloads()
{
	// shuffle the mirrors randomly
	std::random_device rd;
	std::mt19937 rng(rd());
	std::shuffle(m_mirrors.begin(), m_mirrors.end(), rng);

	// tell parent to download the libs
	for(auto lib: m_libs)
	{
		lib->setMirrors(m_mirrors);
		m_parent_job->addNetAction(lib);
	}
}

void ForgeMirrors::downloadProgress(qint64 bytesReceived, qint64 bytesTotal)
{
	m_total_progress = bytesTotal;
	m_progress = bytesReceived;
	emit progress(m_index_within_job, bytesReceived, bytesTotal);
}

void ForgeMirrors::downloadReadyRead()
{
}
