#ifndef __EASYLZMA_LZIP_HEADER__
#define __EASYLZMA_LZIP_HEADER__

#include "common_internal.h"

/* lzip file format documented here:
 * http://download.savannah.gnu.org/releases-noredirect/lzip/manual/ */

void initializeLZIPFormatHandler(struct elzma_format_handler *hand);

#endif
