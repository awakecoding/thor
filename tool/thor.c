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

#include <thor/thor.h>
#include <thor/color.h>
#include <thor/image.h>
#include <thor/settings.h>

#ifndef _WIN32

#include <time.h>
#include <sys/time.h>

#define _strdup strdup
#define	 sprintf_s snprintf

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

int main_enc(int argc, char **argv)
{
	int width;
	int height;
	FILE* output;
	int srcStep[3];
	uint8_t* buffer;
	uint32_t size;
	uint8_t* pSrc[3];
	uint8_t hdrbuf[8];
	thor_image_t img;
	uint32_t thorSize;
	int frame_index = 0;
	int total_time = 0;
	int total_size = 0;
	thor_encoder_t* enc;
	uint32_t beg, end, diff;
	thor_frame_header_t fhdr;
	thor_sequence_header_t shdr;
	thor_encoder_settings_t* settings;
	const char* input_file;
	const char* output_file;

	memset(&shdr, 0, sizeof(shdr));
	memset(&fhdr, 0, sizeof(fhdr));

	/* Read commands from command line and from configuration file(s) */
	if (argc < 3)
	{
		fprintf(stdout, "usage: %s <parameters>\n", argv[0]);
		return -1;
	}

	settings = thor_encoder_settings_new(argc, argv);

	if (!settings)
	{
		fprintf(stderr, "Error while reading encoder parameters.");
		return -1;
	}

	if (thor_encoder_settings_validate(settings) < 0)
		return -1;

	input_file = settings->infilestr;
	output_file = settings->outfilestr;

	thor_image_read(&img, input_file);
	width = img.width;
	height = img.height;

	enc = thor_encoder_new(settings);

	settings->width = width;
	settings->height = height;

	size = width * height * 4;
	buffer = (uint8_t*) malloc(size);

	srcStep[0] = width;
	srcStep[1] = width / 2;
	srcStep[2] = width / 2;
	pSrc[0] = (uint8_t*) malloc(height * srcStep[0] * sizeof(uint8_t));
	pSrc[1] = (uint8_t*) malloc(height / 2 * srcStep[1] * sizeof(uint8_t));
	pSrc[2] = (uint8_t*) malloc(height / 2 * srcStep[2] * sizeof(uint8_t));

	/* Read input frame */

	if ((img.type == THOR_IMAGE_PNG) || (img.type == THOR_IMAGE_BMP))
	{
		thor_RGBToYUV420_8u_P3AC4R(img.data, img.scanline, pSrc, srcStep, width, height);
		free(img.data);
	}

	output = fopen(output_file, "wb");

	if (!output)
		return -1;

	shdr.width = width;
	shdr.height = height;
	shdr.pb_split_enable = settings->enable_pb_split;
	shdr.tb_split_enable = settings->enable_tb_split;
	shdr.max_num_ref = settings->max_num_ref;
	shdr.num_reorder_pics = settings->num_reorder_pics;
	shdr.max_delta_qp = settings->max_delta_qp;
	shdr.deblocking = settings->deblocking;
	shdr.clpf = settings->clpf;
	shdr.use_block_contexts = settings->use_block_contexts;
	shdr.enable_bipred = settings->enable_bipred;

	//fprintf(stderr, "width: %d height: %d pb_split_enable: %d tb_split_enable: %d max_num_ref: %d max_num_reorder_pics: %d max_delta_qp: %d deblocking: %d clpf: %d use_block_contexts: %d enable_bipred: %d\n",
	//	shdr.width, shdr.height, shdr.pb_split_enable, shdr.tb_split_enable, shdr.max_num_ref, shdr.num_reorder_pics, shdr.max_delta_qp, shdr.deblocking, shdr.clpf, shdr.use_block_contexts, shdr.enable_bipred);

	thor_encoder_set_sequence_header(enc, &shdr);

	thor_write_sequence_header(hdrbuf, &shdr);

	if (fwrite(hdrbuf, 1, 8, output) != 8)
		return -1;

	if (img.type == THOR_IMAGE_Y4M)
	{
		for (frame_index = 0; frame_index < img.y4m_frame_count; frame_index++)
		{
			thor_y4m_read_frame(&img, pSrc, srcStep);

			beg = thor_get_tick_count();
			thorSize = thor_encode(enc, pSrc, srcStep, buffer, size);
			end = thor_get_tick_count();

			diff = end - beg;
			fhdr.size = thorSize;

			fprintf(stderr, "thor_encode[%03d]: %d ms %d KB\n",
				frame_index, diff, thorSize / 1024);

			total_time += diff;
			total_size += fhdr.size;

			thor_write_frame_header(hdrbuf, &fhdr);

			if (fwrite(hdrbuf, 1, 8, output) != 8)
				return -1;

			if (fwrite(buffer, 1, thorSize, output) != thorSize)
				return -1;
		}

		fclose(img.fp);
	}
	else
	{
		beg = thor_get_tick_count();
		thorSize = thor_encode(enc, pSrc, srcStep, buffer, size);
		end = thor_get_tick_count();

		diff = end - beg;
		fhdr.size = thorSize;

		fprintf(stderr, "thor_encode[%03d]: %d ms %d KB\n",
			0, diff, thorSize / 1024);

		total_time += diff;
		total_size += fhdr.size;

		thor_write_frame_header(hdrbuf, &fhdr);

		if (fwrite(hdrbuf, 1, 8, output) != 8)
			return -1;

		if (fwrite(buffer, 1, thorSize, output) != thorSize)
			return -1;

		frame_index++;
	}

	fclose(output);

	free(buffer);

	free(pSrc[0]);
	free(pSrc[1]);
	free(pSrc[2]);

	fprintf(stderr, "thor_encode: total: %d ms %d KB average: %d ms %d KB %f fps\n",
		total_time, total_size / 1024, total_time / frame_index, (total_size / frame_index) / 1024,
		1000.0f / ((float) total_time / (float) frame_index));

	thor_encoder_free(enc);

	return 0;
}

