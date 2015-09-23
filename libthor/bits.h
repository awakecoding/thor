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

#ifndef THOR_BITS_H
#define THOR_BITS_H

#include <stdio.h>
#include <stdint.h>

#include "thor.h"

void putbits(unsigned int n, unsigned int val, stream_t* str);
void flush_all_bits(stream_t* str);
void flush_bitbuf(stream_t* str);
int get_bit_pos(stream_t* str);

void write_stream_pos(stream_t* stream, stream_pos_t* stream_pos);
void read_stream_pos(stream_pos_t* stream_pos, stream_t* stream);

int fillbfr(stream_t* str);
unsigned int showbits(stream_t* str, int n);
unsigned int getbits1(stream_t* str);
int flushbits(stream_t* str, int n);
unsigned int getbits(stream_t* str, int n);

int read_delta_qp(stream_t* stream);
void read_mv(stream_t* stream, mv_t* mv, mv_t* mvp);
void read_coeff(stream_t* stream, int16_t* coeff, int size, int type);
int read_block(decoder_info_t* decoder_info, stream_t* stream, block_info_dec_t* block_info, frame_type_t frame_type);

int write_delta_qp(stream_t* stream, int delta_qp);
void write_mv(stream_t* stream, mv_t* mv, mv_t* mvp);
void write_coeff(stream_t* stream, int16_t* coeff, int size, int type);
int write_block(stream_t* stream, write_data_t* write_data);
int find_code(int run, int level, int maxrun, int type, int eob);

#endif /* THOR_BITS_H */
