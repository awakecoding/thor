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
	"-max_num_ref", "1",
	"-use_block_contexts", "1",
	"-enable_bipred", "0",
	"-encoder_speed", "2"
};

int thor_default_param_count = sizeof(thor_default_params) / sizeof(const char*);

void thor_read_sequence_header(uint8_t* buffer, thor_sequence_header_t* shdr)
{
	stream_t stream;

	stream.incnt = 0;
	stream.bitcnt = 0;
	stream.capacity = 8;
	stream.rdbfr = buffer;
	stream.rdptr = stream.rdbfr;

	shdr->width = getbits(&stream, 16);
	shdr->height = getbits(&stream, 16);
	shdr->pb_split_enable = getbits(&stream, 1);
	shdr->tb_split_enable = getbits(&stream, 1);
	shdr->max_num_ref = getbits(&stream, 3);
	shdr->num_reorder_pics = getbits(&stream, 4);
	shdr->max_delta_qp = getbits(&stream, 2);
	shdr->deblocking = getbits(&stream, 1);
	shdr->clpf = getbits(&stream, 1);
	shdr->use_block_contexts = getbits(&stream, 1);
	shdr->enable_bipred = getbits(&stream, 1);
	getbits(&stream, 17); /* pad */
}

void thor_write_sequence_header(uint8_t* buffer, thor_sequence_header_t* shdr)
{
	stream_t stream;

	stream.bitstream = buffer;
	stream.bytepos = 0;
	stream.bitrest = 32;
	stream.bitbuf = 0;
	stream.bytesize = 8;

	putbits(16, shdr->width, &stream);
	putbits(16, shdr->height, &stream);
	putbits(1, shdr->pb_split_enable, &stream);
	putbits(1, shdr->tb_split_enable, &stream);
	putbits(3, shdr->max_num_ref, &stream);
	putbits(4, shdr->num_reorder_pics, &stream);
	putbits(2, shdr->max_delta_qp, &stream);
	putbits(1, shdr->deblocking, &stream);
	putbits(1, shdr->clpf, &stream);
	putbits(1, shdr->use_block_contexts, &stream);
	putbits(1, shdr->enable_bipred, &stream);
	putbits(17, 0, &stream); /* pad */

	flush_bitbuf(&stream);
}

void thor_read_frame_header(uint8_t* buffer, thor_frame_header_t* fhdr)
{
	stream_t stream;

	stream.incnt = 0;
	stream.bitcnt = 0;
	stream.capacity = 8;
	stream.rdbfr = buffer;
	stream.rdptr = stream.rdbfr;

	fhdr->size = getbits(&stream, 32);
	fhdr->reserved1 = getbits(&stream, 8);
	fhdr->reserved2 = getbits(&stream, 8);
	fhdr->reserved3 = getbits(&stream, 8);
	fhdr->reserved4 = getbits(&stream, 8);
}

void thor_write_frame_header(uint8_t* buffer, thor_frame_header_t*fhdr)
{
	stream_t stream;

	stream.bitstream = buffer;
	stream.bytepos = 0;
	stream.bitrest = 32;
	stream.bitbuf = 0;
	stream.bytesize = 8;

	putbits(32, fhdr->size, &stream);
	putbits(8, fhdr->reserved1, &stream);
	putbits(8, fhdr->reserved2, &stream);
	putbits(8, fhdr->reserved3, &stream);
	putbits(8, fhdr->reserved4, &stream);

	flush_bitbuf(&stream);
}

