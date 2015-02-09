#include "BaseExternalTool.h"

#include <QProcess>
#include <QDir>
#include <QInputDialog>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "BaseInstance.h"

BaseExternalTool::BaseExternalTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
	: QObject(parent), m_instance(instance), globalSettings(settings)
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

BaseDetachedTool::BaseDetachedTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
	: BaseExternalTool(settings, instance, parent)
{

}

void BaseDetachedTool::run()
{
	runImpl();
}


BaseExternalToolFactory::~BaseExternalToolFactory()
{
}

BaseDetachedTool *BaseDetachedToolFactory::createDetachedTool(InstancePtr instance,
															  QObject *parent)
{
	return qobject_cast<BaseDetachedTool *>(createTool(instance, parent));
}
