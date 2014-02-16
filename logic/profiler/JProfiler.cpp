#include "JProfiler.h"

#include <QDir>
#include <QMessageBox>

#include "settingsobject.h"
#include "logic/MinecraftProcess.h"
#include "logic/BaseInstance.h"
#include "MultiMC.h"

JProfiler::JProfiler(BaseInstance *instance, QObject *parent) : BaseProfiler(instance, parent)
{
}

void JProfiler::beginProfilingImpl(MinecraftProcess *process)
{
	int port = MMC->settings()->get("JProfilerPort").toInt();
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "-d" << QString::number(pid(process)) << "--gui"
										 << "-p" << QString::number(port));
	profiler->setProgram(QDir(MMC->settings()->get("JProfilerPath").toString())
							 .absoluteFilePath("bin/jpenable"));
	connect(profiler, &QProcess::started, [this, port]()
	{ emit readyToLaunch(tr("Listening on port: %1").arg(port)); });
	connect(profiler,
			static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			[this](int exit, QProcess::ExitStatus status)
	{
		if (exit != 0 || status == QProcess::CrashExit)
		{
			emit abortLaunch(tr("Profiler aborted"));
		}
		m_profilerProcess->deleteLater();
	});
	profiler->start();
}

void JProfilerFactory::registerSettings(SettingsObject *settings)
{
	settings->registerSetting("JProfilerPath");
	settings->registerSetting("JProfilerPort", 42042);
}

BaseProfiler *JProfilerFactory::createProfiler(BaseInstance *instance, QObject *parent)
{
	return new JProfiler(instance, parent);
}

bool JProfilerFactory::check(QString *error)
{
	return check(MMC->settings()->get("JProfilerPath").toString(), error);
}

bool JProfilerFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Empty path");
		return false;
	}
	QDir dir(path);
	if (!dir.exists())
	{
		*error = QObject::tr("Path does not exist");
		return false;
	}
	if (!dir.exists("bin") || !dir.exists("bin/jprofiler") || !dir.exists("bin/agent.jar"))
	{
		*error = QObject::tr("Invalid JProfiler install");
		return false;
	}
	return true;
}
