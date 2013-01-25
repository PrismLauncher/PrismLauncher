#ifndef _BSPATCH_H
#define _BSPATCH_H

#ifdef __cplusplus
extern "C" {
#endif

enum BSPatchError
{
	ERR_CORRUPT_PATCH,
	ERR_OTHER,
	ERR_NONE,
};

/**
 * patch oldfile by using patchfile and write the output to newfile.
 *
 * Returns ERR_NONE if successful
 */
int bspatch(const char * oldfile, const char * newfile, const char * patchfile);

#ifdef __cplusplus
}
#endif


#endif
