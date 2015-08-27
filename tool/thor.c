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

// Coding order to display order
static const int cd1[1] = {0};
static const int cd2[2] = {1,0};
static const int cd4[4] = {3,1,0,2};
static const int cd8[8] = {7,3,1,5,0,2,4,6};
static const int cd16[16] = {15,7,3,11,1,5,9,13,0,2,4,6,8,10,12,14};
//static const int* dyadic_reorder_code_to_display[5] = {cd1,cd2,cd4,cd8,cd16};

// Display order to coding order
static const int dc1[1+1] = {-1,0};
static const int dc2[2+1] = {-2,1,0};
static const int dc4[4+1] = {-4,2,1,3,0};
static const int dc8[8+1] = {-8,4,2,5,1,6,3,7,0};
static const int dc16[16+1] = {-16,8,4,9,2,10,5,11,1,12,6,13,3,14,7,15,0};
//static const int* dyadic_reorder_display_to_code[5] = {dc1,dc2,dc4,dc8,dc16};

int print_dec_stats(decoder_info_t* decoder_info)
{
	int i, j;
	uint32_t tot_bits[2] = {0};
	bit_count_t bit_count = decoder_info->bit_count;

	for (i = 0; i < 2; i++)
	{
		tot_bits[i] = bit_count.frame_header[i] + bit_count.super_mode[i] +
				bit_count.intra_mode[i] + bit_count.mv[i] + bit_count.skip_idx[i] +
				bit_count.coeff_y[i] + bit_count.coeff_u[i] + bit_count.coeff_v[i] +
				bit_count.cbp[i] + bit_count.clpf[i];
	}
	tot_bits[0] += bit_count.sequence_header;
	int ni = bit_count.frame_type[0];
	int np = bit_count.frame_type[1];

	if (np==0) np = (1<<30); //Hack to avoid division by zero if there are no P frames

	printf("\n\nBIT STATISTICS:\n");
	printf("Sequence header: %4d\n",bit_count.sequence_header);
	printf("                           I pictures:           P pictures:\n");
	printf("                           total    average      total    average\n");
	printf("Frame header:          %9d  %9d  %9d  %9d\n",bit_count.frame_header[0],bit_count.frame_header[0]/ni,bit_count.frame_header[1],bit_count.frame_header[1]/np);
	printf("Super mode:            %9d  %9d  %9d  %9d\n",bit_count.super_mode[0],bit_count.super_mode[0]/ni,bit_count.super_mode[1],bit_count.super_mode[1]/np);
	printf("Intra mode:            %9d  %9d  %9d  %9d\n",bit_count.intra_mode[0],bit_count.intra_mode[0]/ni,bit_count.intra_mode[1],bit_count.intra_mode[1]/np);
	printf("MV:                    %9d  %9d  %9d  %9d\n",bit_count.mv[0],bit_count.mv[0],bit_count.mv[1],bit_count.mv[1]/np);
	printf("Skip idx:              %9d  %9d  %9d  %9d\n",bit_count.skip_idx[0],bit_count.skip_idx[0],bit_count.skip_idx[1],bit_count.skip_idx[1]/np);
	printf("Coeff_y:               %9d  %9d  %9d  %9d\n",bit_count.coeff_y[0],bit_count.coeff_y[0]/ni,bit_count.coeff_y[1],bit_count.coeff_y[1]/np);
	printf("Coeff_u:               %9d  %9d  %9d  %9d\n",bit_count.coeff_u[0],bit_count.coeff_u[0]/ni,bit_count.coeff_u[1],bit_count.coeff_u[1]/np);
	printf("Coeff_v:               %9d  %9d  %9d  %9d\n",bit_count.coeff_v[0],bit_count.coeff_v[0]/ni,bit_count.coeff_v[1],bit_count.coeff_v[1]/np);
	printf("CBP (TU-split):        %9d  %9d  %9d  %9d\n",bit_count.cbp[0],bit_count.cbp[0]/ni,bit_count.cbp[1],bit_count.cbp[1]/np);
	printf("CLPF:                  %9d  %9d  %9d  %9d\n",bit_count.clpf[0],bit_count.clpf[0]/ni,bit_count.clpf[1],bit_count.clpf[1]/np);
	printf("Total:                 %9d  %9d  %9d  %9d\n",tot_bits[0],tot_bits[0],tot_bits[1],tot_bits[1]/np);
	printf("-----------------------------------------------------------------\n\n");

	printf("PARAMETER STATISTICS:\n");
	printf("                           I pictures:           P pictures:\n");
	printf("                           total    average      total    average\n");
	printf("Skip-blocks (8x8):     %9d  %9d  %9d  %9d\n",bit_count.mode[0][0],bit_count.mode[0][0]/ni,bit_count.mode[1][0],bit_count.mode[1][0]/np);
	printf("Intra-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.mode[0][1],bit_count.mode[0][1]/ni,bit_count.mode[1][1],bit_count.mode[1][1]/np);
	printf("Inter-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.mode[0][2],bit_count.mode[0][2]/ni,bit_count.mode[1][2],bit_count.mode[1][2]/np);
	printf("Bipred-blocks (8x8):   %9d  %9d  %9d  %9d\n",bit_count.mode[0][3],bit_count.mode[0][3]/ni,bit_count.mode[1][3],bit_count.mode[1][3]/np);

	printf("\n");
	printf("8x8-blocks (8x8):      %9d  %9d  %9d  %9d\n",bit_count.size[0][0],bit_count.size[0][0]/ni,bit_count.size[1][0],bit_count.size[1][0]/np);
	printf("16x16-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size[0][1],bit_count.size[0][1]/ni,bit_count.size[1][1],bit_count.size[1][1]/np);
	printf("32x32-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size[0][2],bit_count.size[0][2]/ni,bit_count.size[1][2],bit_count.size[1][2]/np);
	printf("64x64-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size[0][3],bit_count.size[0][3]/ni,bit_count.size[1][3],bit_count.size[1][3]/np);

	printf("\n");
	printf("Mode and size distribution for P- pictures:\n");
	printf("                            SKIP      INTRA      INTER     BIPRED\n");
	printf("8x8-blocks (8x8):      %9d  %9d  %9d  %9d\n",bit_count.size_and_mode[0][0],bit_count.size_and_mode[0][1],bit_count.size_and_mode[0][2],bit_count.size_and_mode[0][3]);
	printf("16x16-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size_and_mode[1][0],bit_count.size_and_mode[1][1],bit_count.size_and_mode[1][2],bit_count.size_and_mode[1][3]);
	printf("32x32-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size_and_mode[2][0],bit_count.size_and_mode[2][1],bit_count.size_and_mode[2][2],bit_count.size_and_mode[2][3]);
	printf("64x64-blocks (8x8):    %9d  %9d  %9d  %9d\n",bit_count.size_and_mode[3][0],bit_count.size_and_mode[3][1],bit_count.size_and_mode[3][2],bit_count.size_and_mode[3][3]);

	int idx;
	int num=9;
	printf("\nSuper-mode distribution for P pictures:\n");
	int index;
	for (index=0;index<1;index++)
	{
		for (idx=0;idx<4;idx++)
		{
			int size = 8<<idx;
			printf("%2d x %2d-blocks (8x8): ",size,size);
			for (i=0;i<num;i++)
			{
				printf("%8d",bit_count.super_mode_stat[index][idx][i]/1);
			}
			printf("\n");
		}
	}

	printf("\n");
	printf("Ref_idx and size distribution for P pictures:\n");
	int size;
	int max_num_ref = 4;

	for (i=0;i<4;i++)
	{
		size = 1<<(i+3);
		printf("%2d x %2d-blocks: ",size,size);
		for (j=0;j<max_num_ref;j++)
		{
			printf("%6d",bit_count.size_and_ref_idx[i][j]);
		}
		printf("\n");
	}

	{
		int sum = 0;
		printf("\nbi-ref:  ");
		for (j=0;j<max_num_ref*max_num_ref;j++)
		{
			sum += bit_count.bi_ref[j];
			printf("%7d",bit_count.bi_ref[j]);
		}
		printf("\n");
	}

	printf("-----------------------------------------------------------------\n");

	return 1;
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

	thor_decode(dec, &buffer[8], size - 8, pDst, dstStep);

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

	size = thor_encode(enc, pSrc, srcStep, &buffer[8], size - 8) + 8;

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
