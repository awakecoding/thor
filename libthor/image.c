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

#include "image.h"

#include "../external/lodepng/lodepng.h"

#pragma pack(push, 1)

struct _THOR_BITMAP_FILE_HEADER
{
	uint8_t bfType[2];
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
};
typedef struct _THOR_BITMAP_FILE_HEADER THOR_BITMAP_FILE_HEADER;

struct _THOR_BITMAP_INFO_HEADER
{
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
};
typedef struct _THOR_BITMAP_INFO_HEADER THOR_BITMAP_INFO_HEADER;

struct _THOR_BITMAP_CORE_HEADER
{
	uint32_t bcSize;
	uint16_t bcWidth;
	uint16_t bcHeight;
	uint16_t bcPlanes;
	uint16_t bcBitCount;
};
typedef struct _THOR_BITMAP_CORE_HEADER THOR_BITMAP_CORE_HEADER;

#pragma pack(pop)

int thor_bitmap_write(const char* filename, uint8_t* data, int width, int height, int bpp)
{
	FILE* fp;
	int status = 1;
	THOR_BITMAP_FILE_HEADER bf;
	THOR_BITMAP_INFO_HEADER bi;

	fp = fopen(filename, "w+b");

	if (!fp)
		return -1;

	bf.bfType[0] = 'B';
	bf.bfType[1] = 'M';
	bf.bfReserved1 = 0;
	bf.bfReserved2 = 0;
	bf.bfOffBits = sizeof(THOR_BITMAP_FILE_HEADER) + sizeof(THOR_BITMAP_INFO_HEADER);
	bi.biSizeImage = width * height * (bpp / 8);
	bf.bfSize = bf.bfOffBits + bi.biSizeImage;

	bi.biWidth = width;
	bi.biHeight = -1 * height;
	bi.biPlanes = 1;
	bi.biBitCount = bpp;
	bi.biCompression = 0;
	bi.biXPelsPerMeter = width;
	bi.biYPelsPerMeter = height;
	bi.biClrUsed = 0;
	bi.biClrImportant = 0;
	bi.biSize = sizeof(THOR_BITMAP_INFO_HEADER);

	if (fwrite((void*) &bf, sizeof(THOR_BITMAP_FILE_HEADER), 1, fp) != 1 ||
	    fwrite((void*) &bi, sizeof(THOR_BITMAP_INFO_HEADER), 1, fp) != 1 ||
	    fwrite((void*) data, bi.biSizeImage, 1, fp) != 1)
	{
		status = -1;
	}

	fclose(fp);

	return status;
}

int thor_image_write(thor_image_t* image, const char* filename)
{
	int status = -1;

	if (image->type == THOR_IMAGE_BITMAP)
	{
		status = thor_bitmap_write(filename, image->data, image->width, image->height, image->bitsPerPixel);
	}
	else
	{
		status = lodepng_encode32_file(filename, image->data, image->width, image->height) ? -1 : 1;
	}

	return status;
}

int thor_image_png_read_fp(thor_image_t* image, FILE* fp)
{
	int size;
	int status;
	uint8_t* data;
	uint32_t width;
	uint32_t height;

	fseek(fp, 0, SEEK_END);
	size = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	data = (uint8_t*) malloc(size);

	if (!data)
		return -1;

	if (fread((void*) data, size, 1, fp) != 1)
		return -1;

	status = lodepng_decode32(&(image->data), &width, &height, data, size);

	free(data);

	if (status)
		return -1;

	image->width = width;
	image->height = height;

	image->bitsPerPixel = 32;
	image->bytesPerPixel = 4;
	image->scanline = image->bytesPerPixel * image->width;

	return 1;
}

int thor_image_png_read_buffer(thor_image_t* image, uint8_t* buffer, int size)
{
	int status;
	uint32_t width;
	uint32_t height;

	status = lodepng_decode32(&(image->data), &width, &height, buffer, size);

	if (status)
		return -1;

	image->width = width;
	image->height = height;

	image->bitsPerPixel = 32;
	image->bytesPerPixel = 4;
	image->scanline = image->bytesPerPixel * image->width;

	return 1;
}

int thor_image_bitmap_read_fp(thor_image_t* image, FILE* fp)
{
	int index;
	int vflip;
	uint8_t* pDstData;
	THOR_BITMAP_FILE_HEADER bf;
	THOR_BITMAP_INFO_HEADER bi;

	if (fread((void*) &bf, sizeof(THOR_BITMAP_FILE_HEADER), 1, fp) != 1)
		return -1;

	if ((bf.bfType[0] != 'B') || (bf.bfType[1] != 'M'))
		return -1;

	image->type = THOR_IMAGE_BITMAP;

	if (fread((void*) &bi, sizeof(THOR_BITMAP_INFO_HEADER), 1, fp) != 1)
		return -1;

	if (ftell(fp) != bf.bfOffBits)
	{
		fseek(fp, bf.bfOffBits, SEEK_SET);
	}

	image->width = bi.biWidth;

	if (bi.biHeight < 0)
	{
		vflip = 0;
		image->height = -1 * bi.biHeight;
	}
	else
	{
		vflip = 1;
		image->height = bi.biHeight;
	}

	image->bitsPerPixel = bi.biBitCount;
	image->bytesPerPixel = (image->bitsPerPixel / 8);
	image->scanline = (bi.biSizeImage / image->height);

	image->data = (uint8_t*) malloc(bi.biSizeImage);

	if (!image->data)
		return -1;

	if (!vflip)
	{
		if (fread((void*) image->data, bi.biSizeImage, 1, fp) != 1)
		{
			free(image->data);
			image->data = NULL;
			return -1;
		}
	}
	else
	{
		pDstData = &(image->data[(image->height - 1) * image->scanline]);

		for (index = 0; index < image->height; index++)
		{
			if (fread((void*) pDstData, image->scanline, 1, fp) != 1)
			{
				free(image->data);
				image->data = NULL;
				return -1;
			}
			pDstData -= image->scanline;
		}
	}

	return 1;
}

