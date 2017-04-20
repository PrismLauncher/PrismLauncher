#pragma once

#include "tasks/Task.h"
#include "net/NetJob.h"
#include "PackManifest.h"

#include "multimc_logic_export.h"

namespace Curse
{
class MULTIMC_LOGIC_EXPORT FileResolvingTask : public Task
{
	Q_OBJECT
public:
	explicit FileResolvingTask(QVector<Curse::File> &toProcess);
	const QVector<Curse::File> &getResults() const
	{
		return m_toProcess;
	}

protected:
	virtual void executeTask() override;

protected slots:
	void netJobFinished();

private: /* data */
	QVector<Curse::File> m_toProcess;
	QVector<QByteArray> results;
	NetJobPtr m_dljob;
};
}
