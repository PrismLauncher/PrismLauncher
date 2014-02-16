#include "BaseExternalTool.h"

#include <QProcess>
#include <QDir>
#include <QInputDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "BaseInstance.h"
#include "MultiMC.h"

BaseExternalTool::BaseExternalTool(BaseInstance *instance, QObject *parent)
	: QObject(parent), m_instance(instance)
{
}

BaseExternalTool::~BaseExternalTool()
{
}

qint64 BaseExternalTool::pid(QProcess *process)
{
#ifdef Q_OS_WIN
	struct _PROCESS_INFORMATION *procinfo = process->pid();
	return procinfo->dwProcessId;
#else
	return process->pid();
#endif
}

QString BaseExternalTool::getSave() const
{
	QDir saves(m_instance->minecraftRoot() + "/saves");
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
	const QString save = QInputDialog::getItem(
		MMC->activeWindow(), tr("MCEdit"), tr("Choose which world to open:"),
		worlds, 0, false, &ok);
	if (ok)
	{
		return saves.absoluteFilePath(save);
	}
	return QString();
}


BaseDetachedTool::BaseDetachedTool(BaseInstance *instance, QObject *parent)
	: BaseExternalTool(instance, parent)
{

}

void BaseDetachedTool::run()
{
	runImpl();
}


BaseExternalToolFactory::~BaseExternalToolFactory()
{
}

BaseDetachedTool *BaseDetachedToolFactory::createDetachedTool(BaseInstance *instance, QObject *parent)
{
	return qobject_cast<BaseDetachedTool *>(createTool(instance, parent));
}
