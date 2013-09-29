/*
 * Copyright (c) 2003, 2008, Oracle and/or its affiliates. All rights reserved.
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
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <assert.h>
#include <limits.h>
#include <time.h>
#include <stdint.h>

#include "constants.h"
#include "utils.h"
#include "defines.h"
#include "bytes.h"
#include "coding.h"
#include "unpack200.h"
#include "unpack.h"
#include "zip.h"

// Callback for fetching data, Unix style.
static int64_t read_input_via_stdio(unpacker *u, void *buf, int64_t minlen, int64_t maxlen)
{
	assert(u->infileptr != nullptr);
	assert(minlen <= maxlen); // don't talk nonsense
	int64_t numread = 0;
	char *bufptr = (char *)buf;
	while (numread < minlen)
	{
		// read available input, up to buf.length or maxlen
		int readlen = (1 << 16);
		if (readlen > (maxlen - numread))
			readlen = (int)(maxlen - numread);
		int nr = 0;

		nr = (int)fread(bufptr, 1, readlen, u->infileptr);
		if (nr <= 0)
		{
			if (errno != EINTR)
				break;
			nr = 0;
		}
		numread += nr;
		bufptr += nr;
		assert(numread <= maxlen);
	}
	return numread;
}

enum
{
	EOF_MAGIC = 0,
	BAD_MAGIC = -1
};

static int read_magic(unpacker *u, char peek[], int peeklen)
{
	assert(peeklen == 4); // magic numbers are always 4 bytes
	int64_t nr = (u->read_input_fn)(u, peek, peeklen, peeklen);
	if (nr != peeklen)
	{
		return (nr == 0) ? EOF_MAGIC : BAD_MAGIC;
	}
	int magic = 0;
	for (int i = 0; i < peeklen; i++)
	{
		magic <<= 8;
		magic += peek[i] & 0xFF;
	}
	return magic;
}

void unpack_200(std::string input_path, std::string output_path)
{
	unpacker u;
	int status = 0;

	FILE *input = fopen(input_path.c_str(), "rb");
	if (!input)
	{
		throw std::runtime_error("Can't open input file" + input_path);
	}
	FILE *output = fopen(output_path.c_str(), "wb");
	if (!output)
	{
		fclose(output);
		throw std::runtime_error("Can't open output file" + output_path);
	}
	u.init(read_input_via_stdio);

	// initialize jar output
	// the output takes ownership of the file handle
	jar jarout;
	jarout.init(&u);
	jarout.jarfp = output;

	// the input doesn't
	u.infileptr = input;

	// read the magic!
	char peek[4];
	int magic;
	magic = read_magic(&u, peek, (int)sizeof(peek));

	// if it is a gzip encoded file, we need an extra gzip input filter
	if ((magic & GZIP_MAGIC_MASK) == GZIP_MAGIC)
	{
		gunzip *gzin = NEW(gunzip, 1);
		gzin->init(&u);
		// FIXME: why the side effects? WHY?
		u.gzin->start(magic);
		u.start();
	}
	else
	{
		// otherwise, feed the bytes to the unpacker directly
		u.start(peek, sizeof(peek));
	}

	// Note:  The checks to u.aborting() are necessary to gracefully
	// terminate processing when the first segment throws an error.
	for (;;)
	{
		// Each trip through this loop unpacks one segment
		// and then resets the unpacker.
		for (unpacker::file *filep; (filep = u.get_next_file()) != nullptr;)
		{
			u.write_file_to_jar(filep);
		}

		// Peek ahead for more data.
		magic = read_magic(&u, peek, (int)sizeof(peek));
		if (magic != (int)JAVA_PACKAGE_MAGIC)
		{
			if (magic != EOF_MAGIC)
				unpack_abort("garbage after end of pack archive");
			break; // all done
		}

		// Release all storage from parsing the old segment.
		u.reset();
		// Restart, beginning with the peek-ahead.
		u.start(peek, sizeof(peek));
	}
	u.finish();
	u.free(); // tidy up malloc blocks
	fclose(input);
}
