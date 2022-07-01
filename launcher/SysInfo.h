#include <QString>
#include "settings/SettingsObject.h"
#ifdef Q_OS_MACOS
#include <sys/sysctl.h>
#endif

namespace SysInfo {
QString currentSystem();
QString currentArch(const SettingsObjectPtr& settingsObj);
QString runCheckerForArch(const SettingsObjectPtr& settingsObj);
QString useQTForArch();
}

