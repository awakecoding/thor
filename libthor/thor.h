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

#ifndef THOR_MAIN_H
#define THOR_MAIN_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#define clip255(n) min(255, max(0, (n)))
#define clip(n, low, high) min ((high), max ((n), (low)))

#define MAX_BLOCK_SIZE 64        //Maximum block size
#define MIN_BLOCK_SIZE 8         //Minimum block size
#define MIN_PB_SIZE 4            //Minimum pu block size
#define MAX_QUANT_SIZE 16        //Maximum quantization block size
#define MAX_BUFFER_SIZE 4000000  //Maximum compressed buffer size per frame
#define MAX_TR_SIZE 64           //Maximum transform size
#define PADDING_Y 96             //One-sided padding range for luma
#define MAX_UINT32 1<<31         //Used e.g. to initialize search for minimum cost
#define EARLY_SKIP_BLOCK_SIZE 8  //Transform size for early skip check
#define MAX_REF_FRAMES 17        //Maximum number of reference frames
#define MAX_REORDER_BUFFER 32    //Maximum number of frames to store for reordering
#define CLPF_PERIOD 4            //Period of CLPF frames
#define CLPF_BIAS 101            //Bias used for CLPF on/off decision
#define DYADIC_CODING 1          // Support hierarchical B frames

/* Experimental */
#define ENABLE_SKIP_BIPRED 1     //Enable bipred in skip mode
#define HEVC_INTERPOLATION 0     //Enable HEVC interpolation filter
#define FILTER_HOR_AND_VER 0     //Filter horizontal and vertical intra modes (1,14,1)
#define NEW_BLOCK_STRUCTURE 0    //Non-quadtree block structure
#define NO_SUBBLOCK_SKIP 1       //Force only zero skip mv in subblocks
#define LIMIT_INTRA_MODES 1      //Allow maximum 8 intra modes
#define NEW_DEBLOCK_TEST 1       //New test for deblocking a block edge
#define NEW_MV_TEST 1            //New MV test for deblocking a line
#define NEW_DEBLOCK_FILTER 1     //New deblocking filter
#define LIMITED_SKIP 1           //Limit number of skip candidates

#if LIMITED_SKIP
#define MAX_NUM_SKIP 2           //Maximum number of skip candidates
#if NO_SUBBLOCK_SKIP
#define TWO_MVP 0                //Choose one of two motion vectors for mvp
#else
#define TWO_MVP 1                //Choose one of two motion vectors for mvp
#endif
#else
#define MAX_NUM_SKIP 4           //Maximum number of skip candidates
#define TWO_MVP 0                //Choose one of two motion vectors for mvp
#endif
/* Testing and analysis*/
#define STAT 0                   //Extended statistics printout in decoder

static inline void fatalerror(char error_text[])
{
	fprintf(stderr,"Run-time error...\n");
	fprintf(stderr,"%s\n",error_text);
	fprintf(stderr,"...now exiting to system...\n");
	exit(1);
}

typedef struct
{
	double y;
	double u;
	double v;
} snrvals;

typedef struct
{
	uint8_t *y;
	uint8_t *u;
	uint8_t *v;
	int width;
	int height;
	int stride_y;
	int stride_c;
	int offset_y;
	int offset_c;
	int frame_num;
} yuv_frame_t;

typedef enum {     // Order matters: log2(size)-2
	TR_4x4 = 0,
	TR_8x8 = 1,
	TR_16x16 = 2,
	TR_32x32 = 3,
	TR_SIZES
} trsizes;

typedef enum {
	I_FRAME,
	P_FRAME,
	B_FRAME
} frame_type_t;

typedef enum {
	MODE_SKIP = 0,
	MODE_INTRA,
	MODE_INTER,
	MODE_BIPRED,
	MODE_MERGE,
	MAX_NUM_MODES
} block_mode_t;

typedef enum {
	PART_NONE = 0,
	PART_HOR,
	PART_VER,
	PART_QUAD
} part_t;

typedef struct
{
	int16_t x;
	int16_t y;
} mv_t;

typedef struct
{
	int16_t x;
	int16_t y;
	int32_t ref_idx;
} mvr_t;

typedef struct
{
	int16_t x0;
	int16_t y0;
	int32_t ref_idx0;
	int16_t x1;
	int16_t y1;
	int32_t ref_idx1;
	int32_t dir;
} mvb_t;

typedef struct
{
	int y;
	int u;
	int v;
} cbp_t;

