#pragma once
#include <QString>

namespace Sys
{
struct KernelInfo
{
	QString kernelName;
	QString kernelVersion;
};

KernelInfo getKernelInfo();

uint64_t getSystemRam();

bool isSystem64bit();

bool isCPU64bit();
}
