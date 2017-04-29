#pragma once

#include "Sink.h"

namespace Net {
/*
 * Sink object for downloads that uses an external QByteArray it doesn't own as a target.
 */
class ByteArraySink : public Sink
{
public:
	ByteArraySink(QByteArray *output)
		:m_output(output)
	{
		// nil
	};

	virtual ~ByteArraySink()
	{
		// nil
	}

public:
	Task::Status init(QNetworkRequest & request) override
	{
		m_output->clear();
		if(initAllValidators(request))
			return Task::Status::InProgress;
		return Task::Status::Failed;
	};

	Task::Status write(QByteArray & data) override
	{
		m_output->append(data);
		if(writeAllValidators(data))
			return Task::Status::InProgress;
		return Task::Status::Failed;
	}

	Task::Status abort() override
	{
		m_output->clear();
		failAllValidators();
		return Task::Status::Failed;
	}

	Task::Status finalize(QNetworkReply &reply) override
	{
		if(finalizeAllValidators(reply))
			return Task::Status::Finished;
		return Task::Status::Failed;
	}

	bool hasLocalData() override
	{
		return false;
	}

private:
	QByteArray * m_output;
};
}
