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

#ifndef THOR_SETTINGS_H
#define THOR_SETTINGS_H

#include <thor/thor.h>

struct thor_encoder_settings_s
{
	unsigned int width;
	unsigned int height;
	unsigned int qp;
	char* infilestr;
	char* outfilestr;
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
	int inter_mode;
	int merge_mode;
};

struct thor_decoder_settings_s
{
	unsigned int width;
	unsigned int height;
};

#ifdef __cplusplus
extern "C" {
#endif

THOR_EXPORT thor_encoder_settings_t* thor_encoder_settings_new(int argc, char** argv);
THOR_EXPORT void thor_encoder_settings_free(thor_encoder_settings_t* settings);
THOR_EXPORT int thor_encoder_settings_validate(thor_encoder_settings_t* settings);

THOR_EXPORT thor_decoder_settings_t* thor_decoder_settings_new(int argc, char** argv);
THOR_EXPORT void thor_decoder_settings_free(thor_decoder_settings_t* settings);
THOR_EXPORT int thor_decoder_settings_validate(thor_decoder_settings_t* settings);

#ifdef __cplusplus
}
#endif

#endif /* THOR_SETTINGS_H */

