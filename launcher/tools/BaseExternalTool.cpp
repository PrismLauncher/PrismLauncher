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

BaseExternalTool::~BaseExternalTool() {}

BaseExternalToolFactory::~BaseExternalToolFactory() {}