int main_dec(int argc, char** argv)
{
	int width;
	int height;
	int status;
	int index = 0;
	int dstStep[3];
	uint8_t* pDst[3];
	uint8_t* buffer;
	uint32_t size;
	uint8_t hdrbuf[8];
	FILE* input = NULL;
	thor_decoder_t* dec;
	thor_image_t* img;
	char* name_base = NULL;
	char* name_ext = NULL;
	const char* input_file;
	const char* output_file;
	uint32_t beg, end, diff;
	thor_frame_header_t fhdr;
	thor_sequence_header_t shdr;

	dec = thor_decoder_new(NULL);

	if (argc < 3)
	{
		fprintf(stdout, "usage: %s infile outfile\n", argv[0]);
		return -1;
	}

	input_file = argv[1];
	output_file = argv[2];

	img = thor_image_new();

	if (strstr(output_file, ".png"))
		img->type = THOR_IMAGE_PNG;
	else if (strstr(output_file, ".bmp"))
		img->type = THOR_IMAGE_BMP;
	else if (strstr(output_file, ".y4m"))
		img->type = THOR_IMAGE_Y4M;

	input = fopen(input_file, "rb");

	if (fread(hdrbuf, 1, 8, input) != 8)
		return -1;

	thor_read_sequence_header(hdrbuf, &shdr);

	width = shdr.width;
	height = shdr.height;

	size = width * height * 4;
	buffer = (uint8_t*) malloc(size);

	if ((img->type == THOR_IMAGE_PNG) || (img->type == THOR_IMAGE_BMP))
	{
		char* p;

		img->scanline = width * 4;
		img->data = (uint8_t*) malloc(img->scanline * height);

		name_base = _strdup(output_file);
		p = strchr(name_base, '.');
		*p = '\0';

		name_ext = p + 1;
	}

	dstStep[0] = width;
	dstStep[1] = width / 2;
	dstStep[2] = width / 2;
	pDst[0] = (uint8_t*) malloc(height * dstStep[0] * sizeof(uint8_t));
	pDst[1] = (uint8_t*) malloc(height / 2 * dstStep[1] * sizeof(uint8_t));
	pDst[2] = (uint8_t*) malloc(height / 2 * dstStep[2] * sizeof(uint8_t));

	thor_decoder_set_sequence_header(dec, &shdr);

	while (1)
	{
		if (fread(hdrbuf, 1, 8, input) != 8)
			break;

		thor_read_frame_header(hdrbuf, &fhdr);

		status = fread(buffer, 1, fhdr.size, input);

		if (status != fhdr.size)
		{
			fprintf(stderr, "incomplete frame data: actual: %d, expected: %d\n", status, fhdr.size);
			break;
		}

		beg = thor_get_tick_count();
		thor_decode(dec, buffer, fhdr.size, pDst, dstStep);
		end = thor_get_tick_count();

		diff = end - beg;

		fprintf(stderr, "thor_decode[%03d]: %d ms\n", index, diff);

		if ((img->type == THOR_IMAGE_PNG) || (img->type == THOR_IMAGE_BMP))
		{
			char frame_file[256];

			img->width = width;
			img->height = height;
			img->bitsPerPixel = 32;
			img->bytesPerPixel = 4;

			thor_YUV420ToRGB_8u_P3AC4R((const uint8_t**) pDst, dstStep, img->data, img->scanline, width, height);

			sprintf_s(frame_file, sizeof(frame_file), "%s_%03d.%s", name_base, index, name_ext);

			thor_image_write(img, frame_file);
		}
		else if (img->type == THOR_IMAGE_Y4M)
		{
			img->width = width;
			img->height = height;

			thor_image_write(img, output_file);
			thor_y4m_write_frame(img, pDst, dstStep);
		}

		index++;
	}

	thor_image_free(img, 1);
	free(name_base);

	free(pDst[0]);
	free(pDst[1]);
	free(pDst[2]);

	free(buffer);
	fclose(input);

	thor_decoder_free(dec);

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
