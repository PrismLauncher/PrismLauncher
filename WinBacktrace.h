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

#pragma once

#include <windows.h>

#ifndef SF_STR_LEN
// The max length of all strings in the StackFrame struct.
// Because it must be stack allocated, this must be known at compile time.
// Stuff longer than this will be truncated.
// Defaults to 4096 (4kB)
#define SF_STR_LEN 4096
#endif

// Data structure for holding information about a stack frame.
// There's some more hackery in here so it can be allocated on the stack.
struct StackFrame
{
	// The address of this stack frame.
	void* address;

	// The name of the function at this address.
	char funcName[SF_STR_LEN];
};

// This function walks through the given CONTEXT structure, extracting a
// backtrace from it.
// The backtrace will be put into the array given by the `stack` argument
// with a maximum length of `size`.
// This function returns the size of the backtrace retrieved.
size_t getBacktrace(StackFrame* stack, size_t size, CONTEXT ctx);
