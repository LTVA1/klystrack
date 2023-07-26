/*
Copyright (c) 2009-2010 Tero Lindeman (kometbomb)
Copyright (c) 2021-2023 Georgy Saraykin (LTVA1 a.k.a. LTVA) and contributors

Permission is hereby granted, free of charge, to any person
obtaining a copy of this software and associated documentation
files (the "Software"), to deal in the Software without
restriction, including without limitation the rights to use,
copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the
Software is furnished to do so, subject to the following
conditions:

The above copyright notice and this permission notice shall be
included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
OTHER DEALINGS IN THE SOFTWARE.
*/

#include "fur.h"
#include "zlib.h"

#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "fur/furmodule.h"

//from https://www.zlib.net/zlib_how.html

#define CHUNK (1 << 15)

/* Decompress from file source to file dest until stream ends or EOF.
inf() returns Z_OK on success, Z_MEM_ERROR if memory could not be
allocated for processing, Z_DATA_ERROR if the deflate data is
invalid or incomplete, Z_VERSION_ERROR if the version of zlib.h and
the version of the library linked do not match, or Z_ERRNO if there
is an error reading or writing the files. */
static int inflate_furnace_module(FILE *source, FILE *dest)
{
	int ret;
	unsigned have;
	z_stream strm;
	unsigned char in[CHUNK];
	unsigned char out[CHUNK];

	/* allocate inflate state */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = 0;
	strm.next_in = Z_NULL;
	ret = inflateInit(&strm);

	if (ret != Z_OK)
	{
		return ret;
	}

	/* decompress until deflate stream ends or end of file */
	do
	{
		strm.avail_in = fread(in, 1, CHUNK, source);

		if (ferror(source))
		{
			(void)inflateEnd(&strm);
			return Z_ERRNO;
		}

		if (strm.avail_in == 0)
		{
			break;
		}

		strm.next_in = in;

		/* run inflate() on input until output buffer not full */
		do 
		{
			strm.avail_out = CHUNK;
			strm.next_out = out;
			ret = inflate(&strm, Z_NO_FLUSH);
			assert(ret != Z_STREAM_ERROR);  /* state not clobbered */

			switch (ret)
			{
				case Z_NEED_DICT:
				{
					ret = Z_DATA_ERROR;     /* and fall through */
				}

				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
				{
					(void)inflateEnd(&strm);
					return ret;
				}
			}

			have = CHUNK - strm.avail_out;

			if (fwrite(out, 1, have, dest) != have || ferror(dest))
			{
				(void)inflateEnd(&strm);
				return Z_ERRNO;
			}
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */
	} while (ret != Z_STREAM_END);

	/* clean up and return */
	(void)inflateEnd(&strm);
	return ret == Z_STREAM_END ? Z_OK : Z_DATA_ERROR;
}

int import_fur(FILE *f)
{
	char header[30] = { 0 };

	debug("Starting Furnace module import...");
	
	fread(header, 1, strlen(FUR_HEADER_SIG), f);
	
	if(strcmp(header, FUR_HEADER_SIG) != 0) //maybe module is compressed with zlib?
	{
		fseek(f, 0, SEEK_SET);

		char dir_path[10000];

		getcwd(dir_path, 10000);

		char file_path[10000];

		time_t now_time;
		time(&now_time);
		struct tm *now_tm = localtime(&now_time);

		if (!now_tm)
		{
			debug("Failed to retrieve current time!");
		}

		snprintf(file_path, 10000, "%s/uncompressed%04d%02d%02d-%02d%02d%02d.fur", dir_path, 
			now_tm->tm_year + 1900, now_tm->tm_mon + 1, now_tm->tm_mday, now_tm->tm_hour, now_tm->tm_min, now_tm->tm_sec); //so we are sure such file does not already exist

		FILE* uncompressed_module = fopen(file_path, "wb");

		if(inflate_furnace_module(f, uncompressed_module) == Z_OK)
		{
			debug("successfully unpacked module");
			fseek(uncompressed_module, 0, SEEK_SET);
			import_furnace_module(uncompressed_module);
			fclose(uncompressed_module);
			//remove(file_path);
		}

		else
		{
			debug("module is corrupt?");
			fclose(uncompressed_module);
			//remove(file_path);

			return 0;
		}
	}
	
	else //module isn't compressed!
	{
		debug("Module is uncompressed, proceeding...");
		fseek(f, 0, SEEK_SET);
		import_furnace_module(f);
	}

	return 1;
}