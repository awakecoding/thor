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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include <thor.h>

#include "image.h"
#include "color.h"
#include "frame.h"
#include "bits.h"
#include "simd.h"
#include "params.h"
#include "snr.h"

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
static const int* dyadic_reorder_code_to_display[5] = {cd1,cd2,cd4,cd8,cd16};

// Display order to coding order
static const int dc1[1+1] = {-1,0};
static const int dc2[2+1] = {-2,1,0};
static const int dc4[4+1] = {-4,2,1,3,0};
static const int dc8[8+1] = {-8,4,2,5,1,6,3,7,0};
static const int dc16[16+1] = {-16,8,4,9,2,10,5,11,1,12,6,13,3,14,7,15,0};
static const int* dyadic_reorder_display_to_code[5] = {dc1,dc2,dc4,dc8,dc16};

static int reorder_frame_offset(int idx, int sub_gop)
{
#if DYADIC_CODING
	return dyadic_reorder_code_to_display[log2i(sub_gop)][idx]-sub_gop+1;
#else
	if (idx==0) return 0;
	else return idx-sub_gop;
#endif
}

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

void thor_read_sequence_header(uint8_t* buffer, thor_sequence_header_t* hdr)
{
	stream_t stream;

	stream.incnt = 0;
	stream.bitcnt = 0;
	stream.capacity = 8;
	stream.rdbfr = buffer;
	stream.rdptr = stream.rdbfr;

	hdr->width = getbits(&stream, 16);
	hdr->height = getbits(&stream, 16);
	hdr->pb_split_enable = getbits(&stream, 1);
	hdr->tb_split_enable = getbits(&stream, 1);
	hdr->max_num_ref = getbits(&stream, 2) + 1;
	hdr->num_reorder_pics = getbits(&stream, 4);
	hdr->max_delta_qp = getbits(&stream, 2);
	hdr->deblocking = getbits(&stream, 1);
	hdr->clpf = getbits(&stream, 1);
	hdr->use_block_contexts = getbits(&stream, 1);
	hdr->enable_bipred = getbits(&stream, 1);
	getbits(&stream, 18); /* pad */
}

void thor_write_sequence_header(uint8_t* buffer, thor_sequence_header_t* hdr)
{
	stream_t stream;

	stream.bitstream = buffer;
	stream.bytepos = 0;
	stream.bitrest = 32;
	stream.bitbuf = 0;

	putbits(16, hdr->width, &stream);
	putbits(16, hdr->height, &stream);
	putbits(1, hdr->pb_split_enable, &stream);
	putbits(1, hdr->tb_split_enable, &stream);
	putbits(2, hdr->max_num_ref-1, &stream);
	putbits(4, hdr->num_reorder_pics, &stream);
	putbits(2, hdr->max_delta_qp, &stream);
	putbits(1, hdr->deblocking, &stream);
	putbits(1, hdr->clpf, &stream);
	putbits(1, hdr->use_block_contexts, &stream);
	putbits(1, hdr->enable_bipred, &stream);
	putbits(18, 0, &stream); /* pad */

	flush_bitbuf(&stream);
}

