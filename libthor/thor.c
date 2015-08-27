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

#include "thor.h"

#include "bits.h"
#include "simd.h"
#include "frame.h"
#include "params.h"

const char* thor_default_params[] =
{
	"thor",
	"-n", "1",
	"-f", "60",
	"-qp", "32",
	"-HQperiod", "12",
	"-mqpP", "1.2",
	"-dqpI", "-2",
	"-lambda_coeffI", "1.2",
	"-lambda_coeffP", "1.2",
	"-intra_rdo", "0",
	"-enable_tb_split", "0",
	"-enable_pb_split", "0",
	"-early_skip_thr", "1.0",
	"-max_num_ref", "2",
	"-use_block_contexts", "1",
	"-enable_bipred", "0",
	"-encoder_speed", "2"
};

int thor_default_param_count = sizeof(thor_default_params) / sizeof(const char*);

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

int thor_encode(thor_encoder_t* ctx, uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, uint32_t dstSize)
{
	int status = 0;
	uint8_t* pRec[3];
	yuv_frame_t rec;
	yuv_frame_t frame;
	stream_t* stream = &ctx->stream;
	encoder_info_t* info = &ctx->info;

	frame.width = ctx->width;
	frame.height = ctx->height;
	frame.stride_y = srcStep[0];
	frame.stride_c = srcStep[1];
	frame.offset_y = 0;
	frame.offset_c = 0;
	frame.y = pSrc[0];
	frame.u = pSrc[1];
	frame.v = pSrc[2];

	pRec[0] = (uint8_t*) malloc((ctx->height * srcStep[0]) + (ctx->height * srcStep[1]));
	pRec[1] = pRec[0] + (ctx->height * srcStep[0]);
	pRec[2] = pRec[1] + ((ctx->height / 2) * srcStep[2]);

	rec.width = ctx->width;
	rec.height = ctx->height;
	rec.stride_y = srcStep[0];
	rec.stride_c = srcStep[1];
	rec.offset_y = 0;
	rec.offset_c = 0;
	rec.y = pRec[0];
	rec.u = pRec[1];
	rec.v = pRec[2];

	frame.frame_num = info->frame_info.frame_num;

	stream->bitstream = pDst;
	stream->bitbuf = 0;
	stream->bitrest = 32;
	stream->bytepos = 0;
	stream->bytesize = dstSize;

	info->frame_info.frame_num = 0;
	info->rec = &rec;
	info->orig = &frame;
	info->rec->frame_num = info->frame_info.frame_num;
	info->frame_info.frame_type = I_FRAME;
	info->frame_info.qp = ctx->params->qp + ctx->params->dqpI;
	info->frame_info.num_ref = min(0, ctx->params->max_num_ref);
	info->frame_info.num_intra_modes = 4;

	encode_frame(info);

	flush_all_bits(stream);
	status = stream->bytepos;

	free(pRec[0]);

	return status;
}

void thor_encoder_set_sequence_header(thor_encoder_t* ctx, thor_sequence_header_t* hdr)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	encoder_info_t* info = &ctx->info;

	memcpy(&ctx->hdr, hdr, sizeof(thor_sequence_header_t));

	info->width = ctx->width = hdr->width;
	info->height = ctx->height = hdr->height;

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		create_yuv_frame(&ref[i], ctx->width, ctx->height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
		info->ref[i] = &ref[i];
	}

	info->deblock_data = (deblock_data_t*) malloc((ctx->height / MIN_PB_SIZE) * (ctx->width / MIN_PB_SIZE) * sizeof(deblock_data_t));
}

thor_encoder_t* thor_encoder_new()
{
	thor_encoder_t* ctx;

	init_use_simd();

	ctx = (thor_encoder_t*) calloc(1, sizeof(thor_encoder_t));

	if (!ctx)
		return NULL;

	ctx->info.stream = &ctx->stream;

	ctx->params = parse_config_params(thor_default_param_count, (char**) thor_default_params);

	if (!ctx->params)
		return NULL;

	ctx->info.params = ctx->params;

	return ctx;
}

void thor_encoder_free(thor_encoder_t* ctx)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	encoder_info_t* info = &ctx->info;

	if (!ctx)
		return;

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		close_yuv_frame(&ref[i]);
	}

	free(info->deblock_data);

	delete_config_params(ctx->params);

	free(ctx);
}

int thor_decode(thor_decoder_t* ctx, uint8_t* pSrc, uint32_t srcSize, uint8_t* pDst[3], int dstStep[3])
{
	yuv_frame_t frame;
	stream_t* stream = &ctx->stream;
	decoder_info_t* info = &ctx->info;

	stream->incnt = 0;
	stream->bitcnt = 0;
	stream->capacity = srcSize;
	stream->rdbfr = pSrc;
	stream->rdptr = stream->rdbfr;

	frame.width = ctx->width;
	frame.height = ctx->height;
	frame.stride_y = dstStep[0];
	frame.stride_c = dstStep[1];
	frame.offset_y = 0;
	frame.offset_c = 0;
	frame.y = pDst[0];
	frame.u = pDst[1];
	frame.v = pDst[2];

	info->frame_info.decode_order_frame_num = 0;
	info->frame_info.display_frame_num = 0;
	info->rec = &frame;
	info->frame_info.num_ref = min(ctx->frame_num, info->max_num_ref);
	info->rec->frame_num = info->frame_info.display_frame_num;

	decode_frame(info);

	return 1;
}

void thor_decoder_set_sequence_header(thor_decoder_t* ctx, thor_sequence_header_t* hdr)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	decoder_info_t* info = &ctx->info;

	memcpy(&ctx->hdr, hdr, sizeof(thor_sequence_header_t));

	info->width = ctx->width = hdr->width;
	info->height = ctx->height = hdr->height;

	info->pb_split_enable = hdr->pb_split_enable;
	info->tb_split_enable = hdr->tb_split_enable;
	info->max_num_ref = hdr->max_num_ref;
	info->num_reorder_pics = hdr->num_reorder_pics;
	info->max_delta_qp = hdr->max_delta_qp;
	info->deblocking = hdr->deblocking;
	info->clpf = hdr->clpf;
	info->use_block_contexts = hdr->use_block_contexts;
	info->bipred = hdr->enable_bipred;
	info->bit_count.sequence_header = 64;

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		create_yuv_frame(&ref[i], ctx->width, ctx->height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
		info->ref[i] = &ref[i];
	}

	info->deblock_data = (deblock_data_t*) malloc((ctx->height / MIN_PB_SIZE) * (ctx->width / MIN_PB_SIZE) * sizeof(deblock_data_t));
}

thor_decoder_t* thor_decoder_new()
{
	thor_decoder_t* ctx;

	init_use_simd();

	ctx = (thor_decoder_t*) calloc(1, sizeof(thor_decoder_t));

	if (!ctx)
		return NULL;

	ctx->info.stream = &ctx->stream;

	return ctx;
}

void thor_decoder_free(thor_decoder_t* ctx)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	decoder_info_t* info = &ctx->info;

	if (!ctx)
		return;

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		close_yuv_frame(&ref[i]);
	}

	free(info->deblock_data);

	free(ctx);
}
