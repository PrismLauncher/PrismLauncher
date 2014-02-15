#include "JProfiler.h"

#include <QDir>
#include <QMessageBox>

#include "settingsobject.h"
#include "logic/MinecraftProcess.h"
#include "logic/OneSixInstance.h"
#include "MultiMC.h"

JProfiler::JProfiler(OneSixInstance *instance, QObject *parent) : BaseProfiler(instance, parent)
{
}

void JProfiler::beginProfilingImpl(MinecraftProcess *process)
{
	int port = MMC->settings()->get("JProfilerPort").toInt();
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "-d" << QString::number(process->pid()) << "--gui"
										 << "-p" << QString::number(port));
	profiler->setProgram(QDir(MMC->settings()->get("JProfilerPath").toString())
							 .absoluteFilePath("bin/jpenable"));
	connect(profiler, &QProcess::started, [this, port]()
	{ emit readyToLaunch(tr("Listening on port: %1").arg(port)); });
	connect(profiler, SIGNAL(finished(int)), profiler, SLOT(deleteLater()));
	profiler->start();
	QMessageBox::information(0, tr("JProfiler"),
							 tr("JProfiler started and listening on port %1").arg(port));
}

void JProfilerFactory::registerSettings(SettingsObject *settings)
{
	settings->registerSetting("JProfilerPath");
	settings->registerSetting("JProfilerPort", 42042);
}

BaseProfiler *JProfilerFactory::createProfiler(OneSixInstance *instance, QObject *parent)
{
	return new JProfiler(instance, parent);
}

bool JProfilerFactory::check(const QString &path, QString *error)
{
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
