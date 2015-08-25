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

int thor_encode(thor_sequence_header_t* hdr, uint8_t* pSrc[3], int srcStep[3], uint8_t* pDst, uint32_t dstSize);
int thor_decode(thor_sequence_header_t* hdr, uint8_t* pSrc, uint32_t srcSize, uint8_t* pDst[3], int dstStep[3]);

#endif /* THOR_API_H */