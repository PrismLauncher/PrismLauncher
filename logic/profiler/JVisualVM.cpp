#include "JVisualVM.h"

#include <QDir>
#include <QStandardPaths>

#include "settingsobject.h"
#include "logic/MinecraftProcess.h"
#include "logic/OneSixInstance.h"

JVisualVM::JVisualVM(OneSixInstance *instance, QObject *parent) : BaseProfiler(instance, parent)
{
}

void JVisualVM::beginProfilingImpl(MinecraftProcess *process)
{
	QProcess *profiler = new QProcess(this);
	profiler->setArguments(QStringList() << "--openpid" << QString::number(pid(process)));
	profiler->setProgram("jvisualvm");
	connect(profiler, &QProcess::started, [this]()
	{ emit readyToLaunch(tr("JVisualVM started")); });
	connect(profiler, SIGNAL(finished(int)), profiler, SLOT(deleteLater()));
	profiler->start();
}

void JVisualVMFactory::registerSettings(SettingsObject *settings)
{
	settings->registerSetting("JVisualVMPath");
}

BaseProfiler *JVisualVMFactory::createProfiler(OneSixInstance *instance, QObject *parent)
{
	return new JVisualVM(instance, parent);
}

bool JVisualVMFactory::check(const QString &path, QString *error)
{
	QString resolved = QStandardPaths::findExecutable(path);
	if (resolved.isEmpty() && !QDir::isAbsolutePath(path))
	{
		*error = QObject::tr("Invalid path to JVisualVM");
		return false;
	}
	return true;
}
