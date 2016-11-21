#pragma once
#include <QString>

namespace Sys
{
/**
 * Get operation system name and version.
 * @return os       A QString with the name and version of the operating system.
 */
QString getSystemInfo();

uint64_t getSystemRam();

bool isSystem64bit();

bool isCPU64bit();
}
