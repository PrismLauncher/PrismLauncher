#include "hoedown/version.h"

void
hoedown_version(int *major, int *minor, int *revision)
{
    *major = HOEDOWN_VERSION_MAJOR;
    *minor = HOEDOWN_VERSION_MINOR;
    *revision = HOEDOWN_VERSION_REVISION;
}
