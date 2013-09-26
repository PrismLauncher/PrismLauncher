/*
 * Copyright (c) 2001, 2009, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */

// random definitions

#ifdef _MSC_VER
#include <windows.h>
#include <winuser.h>
#else
#include <unistd.h>
#endif

#ifndef FULL
#define FULL 1 /* Adds <500 bytes to the zipped final product. */
#endif

#if FULL // define this if you want debugging and/or compile-time attributes
#define IF_FULL(x) x
#else
#define IF_FULL(x) /*x*/
#endif

// Error messages that we have
#define ERROR_ENOMEM "Native allocation failed"
#define ERROR_FORMAT "Corrupted pack file"
#define ERROR_RESOURCE "Cannot extract resource file"
#define ERROR_OVERFLOW "Internal buffer overflow"
#define ERROR_INTERNAL "Internal error"

#define LOGFILE_STDOUT "-"
#define LOGFILE_STDERR ""

#define lengthof(array) (sizeof(array) / sizeof(array[0]))

#define NEW(T, n) (T *) must_malloc((int)(scale_size(n, sizeof(T))))
#define U_NEW(T, n) (T *) u->alloc(scale_size(n, sizeof(T)))
#define T_NEW(T, n) (T *) u->temp_alloc(scale_size(n, sizeof(T)))

// bytes and byte arrays

typedef unsigned int uint;

#ifdef _MSC_VER
typedef LONGLONG jlong;
typedef DWORDLONG julong;
#define MKDIR(dir) mkdir(dir)
#define getpid() _getpid()
#define PATH_MAX MAX_PATH
#define dup2(a, b) _dup2(a, b)
#define strcasecmp(s1, s2) _stricmp(s1, s2)
#define tempname _tempname
#define sleep Sleep
#else
typedef signed char byte;
#ifdef _LP64
typedef long jlong;
typedef long unsigned julong;
#else
typedef long long jlong;
typedef long long unsigned julong;
#endif
#define MKDIR(dir) mkdir(dir, 0777);
#endif

/* Must cast to void *, then size_t, then int. */
#define ptrlowbits(x) ((int)(size_t)(void *)(x))

/* Back and forth from jlong to pointer */
#define ptr2jlong(x) ((jlong)(size_t)(void *)(x))
#define jlong2ptr(x) ((void *)(size_t)(x))

// Keys used by Java:
#define UNPACK_DEFLATE_HINT "unpack.deflate.hint"

#define COM_PREFIX "com.sun.java.util.jar.pack."
#define UNPACK_MODIFICATION_TIME COM_PREFIX "unpack.modification.time"
#define DEBUG_VERBOSE COM_PREFIX "verbose"

#define ZIP_ARCHIVE_MARKER_COMMENT "PACK200"

// The following are not known to the Java classes:
#define UNPACK_REMOVE_PACKFILE COM_PREFIX "unpack.remove.packfile"

// Called from unpacker layers
#define _CHECK_DO(t, x)                                                                        \
	{                                                                                          \
		if (t)                                                                                 \
		{                                                                                      \
			x;                                                                                 \
		}                                                                                      \
	}

#define CHECK _CHECK_DO(aborting(), return)
#define CHECK_(y) _CHECK_DO(aborting(), return y)
#define CHECK_0 _CHECK_DO(aborting(), return 0)

#define CHECK_NULL(p) _CHECK_DO((p) == nullptr, return)
#define CHECK_NULL_(y, p) _CHECK_DO((p) == nullptr, return y)
#define CHECK_NULL_0(p) _CHECK_DO((p) == nullptr, return 0)

#define CHECK_COUNT(t)                                                                         \
	if (t < 0)                                                                                 \
	{                                                                                          \
		abort("bad value count");                                                              \
	}                                                                                          \
	CHECK

#define STR_TRUE "true"
#define STR_FALSE "false"

#define STR_TF(x) ((x) ? STR_TRUE : STR_FALSE)
#define BOOL_TF(x) (((x) != nullptr &&strcmp((x), STR_TRUE) == 0) ? true : false)

#define DEFAULT_ARCHIVE_MODTIME 1060000000 // Aug 04, 2003 5:26 PM PDT
