#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "settingsobject.h"
#include "logic/MinecraftProcess.h"
#include "logic/BaseInstance.h"
#include "MultiMC.h"

JVisualVM::JVisualVM(InstancePtr instance, QObject *parent) : BaseProfiler(instance, parent)
{
}

void JVisualVM::beginProfilingImpl(MinecraftProcess *process)
{
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "--openpid" << QString::number(pid(process)));
	profiler->setProgram(MMC->settings()->get("JVisualVMPath").toString());
	connect(profiler, &QProcess::started, [this]()
	{ emit readyToLaunch(tr("JVisualVM started")); });
	connect(profiler,
			static_cast<void (QProcess::*)(int, QProcess::ExitStatus)>(&QProcess::finished),
			[this](int exit, QProcess::ExitStatus status)
	{
		if (exit != 0 || status == QProcess::CrashExit)
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

void JVisualVMFactory::registerSettings(SettingsObject *settings)
{
	QString defaultValue = QStandardPaths::findExecutable("jvisualvm");
	if (defaultValue.isNull())
	{
		defaultValue = QStandardPaths::findExecutable("visualvm");
	}
	settings->registerSetting("JVisualVMPath", defaultValue);
}

BaseExternalTool *JVisualVMFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new JVisualVM(instance, parent);
}

bool JVisualVMFactory::check(QString *error)
{
	return check(MMC->settings()->get("JVisualVMPath").toString(), error);
}

bool JVisualVMFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Empty path");
		return false;
	}
	QString resolved = QStandardPaths::findExecutable(path);
	if (resolved.isEmpty() && !QDir::isAbsolutePath(path))
	{
		*error = QObject::tr("Invalid path to JVisualVM");
		return false;
	}
	return true;
}