int thor_encode(thor_encoder_t* ctx, uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, uint32_t dstSize)
{
	int status = 0;
	yuv_frame_t frame;
	int r, r0, r1, r2, r3;
	stream_t* stream = &ctx->stream;
	encoder_info_t* info = &ctx->info;
	thor_encoder_settings_t* settings = ctx->settings;

	frame.width = ctx->width;
	frame.height = ctx->height;
	frame.stride_y = srcStep[0];
	frame.stride_c = srcStep[1];
	frame.offset_y = 0;
	frame.offset_c = 0;
	frame.y = pSrc[0];
	frame.u = pSrc[1];
	frame.v = pSrc[2];
	frame.frame_num = ctx->frame_num;

	stream->bitstream = pDst;
	stream->bitbuf = 0;
	stream->bitrest = 32;
	stream->bytepos = 0;
	stream->bytesize = dstSize;

	info->rec = &ctx->rec;
	info->orig = &frame;
	info->rec->frame_num = ctx->frame_num;

	if (settings->max_num_ref > 0)
		info->frame_info.frame_type = (ctx->frame_num == 0) ? I_FRAME : P_FRAME;
	else
		info->frame_info.frame_type = I_FRAME;

	info->frame_info.frame_num = ctx->frame_num;

	if (info->frame_info.frame_type == I_FRAME)
	{
		info->frame_info.qp = settings->qp + settings->dqpI;
	}
	else
	{
		if (settings->HQperiod && (ctx->frame_num % settings->HQperiod))
		{
			info->frame_info.qp = (int) (settings->mqpP * (float) settings->qp) + settings->dqpP;
		}
		else
		{
			info->frame_info.qp = settings->qp;
		}
	}

	info->frame_info.num_ref = min(ctx->frame_num, settings->max_num_ref);

	if (info->frame_info.num_ref == 1)
	{
		/* If num_ref==1 always use most recent frame */
		info->frame_info.ref_array[0] = 0;
	}
	else if (info->frame_info.num_ref == 2)
	{
		/* If num_ref==2 use most recent LQ frame and most recent HQ frame */
		r0 = 0;
		r1 = ((ctx->frame_num + settings->HQperiod - 2) % settings->HQperiod) + 1;
		info->frame_info.ref_array[0] = r0;
		info->frame_info.ref_array[1] = r1;
	}
	else if (info->frame_info.num_ref == 3)
	{
		r0 = 0;
		r1 = ((ctx->frame_num + settings->HQperiod - 2) % settings->HQperiod) + 1;
		r2 = (r1 == 1) ? 2 : 1;
		info->frame_info.ref_array[0] = r0;
		info->frame_info.ref_array[1] = r1;
		info->frame_info.ref_array[2] = r2;
	}
	else if (info->frame_info.num_ref == 4)
	{
		r0 = 0;
		r1 = ((ctx->frame_num + settings->HQperiod - 2) % settings->HQperiod) + 1;
		r2 = (r1 == 1) ? 2 : 1;
		r3 = r2 + 1;

		if (r3 == r1)
			r3 += 1;

		info->frame_info.ref_array[0] = r0;
		info->frame_info.ref_array[1] = r1;
		info->frame_info.ref_array[2] = r2;
		info->frame_info.ref_array[3] = r3;
	}
	else
	{
		for (r = 0; r <info->frame_info.num_ref; r++)
		{
			info->frame_info.ref_array[r] = r;
		}
	}

	if (settings->intra_rdo)
	{
		if (info->frame_info.frame_type == I_FRAME)
			info->frame_info.num_intra_modes = 10;
		else
			info->frame_info.num_intra_modes = settings->encoder_speed > 0 ? 4 : 10;
	}
	else
	{
		info->frame_info.num_intra_modes = 4;
	}

	encode_frame(info);

	ctx->frame_num++;

	flush_all_bits(stream);
	status = stream->bytepos;

	return status;
}

thor_encoder_settings_t* thor_encoder_get_settings(thor_encoder_t* ctx)
{
	if (!ctx)
		return NULL;

	return ctx->settings;
}

void thor_encoder_set_sequence_header(thor_encoder_t* ctx, thor_sequence_header_t* hdr)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	encoder_info_t* info = &ctx->info;

	memcpy(&ctx->hdr, hdr, sizeof(thor_sequence_header_t));

	info->width = ctx->width = hdr->width;
	info->height = ctx->height = hdr->height;

	create_yuv_frame(&ctx->rec, ctx->width, ctx->height, 0, 0, 0, 0);

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		create_yuv_frame(&ref[i], ctx->width, ctx->height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
		info->ref[i] = &ref[i];
	}

	info->deblock_data = (deblock_data_t*) malloc((ctx->height / MIN_PB_SIZE) * (ctx->width / MIN_PB_SIZE) * sizeof(deblock_data_t));
}

thor_encoder_t* thor_encoder_new(thor_encoder_settings_t* settings)
{
	thor_encoder_t* ctx;

	init_use_simd();

	ctx = (thor_encoder_t*) calloc(1, sizeof(thor_encoder_t));

	if (!ctx)
		return NULL;

	ctx->info.stream = &ctx->stream;

	ctx->settings = settings;

	if (!ctx->settings)
		ctx->settings = thor_encoder_settings_new(thor_default_param_count, (char**) thor_default_params);

	if (!ctx->settings)
		return NULL;

	ctx->info.params = ctx->settings;

	return ctx;
}

void thor_encoder_free(thor_encoder_t* ctx)
{
	int i;
	yuv_frame_t* ref = ctx->ref;
	encoder_info_t* info = &ctx->info;

	if (!ctx)
		return;

	close_yuv_frame(&ctx->rec);

	for (i = 0; i < MAX_REF_FRAMES; i++)
	{
		close_yuv_frame(&ref[i]);
	}

	free(info->deblock_data);

	thor_encoder_settings_free(ctx->settings);

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

	info->frame_info.decode_order_frame_num = ctx->frame_num;
	info->frame_info.display_frame_num = ctx->frame_num;
	info->rec = &frame;
	info->frame_info.num_ref = min(ctx->frame_num, info->max_num_ref);
	info->rec->frame_num = info->frame_info.display_frame_num;

	decode_frame(info);

	ctx->frame_num++;

	return 1;
}

thor_decoder_settings_t* thor_decoder_get_settings(thor_decoder_t* ctx)
{
	if (!ctx)
		return NULL;

	return ctx->settings;
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

thor_decoder_t* thor_decoder_new(thor_decoder_settings_t* settings)
{
	thor_decoder_t* ctx;

	init_use_simd();

	ctx = (thor_decoder_t*) calloc(1, sizeof(thor_decoder_t));

	if (!ctx)
		return NULL;

	ctx->info.stream = &ctx->stream;

	ctx->settings = settings;

	if (!ctx->settings)
		ctx->settings = thor_decoder_settings_new(0, NULL);

	if (!ctx->settings)
		return NULL;

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

	thor_decoder_settings_free(ctx->settings);

	free(ctx);
}
