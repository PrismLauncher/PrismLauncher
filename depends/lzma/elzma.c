/*
 * Written in 2009 by Lloyd Hilaiel
 *
 * License
 *
 * All the cruft you find here is public domain.  You don't have to credit
 * anyone to use this code, but my personal request is that you mention
 * Igor Pavlov for his hard, high quality work.
 *
 * command line elzma tool for lzma compression
 *
 * At time of writing, the primary purpose of this tool is to test the
 * easylzma library.
 *
 * TODO:
 *  - stdin/stdout support
 *  - multiple file support
 *  - much more
 */

#include "include/compress.h"
#include "include/decompress.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#ifdef WIN32
#include <stdio.h>
#define unlink _unlink
#else
#include <unistd.h>
#endif

int deleteFile(const char *path)
{
	return unlink(path);
}

/* a utility to open a pair of files */
/* XXX: respect overwrite flag */
static int openFiles(const char *ifname, FILE **inFile, const char *ofname, FILE **outFile,
					 int overwrite)
{
	*inFile = fopen(ifname, "rb");
	if (*inFile == NULL)
	{
		fprintf(stderr, "couldn't open '%s' for reading\n", ifname);
		return 1;
	}

	*outFile = fopen(ofname, "wb");
	if (*outFile == NULL)
	{
		fprintf(stderr, "couldn't open '%s' for writing\n", ofname);
		return 1;
	}

	return 0;
}

#define ELZMA_COMPRESS_USAGE                                                                   \
	"Compress files using the LZMA algorithm (in place by default).\n"                         \
	"\n"                                                                                       \
	"Usage: elzma [options] [file]\n"                                                          \
	"  -1 .. -9          compression level, -1 is fast, -9 is best (default 5)\n"              \
	"  -f, --force       overwrite output files if they exist\n"                               \
	"  -h, --help        output this message and exit\n"                                       \
	"  -k, --keep        don't delete input files\n"                                           \
	"  --lzip            compress to lzip disk format (.lz extension)\n"                       \
	"  --lzma            compress to LZMA-Alone disk format (.lzma extension)\n"               \
	"  -v, --verbose     output verbose status information while compressing\n"                \
	"  -z, --compress    compress files (default when invoking elzma program)\n"               \
	"  -d, --decompress  decompress files (default when invoking unelzma program)\n"           \
	"\n"                                                                                       \
	"Advanced Options:\n"                                                                      \
	"  -s --set-max-dict (advanced) specify maximum dictionary size in bytes\n"

/* parse arguments populating output parameters, return nonzero on failure */
static int parseCompressArgs(int argc, char **argv, unsigned char *level, char **fname,
							 unsigned int *maxDictSize, unsigned int *verbose,
							 unsigned int *keep, unsigned int *overwrite,
							 elzma_file_format *format)
{
	int i;

	if (argc < 2)
		return 1;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			char *val = NULL;
			char *arg = &(argv[i][1]);
			if (arg[0] == '-')
				arg++;

			/* now see what argument this is */
			if (!strcmp(arg, "h") || !strcmp(arg, "help"))
			{
				return 1;
			}
			else if (!strcmp(arg, "s") || !strcmp(arg, "set-max-dict"))
			{
				unsigned int j = 0;
				val = argv[++i];

				/* validate argument is numeric */
				for (j = 0; j < strlen(val); j++)
				{
					if (val[j] < '0' || val[j] > '9')
						return 1;
				}

				*maxDictSize = strtoul(val, (char **)NULL, 10);

				/* don't allow dictionary sizes less than 8k */
				if (*maxDictSize < (1 < 13))
					*maxDictSize = 1 < 13;
				else
				{
					/* make sure dict size is compatible with lzip,
					 * this will effectively collapse it to a close power
					 * of 2 */
					*maxDictSize = elzma_get_dict_size(*maxDictSize);
				}
			}
			else if (!strcmp(arg, "v") || !strcmp(arg, "verbose"))
			{
				*verbose = 1;
			}
			else if (!strcmp(arg, "f") || !strcmp(arg, "force"))
			{
				*overwrite = 1;
			}
			else if (!strcmp(arg, "k") || !strcmp(arg, "keep"))
			{
				*keep = 1;
			}
			else if (strlen(arg) == 1 && arg[0] >= '1' && arg[0] <= '9')
			{
				*level = arg[0] - '0';
			}
			else if (!strcmp(arg, "lzma"))
			{
				*format = ELZMA_lzma;
			}
			else if (!strcmp(arg, "lzip"))
			{
				*format = ELZMA_lzip;
			}
			else if (!strcmp(arg, "z") || !strcmp(arg, "d") || !strcmp(arg, "compress") ||
					 !strcmp(arg, "decompress"))
			{
				/* noop */
			}
			else
			{
				return 1;
			}
		}
		else
		{
			*fname = argv[i];
			break;
		}
	}

	/* proper number of arguments? */
	if (i != argc - 1 || *fname == NULL)
		return 1;

	return 0;
}

