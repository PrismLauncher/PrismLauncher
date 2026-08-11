#include "BaseExternalTool.h"

#include <QDir>
#include <QProcess>

#ifdef Q_OS_WIN
#include <windows.h>
#endif

BaseExternalTool::BaseExternalTool(QObject* parent)
    : QObject(parent) {}

BaseExternalTool::~BaseExternalTool() {}

BaseExternalToolFactory::~BaseExternalToolFactory() {}