typedef struct
{
	block_mode_t mode;
	cbp_t cbp;
	uint8_t size;
	mvb_t mvb;
	uint8_t tb_split;
	part_t pb_part;
} deblock_data_t;

typedef enum {
	MODE_DC = 0,
	MODE_PLANAR,
	MODE_HOR,
	MODE_VER,
	MODE_UPLEFT,
	MODE_UPRIGHT,
	MODE_UPUPRIGHT,
	MODE_UPUPLEFT,
	MODE_UPLEFTLEFT,
	MODE_DOWNLEFTLEFT,
	MAX_NUM_INTRA_MODES
} intra_mode_t;

typedef struct
{
	block_mode_t mode;
	intra_mode_t intra_mode;
	int skip_idx;
	int PBpart;
	mv_t mv_arr0[4];
	mv_t mv_arr1[4];
	int ref_idx0;
	int ref_idx1;
	int dir;
} pred_data_t;

typedef struct
{
	int mode;
	int skip_idx;
	int tb_split;
	int pb_part;
	mv_t mv[4];
} best_rdo_block_params_t;

typedef struct
{
	uint16_t ypos;
	uint16_t xpos;
	uint8_t size;
	uint8_t bwidth;
	uint8_t bheight;
} block_pos_t;

typedef struct
{
	int8_t split;
	int8_t cbp;
	int8_t mode;
	int8_t size;
	int8_t index;
} block_context_t;

typedef struct
{
	block_mode_t mode;
	intra_mode_t intra_mode;
	mv_t mvp;
	cbp_t *cbp;
	int16_t *coeffq_y;
	int16_t *coeffq_u;
	int16_t *coeffq_v;
	uint8_t size;
	int skip_idx;
	int num_skip_vec;
	mv_t mv_arr[4]; //TODO: collapse with mv_arr0
	mv_t mv_arr0[4];
	mv_t mv_arr1[4];
	int ref_idx0;
	int ref_idx1;
	int max_num_pb_part;
	int max_num_tb_part;
	int tb_part;
	int pb_part;
	int frame_type;
	int num_ref;
	int ref_idx; //TODO: collapse with ref_idx0
	int num_intra_modes;
	int delta_qp;
	int max_delta_qp;
	block_context_t *block_context;
	int enable_bipred;
	int encode_rectangular_size;
#if TWO_MVP
	int mv_idx;
#endif
} write_data_t;

typedef struct
{
	/* For bit ccount */
	uint32_t sequence_header;
	uint32_t frame_header[2];
	uint32_t super_mode[2];
	uint32_t mv[2];
	uint32_t intra_mode[2];
	uint32_t skip_idx[2];
	uint32_t coeff_y[2];
	uint32_t coeff_u[2];
	uint32_t coeff_v[2];
	uint32_t cbp[2];
	uint32_t clpf[2];

	/* For statistics */
	uint32_t mode[2][4];
	uint32_t size[2][4];
	uint32_t size_and_mode[4][4];
	uint32_t frame_type[2];
	uint32_t super_mode_stat[9][4][MAX_REF_FRAMES+8]; //split_flag,mode and ref_idx for each context(18) and block size(4)
	uint32_t cbp_stat[2][16];
	uint32_t cbp2_stat[3][2][2][4][9];
	uint32_t size_and_intra_mode[2][4][MAX_NUM_INTRA_MODES];
	uint32_t size_and_ref_idx[4][MAX_REF_FRAMES];
	uint32_t bi_ref[MAX_REF_FRAMES*MAX_REF_FRAMES];
} bit_count_t;

typedef struct
{
	/* putbits */
	uint32_t bytesize;     //Buffer size - typically maximum compressed frame size
	uint32_t bytepos;      //Byte position in bitstream
	uint8_t *bitstream;   //Compressed bit stream
	uint32_t bitbuf;       //Recent bits not written the bitstream yet
	uint32_t bitrest;      //Empty bits in bitbuf

	/* getbits */
	uint8_t* rdbfr;
	uint8_t* rdptr;
	unsigned int inbfr;
	int incnt;
	int bitcnt;
	int capacity;
} stream_t;

typedef struct
{
	uint32_t bytepos;      //Byte position in bitstream
	uint32_t bitbuf;       //Recent bits not written the bitstream yet
	uint32_t bitrest;      //Empty bits in bitbuf
} stream_pos_t;

