#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QFileDialog>

#include "settingsobject.h"
#include "logic/BaseInstance.h"
#include "MultiMC.h"

MCEditTool::MCEditTool(BaseInstance *instance, QObject *parent)
	: BaseDetachedTool(instance, parent)
{
}

void MCEditTool::runImpl()
{
	const QString mceditPath = MMC->settings()->get("MCEditPath").toString();
	const QString save = QFileDialog::getExistingDirectory(
		MMC->activeWindow(), tr("MCEdit"),
		QDir(m_instance->minecraftRoot()).absoluteFilePath("saves"));
	if (save.isEmpty())
	{
		return;
	}
	const QString program =
		QDir(mceditPath).absoluteFilePath("mcedit.py");
	QProcess::startDetached(program, QStringList() << save, mceditPath);
}

void MCEditFactory::registerSettings(SettingsObject *settings)
{
	settings->registerSetting("MCEditPath");
}
BaseExternalTool *MCEditFactory::createTool(BaseInstance *instance, QObject *parent)
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
	if (!dir.exists("mcedit.py"))
	{
		*error = QObject::tr("Path does not contain mcedit.py");
		return false;
	}
	return true;
}
