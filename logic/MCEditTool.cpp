#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QFileDialog>
#include <QInputDialog>

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
	const QDir saves = QDir(m_instance->minecraftRoot() + "/saves");
	bool ok = true;
	const QString save = QInputDialog::getItem(
		MMC->activeWindow(), tr("MCEdit"), tr("Choose which world to open:"),
		saves.entryList(QDir::Dirs | QDir::NoDotAndDotDot), 0, false, &ok);
	if (save.isEmpty() || !ok)
	{
		return;
	}
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
	QProcess::startDetached(program, QStringList() << saves.absoluteFilePath(save), mceditPath);
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
	if (!dir.exists("mcedit.py") && !dir.exists("mcedit.exe"))
	{
		*error = QObject::tr("Path does not contain mcedit.py");
		return false;
	}
	return true;
}