typedef struct
{
	unsigned int width;
	unsigned int height;
	unsigned int qp;
	char *infilestr;
	char *outfilestr;
	unsigned int file_headerlen;
	unsigned int frame_headerlen;
	unsigned int num_frames;
	int skip;
	float frame_rate;
	float lambda_coeffI;
	float lambda_coeffP;
	float lambda_coeffB;
	float early_skip_thr;
	int enable_tb_split;
	int enable_pb_split;
	int max_num_ref;
	int HQperiod;
	int num_reorder_pics;
	int dqpP;
	int dqpB;
	float mqpP;
	float mqpB;
	int dqpI;
	int intra_period;
	int intra_rdo;
	int rdoq;
	int max_delta_qp;
	int encoder_speed;
	int deblocking;
	int clpf;
	int snrcalc;
	int use_block_contexts;
	int enable_bipred;
} enc_params;

typedef struct
{
	uint8_t y[MAX_BLOCK_SIZE*MAX_BLOCK_SIZE];
	uint8_t u[MAX_BLOCK_SIZE/2*MAX_BLOCK_SIZE/2];
	uint8_t v[MAX_BLOCK_SIZE/2*MAX_BLOCK_SIZE/2];
} yuv_block_t;

typedef struct
{
	block_pos_t block_pos;
	yuv_block_t *rec_block;
	yuv_block_t *org_block;
	pred_data_t pred_data;
	int num_skip_vec;
	mvb_t mvb_skip[MAX_NUM_SKIP];
	int num_merge_vec;
	mvb_t mvb_merge[MAX_NUM_SKIP];
	mv_t mvp;
	int tb_param;
	int max_num_pb_part;
	int max_num_tb_part;
	cbp_t cbp;
	int delta_qp;
	block_context_t *block_context;
	int final_encode;
} block_info_t;

typedef struct
{
	frame_type_t frame_type;
	uint8_t qp;
	uint8_t qpb;
	int num_ref;
	int ref_array[MAX_REF_FRAMES];
	double lambda;
	int num_intra_modes;
	int frame_num;
	int display_frame_num;
	int decode_order_frame_num;
} frame_info_t;

typedef struct
{
	block_info_t *block_info;
	frame_info_t frame_info;
	enc_params *params;
	yuv_frame_t *orig;
	yuv_frame_t *rec;
	yuv_frame_t *ref[MAX_REF_FRAMES];
	stream_t *stream;
	deblock_data_t *deblock_data;
	int width;
	int height;
	int depth;
} encoder_info_t;

typedef struct
{
	block_pos_t block_pos;
	pred_data_t pred_data;
	int num_skip_vec;
	mvr_t mvr_skip[MAX_NUM_SKIP];
	int tb_split;
	cbp_t cbp;
	int16_t *coeffq_y;
	int16_t *coeffq_u;
	int16_t *coeffq_v;
	int delta_qp;
} block_info_dec_t;

typedef struct
{
	frame_info_t frame_info;
	yuv_frame_t *rec;
	yuv_frame_t *ref[MAX_REF_FRAMES];
	stream_t *stream;
	deblock_data_t *deblock_data;
	int width;
	int height;
	bit_count_t bit_count;
	int pb_split_enable;
	int max_num_ref;
	int num_reorder_pics;
	int max_delta_qp;
	int deblocking;
	int clpf;
	int tb_split_enable;
	int mode;
	int ref_idx;
	int super_mode;
	int use_block_contexts;
	block_context_t *block_context;
	int bipred;
	int depth;
} decoder_info_t;

/**
 * thor sequence header:
 *
 * width			16 bits		0-15
 * height			16 bits		16-31
 * pb_split_enable		1 bit		32
 * tb_split_enable		1 bit		33
 * max_num_ref			2 bits		34-35
 * num_reorder_pics		4 bits		36-39
 * max_delta_qp			2 bits		40-41
 * deblocking			1 bit		42
 * clpf				1 bit		43
 * use_block_contexts		1 bit		44
 * enable_bipred		1 bit		45
*/

typedef struct
{
	uint16_t width;
	uint16_t height;
	uint8_t pb_split_enable;
	uint8_t tb_split_enable;
	uint8_t max_num_ref;
	uint8_t num_reorder_pics;
	uint8_t max_delta_qp;
	uint8_t deblocking;
	uint8_t clpf;
	uint8_t use_block_contexts;
	uint8_t enable_bipred;
} thor_sequence_header_t;

#endif
