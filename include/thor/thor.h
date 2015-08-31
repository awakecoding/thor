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

#ifndef THOR_API_H
#define THOR_API_H

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#if defined _WIN32 || defined __CYGWIN__
#ifdef __GNUC__
#define THOR_EXPORT __attribute__((dllexport))
#else
#define THOR_EXPORT __declspec(dllexport)
#endif
#else
#if __GNUC__ >= 4
#define THOR_EXPORT   __attribute__ ((visibility("default")))
#else
#define THOR_EXPORT
#endif
#endif

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

typedef struct
{
	uint32_t size;
	uint8_t reserved1;
	uint8_t reserved2;
	uint8_t reserved3;
	uint8_t reserved4;
} thor_frame_header_t;

typedef struct thor_encoder_s thor_encoder_t;
typedef struct thor_decoder_s thor_decoder_t;

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Thor Common API
 */

THOR_EXPORT void thor_read_sequence_header(uint8_t* buffer, thor_sequence_header_t* shdr);
THOR_EXPORT void thor_write_sequence_header(uint8_t* buffer, thor_sequence_header_t* shdr);

THOR_EXPORT void thor_read_frame_header(uint8_t* buffer, thor_frame_header_t* fhdr);
THOR_EXPORT void thor_write_frame_header(uint8_t* buffer, thor_frame_header_t* fhdr);

/**
 * Thor Encoder API
 */

THOR_EXPORT void thor_encoder_set_sequence_header(thor_encoder_t* ctx, thor_sequence_header_t* hdr);

THOR_EXPORT int thor_encode(thor_encoder_t* ctx, uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, uint32_t dstSize);

THOR_EXPORT thor_encoder_t* thor_encoder_new();
THOR_EXPORT void thor_encoder_free(thor_encoder_t* ctx);

/**
 * Thor Decoder API
 */

THOR_EXPORT void thor_decoder_set_sequence_header(thor_decoder_t* ctx, thor_sequence_header_t* hdr);

THOR_EXPORT int thor_decode(thor_decoder_t* ctx, uint8_t* pSrc, uint32_t srcSize, uint8_t* pDst[3], int dstStep[3]);

THOR_EXPORT thor_decoder_t* thor_decoder_new();
THOR_EXPORT void thor_decoder_free(thor_decoder_t* ctx);

#ifdef __cplusplus
}
#endif

#endif /* THOR_API_H */
