/*-
 * Copyright 2003-2005 Colin Percival
 * All rights reserved
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted providing that the following conditions 
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT,
 * STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#ifdef _MSC_VER
	// bogus 'secure' nonsense
	#define _CRT_SECURE_NO_WARNINGS
	// bogus signed/unsigned mismatch
	#pragma warning( disable: 4018)
#endif

#ifdef _WIN32
	#include <sys/types.h>
	#define ssize_t size_t
#else
	#include <unistd.h>
#endif

#if defined __APPLE__ && defined __MACH__
	typedef unsigned char u_char;
#endif

#include "bzlib.h"
#include "bspatch.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <fcntl.h>

static off_t offtin(u_char *buf)
{
	off_t y;

	y=buf[7]&0x7F;
	y=y*256;y+=buf[6];
	y=y*256;y+=buf[5];
	y=y*256;y+=buf[4];
	y=y*256;y+=buf[3];
	y=y*256;y+=buf[2];
	y=y*256;y+=buf[1];
	y=y*256;y+=buf[0];

	if(buf[7]&0x80) y=-y;

	return y;
}

int bspatch(const char * oldfile, const char * newfile, const char * patchfile)
{
	FILE * f, * cpf, * dpf, * epf;
	BZFILE * cpfbz2, * dpfbz2, * epfbz2;
	int cbz2err, dbz2err, ebz2err;
	FILE * temp;
	ssize_t oldsize,newsize;
	ssize_t bzctrllen,bzdatalen;
	unsigned char header[32],buf[8];
	unsigned char *old_contents;
	unsigned char *new_contents;
	off_t oldpos,newpos;
	off_t ctrl[3];
	off_t lenread;
	off_t i;

	/* Open patch file */
	if ((f = fopen(patchfile, "rb")) == NULL)
	{
		//err(1, "fopen(%s)", argv[3]);
		return ERR_OTHER;
	}

	/*
	File format:
		0	8	"BSDIFF40"
		8	8	X
		16	8	Y
		24	8	sizeof(newfile)
		32	X	bzip2(control block)
		32+X	Y	bzip2(diff block)
		32+X+Y	???	bzip2(extra block)
	with control block a set of triples (x,y,z) meaning "add x bytes
	from oldfile to x bytes from the diff block; copy y bytes from the
	extra block; seek forwards in oldfile by z bytes".
	*/

	/* Read header */
	if (fread(header, 1, 32, f) < 32)
	{
		if (feof(f))
		{
			//errx(1, "Corrupt patch\n");
			return ERR_CORRUPT_PATCH;
		}
		//err(1, "fread(%s)", argv[3]);
		return ERR_OTHER;
	}

	/* Check for appropriate magic */
	if (memcmp(header, "BSDIFF40", 8) != 0)
	{
		//errx(1, "Corrupt patch\n");
		return ERR_CORRUPT_PATCH;
	}

	/* Read lengths from header */
	bzctrllen=offtin(header+8);
	bzdatalen=offtin(header+16);
	newsize=offtin(header+24);
	if((bzctrllen<0) || (bzdatalen<0) || (newsize<0))
	{
		//errx(1,"Corrupt patch\n");
		return ERR_CORRUPT_PATCH;
	}

	/* Close patch file and re-open it via libbzip2 at the right places */
	if (fclose(f))
	{
		//err(1, "fclose(%s)", argv[3]);
		return ERR_OTHER;
	}
	if ((cpf = fopen(patchfile, "rb")) == NULL)
	{
		//err(1, "fopen(%s)", argv[3]);
		return ERR_OTHER;
	}
	if (fseek(cpf, 32, SEEK_SET))
	{
		// err(1, "fseeko(%s, %lld)", argv[3], (long long)32);
		return ERR_OTHER;
	}
	if ((cpfbz2 = BZ2_bzReadOpen(&cbz2err, cpf, 0, 0, NULL, 0)) == NULL)
	{
		//errx(1, "BZ2_bzReadOpen, bz2err = %d", cbz2err);
		return ERR_OTHER;
	}
	if ((dpf = fopen(patchfile, "rb")) == NULL)
	{
		//err(1, "fopen(%s)", argv[3]);
		return ERR_OTHER;
	}
	if (fseek(dpf, 32 + bzctrllen, SEEK_SET))
	{
		//err(1, "fseeko(%s, %lld)", argv[3], (long long)(32 + bzctrllen));
		return ERR_OTHER;
	}
	if ((dpfbz2 = BZ2_bzReadOpen(&dbz2err, dpf, 0, 0, NULL, 0)) == NULL)
	{
		//errx(1, "BZ2_bzReadOpen, bz2err = %d", dbz2err);
		return ERR_OTHER;
	}
	if ((epf = fopen(patchfile, "rb")) == NULL)
	{
		//err(1, "fopen(%s)", argv[3]);
		return ERR_OTHER;
	}
	if (fseek(epf, 32 + bzctrllen + bzdatalen, SEEK_SET))
	{
		//err(1, "fseeko(%s, %lld)", argv[3], (long long)(32 + bzctrllen + bzdatalen));
		return ERR_OTHER;
	}
	if ((epfbz2 = BZ2_bzReadOpen(&ebz2err, epf, 0, 0, NULL, 0)) == NULL)
	{
		//errx(1, "BZ2_bzReadOpen, bz2err = %d", ebz2err);
		return ERR_OTHER;
	}

	if((temp=fopen(oldfile,"rb")) == NULL)
	{
		return ERR_OTHER;
	}
	if((fseek(temp,0,SEEK_END))!=0)
	{
		return ERR_OTHER;
	}
	oldsize = ftell(temp);
	if((old_contents=malloc(oldsize+1))==NULL)
	{
		return ERR_OTHER;
	}
	rewind(temp);
	if(fread(old_contents,oldsize,1,temp)!=1)
	{
		return ERR_OTHER;
	}
	if(fclose(temp)==EOF)
	{
		return ERR_OTHER;
	}
	if((new_contents=malloc(newsize+1))==NULL)
	{
		//err(1,NULL);
		return ERR_OTHER;
	}

	oldpos=0;newpos=0;
	while(newpos<newsize)
	{
		/* Read control data */
		for(i=0;i<=2;i++) {
			lenread = BZ2_bzRead(&cbz2err, cpfbz2, buf, 8);
			if ((lenread < 8) || ((cbz2err != BZ_OK) && (cbz2err != BZ_STREAM_END)))
			{
				//errx(1, "Corrupt patch\n");
				return ERR_CORRUPT_PATCH;
			}
			ctrl[i]=offtin(buf);
		};

		/* Sanity-check */
		if(newpos+ctrl[0]>newsize)
		{
			//errx(1,"Corrupt patch\n");
			return ERR_CORRUPT_PATCH;
		}

		/* Read diff string */
		lenread = BZ2_bzRead(&dbz2err, dpfbz2, new_contents + newpos, ctrl[0]);
		if ((lenread < ctrl[0]) || ((dbz2err != BZ_OK) && (dbz2err != BZ_STREAM_END)))
		{
			//errx(1, "Corrupt patch\n");
			return ERR_CORRUPT_PATCH;
		}

		/* Add old data to diff string */
		for(i=0;i<ctrl[0];i++)
		{
			if((oldpos+i>=0) && (oldpos+i<oldsize))
			{
				new_contents[newpos+i]+=old_contents[oldpos+i];
			}
		}

		/* Adjust pointers */
		newpos+=ctrl[0];
		oldpos+=ctrl[0];

		/* Sanity-check */
		if(newpos+ctrl[1]>newsize)
		{
			//errx(1,"Corrupt patch\n");
			return ERR_CORRUPT_PATCH;
		}

		/* Read extra string */
		lenread = BZ2_bzRead(&ebz2err, epfbz2, new_contents + newpos, ctrl[1]);
		if ((lenread < ctrl[1]) || ((ebz2err != BZ_OK) && (ebz2err != BZ_STREAM_END)))
		{
			//errx(1, "Corrupt patch\n");
			return ERR_CORRUPT_PATCH;
		}

		/* Adjust pointers */
		newpos+=ctrl[1];
		oldpos+=ctrl[2];
	};

	/* Clean up the bzip2 reads */
	BZ2_bzReadClose(&cbz2err, cpfbz2);
	BZ2_bzReadClose(&dbz2err, dpfbz2);
	BZ2_bzReadClose(&ebz2err, epfbz2);
	if (fclose(cpf) || fclose(dpf) || fclose(epf))
	{
		//err(1, "fclose(%s)", argv[3]);
		return ERR_OTHER;
	}

	/* Write the new file */
	if(
		((temp=fopen(newfile,"wb"))==NULL) ||
		(fwrite(new_contents,newsize,1,temp)==0) ||
		(fclose(temp)==EOF)
	)
	{
		//err(1,"%s",argv[2]);
		return ERR_OTHER;
	}

	free(new_contents);
	free(old_contents);

	return ERR_NONE;
}
