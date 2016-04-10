/*
 * Copyright (c) 2001, 2008, Oracle and/or its affiliates. All rights reserved.
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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <stdint.h>

#include <sys/stat.h>

#ifdef _MSC_VER
#include <direct.h>
#include <io.h>
#include <process.h>
#else
#include <unistd.h>
#endif

#include "constants.h"
#include "defines.h"
#include "bytes.h"
#include "utils.h"

#include "unpack.h"

void *must_malloc(size_t size)
{
	size_t msize = size;
	void *ptr = (msize > PSIZE_MAX) ? nullptr : malloc(msize);
	if (ptr != nullptr)
	{
		memset(ptr, 0, size);
	}
	else
	{
		throw std::runtime_error(ERROR_ENOMEM);
	}
	return ptr;
}

void unpack_abort(const char *msg)
{
	if (msg == nullptr)
		msg = "corrupt pack file or internal error";
	throw std::runtime_error(msg);
}