/* callbacks for streamed input and output */
static size_t elzmaWriteFunc(void *ctx, const void *buf, size_t size)
{
	size_t wt;
	FILE *f = (FILE *)ctx;
	assert(f != NULL);

	wt = fwrite(buf, 1, size, f);

	return wt;
}

static int elzmaReadFunc(void *ctx, void *buf, size_t *size)
{
	FILE *f = (FILE *)ctx;
	assert(f != NULL);
	*size = fread(buf, 1, *size, f);

	return 0;
}

static void printProgressHeader(void)
{
	printf("|0%%                            50%%                          100%%|\n");
}

static void endProgress(int pCtx)
{
	while (pCtx++ < 64)
	{
		printf(".");
	}
	printf("|\n");
}

static void elzmaProgressFunc(void *ctx, size_t complete, size_t total)
{
	int *dots = (int *)ctx;
	int wantDots = (int)(64 * (double)complete / (double)total);
	if (*dots == 0)
	{
		printf("|");
		(*dots)++;
	}
	while (wantDots > *dots)
	{
		printf(".");
		(*dots)++;
	}
	fflush(stdout);
}

static int doCompress(int argc, char **argv)
{
	/* default compression parameters, some of which may be overridded by
	 * command line arguments */
	unsigned char level = 5;
	unsigned char lc = ELZMA_LC_DEFAULT;
	unsigned char lp = ELZMA_LP_DEFAULT;
	unsigned char pb = ELZMA_PB_DEFAULT;
	unsigned int maxDictSize = ELZMA_DICT_SIZE_DEFAULT_MAX;
	unsigned int dictSize = 0;
	elzma_file_format format = ELZMA_lzma;
	char *ext = ".lzma";
	char *ifname = NULL;
	char *ofname = NULL;
	unsigned int verbose = 0;
	FILE *inFile = NULL;
	FILE *outFile = NULL;
	elzma_compress_handle hand = NULL;
	/* XXX: large file support */
	unsigned int uncompressedSize = 0;
	unsigned int keep = 0;
	unsigned int overwrite = 0;

	if (0 != parseCompressArgs(argc, argv, &level, &ifname, &maxDictSize, &verbose, &keep,
							   &overwrite, &format))
	{
		fprintf(stderr, ELZMA_COMPRESS_USAGE);
		return 1;
	}

	/* extension switching based on compression type*/
	if (format == ELZMA_lzip)
		ext = ".lz";

	/* generate output file name */
	{
		ofname = malloc(strlen(ifname) + strlen(ext) + 1);
		ofname[0] = 0;
		strcat(ofname, ifname);
		strcat(ofname, ext);
	}

	/* now attempt to open input and ouput files */
	/* XXX: stdin/stdout support */
	if (0 != openFiles(ifname, &inFile, ofname, &outFile, overwrite))
	{
		return 1;
	}

	/* set uncompressed size */
	if (0 != fseek(inFile, 0, SEEK_END) || 0 == (uncompressedSize = ftell(inFile)) ||
		0 != fseek(inFile, 0, SEEK_SET))
	{
		fprintf(stderr, "error seeking input file (%s) - zero length?\n", ifname);
		deleteFile(ofname);
		return 1;
	}

	/* determine a reasonable dictionary size given input size */
	dictSize = elzma_get_dict_size(uncompressedSize);
	if (dictSize > maxDictSize)
		dictSize = maxDictSize;

	if (verbose)
	{
		printf("compressing '%s' to '%s'\n", ifname, ofname);
		printf("lc/lp/pb = %u/%u/%u | dictionary size = %u bytes\n", lc, lp, pb, dictSize);
		printf("input file is %u bytes\n", uncompressedSize);
	}

	/* allocate a compression handle */
	hand = elzma_compress_alloc();
	if (hand == NULL)
	{
		fprintf(stderr, "couldn't allocate compression object\n");
		deleteFile(ofname);
		return 1;
	}

	if (ELZMA_E_OK !=
		elzma_compress_config(hand, lc, lp, pb, level, dictSize, format, uncompressedSize))
	{
		fprintf(stderr, "couldn't configure compression with "
						"provided parameters\n");
		deleteFile(ofname);
		return 1;
	}

	{
		int rv;
		int pCtx = 0;

		if (verbose)
			printProgressHeader();

		rv = elzma_compress_run(hand, elzmaReadFunc, (void *)inFile, elzmaWriteFunc,
								(void *)outFile, (verbose ? elzmaProgressFunc : NULL), &pCtx);

		if (verbose)
			endProgress(pCtx);

		if (ELZMA_E_OK != rv)
		{
			fprintf(stderr, "error compressing\n");
			deleteFile(ofname);
			return 1;
		}
	}

	/* clean up */
	elzma_compress_free(&hand);
	fclose(inFile);
	fclose(outFile);
	free(ofname);

	if (!keep)
		deleteFile(ifname);

	return 0;
}

