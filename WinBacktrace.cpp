/* Copyright 2014 MultiMC Contributors
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

// CAUTION:
// This file contains all manner of hackery and insanity.
// I will not be responsible for any loss of sanity due to reading this code.
// Here be dragons!

#include "WinBacktrace.h"

#include <windows.h>

#ifndef __i386__
#error WinBacktrace is only supported on x86 architectures.
#endif


void dumpInfo(StackFrame* frame, const BYTE* caller, HINSTANCE hInst);

// We need to do some crazy shit to walk through the stack.
// Windows unwinds the stack when an exception is thrown, so we
// need to examine the EXCEPTION_POINTERS's CONTEXT.
size_t getBacktrace(StackFrame *stack, size_t size, CONTEXT ctx)
{
	// Written using information and a bit of pseudocode from
	// http://www.eptacom.net/pubblicazioni/pub_eng/except.html
	// This is probably one of the most horrifying things I've ever written.

	// This tracks whether the current EBP is valid.
	// When an invalid EBP is encountered, we stop walking the stack.
	bool validEBP = true;
	DWORD ebp =	ctx.Ebp; // The current EBP (Extended Base Pointer)
	DWORD eip = ctx.Eip;
	int i;
	for (i = 0; i < size; i++)
	{
		if (ebp & 3)
			validEBP = false;
		// FIXME: This function is obsolete, according to MSDN.
		else if (IsBadReadPtr((void*) ebp, 8))
			validEBP = false;

		if (!validEBP) break;

		// Find the caller.
		// On the first iteration, the caller is whatever EIP points to.
		// On successive iterations, the caller is the byte after EBP.
		BYTE* caller = !i ? (BYTE*)eip : *((BYTE**) ebp + 1);
		// The first ebp is the EBP from the CONTEXT.
		// On successive iterations, the EBP is the DWORD that the previous EBP points to.
		ebp	= !i ? ebp : *(DWORD*)ebp;

		// Find the caller's module.
		// We'll use VirtualQuery to get information about the caller's address.
		MEMORY_BASIC_INFORMATION mbi;
		VirtualQuery(caller, &mbi, sizeof(mbi));

		// We can get the instance handle from the allocation base.
		HINSTANCE hInst = (HINSTANCE)mbi.AllocationBase;

		// If the handle is 0, then the EBP is invalid.
		if (hInst == 0) validEBP = false;
		// Otherwise, dump info about the caller.
		else stack[i].address = (void*)caller;
	}

	return i;
}