int thor_decode(thor_sequence_header_t* hdr, uint8_t* pSrc, uint32_t srcSize, uint8_t* pDst[3], int dstStep[3])
{
	int r;
	int width;
	int height;
	stream_t stream;
	yuv_frame_t frame;
	int decode_frame_num = 0;
	decoder_info_t dec_info;
	yuv_frame_t ref[MAX_REF_FRAMES];

	init_use_simd();

	memset(&dec_info, 0, sizeof(dec_info));

	width = hdr->width;
	height = hdr->height;

	stream.incnt = 0;
	stream.bitcnt = 0;
	stream.capacity = srcSize;
	stream.rdbfr = pSrc;
	stream.rdptr = stream.rdbfr;
	dec_info.stream = &stream;

	frame.width = width;
	frame.height = height;
	frame.stride_y = dstStep[0];
	frame.stride_c = dstStep[1];
	frame.offset_y = 0;
	frame.offset_c = 0;
	frame.y = pDst[0];
	frame.u = pDst[1];
	frame.v = pDst[2];

	dec_info.width = width;
	dec_info.height = height;
	dec_info.pb_split_enable = hdr->pb_split_enable;
	dec_info.tb_split_enable = hdr->tb_split_enable;
	dec_info.max_num_ref = hdr->max_num_ref;
	dec_info.num_reorder_pics = hdr->num_reorder_pics;
	dec_info.max_delta_qp = hdr->max_delta_qp;
	dec_info.deblocking = hdr->deblocking;
	dec_info.clpf = hdr->clpf;
	dec_info.use_block_contexts = hdr->use_block_contexts;
	dec_info.bipred = hdr->enable_bipred;
	dec_info.bit_count.sequence_header = 64;

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		create_yuv_frame(&ref[r], width, height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
		dec_info.ref[r] = &ref[r];
	}

	dec_info.deblock_data = (deblock_data_t *) malloc((height / MIN_PB_SIZE) * (width / MIN_PB_SIZE) * sizeof(deblock_data_t));

	dec_info.frame_info.decode_order_frame_num = 0;
	dec_info.frame_info.display_frame_num = 0;

	dec_info.rec = &frame;
	dec_info.frame_info.num_ref = min(decode_frame_num, dec_info.max_num_ref);
	dec_info.rec->frame_num = dec_info.frame_info.display_frame_num;

	decode_frame(&dec_info);

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		close_yuv_frame(&ref[r]);
	}

	free(dec_info.deblock_data);

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
	uint32_t ysize;
	uint32_t csize;
	FILE* infile = NULL;
	FILE* outfile = NULL;
	thor_sequence_header_t hdr;

	if (argc < 2)
	{
		fprintf(stdout, "usage: %s infile [outfile]\n", argv[0]);
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

	thor_decode(&hdr, &buffer[8], size - 8, pDst, dstStep);

	if (argc > 2)
	{
		if (!(outfile = fopen(argv[2], "wb")))
		{
			fprintf(stderr, "Could not open out-file for writing.");
			rferror("");
		}
	}

	ysize = width * height;
	csize = ysize / 4;

	if (1)
	{
		int rgbStep;
		uint8_t* pRgb;

		rgbStep = width * 4;
		pRgb = (uint8_t*) malloc(rgbStep * height);

		thor_YUV420ToRGB_8u_P3AC4R((const uint8_t**) pDst, dstStep, pRgb, rgbStep, width, height);

		thor_png_write("test.png", pRgb, width, height, 32);

		free(pRgb);
	}

	fprintf(outfile, "YUV4MPEG2 W%d H%d F30:1 Ip A1:1\n", width, height);
	fprintf(outfile, "FRAME\n");

	if (fwrite(pDst[0], 1, ysize, outfile) != ysize)
	{
		fatalerror("Error writing Y to file");
	}
	if (fwrite(pDst[1], 1, csize, outfile) != csize)
	{
		fatalerror("Error writing U to file");
	}
	if (fwrite(pDst[2], 1, csize, outfile) != csize)
	{
		fatalerror("Error writing V to file");
	}

	free(pDst[0]);
	free(pDst[1]);
	free(pDst[2]);

	free(buffer);

	return 0;
}

