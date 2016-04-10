#include "MCEditTool.h"

#include <QDir>
#include <QProcess>
#include <QUrl>

#include "settings/SettingsObject.h"
#include "BaseInstance.h"
#include "minecraft/MinecraftInstance.h"

MCEditTool::MCEditTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
	: BaseDetachedTool(settings, instance, parent)
{
}

QString MCEditTool::getSave() const
{
	// FIXME: mixing logic and UI!!!!
	auto mcInstance = std::dynamic_pointer_cast<MinecraftInstance>(m_instance);
	if(!mcInstance)
	{
		return QString();
	}
	QDir saves(mcInstance->minecraftRoot() + "/saves");
	QStringList worlds = saves.entryList(QDir::Dirs | QDir::NoDotAndDotDot);
	QMutableListIterator<QString> it(worlds);
	while (it.hasNext())
	{
		it.next();
		if (!QDir(saves.absoluteFilePath(it.value())).exists("level.dat"))
		{
			it.remove();
		}
	}
	bool ok = true;
	/*
	const QString save = QInputDialog::getItem(QApplication::activeWindow(), tr("MCEdit"), tr("Choose which world to open:"),
		worlds, 0, false, &ok);
	if (ok)
	{
		return saves.absoluteFilePath(save);
	}
	*/
	return QString();
}

void MCEditTool::runImpl()
{
	const QString mceditPath = globalSettings->get("MCEditPath").toString();
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
	#ifdef Q_OS_LINUX
	if (mceditDir.exists("mcedit.py"))
	{
		program = mceditDir.absoluteFilePath("mcedit.py");
	}
	else if (mceditDir.exists("mcedit.sh"))
	{
		program = mceditDir.absoluteFilePath("mcedit.sh");
	}
	#elif defined(Q_OS_WIN32)
	if (mceditDir.exists("mcedit.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit.exe");
	}
	else if (mceditDir.exists("mcedit2.exe"))
	{
		program = mceditDir.absoluteFilePath("mcedit2.exe");
	}
	#endif
	/*
	if(program.size())
	{
		DesktopServices::openFile(program, save, mceditPath);
	}
	*/
#endif
}

void MCEditFactory::registerSettings(SettingsObjectPtr settings)
{
	settings->registerSetting("MCEditPath");
	globalSettings = settings;
}
BaseExternalTool *MCEditFactory::createTool(InstancePtr instance, QObject *parent)
{
	return new MCEditTool(globalSettings, instance, parent);
}
bool MCEditFactory::check(QString *error)
{
	return check(globalSettings->get("MCEditPath").toString(), error);
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
	if (!dir.exists("mcedit.sh") && !dir.exists("mcedit.py") && !dir.exists("mcedit.exe") && !dir.exists("Contents") && !dir.exists("mcedit2.exe"))
	{
		*error = QObject::tr("Path does not seem to be a MCEdit path");
		return false;
	}
	return true;
}