int thor_image_bitmap_read_buffer(thor_image_t* image, uint8_t* buffer, int size)
{
	int index;
	int vflip;
	uint8_t* pSrcData;
	uint8_t* pDstData;
	THOR_BITMAP_FILE_HEADER bf;
	THOR_BITMAP_INFO_HEADER bi;

	pSrcData = buffer;

	memcpy(&bf, pSrcData, sizeof(THOR_BITMAP_FILE_HEADER));
	pSrcData += sizeof(THOR_BITMAP_FILE_HEADER);

	if ((bf.bfType[0] != 'B') || (bf.bfType[1] != 'M'))
		return -1;

	image->type = THOR_IMAGE_BITMAP;

	memcpy(&bi, pSrcData, sizeof(THOR_BITMAP_INFO_HEADER));
	pSrcData += sizeof(THOR_BITMAP_INFO_HEADER);

	if ((pSrcData - buffer) != bf.bfOffBits)
	{
		pSrcData = &buffer[bf.bfOffBits];
	}

	image->width = bi.biWidth;

	if (bi.biHeight < 0)
	{
		vflip = 0;
		image->height = -1 * bi.biHeight;
	}
	else
	{
		vflip = 1;
		image->height = bi.biHeight;
	}

	image->bitsPerPixel = bi.biBitCount;
	image->bytesPerPixel = (image->bitsPerPixel / 8);
	image->scanline = (bi.biSizeImage / image->height);

	image->data = (uint8_t*) malloc(bi.biSizeImage);

	if (!image->data)
		return -1;

	if (!vflip)
	{
		memcpy(image->data, pSrcData, bi.biSizeImage);
		pSrcData += bi.biSizeImage;
	}
	else
	{
		pDstData = &(image->data[(image->height - 1) * image->scanline]);

		for (index = 0; index < image->height; index++)
		{
			memcpy(pDstData, pSrcData, image->scanline);
			pSrcData += image->scanline;
			pDstData -= image->scanline;
		}
	}

	return 1;
}

int thor_image_read(thor_image_t* image, const char* filename)
{
	FILE* fp;
	uint8_t sig[8];
	int status = -1;

	fp = fopen(filename, "r+b");

	if (!fp)
		return -1;

	if (fread((void*) &sig, sizeof(sig), 1, fp) != 1 || fseek(fp, 0, SEEK_SET) < 0)
	{
		fclose(fp);
		return -1;
	}

	if ((sig[0] == 'B') && (sig[1] == 'M'))
	{
		image->type = THOR_IMAGE_BITMAP;
		status = thor_image_bitmap_read_fp(image, fp);
	}
	else if ((sig[0] == 0x89) && (sig[1] == 'P') && (sig[2] == 'N') && (sig[3] == 'G') &&
		 (sig[4] == '\r') && (sig[5] == '\n') && (sig[6] == 0x1A) && (sig[7] == '\n'))
	{
		image->type = THOR_IMAGE_PNG;
		status = thor_image_png_read_fp(image, fp);
	}
	fclose(fp);

	return status;
}

int thor_image_read_buffer(thor_image_t* image, uint8_t* buffer, int size)
{
	uint8_t sig[8];
	int status = -1;

	if (size < 8)
		return -1;

	memcpy(sig, buffer, 8);

	if ((sig[0] == 'B') && (sig[1] == 'M'))
	{
		image->type = THOR_IMAGE_BITMAP;
		status = thor_image_bitmap_read_buffer(image, buffer, size);
	}
	else if ((sig[0] == 0x89) && (sig[1] == 'P') && (sig[2] == 'N') && (sig[3] == 'G') &&
		 (sig[4] == '\r') && (sig[5] == '\n') && (sig[6] == 0x1A) && (sig[7] == '\n'))
	{
		image->type = THOR_IMAGE_PNG;
		status = thor_image_png_read_buffer(image, buffer, size);
	}

	return status;
}

thor_image_t* thor_image_new()
{
	thor_image_t * image;

	image = (thor_image_t *) calloc(1, sizeof(thor_image_t));

	if (!image)
		return NULL;

	return image;
}

void thor_image_free(thor_image_t * image, int freeBuffer)
{
	if (!image)
		return;

	if (freeBuffer)
		free(image->data);

	free(image);
}

#include "../external/lodepng/lodepng.c"
