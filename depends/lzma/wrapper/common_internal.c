/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 */

#include "common_internal.h"

static void *elzmaAlloc(void *p, size_t size)
{
	struct elzma_alloc_struct *as = (struct elzma_alloc_struct *)p;
	if (as->clientMallocFunc)
	{
		return as->clientMallocFunc(as->clientMallocContext, size);
	}
	return malloc(size);
}

static void elzmaFree(void *p, void *address)
{
	struct elzma_alloc_struct *as = (struct elzma_alloc_struct *)p;
	if (as->clientFreeFunc)
	{
		as->clientFreeFunc(as->clientMallocContext, address);
	}
	else
	{
		free(address);
	}
}

void init_alloc_struct(struct elzma_alloc_struct *as, elzma_malloc clientMallocFunc,
					   void *clientMallocContext, elzma_free clientFreeFunc,
					   void *clientFreeContext)
{
	as->Alloc = elzmaAlloc;
	as->Free = elzmaFree;
	as->clientMallocFunc = clientMallocFunc;
	as->clientMallocContext = clientMallocContext;
	as->clientFreeFunc = clientFreeFunc;
	as->clientFreeContext = clientFreeContext;
}
