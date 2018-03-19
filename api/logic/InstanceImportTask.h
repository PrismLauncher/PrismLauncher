#pragma once

#include "InstanceTask.h"
#include "multimc_logic_export.h"
#include "net/NetJob.h"
#include <QUrl>
#include <QFuture>
#include <QFutureWatcher>
#include "settings/SettingsObject.h"
#include "QObjectPtr.h"

class QuaZip;
class BaseInstanceProvider;
namespace Flame
{
	class FileResolvingTask;
}

class MULTIMC_LOGIC_EXPORT InstanceImportTask : public InstanceTask
{
	Q_OBJECT
public:
	explicit InstanceImportTask(const QUrl sourceUrl);

protected:
	//! Entry point for tasks.
	virtual void executeTask() override;

private:
	void processZipPack();
	void processMultiMC();
	void processFlame();

private slots:
	void downloadSucceeded();
	void downloadFailed(QString reason);
	void downloadProgressChanged(qint64 current, qint64 total);
	void extractFinished();
	void extractAborted();

private: /* data */
	NetJobPtr m_filesNetJob;
	shared_qobject_ptr<Flame::FileResolvingTask> m_modIdResolver;
	QUrl m_sourceUrl;
	QString m_archivePath;
	bool m_downloadRequired = false;
	std::unique_ptr<QuaZip> m_packZip;
	QFuture<QStringList> m_extractFuture;
	QFutureWatcher<QStringList> m_extractFutureWatcher;
	enum class ModpackType{
		Unknown,
		MultiMC,
		Flame
	} m_modpackType = ModpackType::Unknown;
};
