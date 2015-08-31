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

#define THOR_IMAGE_NONE			0
#define THOR_IMAGE_BMP			1
#define THOR_IMAGE_PNG			2
#define THOR_IMAGE_Y4M			3

struct thor_image_s
{
	int type;
	FILE* fp;
	int width;
	int height;
	uint8_t* data;
	int scanline;
	int bitsPerPixel;
	int bytesPerPixel;

	double y4m_fps;
	int y4m_frame_index;
	int y4m_frame_count;
	int y4m_file_hdrlen;
	int y4m_frame_hdrlen;
};
typedef struct thor_image_s thor_image_t;

#ifdef __cplusplus
extern "C" {
#endif

THOR_EXPORT int thor_bmp_write(const char* filename, uint8_t* data, int width, int height, int bpp);
THOR_EXPORT int thor_png_write(const char* filename, uint8_t* data, int width, int height, int bpp);

THOR_EXPORT int thor_y4m_read_frame(thor_image_t* img, uint8_t* pDst[3], int32_t dstStep[3]);
THOR_EXPORT int thor_y4m_write_frame(thor_image_t* img, uint8_t* pSrc[3], int32_t srcStep[3]);

THOR_EXPORT int thor_image_read(thor_image_t* img, const char* filename);
THOR_EXPORT int thor_image_write(thor_image_t* img, const char* filename);

THOR_EXPORT thor_image_t* thor_image_new();
THOR_EXPORT void thor_image_free(thor_image_t* img, int freeBuffer);

#ifdef __cplusplus
}
#endif

#endif /* THOR_IMAGE_H */
