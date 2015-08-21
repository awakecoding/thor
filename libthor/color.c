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

#include "color.h"

/**
 * | R |    ( | 256     0    403 | |    Y    | )
 * | G | = (  | 256   -48   -120 | | U - 128 |  ) >> 8
 * | B |    ( | 256   475      0 | | V - 128 | )
 *
 * | Y |    ( |  54   183     18 | | R | )         |  0  |
 * | U | = (  | -29   -99    128 | | G |  ) >> 8 + | 128 |
 * | V |    ( | 128  -116    -12 | | B | )         | 128 |
 */

void thor_YUV420ToRGB_8u_P3AC4R(const uint8_t* pSrc[3], int srcStep[3],
			       uint8_t* pDst, int dstStep, int width, int height)
{
	int x, y;
	int dstPad;
	int srcPad[3];
	uint8_t Y, U, V;
	int halfWidth;
	int halfHeight;
	const uint8_t* pY;
	const uint8_t* pU;
	const uint8_t* pV;
	int R, G, B;
	int Yp, Up, Vp;
	int Up48, Up475;
	int Vp403, Vp120;
	uint8_t* pRGB = pDst;
	int nWidth, nHeight;
	int lastRow, lastCol;

	pY = pSrc[0];
	pU = pSrc[1];
	pV = pSrc[2];

	lastCol = width & 0x01;
	lastRow = height & 0x01;

	nWidth = (width + 1) & ~0x0001;
	nHeight = (height + 1) & ~0x0001;

	halfWidth = nWidth / 2;
	halfHeight = nHeight / 2;

	srcPad[0] = (srcStep[0] - nWidth);
	srcPad[1] = (srcStep[1] - halfWidth);
	srcPad[2] = (srcStep[2] - halfWidth);

	dstPad = (dstStep - (nWidth * 4));

	for (y = 0; y < halfHeight; )
	{
		if (++y == halfHeight)
			lastRow <<= 1;

		for (x = 0; x < halfWidth; )
		{
			if (++x == halfWidth)
				lastCol <<= 1;

			U = *pU++;
			V = *pV++;

			Up = U - 128;
			Vp = V - 128;

			Up48 = 48 * Up;
			Up475 = 475 * Up;

			Vp403 = Vp * 403;
			Vp120 = Vp * 120;

			/* 1st pixel */

			Y = *pY++;
			Yp = Y << 8;

			R = (Yp + Vp403) >> 8;
			G = (Yp - Up48 - Vp120) >> 8;
			B = (Yp + Up475) >> 8;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;

			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;

			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			*pRGB++ = (uint8_t) B;
			*pRGB++ = (uint8_t) G;
			*pRGB++ = (uint8_t) R;
			*pRGB++ = 0xFF;

			/* 2nd pixel */

			if (!(lastCol & 0x02))
			{
				Y = *pY++;
				Yp = Y << 8;

				R = (Yp + Vp403) >> 8;
				G = (Yp - Up48 - Vp120) >> 8;
				B = (Yp + Up475) >> 8;

				if (R < 0)
					R = 0;
				else if (R > 255)
					R = 255;

				if (G < 0)
					G = 0;
				else if (G > 255)
					G = 255;

				if (B < 0)
					B = 0;
				else if (B > 255)
					B = 255;

				*pRGB++ = (uint8_t) B;
				*pRGB++ = (uint8_t) G;
				*pRGB++ = (uint8_t) R;
				*pRGB++ = 0xFF;
			}
			else
			{
				pY++;
				pRGB += 4;
				lastCol >>= 1;
			}
		}

		pY += srcPad[0];
		pU -= halfWidth;
		pV -= halfWidth;
		pRGB += dstPad;

		for (x = 0; x < halfWidth; )
		{
			if (++x == halfWidth)
				lastCol <<= 1;

			U = *pU++;
			V = *pV++;

			Up = U - 128;
			Vp = V - 128;

			Up48 = 48 * Up;
			Up475 = 475 * Up;

			Vp403 = Vp * 403;
			Vp120 = Vp * 120;

			/* 3rd pixel */

			Y = *pY++;
			Yp = Y << 8;

			R = (Yp + Vp403) >> 8;
			G = (Yp - Up48 - Vp120) >> 8;
			B = (Yp + Up475) >> 8;

			if (R < 0)
				R = 0;
			else if (R > 255)
				R = 255;

			if (G < 0)
				G = 0;
			else if (G > 255)
				G = 255;

			if (B < 0)
				B = 0;
			else if (B > 255)
				B = 255;

			*pRGB++ = (uint8_t) B;
			*pRGB++ = (uint8_t) G;
			*pRGB++ = (uint8_t) R;
			*pRGB++ = 0xFF;

			/* 4th pixel */

			if (!(lastCol & 0x02))
			{
				Y = *pY++;
				Yp = Y << 8;

				R = (Yp + Vp403) >> 8;
				G = (Yp - Up48 - Vp120) >> 8;
				B = (Yp + Up475) >> 8;

				if (R < 0)
					R = 0;
				else if (R > 255)
					R = 255;

				if (G < 0)
					G = 0;
				else if (G > 255)
					G = 255;

				if (B < 0)
					B = 0;
				else if (B > 255)
					B = 255;

				*pRGB++ = (uint8_t) B;
				*pRGB++ = (uint8_t) G;
				*pRGB++ = (uint8_t) R;
				*pRGB++ = 0xFF;
			}
			else
			{
				pY++;
				pRGB += 4;
				lastCol >>= 1;
			}
		}

		pY += srcPad[0];
		pU += srcPad[1];
		pV += srcPad[2];
		pRGB += dstPad;
	}
}

