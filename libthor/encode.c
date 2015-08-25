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

static void thor_write_sequence_header(uint8_t* buffer, thor_sequence_header_t* hdr)
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

int thor_encode(thor_sequence_header_t* hdr, uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, uint32_t dstSize)
{
	int r;
	int status = 0;
	int width;
	int height;
	uint8_t* pRec[3];
	yuv_frame_t rec;
	yuv_frame_t frame;
	stream_t stream;
	enc_params* params;
	encoder_info_t enc_info;
	yuv_frame_t ref[MAX_REF_FRAMES];

	init_use_simd();

	params = parse_config_params(thor_default_param_count, (char**) thor_default_params);

	if (!params)
		return -1;

	width = hdr->width;
	height = hdr->height;

	frame.width = width;
	frame.height = height;
	frame.stride_y = srcStep[0];
	frame.stride_c = srcStep[1];
	frame.offset_y = 0;
	frame.offset_c = 0;
	frame.y = pSrc[0];
	frame.u = pSrc[1];
	frame.v = pSrc[2];

	pRec[0] = (uint8_t*) malloc(height * srcStep[0] * sizeof(uint8_t));
	pRec[1] = (uint8_t*) malloc(height / 2 * srcStep[1] * sizeof(uint8_t));
	pRec[2] = (uint8_t*) malloc(height / 2 * srcStep[2] * sizeof(uint8_t));

	rec.width = width;
	rec.height = height;
	rec.stride_y = srcStep[0];
	rec.stride_c = srcStep[1];
	rec.offset_y = 0;
	rec.offset_c = 0;
	rec.y = pRec[0];
	rec.u = pRec[1];
	rec.v = pRec[2];

	frame.frame_num = enc_info.frame_info.frame_num;

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		create_yuv_frame(&ref[r], width, height, PADDING_Y, PADDING_Y, PADDING_Y / 2, PADDING_Y / 2);
		enc_info.ref[r] = &ref[r];
	}

	stream.bitstream = pDst;
	stream.bitbuf = 0;
	stream.bitrest = 32;
	stream.bytepos = 0;
	stream.bytesize = dstSize;

	enc_info.params = params;
	enc_info.orig = &frame;

	enc_info.stream = &stream;
	enc_info.width = width;
	enc_info.height = height;

	enc_info.deblock_data = (deblock_data_t*) malloc((height / MIN_PB_SIZE) * (width / MIN_PB_SIZE) * sizeof(deblock_data_t));

	thor_write_sequence_header(stream.bitstream, hdr);
	stream.bytepos += 8;

	enc_info.frame_info.frame_num = 0;
	enc_info.rec = &rec;
	enc_info.rec->frame_num = enc_info.frame_info.frame_num;
	enc_info.frame_info.frame_type = I_FRAME;
	enc_info.frame_info.qp = params->qp + params->dqpI;
	enc_info.frame_info.num_ref = min(0, params->max_num_ref);
	enc_info.frame_info.num_intra_modes = 4;

	/* Encode frame */
	encode_frame(&enc_info);

	flush_all_bits(&stream);
	status = stream.bytepos;

	for (r = 0; r < MAX_REF_FRAMES; r++)
	{
		close_yuv_frame(&ref[r]);
	}

	free(enc_info.deblock_data);

	free(pRec[0]);
	free(pRec[1]);
	free(pRec[2]);

	return status;
}