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

#ifndef THOR_BLOCK_H
#define THOR_BLOCK_H

#include "thor.h"
#include "simd.h"

int get_left_available(int ypos, int xpos, int size, int width);
int get_up_available(int ypos, int xpos, int size, int width);
int get_upright_available(int ypos, int xpos, int size, int width);
int get_downleft_available(int ypos, int xpos, int size, int height);

void dequantize(int16_t* RESTRICT coeff, int16_t* RESTRICT rcoeff, int quant, int size);
void reconstruct_block(int16_t* RESTRICT block, uint8_t* RESTRICT pblock, uint8_t* RESTRICT rec, int size, int stride);

void find_block_contexts(int ypos, int xpos, int height, int width, int size, deblock_data_t* deblock_data, block_context_t* block_context, int enable);

void clpf_block(uint8_t* rec, int x0, int x1, int y0, int y1, int stride);

void process_block_dec(decoder_info_t* encoder_info, int size, int yposY, int xposY);

int process_block(encoder_info_t* encoder_info, int size, int yposY, int xposY, int qp);
int detect_clpf(uint8_t* org, uint8_t* rec, int x0, int x1, int y0, int y1, int stride);

#endif /* THOR_BLOCK_H */