#define ELZMA_DECOMPRESS_USAGE                                                                 \
	"Decompress files compressed using the LZMA algorithm (in place by default).\n"            \
	"\n"                                                                                       \
	"Usage: unelzma [options] [file]\n"                                                        \
	"  -f, --force      overwrite output files if they exist\n"                                \
	"  -h, --help       output this message and exit\n"                                        \
	"  -k, --keep       don't delete input files\n"                                            \
	"  -v, --verbose    output verbose status information while decompressing\n"               \
	"  -z, --compress   compress files (default when invoking elzma program)\n"                \
	"  -d, --decompress decompress files (default when invoking unelzma program)\n"            \
	"\n"
/* parse arguments populating output parameters, return nonzero on failure */
static int parseDecompressArgs(int argc, char **argv, char **fname, unsigned int *verbose,
							   unsigned int *keep, unsigned int *overwrite)
{
	int i;

	if (argc < 2)
		return 1;

	for (i = 1; i < argc; i++)
	{
		if (argv[i][0] == '-')
		{
			char *arg = &(argv[i][1]);
			if (arg[0] == '-')
				arg++;

			/* now see what argument this is */
			if (!strcmp(arg, "h") || !strcmp(arg, "help"))
			{
				return 1;
			}
			else if (!strcmp(arg, "v") || !strcmp(arg, "verbose"))
			{
				*verbose = 1;
			}
			else if (!strcmp(arg, "k") || !strcmp(arg, "keep"))
			{
				*keep = 1;
			}
			else if (!strcmp(arg, "f") || !strcmp(arg, "force"))
			{
				*overwrite = 1;
			}
			else if (!strcmp(arg, "z") || !strcmp(arg, "d") || !strcmp(arg, "compress") ||
					 !strcmp(arg, "decompress"))
			{
				/* noop */
			}
			else
			{
				return 1;
			}
		}
		else
		{
			*fname = argv[i];
			break;
		}
	}

	/* proper number of arguments? */
	if (i != argc - 1 || *fname == NULL)
		return 1;

	return 0;
}

