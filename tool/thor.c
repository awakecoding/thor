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

#include "frame.h"
#include "bits.h"
#include "simd.h"
#include "strings.h"
#include "snr.h"
#include "vlc.h"
#include "transform.h"

void rferror(char error_text[])
{
	fprintf(stderr,"Run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	exit(1);
}

void parse_arg(int argc, char** argv, FILE **infile, FILE **outfile)
{
	if (argc < 2)
	{
		fprintf(stdout, "usage: %s infile [outfile]\n", argv[0]);
		rferror("Wrong number of arguments.");
	}

	if (!(*infile = fopen(argv[1], "rb")))
	{
		fprintf(stderr, "Could not open in-file for reading: %s\n", argv[1]);
		rferror("");
	}

	if (argc > 2)
	{
		if (!(*outfile = fopen(argv[2], "wb")))
		{
			fprintf(stderr, "Could not open out-file for writing.");
			rferror("");
		}
	}
	else
	{
		*outfile = NULL;
	}
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
	for (index=0;index<1;index++){
		for (idx=0;idx<4;idx++){
			int size = 8<<idx;
			printf("%2d x %2d-blocks (8x8): ",size,size);
			for (i=0;i<num;i++){
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
		for (j=0;j<max_num_ref;j++){
			printf("%6d",bit_count.size_and_ref_idx[i][j]);
		}
		printf("\n");
	}

	{
		int sum = 0;
		printf("\nbi-ref:  ");
		for (j=0;j<max_num_ref*max_num_ref;j++){
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
	FILE* infile;
	FILE *outfile;
	decoder_info_t decoder_info;
	stream_t stream;
	yuv_frame_t rec[MAX_REORDER_BUFFER];
	yuv_frame_t ref[MAX_REF_FRAMES];
	int rec_available[MAX_REORDER_BUFFER]={0};
	int rec_buffer_idx;
	int decode_frame_num = 0;
	int frame_count = 0;
	int last_frame_output = -1;
	int width;
	int height;
	int r;
	int sub_gop=1;

	init_use_simd();

	parse_arg(argc, argv, &infile, &outfile);

	fseek(infile, 0, SEEK_END);
	int input_file_size = ftell(infile);
	fseek(infile, 0, SEEK_SET);

	initbits_dec(infile, &stream);

	decoder_info.stream = &stream;

	memset(&decoder_info.bit_count,0,sizeof(bit_count_t));

	int bit_start = stream.bitcnt;
	/* Read sequence header */
	width = getbits(&stream,16);
	height = getbits(&stream,16);

	decoder_info.width = width;
	decoder_info.height = height;
	printf("width=%4d height=%4d\n",width,height);

	decoder_info.pb_split_enable = getbits(&stream,1);
	printf("pb_split_enable=%1d\n",decoder_info.pb_split_enable);

	decoder_info.tb_split_enable = getbits(&stream,1);
	printf("tb_split_enable=%1d\n",decoder_info.tb_split_enable);

	decoder_info.max_num_ref = getbits(&stream,2) + 1;

	decoder_info.num_reorder_pics = getbits(&stream,4);
	sub_gop = 1+decoder_info.num_reorder_pics;

	decoder_info.max_delta_qp = getbits(&stream,2);

	decoder_info.deblocking = getbits(&stream,1);
	decoder_info.clpf = getbits(&stream,1);
	decoder_info.use_block_contexts = getbits(&stream,1);
	decoder_info.bipred = getbits(&stream,1);

	decoder_info.bit_count.sequence_header += (stream.bitcnt - bit_start);

	for (r = 0; r < MAX_REORDER_BUFFER;r++)
	{
		create_yuv_frame(&rec[r],width,height,0,0,0,0);
	}

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		create_yuv_frame(&ref[r],width,height,PADDING_Y,PADDING_Y,PADDING_Y/2,PADDING_Y/2);
		decoder_info.ref[r] = &ref[r];
	}

	decoder_info.deblock_data = (deblock_data_t*) malloc((height/MIN_PB_SIZE) * (width/MIN_PB_SIZE) * sizeof(deblock_data_t));

	while (stream.bitcnt < 8*input_file_size - 8)
	{
		decoder_info.frame_info.decode_order_frame_num = decode_frame_num;
		decoder_info.frame_info.display_frame_num = (frame_count/sub_gop)*sub_gop+reorder_frame_offset(frame_count % sub_gop, sub_gop);

		if (decoder_info.frame_info.display_frame_num >= 0)
		{
			rec_buffer_idx = decoder_info.frame_info.display_frame_num%MAX_REORDER_BUFFER;
			decoder_info.rec = &rec[rec_buffer_idx];
			decoder_info.frame_info.num_ref = min(decode_frame_num,decoder_info.max_num_ref);
			decoder_info.rec->frame_num = decoder_info.frame_info.display_frame_num;
			decode_frame(&decoder_info);
			rec_available[rec_buffer_idx]=1;

			rec_buffer_idx = (last_frame_output+1)%MAX_REORDER_BUFFER;
			if (rec_available[rec_buffer_idx])
			{
				last_frame_output++;
				write_yuv_frame(&rec[rec_buffer_idx],width,height,outfile);
				rec_available[rec_buffer_idx] = 0;
			}
			printf("decode_frame_num=%4d display_frame_num=%4d input_file_size=%12d bitcnt=%12d\n",
					decode_frame_num,decoder_info.frame_info.display_frame_num,input_file_size,stream.bitcnt);
			decode_frame_num++;
		}
		frame_count++;
	}
	// Output the tail
	int i;

	for (i = 1; i <= MAX_REORDER_BUFFER; ++i)
	{
		rec_buffer_idx=(last_frame_output+i) % MAX_REORDER_BUFFER;
		if (rec_available[rec_buffer_idx])
			write_yuv_frame(&rec[rec_buffer_idx],width,height,outfile);
		else
			break;
	}

	print_dec_stats(&decoder_info);

	for (r = 0; r < MAX_REORDER_BUFFER; r++)
	{
		close_yuv_frame(&rec[r]);
	}

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		close_yuv_frame(&ref[r]);
	}

	free(decoder_info.deblock_data);

	return 0;
}

int main_enc(int argc, char **argv)
{
	FILE *infile, *strfile;
	uint32_t input_file_size;
	yuv_frame_t orig,ref[MAX_REF_FRAMES];
	yuv_frame_t rec[MAX_REORDER_BUFFER];
	int num_encoded_frames,num_bits,start_bits,end_bits;
	int sub_gop=1;
	int rec_buffer_idx;
	int frame_num,frame_num0,k,r;
	int frame_offset;
	int ysize,csize,frame_size;
	int width,height,input_stride_y,input_stride_c;
	uint32_t acc_num_bits;
	snrvals psnr;
	snrvals accsnr;
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
		//TODO: Use Long-term frame instead of a large sliding window
		create_yuv_frame(&ref[r],width,height,PADDING_Y,PADDING_Y,PADDING_Y/2,PADDING_Y/2);
	}

	/* Initialize main bit stream */
	stream_t stream;
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

	/* Write sequence header */
	start_bits = get_bit_pos(&stream);
	putbits(16,width,&stream);
	putbits(16,height,&stream);
	putbits(1,params->enable_pb_split,&stream);
	putbits(1,params->enable_tb_split,&stream);
	putbits(2,params->max_num_ref-1,&stream); //TODO: Support more than 4 reference frames
	putbits(4,params->num_reorder_pics,&stream);// Max 15 reordered pictures
	putbits(2,params->max_delta_qp,&stream);
	putbits(1,params->deblocking,&stream);
	putbits(1,params->clpf,&stream);
	putbits(1,params->use_block_contexts,&stream);
	putbits(1,params->enable_bipred,&stream);

	end_bits = get_bit_pos(&stream);
	num_bits = end_bits-start_bits;
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
			read_yuv_frame(&orig,width,height,infile);
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
