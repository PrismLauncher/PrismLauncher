#pragma once
#include <QString>

namespace Sys
{
const uint64_t megabyte = 1024ull * 1024ull;
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
