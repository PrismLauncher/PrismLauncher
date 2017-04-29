#include "FileSink.h"
#include <QFile>
#include <QFileInfo>
#include "Env.h"
#include "FileSystem.h"

namespace Net {

FileSink::FileSink(QString filename)
	:m_filename(filename)
{
	// nil
};

FileSink::~FileSink()
{
	// nil
};

Task::Status FileSink::init(QNetworkRequest& request)
{
	auto result = initCache(request);
	if(result != Task::Status::InProgress)
	{
		return result;
	}
	// create a new save file and open it for writing
	if (!FS::ensureFilePathExists(m_filename))
	{
		qCritical() << "Could not create folder for " + m_filename;
		return Task::Status::Failed;
	}
	wroteAnyData = false;
	m_output_file.reset(new QSaveFile(m_filename));
	if (!m_output_file->open(QIODevice::WriteOnly))
	{
		qCritical() << "Could not open " + m_filename + " for writing";
		return Task::Status::Failed;
	}

	if(initAllValidators(request))
		return Task::Status::InProgress;
	return Task::Status::Failed;
}

Task::Status FileSink::initCache(QNetworkRequest &)
{
	return Task::Status::InProgress;
}

Task::Status FileSink::write(QByteArray& data)
{
	if (!writeAllValidators(data) || m_output_file->write(data) != data.size())
	{
		qCritical() << "Failed writing into " + m_filename;
		m_output_file->cancelWriting();
		m_output_file.reset();
		wroteAnyData = false;
		return Task::Status::Failed;
	}
	wroteAnyData = true;
	return Task::Status::InProgress;
}

Task::Status FileSink::abort()
{
	m_output_file->cancelWriting();
	failAllValidators();
	return Task::Status::Failed;
}

Task::Status FileSink::finalize(QNetworkReply& reply)
{
	// if we wrote any data to the save file, we try to commit the data to the real file.
	if (wroteAnyData)
	{
		// ask validators for data consistency
		// we only do this for actual downloads, not 'your data is still the same' cache hits
		if(!finalizeAllValidators(reply))
			return Task::Status::Failed;
		// nothing went wrong...
		if (!m_output_file->commit())
		{
			qCritical() << "Failed to commit changes to " << m_filename;
			m_output_file->cancelWriting();
			return Task::Status::Failed;
		}
	}
	// then get rid of the save file
	m_output_file.reset();

	return finalizeCache(reply);
}

Task::Status FileSink::finalizeCache(QNetworkReply &)
{
	return Task::Status::Finished;
}

bool FileSink::hasLocalData()
{
	QFileInfo info(m_filename);
	return info.exists() && info.size() != 0;
}
}
