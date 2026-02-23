#include "BaseExternalTool.h"

#include <QDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "BaseInstance.h"

BaseExternalTool::BaseExternalTool(SettingsObject* settings, BaseInstance* instance, QObject* parent)
    : QObject(parent), m_instance(instance), globalSettings(settings)
{}

BaseExternalTool::~BaseExternalTool() = default;

BaseDetachedTool::BaseDetachedTool(SettingsObject* settings, BaseInstance* instance, QObject* parent)
    : BaseExternalTool(settings, instance, parent)
{}

void BaseDetachedTool::run()
{
    runImpl();
}

BaseExternalToolFactory::~BaseExternalToolFactory() = default;

BaseDetachedTool* BaseDetachedToolFactory::createDetachedTool(BaseInstance* instance, QObject* parent)
{
    return qobject_cast<BaseDetachedTool*>(createTool(instance, parent));
}
