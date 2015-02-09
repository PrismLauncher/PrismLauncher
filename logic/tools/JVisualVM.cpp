#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "settings/SettingsObject.h"
#include "BaseProcess.h"
#include "BaseInstance.h"

JVisualVM::JVisualVM(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
	: BaseProfiler(settings, instance, parent)
{
}

void JVisualVM::beginProfilingImpl(BaseProcess *process)
{
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "--openpid" << QString::number(pid(process)));
	profiler->setProgram(globalSettings->get("JVisualVMPath").toString());
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

void JVisualVMFactory::registerSettings(SettingsObjectPtr settings)
{
	QString defaultValue = QStandardPaths::findExecutable("jvisualvm");
	if (defaultValue.isNull())
	{
		defaultValue = QStandardPaths::findExecutable("visualvm");
	}
	settings->registerSetting("JVisualVMPath", defaultValue);
	globalSettings = settings;
}

BaseExternalTool *JVisualVMFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new JVisualVM(globalSettings, instance, parent);
}

bool JVisualVMFactory::check(QString *error)
{
	return check(globalSettings->get("JVisualVMPath").toString(), error);
}

bool JVisualVMFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Empty path");
		return false;
	}
	if (!QDir::isAbsolutePath(path) || !QFileInfo(path).isExecutable() || !path.contains("visualvm"))
	{
		*error = QObject::tr("Invalid path to JVisualVM");
		return false;
	}
	return true;
}
