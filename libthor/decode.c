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

#include "simd.h"
#include "frame.h"

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