void thor_RGBToYUV420_8u_P3AC4R(const uint8_t* pSrc, INT32 srcStep,
				uint8_t* pDst[3], INT32 dstStep[3], int width, int height)
{
	int x, y;
	int dstPad[3];
	int halfWidth;
	int halfHeight;
	uint8_t* pY;
	uint8_t* pU;
	uint8_t* pV;
	int Y, U, V;
	int R, G, B;
	int Ra, Ga, Ba;
	const uint8_t* pRGB;
	int nWidth, nHeight;

	pU = pDst[1];
	pV = pDst[2];

	nWidth = (width + 1) & ~0x0001;
	nHeight = (height + 1) & ~0x0001;

	halfWidth = nWidth / 2;
	halfHeight = nHeight / 2;

	dstPad[0] = (dstStep[0] - nWidth);
	dstPad[1] = (dstStep[1] - halfWidth);
	dstPad[2] = (dstStep[2] - halfWidth);

	for (y = 0; y < halfHeight; y++)
	{
		for (x = 0; x < halfWidth; x++)
		{
			/* 1st pixel */
			pRGB = pSrc + y * 2 * srcStep + x * 2 * 4;
			pY = pDst[0] + y * 2 * dstStep[0] + x * 2;
			Ba = B = pRGB[0];
			Ga = G = pRGB[1];
			Ra = R = pRGB[2];
			Y = (54 * R + 183 * G + 18 * B) >> 8;
			pY[0] = (uint8_t) Y;

			if (x * 2 + 1 < width)
			{
				/* 2nd pixel */
				Ba += B = pRGB[4];
				Ga += G = pRGB[5];
				Ra += R = pRGB[6];
				Y = (54 * R + 183 * G + 18 * B) >> 8;
				pY[1] = (uint8_t) Y;
			}

			if (y * 2 + 1 < height)
			{
				/* 3rd pixel */
				pRGB += srcStep;
				pY += dstStep[0];
				Ba += B = pRGB[0];
				Ga += G = pRGB[1];
				Ra += R = pRGB[2];
				Y = (54 * R + 183 * G + 18 * B) >> 8;
				pY[0] = (uint8_t) Y;

				if (x * 2 + 1 < width)
				{
					/* 4th pixel */
					Ba += B = pRGB[4];
					Ga += G = pRGB[5];
					Ra += R = pRGB[6];
					Y = (54 * R + 183 * G + 18 * B) >> 8;
					pY[1] = (uint8_t) Y;
				}
			}

			/* U */
			Ba >>= 2;
			Ga >>= 2;
			Ra >>= 2;
			U = ((-29 * Ra - 99 * Ga + 128 * Ba) >> 8) + 128;
			if (U < 0)
				U = 0;
			else if (U > 255)
				U = 255;
			*pU++ = (uint8_t) U;

			/* V */
			V = ((128 * Ra - 116 * Ga - 12 * Ba) >> 8) + 128;
			if (V < 0)
				V = 0;
			else if (V > 255)
				V = 255;
			*pV++ = (uint8_t) V;
		}

		pU += dstPad[1];
		pV += dstPad[2];
	}
}
