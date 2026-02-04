#pragma once

#include <cstdint>

#include <QString>

namespace SysInfo {
QString currentSystem();
QString useQTForArch();
QString getSupportedJavaArchitecture();
/**
 * @return Total system memory in mebibytes, or 0 if it could not be determined.
 */
uint64_t getSystemRamMiB();
int suitableMaxMem();
}  // namespace SysInfo
