#include "BaseExternalTool.h"

#include <QProcess>
#include <QDir>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "BaseInstance.h"

BaseExternalTool::BaseExternalTool(SettingsObjectPtr settings, InstancePtr instance, QObject *parent)
    : QObject(parent), hello_developer_i_am_here_to_kindly_tell_you_that_the_following_variable_is_actually_a_member_instance(instance), globalSettings(settings)
{
}

BaseExternalTool::~BaseExternalTool()
{
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
