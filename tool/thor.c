/*
Copyright (c) 2015, Cisco Systems
All rights reserved.

Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice, this
  list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice, this
  list of conditions and the following disclaimer in the documentation and/or
  other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "thor.h"

#include <thor/thor.h>
#include <thor/color.h>

#include "image.h"
#include "params.h"

void rferror(char error_text[])
{
	fprintf(stderr,"Run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	exit(1);
}

#ifndef _WIN32

#include <time.h>

#ifndef CLOCK_MONOTONIC_RAW
#define CLOCK_MONOTONIC_RAW	4
#endif

#endif

uint32_t thor_get_tick_count(void)
{
	uint32_t ticks = 0;

#ifdef _WIN32
	ticks = GetTickCount();
#elif defined(__linux__)
	struct timespec ts;

	if (!clock_gettime(CLOCK_MONOTONIC_RAW, &ts))
		ticks = (ts.tv_sec * 1000) + (ts.tv_nsec / 1000000);
#else
	struct timeval tv;

	if (!gettimeofday(&tv, NULL))
		ticks = (tv.tv_sec * 1000) + (tv.tv_usec / 1000);
#endif

	return ticks;
}

int main_dec(int argc, char** argv)
{
	int width;
	int height;
	int dstStep[3];
	uint8_t* pDst[3];
	uint8_t* buffer;
	uint32_t size;
	uint32_t lumaSize;
	uint32_t chromaSize;
	FILE* infile = NULL;
	FILE* outfile = NULL;
	thor_decoder_t* dec;
	uint32_t beg, end, diff;
	thor_sequence_header_t hdr;

	dec = thor_decoder_new();

	if (argc < 3)
	{
		fprintf(stdout, "usage: %s infile outfile\n", argv[0]);
		rferror("Wrong number of arguments.");
	}

	if (!(infile = fopen(argv[1], "rb")))
	{
		fprintf(stderr, "Could not open in-file for reading: %s\n", argv[1]);
		rferror("");
	}

	fseek(infile, 0, SEEK_END);
	size = ftell(infile);
	fseek(infile, 0, SEEK_SET);
	buffer = (uint8_t*) malloc(size);
	fread(buffer, 1, size, infile);
	fclose(infile);

	thor_read_sequence_header(buffer, &hdr);

	fprintf(stderr, "width: %d height: %d pb_split: %d tb_split: %d max_num_ref: %d max_reorder_pics: %d max_delta_qp: %d deblocking: %d clpf: %d block_contexts: %d bipred: %d\n",
		hdr.width, hdr.height, hdr.pb_split_enable, hdr.tb_split_enable,
		hdr.max_num_ref, hdr.num_reorder_pics, hdr.max_delta_qp, hdr.deblocking,
		hdr.clpf, hdr.use_block_contexts, hdr.enable_bipred);

	width = hdr.width;
	height = hdr.height;

	dstStep[0] = width;
	dstStep[1] = width / 2;
	dstStep[2] = width / 2;
	pDst[0] = (uint8_t*) malloc(height * dstStep[0] * sizeof(uint8_t));
	pDst[1] = (uint8_t*) malloc(height / 2 * dstStep[1] * sizeof(uint8_t));
	pDst[2] = (uint8_t*) malloc(height / 2 * dstStep[2] * sizeof(uint8_t));

	thor_decoder_set_sequence_header(dec, &hdr);

	beg = thor_get_tick_count();
	thor_decode(dec, &buffer[8], size - 8, pDst, dstStep);
	end = thor_get_tick_count();

	diff = end - beg;
	fprintf(stderr, "thor_decode: %d ms\n", diff);

	if (strstr(argv[2], ".png"))
	{
		int rgbStep;
		uint8_t* pRgb;

		rgbStep = width * 4;
		pRgb = (uint8_t*) malloc(rgbStep * height);

		thor_YUV420ToRGB_8u_P3AC4R((const uint8_t**) pDst, dstStep, pRgb, rgbStep, width, height);

		thor_png_write(argv[2], pRgb, width, height, 32);

		free(pRgb);
	}
	else
	{
		lumaSize = width * height;
		chromaSize = lumaSize / 4;

		if (!(outfile = fopen(argv[2], "wb")))
		{
			fprintf(stderr, "Could not open out-file for writing.");
			rferror("");
		}

		fprintf(outfile, "YUV4MPEG2 W%d H%d F30:1 Ip A1:1\n", width, height);
		fprintf(outfile, "FRAME\n");

		if (fwrite(pDst[0], 1, lumaSize, outfile) != lumaSize)
		{
			fatalerror("Error writing Y to file");
		}
		if (fwrite(pDst[1], 1, chromaSize, outfile) != chromaSize)
		{
			fatalerror("Error writing U to file");
		}
		if (fwrite(pDst[2], 1, chromaSize, outfile) != chromaSize)
		{
			fatalerror("Error writing V to file");
		}
	}

	free(pDst[0]);
	free(pDst[1]);
	free(pDst[2]);

	free(buffer);

	thor_decoder_free(dec);

	return 0;
}

int main_enc(int argc, char **argv)
{
	int width;
	int height;
	FILE* infile;
	FILE* strfile;
	int srcStep[3];
	uint8_t* buffer;
	uint32_t size;
	uint8_t* pSrc[3];
	uint32_t lumaSize;
	uint32_t chromaSize;
	thor_image_t img;
	enc_params* params;
	thor_encoder_t* enc;
	uint32_t beg, end, diff;
	thor_sequence_header_t hdr;

	enc = thor_encoder_new();

	/* Read commands from command line and from configuration file(s) */
	if (argc < 3)
	{
		fprintf(stdout,"usage: %s <parameters>\n",argv[0]);
		fatalerror("");
	}
	params = parse_config_params(argc, argv);
	if (params == NULL)
	{
		fatalerror("Error while reading encoder parameters.");
	}
	check_parameters(params);

	if (strstr(params->infilestr, ".png"))
	{
		thor_image_read(&img, params->infilestr);
		params->width = img.width;
		params->height = img.height;
	}

	width = params->width;
	height = params->height;

	size = MAX_BUFFER_SIZE;
	buffer = (uint8_t*) malloc(size);

	srcStep[0] = width;
	srcStep[1] = width / 2;
	srcStep[2] = width / 2;
	pSrc[0] = (uint8_t*) malloc(height * srcStep[0] * sizeof(uint8_t));
	pSrc[1] = (uint8_t*) malloc(height / 2 * srcStep[1] * sizeof(uint8_t));
	pSrc[2] = (uint8_t*) malloc(height / 2 * srcStep[2] * sizeof(uint8_t));

	/* Read input frame */

	if (strstr(params->infilestr, ".png"))
	{
		thor_RGBToYUV420_8u_P3AC4R(img.data, img.scanline, pSrc, srcStep, width, height);
		free(img.data);
	}
	else
	{
		if (!(infile = fopen(params->infilestr, "rb")))
		{
			fatalerror("Could not open in-file for reading.");
		}

		fseek(infile, params->file_headerlen + params->frame_headerlen, SEEK_SET);

		lumaSize = width * height;
		chromaSize = lumaSize / 4;

		if (fread(pSrc[0], 1, lumaSize, infile) != lumaSize)
		{
			fatalerror("Error reading Y from file");
		}
		if (fread(pSrc[1], 1, chromaSize, infile) != chromaSize)
		{
			fatalerror("Error reading U from file");
		}
		if (fread(pSrc[2], 1, chromaSize, infile) != chromaSize)
		{
			fatalerror("Error reading V from file");
		}

		fclose(infile);
	}

	hdr.width = width;
	hdr.height = height;
	hdr.pb_split_enable = params->enable_pb_split;
	hdr.tb_split_enable = params->enable_tb_split;
	hdr.max_num_ref = params->max_num_ref;
	hdr.num_reorder_pics = params->num_reorder_pics;
	hdr.max_delta_qp = params->max_delta_qp;
	hdr.deblocking = params->deblocking;
	hdr.clpf = params->clpf;
	hdr.use_block_contexts = params->use_block_contexts;
	hdr.enable_bipred = params->enable_bipred;

	thor_encoder_set_sequence_header(enc, &hdr);

	thor_write_sequence_header(buffer, &hdr);

	beg = thor_get_tick_count();
	size = thor_encode(enc, pSrc, srcStep, &buffer[8], size - 8) + 8;
	end = thor_get_tick_count();

	diff = end - beg;
	fprintf(stderr, "thor_encode: %d ms\n", diff);

	if (!(strfile = fopen(params->outfilestr, "wb")))
	{
		fatalerror("Could not open out-file for writing.");
	}

	if (strfile)
	{
		if (fwrite(buffer, 1, size, strfile) != size)
		{
			fatalerror("Problem writing bitstream to file.");
		}
	}

	fclose(strfile);

	free(buffer);

	delete_config_params(params);

	free(pSrc[0]);
	free(pSrc[1]);
	free(pSrc[2]);

	thor_encoder_free(enc);

	return 0;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		fprintf(stderr, "usage: thor <enc|dec> <options>\n");
		return 0;
	}

	if (strcmp(argv[1], "enc") == 0)
	{
		argv[1] = argv[0];
		return main_enc(argc - 1, &argv[1]);
	}
	else
	{
		argv[1] = argv[0];
		return main_dec(argc - 1, &argv[1]);
	}

	return 0;
}