int main_enc(int argc, char **argv)
{
	FILE* infile;
	FILE* strfile;
	uint32_t input_file_size;
	thor_sequence_header_t hdr;
	yuv_frame_t orig,ref[MAX_REF_FRAMES];
	yuv_frame_t rec[MAX_REORDER_BUFFER];
	int num_encoded_frames;
	int num_bits;
	int start_bits;
	int end_bits;
	int sub_gop=1;
	int rec_buffer_idx;
	int frame_num,frame_num0,k,r;
	int frame_offset;
	int ysize,csize;
	int frame_size;
	int width,height;
	int input_stride_y;
	int input_stride_c;
	uint32_t acc_num_bits;
	snrvals psnr;
	snrvals accsnr;
	stream_t stream;
	double bit_rate_in_kbps;
	enc_params *params;
	encoder_info_t encoder_info;

	init_use_simd();

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

	/* Open files */
	if (!(infile = fopen(params->infilestr,"rb")))
	{
		fatalerror("Could not open in-file for reading.");
	}
	if (!(strfile = fopen(params->outfilestr,"wb")))
	{
		fatalerror("Could not open out-file for writing.");
	}

	fseek(infile, 0, SEEK_END);
	input_file_size = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	accsnr.y = 0;
	accsnr.u = 0;
	accsnr.v = 0;
	acc_num_bits = 0;

	height = params->height;
	width = params->width;
	input_stride_y = width;
	input_stride_c = width/2;
	ysize = height * width;
	csize = ysize / 4;
	frame_size = ysize + 2*csize;

	/* Create frames*/
	create_yuv_frame(&orig, width, height, 0, 0, 0, 0);

	for (r = 0; r < MAX_REORDER_BUFFER; r++)
	{
		create_yuv_frame(&rec[r], width, height, 0, 0, 0, 0);
	}

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		create_yuv_frame(&ref[r], width, height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
	}

	/* Initialize main bit stream */
	stream.bitstream = (uint8_t*) malloc(MAX_BUFFER_SIZE * sizeof(uint8_t));
	stream.bitbuf = 0;
	stream.bitrest = 32;
	stream.bytepos = 0;
	stream.bytesize = MAX_BUFFER_SIZE;

	/* Configure encoder */
	encoder_info.params = params;
	encoder_info.orig = &orig;

	for (r = 0;r < MAX_REF_FRAMES; r++)
	{
		encoder_info.ref[r] = &ref[r];
	}

	encoder_info.stream = &stream;
	encoder_info.width = width;
	encoder_info.height = height;

	encoder_info.deblock_data = (deblock_data_t*) malloc((height/MIN_PB_SIZE) * (width/MIN_PB_SIZE) * sizeof(deblock_data_t));

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

	thor_write_sequence_header(stream.bitstream, &hdr);

	stream.bytepos += 8;

	num_bits = get_bit_pos(&stream);
	acc_num_bits += num_bits;
	printf("SH:  %4d bits\n",num_bits);

	/* Start encoding sequence */
	num_encoded_frames = 0;
	sub_gop = max(1,params->num_reorder_pics+1);
	for (frame_num0 = params->skip; frame_num0 < (params->skip + params->num_frames) && (frame_num0+sub_gop)*frame_size <= input_file_size; frame_num0+=sub_gop)
	{
		for (k=0; k<sub_gop; k++)
		{
			int r,r0,r1,r2,r3;
			/* Initialize frame info */
			frame_offset = reorder_frame_offset(k,sub_gop);
			frame_num = frame_num0 + frame_offset;
			// If there is an initial I frame and reordering need to jump to the next P frame
			if (frame_num < params->skip)
				continue;

			encoder_info.frame_info.frame_num = frame_num - params->skip;
			rec_buffer_idx = encoder_info.frame_info.frame_num%MAX_REORDER_BUFFER;
			encoder_info.rec = &rec[rec_buffer_idx];
			encoder_info.rec->frame_num = encoder_info.frame_info.frame_num;

			if (params->num_reorder_pics==0)
			{
				if (params->intra_period > 0)
					encoder_info.frame_info.frame_type = ((num_encoded_frames%params->intra_period) == 0 ? I_FRAME : P_FRAME);
				else
					encoder_info.frame_info.frame_type = (num_encoded_frames == 0 ? I_FRAME : P_FRAME);
			}
			else
			{
				if (params->intra_period > 0)
					encoder_info.frame_info.frame_type = ((encoder_info.frame_info.frame_num%params->intra_period) == 0 ? I_FRAME :
							((encoder_info.frame_info.frame_num%sub_gop)==0 ? P_FRAME : B_FRAME));
				else
					encoder_info.frame_info.frame_type = (encoder_info.frame_info.frame_num == 0 ? I_FRAME :
							((encoder_info.frame_info.frame_num%sub_gop)==0 ? P_FRAME : B_FRAME));
			}

			int coded_phase = (num_encoded_frames + sub_gop - 2) % sub_gop + 1;
			int b_level = log2i(coded_phase);

			if (encoder_info.frame_info.frame_type == I_FRAME)
			{
				encoder_info.frame_info.qp = params->qp + params->dqpI;
			}
			else if (params->num_reorder_pics==0)
			{
				if (num_encoded_frames % params->HQperiod)
					encoder_info.frame_info.qp = (int)(params->mqpP*(float)params->qp) + params->dqpP;
				else
					encoder_info.frame_info.qp = params->qp;
			}
			else
			{
				if (encoder_info.frame_info.frame_num % sub_gop)
				{
					float mqpB = params->mqpB;
#if DYADIC_CODING
					mqpB = 1.0+(b_level+1)*((mqpB-1.0)/2.0);
#endif
					encoder_info.frame_info.qp = (int)(mqpB*(float)params->qp) + params->dqpB;
				}
				else
				{
					encoder_info.frame_info.qp = params->qp;
				}
			}

			encoder_info.frame_info.num_ref = min(num_encoded_frames,params->max_num_ref);

			if (params->num_reorder_pics > 0)
			{
#if DYADIC_CODING
				/* if we have a P frame then use the previous P frame as a reference */
				if ((num_encoded_frames-1) % sub_gop == 0)
				{
					if (num_encoded_frames==1)
						encoder_info.frame_info.ref_array[0] = 0;
					else
						encoder_info.frame_info.ref_array[0] = sub_gop-1;
					if (encoder_info.frame_info.num_ref>1 )
						encoder_info.frame_info.ref_array[1] = min(MAX_REF_FRAMES-1,min(num_encoded_frames-1,2*sub_gop-1));

					for (r = 2;r < encoder_info.frame_info.num_ref; r++)
					{
						encoder_info.frame_info.ref_array[r] = r-1;
					}

				}
				else
				{
					int display_phase =  (encoder_info.frame_info.frame_num-1) % sub_gop;
					int ref_offset=sub_gop>>(b_level+1);

					encoder_info.frame_info.ref_array[0]=min(num_encoded_frames-1,coded_phase-dyadic_reorder_display_to_code[log2i(sub_gop)][display_phase-ref_offset+1]-1);
					encoder_info.frame_info.ref_array[1]=min(num_encoded_frames-1,coded_phase-dyadic_reorder_display_to_code[log2i(sub_gop)][display_phase+ref_offset+1]-1);
					/* use most recent frames for the last ref(s)*/
					for (r = 2; r < encoder_info.frame_info.num_ref; r++)
					{
						encoder_info.frame_info.ref_array[r] = r-2;
					}
				}
#else
				/* if we have a P frame then use the previous P frame as a reference */
				if ((num_encoded_frames-1) % sub_gop == 0)
				{
					if (num_encoded_frames==1)
						encoder_info.frame_info.ref_array[0] = 0;
					else
						encoder_info.frame_info.ref_array[0] = sub_gop-1;
					if (encoder_info.frame_info.num_ref>1 )
						encoder_info.frame_info.ref_array[1] = min(MAX_REF_FRAMES-1,min(num_encoded_frames-1,2*sub_gop-1));

					for (r=2;r<encoder_info.frame_info.num_ref;r++){
						encoder_info.frame_info.ref_array[r] = r-1;
					}

				}
				else
				{
					// Use the last encoded frame as the first ref
					if (encoder_info.frame_info.num_ref>0)
					{
						encoder_info.frame_info.ref_array[0] = 0;
					}
					/* Use the subsequent P frame as the 2nd ref */
					int phase = (num_encoded_frames + sub_gop - 2) % sub_gop;

					if (encoder_info.frame_info.num_ref>1)
					{
						if (phase==0)
							encoder_info.frame_info.ref_array[1] = min(sub_gop, num_encoded_frames-1);
						else
							encoder_info.frame_info.ref_array[1] = min(phase, num_encoded_frames-1);
					}
					/* Use the prior P frame as the 3rd ref */
					if (encoder_info.frame_info.num_ref>2)
					{
						encoder_info.frame_info.ref_array[2] = min(phase ? phase + sub_gop : 2*sub_gop, num_encoded_frames-1);
					}
					/* use most recent frames for the last ref(s)*/
					for (r=3;r<encoder_info.frame_info.num_ref;r++)
					{
						encoder_info.frame_info.ref_array[r] = r-3+1;
					}
				}

#endif
			}
			else
			{
				if (encoder_info.frame_info.num_ref==1)
				{
					/* If num_ref==1 always use most recent frame */
					encoder_info.frame_info.ref_array[0] = 0;
				}
				else if (encoder_info.frame_info.num_ref==2)
				{
					/* If num_ref==2 use most recent LQ frame and most recent HQ frame */
					r0 = 0;
					r1 = ((num_encoded_frames + params->HQperiod - 2) % params->HQperiod) + 1;
					encoder_info.frame_info.ref_array[0] = r0;
					encoder_info.frame_info.ref_array[1] = r1;
				}
				else if (encoder_info.frame_info.num_ref==3)
				{
					r0 = 0;
					r1 = ((num_encoded_frames + params->HQperiod - 2) % params->HQperiod) + 1;
					r2 = r1==1 ? 2 : 1;
					encoder_info.frame_info.ref_array[0] = r0;
					encoder_info.frame_info.ref_array[1] = r1;
					encoder_info.frame_info.ref_array[2] = r2;
				}
				else if (encoder_info.frame_info.num_ref==4)
				{
					r0 = 0;
					r1 = ((num_encoded_frames + params->HQperiod - 2) % params->HQperiod) + 1;
					r2 = r1==1 ? 2 : 1;
					r3 = r2+1;
					if (r3==r1) r3 += 1;
					encoder_info.frame_info.ref_array[0] = r0;
					encoder_info.frame_info.ref_array[1] = r1;
					encoder_info.frame_info.ref_array[2] = r2;
					encoder_info.frame_info.ref_array[3] = r3;
				}
				else
				{
					for (r = 0; r <encoder_info.frame_info.num_ref; r++)
					{
						encoder_info.frame_info.ref_array[r] = r;
					}
				}
			}

			if (params->intra_rdo)
			{
				if (encoder_info.frame_info.frame_type == I_FRAME)
				{
					encoder_info.frame_info.num_intra_modes = 10;
				}
				else
				{
					encoder_info.frame_info.num_intra_modes = params->encoder_speed > 0 ? 4 : 10;
				}
			}
			else
			{
				encoder_info.frame_info.num_intra_modes = 4;
			}

			/* Read input frame */
			fseek(infile, frame_num*(frame_size+params->frame_headerlen)+params->file_headerlen+params->frame_headerlen, SEEK_SET);
			read_yuv_frame(&orig, width, height, infile);
			orig.frame_num = encoder_info.frame_info.frame_num;

			/* Encode frame */
			start_bits = get_bit_pos(&stream);
			encode_frame(&encoder_info);
			end_bits =  get_bit_pos(&stream);
			num_bits = end_bits-start_bits;
			num_encoded_frames++;

			/* Compute SNR */
			if (params->snrcalc)
			{
				snr_yuv(&psnr,&orig,&rec[rec_buffer_idx],height,width,input_stride_y,input_stride_c);
			}
			else
			{
				psnr.y =  psnr.u = psnr.v = 0.0;
			}
			accsnr.y += psnr.y;
			accsnr.u += psnr.u;
			accsnr.v += psnr.v;

			acc_num_bits += num_bits;

			if (encoder_info.frame_info.frame_type==I_FRAME)
				fprintf(stdout,"%4d I %4d %10d %10.4f %8.4f %8.4f ",frame_num,encoder_info.frame_info.qp,num_bits,psnr.y,psnr.u,psnr.v);
			else if (encoder_info.frame_info.frame_type==P_FRAME)
				fprintf(stdout,"%4d P %4d %10d %10.4f %8.4f %8.4f ",frame_num,encoder_info.frame_info.qp,num_bits,psnr.y,psnr.u,psnr.v);
			else
				fprintf(stdout,"%4d B %4d %10d %10.4f %8.4f %8.4f ",frame_num,encoder_info.frame_info.qp,num_bits,psnr.y,psnr.u,psnr.v);

			for (r = 0; r < encoder_info.frame_info.num_ref; r++)
			{
				fprintf(stdout,"%3d",encoder_info.frame_info.ref_array[r]);
			}
			fprintf(stdout,"\n");
			fflush(stdout);

			/* Write compressed bits for this frame to file */
			flush_bytebuf(&stream, strfile);
		}
	}

	flush_all_bits(&stream, strfile);
	bit_rate_in_kbps = 0.001*params->frame_rate*(double)acc_num_bits/num_encoded_frames;

	/* Finished encoding sequence */
	fprintf(stdout,"------------------- Average data for all frames ------------------------------\n");
	fprintf(stdout,"kbps            : %12.3f\n",bit_rate_in_kbps);
	fprintf(stdout,"PSNR Y          : %12.3f\n",accsnr.y/num_encoded_frames);
	fprintf(stdout,"PSNR U          : %12.3f\n",accsnr.u/num_encoded_frames);
	fprintf(stdout,"PSNR V          : %12.3f\n",accsnr.v/num_encoded_frames);
	fprintf(stdout,"------------------------------------------------------------------------------\n");

	close_yuv_frame(&orig);

	for (int i=0; i<MAX_REORDER_BUFFER; ++i)
	{
		close_yuv_frame(&rec[i]);
	}

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		close_yuv_frame(&ref[r]);
	}

	fclose(infile);
	fclose(strfile);
	free(stream.bitstream);
	free(encoder_info.deblock_data);
	delete_config_params(params);
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