static int doDecompress(int argc, char **argv)
{
	char *ifname = NULL;
	char *ofname = NULL;
	unsigned int verbose = 0;
	FILE *inFile = NULL;
	FILE *outFile = NULL;
	elzma_decompress_handle hand = NULL;
	unsigned int overwrite = 0;
	unsigned int keep = 0;
	elzma_file_format format;
	const char *lzmaExt = ".lzma";
	const char *lzipExt = ".lz";
	const char *ext = ".lz";

	if (0 != parseDecompressArgs(argc, argv, &ifname, &verbose, &keep, &overwrite))
	{
		fprintf(stderr, ELZMA_DECOMPRESS_USAGE);
		return 1;
	}

	/* generate output file name */
	if (strlen(ifname) > strlen(lzmaExt) &&
		0 == strcmp(lzmaExt, ifname + strlen(ifname) - strlen(lzmaExt)))
	{
		format = ELZMA_lzma;
		ext = lzmaExt;
	}
	else if (strlen(ifname) > strlen(lzipExt) &&
			 0 == strcmp(lzipExt, ifname + strlen(ifname) - strlen(lzipExt)))
	{
		format = ELZMA_lzip;
		ext = lzipExt;
	}
	else
	{
		fprintf(stderr, "input file extension not recognized (expected either "
						"%s or %s)",
				lzmaExt, lzipExt);
		return 1;
	}

	ofname = malloc(strlen(ifname) - strlen(ext));
	ofname[0] = 0;
	strncat(ofname, ifname, strlen(ifname) - strlen(ext));

	/* now attempt to open input and ouput files */
	/* XXX: stdin/stdout support */
	if (0 != openFiles(ifname, &inFile, ofname, &outFile, overwrite))
	{
		return 1;
	}

	hand = elzma_decompress_alloc();
	if (hand == NULL)
	{
		fprintf(stderr, "couldn't allocate decompression object\n");
		deleteFile(ofname);
		return 1;
	}

	if (ELZMA_E_OK != elzma_decompress_run(hand, elzmaReadFunc, (void *)inFile, elzmaWriteFunc,
										   (void *)outFile, format))
	{
		fprintf(stderr, "error decompressing\n");
		deleteFile(ofname);
		return 1;
	}

	elzma_decompress_free(&hand);

	if (!keep)
		deleteFile(ifname);

	return 0;
}

int main(int argc, char **argv)
{
	const char *unelzma = "unelzma";
	const char *unelzmaLose = "unelzma.exe";
	const char *elzma = "elzma";
	const char *elzmaLose = "elzma.exe";

	enum
	{
		RM_NONE,
		RM_COMPRESS,
		RM_DECOMPRESS
	} runmode = RM_NONE;

	/* first we'll determine the mode we're running in, indicated by
	 * the binary name (argv[0]) or by the presence of a flag:
	 * one of -z, -d, -compress, --decompress */
	if ((strlen(argv[0]) >= strlen(unelzma) &&
		 !strcmp((argv[0] + strlen(argv[0]) - strlen(unelzma)), unelzma)) ||
		(strlen(argv[0]) >= strlen(unelzmaLose) &&
		 !strcmp((argv[0] + strlen(argv[0]) - strlen(unelzmaLose)), unelzmaLose)))
	{
		runmode = RM_DECOMPRESS;
	}
	else if ((strlen(argv[0]) >= strlen(elzma) &&
			  !strcmp((argv[0] + strlen(argv[0]) - strlen(elzma)), elzma)) ||
			 (strlen(argv[0]) >= strlen(elzmaLose) &&
			  !strcmp((argv[0] + strlen(argv[0]) - strlen(elzmaLose)), elzmaLose)))
	{
		runmode = RM_COMPRESS;
	}

	/* allow runmode to be overridded by a command line flag, first flag
	 * wins */
	{
		int i;
		for (i = 1; i < argc; i++)
		{
			if (!strcmp(argv[i], "-d") || !strcmp(argv[i], "--decompress"))
			{
				runmode = RM_DECOMPRESS;
				break;
			}
			else if (!strcmp(argv[i], "-z") || !strcmp(argv[i], "--compress"))
			{
				runmode = RM_COMPRESS;
				break;
			}
		}
	}

	if (runmode != RM_COMPRESS && runmode != RM_DECOMPRESS)
	{
		fprintf(stderr, "couldn't determine whether "
						"you want to compress or decompress\n");
		return 1;
	}

	if (runmode == RM_COMPRESS)
		return doCompress(argc, argv);
	return doDecompress(argc, argv);
}
