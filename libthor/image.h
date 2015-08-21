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

#ifndef THOR_IMAGE_H
#define THOR_IMAGE_H

#include "thor.h"

#define THOR_IMAGE_BITMAP		0
#define THOR_IMAGE_PNG			1

struct thor_image_s
{
	int type;
	int width;
	int height;
	uint8_t* data;
	int scanline;
	int bitsPerPixel;
	int bytesPerPixel;
};
typedef struct thor_image_s thor_image_t;

int thor_bitmap_write(const char* filename, uint8_t* data, int width, int height, int bpp);

int thor_image_write(thor_image_t* image, const char* filename);
int thor_image_read(thor_image_t* image, const char* filename);

int thor_image_read_buffer(thor_image_t* image, uint8_t* buffer, int size);

thor_image_t* thor_image_new();
void thor_image_free(thor_image_t* image, int freeBuffer);

#endif /* THOR_IMAGE_H */
