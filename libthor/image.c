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

#include <thor/image.h>

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

int thor_bmp_write(const char* filename, uint8_t* data, int width, int height, int bpp)
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

int thor_png_write(const char* filename, uint8_t* data, int width, int height, int bpp)
{
	int status;

	status = thor_lodepng_encode32_file(filename, data, width, height) ? -1 : 1;

	return status;
}

int thor_y4m_write(thor_image_t* img, const char* filename)
{
	img->fp = fopen(filename, "wb");

	if (!img->fp)
		return -1;

	fprintf(img->fp, "YUV4MPEG2 W%d H%d F30:1 Ip A1:1 C420jpeg XYSCSS=420JPEG\n",
		img->width, img->height);

	return 1;
}

int thor_y4m_write_frame(thor_image_t* img, uint8_t* pSrc[3], int32_t srcStep[3])
{
	uint32_t lumaSize;
	uint32_t chromaSize;

	lumaSize = img->width * img->height;
	chromaSize = lumaSize / 4;

	fprintf(img->fp, "FRAME\n");

	if (fwrite(pSrc[0], 1, lumaSize, img->fp) != lumaSize)
		return -1;

	if (fwrite(pSrc[1], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	if (fwrite(pSrc[2], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	img->frameCount++;

	return 1;
}

int thor_yuv_write(thor_image_t* img, const char* filename)
{
	img->fp = fopen(filename, "wb");

	if (!img->fp)
		return -1;

	return 1;
}

int thor_yuv_write_frame(thor_image_t* img, uint8_t* pSrc[3], int32_t srcStep[3])
{
	uint32_t lumaSize;
	uint32_t chromaSize;

	lumaSize = img->width * img->height;
	chromaSize = lumaSize / 4;

	if (fwrite(pSrc[0], 1, lumaSize, img->fp) != lumaSize)
		return -1;

	if (fwrite(pSrc[1], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	if (fwrite(pSrc[2], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	img->frameCount++;

	return 1;
}

int thor_rgb_write(thor_image_t* img, const char* filename)
{
	img->fp = fopen(filename, "wb");

	if (!img->fp)
		return -1;

	return 1;
}

int thor_rgb_write_frame(thor_image_t* img, uint8_t* pSrc, int32_t srcStep)
{
	int32_t srcSize = srcStep * img->height;

	if (fwrite(pSrc, 1, srcSize, img->fp) != srcSize)
		return -1;

	img->frameCount++;

	return 1;
}

int thor_image_write(thor_image_t* img, const char* filename)
{
	int status = -1;

	if (img->type == THOR_IMAGE_RGB)
	{
		status = thor_rgb_write(img, filename);
	}
	else if (img->type == THOR_IMAGE_YUV)
	{
		status = thor_yuv_write(img, filename);
	}
	else if (img->type == THOR_IMAGE_BMP)
	{
		status = thor_bmp_write(filename, img->data, img->width, img->height, img->bitsPerPixel);
	}
	else if (img->type == THOR_IMAGE_PNG)
	{
		status = thor_lodepng_encode32_file(filename, img->data, img->width, img->height) ? -1 : 1;
	}
	else if (img->type == THOR_IMAGE_Y4M)
	{
		status = thor_y4m_write(img, filename);
	}

	return status;
}

int thor_image_png_read_fp(thor_image_t* img, FILE* fp)
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

	status = thor_lodepng_decode32(&(img->data), &width, &height, data, size);

	free(data);

	if (status)
		return -1;

	img->width = width;
	img->height = height;

	img->bitsPerPixel = 32;
	img->bytesPerPixel = 4;
	img->scanline = img->bytesPerPixel * img->width;

	return 1;
}

int thor_image_bmp_read_fp(thor_image_t* img, FILE* fp)
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

	img->type = THOR_IMAGE_BMP;

	if (fread((void*) &bi, sizeof(THOR_BITMAP_INFO_HEADER), 1, fp) != 1)
		return -1;

	if (ftell(fp) != bf.bfOffBits)
	{
		fseek(fp, bf.bfOffBits, SEEK_SET);
	}

	img->width = bi.biWidth;

	if (bi.biHeight < 0)
	{
		vflip = 0;
		img->height = -1 * bi.biHeight;
	}
	else
	{
		vflip = 1;
		img->height = bi.biHeight;
	}

	img->bitsPerPixel = bi.biBitCount;
	img->bytesPerPixel = (img->bitsPerPixel / 8);
	img->scanline = (bi.biSizeImage / img->height);

	img->data = (uint8_t*) malloc(bi.biSizeImage);

	if (!img->data)
		return -1;

	if (!vflip)
	{
		if (fread((void*) img->data, bi.biSizeImage, 1, fp) != 1)
		{
			free(img->data);
			img->data = NULL;
			return -1;
		}
	}
	else
	{
		pDstData = &(img->data[(img->height - 1) * img->scanline]);

		for (index = 0; index < img->height; index++)
		{
			if (fread((void*) pDstData, img->scanline, 1, fp) != 1)
			{
				free(img->data);
				img->data = NULL;
				return -1;
			}
			pDstData -= img->scanline;
		}
	}

	return 1;
}

int thor_image_y4m_read_fp(thor_image_t* img, FILE* fp)
{
	int pos;
	int len;
	int num;
	int den;
	char* end;
	char buf[256];
	double fps = 30;
	uint32_t width = 0;
	uint32_t height = 0;
	uint32_t lumaSize;
	uint32_t chromaSize;
	uint32_t frameSize;
	uint32_t fileSize;

	len = (int) fread(buf, 1, sizeof(buf), fp);
	buf[255] = '\0';

	if (strncmp(buf, "YUV4MPEG2 ", 10) != 0)
		return -1;

	pos = 10;

	while ((pos < len) && (buf[pos] != '\n'))
	{
		switch (buf[pos++])
		{
			case 'W':
				width = strtol(buf + pos, &end, 10);
				pos = end - buf + 1;
				break;
			case 'H':
				height = strtol(buf + pos, &end, 10);
				pos = end - buf + 1;
				break;
			case 'F':
				den = strtol(buf+pos, &end, 10);
				pos = end - buf + 1;
				num = strtol(buf + pos, &end, 10);
				pos = end - buf + 1;
				fps = (double) (den / num);
				break;
			case 'I':
				if (buf[pos] != 'p') {
					fprintf(stderr, "Only progressive input supported\n");
					return -1;
				}
				break;
			case 'C':
				if (strcmp(buf+pos, "C420")) {
				}
				/* Fallthrough */
			case 'A': /* Ignored */
			case 'X':
			default:
				while (buf[pos] != ' ' && buf[pos] != '\n' && pos < len)
					pos++;
				break;
		}
	}

	if (strncmp(buf + pos, "\nFRAME\n", 7) != 0)
		return -1;

	img->y4m_fps = fps;
	img->y4m_file_hdrlen = pos + 1;
	img->y4m_frame_hdrlen = 6;

	img->width = width;
	img->height = height;

	lumaSize = img->width * img->height;
	chromaSize = lumaSize / 4;
	frameSize = img->y4m_frame_hdrlen + lumaSize + (chromaSize * 2);

	fseek(fp, 0, SEEK_END);
	fileSize = (uint32_t) ftell(fp);
	fseek(fp, img->y4m_file_hdrlen, SEEK_SET);

	img->frameIndex = 0;
	img->frameCount = (fileSize - img->y4m_file_hdrlen) / frameSize;

	return 1;
}

int thor_y4m_read_frame(thor_image_t* img, uint8_t* pDst[3], int32_t dstStep[3])
{
	uint32_t lumaSize;
	uint32_t chromaSize;

	if (fseek(img->fp, img->y4m_frame_hdrlen, SEEK_CUR) != 0)
		return -1;

	lumaSize = img->width * img->height;
	chromaSize = lumaSize / 4;

	if (fread(pDst[0], 1, lumaSize, img->fp) != lumaSize)
		return -1;

	if (fread(pDst[1], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	if (fread(pDst[2], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	img->frameIndex++;

	return 1;
}

int thor_yuv_read_frame(thor_image_t* img, uint8_t* pDst[3], int32_t dstStep[3])
{
	uint32_t lumaSize;
	uint32_t chromaSize;

	lumaSize = img->width * img->height;
	chromaSize = lumaSize / 4;

	if (fread(pDst[0], 1, lumaSize, img->fp) != lumaSize)
		return -1;

	if (fread(pDst[1], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	if (fread(pDst[2], 1, chromaSize, img->fp) != chromaSize)
		return -1;

	img->frameIndex++;

	return 1;
}

int thor_rgb_read_frame(thor_image_t* img, uint8_t* pDst, int32_t dstStep)
{
	uint32_t frameSize = img->scanline * img->height;

	if (fread(pDst, 1, frameSize, img->fp) != frameSize)
		return -1;

	img->frameIndex++;

	return 1;
}

int thor_image_read(thor_image_t* img, const char* filename)
{
	FILE* fp;
	uint8_t sig[10];
	size_t fileSize;
	size_t frameSize;
	int status = -1;

	fp = fopen(filename, "r+b");

	if (!fp)
		return -1;

	fseek(fp, 0, SEEK_END);
	fileSize = ftell(fp);
	fseek(fp, 0, SEEK_SET);

	if (!img->type)
	{
		if (strstr(filename, ".png"))
			img->type = THOR_IMAGE_PNG;
		else if (strstr(filename, ".bmp"))
			img->type = THOR_IMAGE_BMP;
		else if (strstr(filename, ".y4m"))
			img->type = THOR_IMAGE_Y4M;
		else if (strstr(filename, ".yuv"))
			img->type = THOR_IMAGE_YUV;
		else if (strstr(filename, ".rgb"))
			img->type = THOR_IMAGE_RGB;
	}

	if (img->type == THOR_IMAGE_RGB)
	{
		img->fp = fp;
		frameSize = img->scanline * img->height;
		img->frameCount = (int) (fileSize / frameSize);
		return 1;
	}
	else if (img->type == THOR_IMAGE_YUV)
	{
		img->fp = fp;
		frameSize = (img->width * img->height) + ((img->width * img->height) / 2);
		img->frameCount = (int) (fileSize / frameSize);
		return 1;
	}

	if (fread((void*) &sig, sizeof(sig), 1, fp) != 1 || fseek(fp, 0, SEEK_SET) < 0)
	{
		fclose(fp);
		return -1;
	}

	if ((sig[0] == 'B') && (sig[1] == 'M'))
	{
		img->type = THOR_IMAGE_BMP;
		status = thor_image_bmp_read_fp(img, fp);
		fclose(fp);
	}
	else if ((sig[0] == 0x89) && (sig[1] == 'P') && (sig[2] == 'N') && (sig[3] == 'G') &&
		 (sig[4] == '\r') && (sig[5] == '\n') && (sig[6] == 0x1A) && (sig[7] == '\n'))
	{
		img->type = THOR_IMAGE_PNG;
		status = thor_image_png_read_fp(img, fp);
		fclose(fp);
	}
	else if ((sig[0] == 'Y') && (sig[1] == 'U') && (sig[2] == 'V') && (sig[3] == '4') &&
		 (sig[4] == 'M') && (sig[5] == 'P') && (sig[6] == 'E') && (sig[7] == 'G') &&
		 (sig[8] == '2') && (sig[9] == ' '))
	{
		img->fp = fp;
		img->type = THOR_IMAGE_Y4M;
		status = thor_image_y4m_read_fp(img, fp);
	}
	else
	{
		img->type = THOR_IMAGE_NONE;
		fclose(fp);
	}

	if (!img->type)
		return -1;

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

void thor_image_free(thor_image_t * img, int freeBuffer)
{
	if (!img)
		return;

	if (img->fp)
	{
		fclose(img->fp);
		img->fp = NULL;
	}

	if (freeBuffer)
	{
		free(img->data);
		img->data = NULL;
	}

	free(img);
}

#include "../external/lodepng/lodepng.c"
