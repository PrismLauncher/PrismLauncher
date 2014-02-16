#include "BaseExternalTool.h"

#include <QProcess>
#ifdef Q_OS_WIN
#include <windows.h>
#endif

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
