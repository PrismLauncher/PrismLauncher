#include "JProfiler.h"

#include <QDir>
#include <QMessageBox>

#include "settings/SettingsObject.h"
#include "BaseProcess.h"
#include "BaseInstance.h"

JProfiler::JProfiler(SettingsObjectPtr settings, InstancePtr instance,
					 QObject *parent)
	: BaseProfiler(settings, instance, parent)
{
}

void JProfiler::beginProfilingImpl(BaseProcess *process)
{
	int port = globalSettings->get("JProfilerPort").toInt();
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "-d" << QString::number(pid(process)) << "--gui"
										 << "-p" << QString::number(port));
	profiler->setProgram(QDir(globalSettings->get("JProfilerPath").toString())
#ifdef Q_OS_WIN
							 .absoluteFilePath("bin/jpenable.exe"));
#else
							 .absoluteFilePath("bin/jpenable"));
#endif
	connect(profiler, &QProcess::started, [this, port]()
	{ emit readyToLaunch(tr("Listening on port: %1").arg(port)); });
	connect(profiler,
			static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			[this](int exit, QProcess::ExitStatus status)
	{
		if (status == QProcess::CrashExit)
		{
			emit abortLaunch(tr("Profiler aborted"));
		}
		if (m_profilerProcess)
		{
			m_profilerProcess->deleteLater();
			m_profilerProcess = 0;
		}
	});
	profiler->start();
	m_profilerProcess = profiler;
}

void JProfilerFactory::registerSettings(SettingsObjectPtr settings)
{
	settings->registerSetting("JProfilerPath");
	settings->registerSetting("JProfilerPort", 42042);
	globalSettings = settings;
}

BaseExternalTool *JProfilerFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new JProfiler(globalSettings, instance, parent);
}

bool JProfilerFactory::check(QString *error)
{
	return check(globalSettings->get("JProfilerPath").toString(), error);
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
	if (!dir.exists("bin") || !(dir.exists("bin/jprofiler") || dir.exists("bin/jprofiler.exe")) || !dir.exists("bin/agent.jar"))
	{
		*error = QObject::tr("Invalid JProfiler install");
		return false;
	}
	return true;
}
