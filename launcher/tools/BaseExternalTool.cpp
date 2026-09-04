#include "BaseExternalTool.h"

#include <QDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

#include "minecraft/MinecraftInstance.h"

BaseExternalTool::BaseExternalTool(SettingsObject* settings, MinecraftInstance* instance, QObject* parent)
    : QObject(parent), m_instance(instance), m_globalSettings(settings)
{}

BaseExternalTool::~BaseExternalTool() = default;

BaseDetachedTool::BaseDetachedTool(SettingsObject* settings, MinecraftInstance* instance, QObject* parent)
    : BaseExternalTool(settings, instance, parent)
{}

void BaseDetachedTool::run()
{
    runImpl();
}

BaseExternalToolFactory::~BaseExternalToolFactory() = default;

BaseDetachedTool* BaseDetachedToolFactory::createDetachedTool(MinecraftInstance* instance, QObject* parent)
{
    return qobject_cast<BaseDetachedTool*>(createTool(instance, parent));
}
