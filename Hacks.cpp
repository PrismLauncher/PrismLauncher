#include "MultiMC.h"

#ifdef Q_OS_WIN32
extern "C"
{
__declspec(dllexport) uint32_t NvOptimusEnablement = 0x00000001;
}
#endif
