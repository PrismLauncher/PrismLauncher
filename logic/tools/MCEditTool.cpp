#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QDesktopServices>
#include <QUrl>

#include "logic/settings/SettingsObject.h"
#include "logic/BaseInstance.h"
#include "MultiMC.h"

MCEditTool::MCEditTool(InstancePtr instance, QObject *parent)
	: BaseDetachedTool(instance, parent)
{
}

void MCEditTool::runImpl()
{
	const QString mceditPath = MMC->settings()->get("MCEditPath").toString();
	const QString save = getSave();
	if (save.isNull())
	{
		return;
	}
#ifdef Q_OS_OSX
	QProcess *process = new QProcess();
	connect(process, SIGNAL(finished(int, QProcess::ExitStatus)), process, SLOT(deleteLater()));
	process->setProgram(mceditPath);
	process->setArguments(QStringList() << save);
	process->start();
#else
	QDir mceditDir(mceditPath);
	QString program;
	if (mceditDir.exists("mcedit.py"))
	{
		program = mceditDir.absoluteFilePath("mcedit.py");
	}
	else if (mceditDir.exists("mcedit.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit.exe");
	}
	QProcess::startDetached(program, QStringList() << save, mceditPath);
#endif
}

void MCEditFactory::registerSettings(std::shared_ptr<SettingsObject> settings)
{
	settings->registerSetting("MCEditPath");
}
BaseExternalTool *MCEditFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new MCEditTool(instance, parent);
}
bool MCEditFactory::check(QString *error)
{
	return check(MMC->settings()->get("MCEditPath").toString(), error);
}
bool MCEditFactory::check(const QString &path, QString *error)
{
	if (path.isEmpty())
	{
		*error = QObject::tr("Path is empty");
		return false;
	}
	const QDir dir(path);
	if (!dir.exists())
	{
		*error = QObject::tr("Path does not exist");
		return false;
	}
	if (!dir.exists("mcedit.py") && !dir.exists("mcedit.exe") && !dir.exists("Contents"))
	{
		*error = QObject::tr("Path does not seem to be a MCEdit path");
		return false;
	}
	return true;
}
