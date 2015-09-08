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

#include <assert.h>
#include <stdlib.h>

#include "simd.h"
#include "bits.h"
#include "vlc.h"

int get_vlc0_limit(int maxbit, stream_t* str)
{
	int code;
	int tmp = 0;
	int nbit = 0;

	while (tmp==0 && nbit < maxbit){
		tmp = getbits1(str);
		nbit++;
	}

	code = tmp==0 ? maxbit : nbit - 1;

	return code;
}

int get_vlc(int n, stream_t* str)
{
	int cw,bit,zeroes=0,done=0,tmp;
	unsigned int val = 0;
	int first;
	unsigned int lead = 0;

	if (n < 6)
	{
		while (!done && zeroes < 6)
		{
			bit = getbits1(str);
			if (bit)
			{
				cw = getbits(str,n);
				done = 1;
			}
			else zeroes++;
		}

		if (done)
		{
			val = (zeroes<<n)+cw;
		}
		else
		{
			lead = n;
			while (!done)
			{
				first = showbits(str,1);

				if (!first)
				{
					lead++;
					flushbits(str,1);
				}
				else
				{
					tmp = getbits(str,lead+1);
					val = 6 * (1 << n) + tmp - (1 << n);
					done = 1;
				}
			}
		}
	}
	else if (n < 8)
	{
		while (!done)
		{
			bit = getbits1(str);
			if (bit)
			{
				cw = getbits(str,(n-4));
				done = 1;
			}
			else zeroes++;
		}
		val = (zeroes<<(n-4))+cw;
	}
	else if (n == 8)
	{
		if (getbits1(str))
		{
			val = 0;
		}
		else if (getbits1(str))
		{
			val = 1;
		}
		else
		{
			val = 2;
		}
	}
	else if (n == 9)
	{
		if (getbits1(str))
		{
			if (getbits1(str))
			{
				val = getbits(str,3)+3;
			}
			else if (getbits1(str))
			{
				val = getbits1(str)+1;
			}
			else
			{
				val = 0;
			}
		}
		else
		{
			while (!done)
			{
				bit = getbits1(str);
				if (bit)
				{
					cw = getbits(str,4);
					done = 1;
				}
				else zeroes++;
			}
			val = (zeroes<<4)+cw+11;
		}
	}
	else if (n == 10)
	{
		while (!done)
		{
			first = showbits(str,1);
			if (!first)
			{
				lead++;
				flushbits(str,1);
			}
			else
			{
				val = getbits(str,lead+1)-1;
				done = 1;
			}
		}
	}
	else if (n == 11)
	{
		int tmp;
		tmp = getbits(str,1);

		if (tmp)
		{
			val = 0;
		}
		else
		{
			tmp = getbits(str,1);

			if (tmp)
			{
				val = 1;
			}
			else
			{
				tmp = 0;
				val = 0;

				while (tmp==0)
				{
					tmp = getbits(str,1);
					val = val+2;
				}

				val += getbits(str,1);
			}
		}
	}

	else if (n == 12)
	{
		tmp = 0;
		val = 0;

		while (tmp==0 && val<4)
		{
			tmp = getbits(str,1);
			val += (!tmp);
		}
	}
	else if (n == 13)
	{
		tmp = 0;
		val = 0;

		while (tmp==0 && val<6)
		{
			tmp = getbits(str,1);
			val += (!tmp);
		}
	}
	else
	{
		printf("Illegal VLC table number. 0-10 allowed only.");
	}

	return val;
}

int put_vlc(unsigned int n, unsigned int cn, stream_t* str)
{
	unsigned int tmp;
	unsigned int len = 0;
	unsigned int code = 0;

	switch (n)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if ((int)cn < (6 * (1 << n)))
		{
			tmp = 1<<n;
			code = tmp+(cn & (tmp-1));
			len = 1+n+(cn>>n);
		}
		else
		{
			code = cn - (6 * (1 << n)) + (1 << n);
			len = (6-n)+1+2*log2i(code);
		}
		break;
	case 6:
	case 7: //TODO: Remove this if not used
		tmp = 1<<(n-4);
		code = tmp+cn%tmp;
		len = 1+(n-4)+(cn>>(n-4));
		break;
	case 8:
		if (cn == 0)
		{
			code = 1;
			len = 1;
		}
		else if (cn == 1)
		{
			code = 1;
			len = 2;
		}
		else if (cn == 2)
		{
			code = 0;
			len = 2;
		}
		else fatalerror("Code number too large for VLC8.");
		break;
	case 9:
		if (cn == 0)
		{
			code = 4;
			len = 3;
		}
		else if (cn == 1)
		{
			code = 10;
			len = 4;
		}
		else if (cn == 2)
		{
			code = 11;
			len = 4;
		}
		else if (cn < 11)
		{
			code = cn+21;
			len = 5;
		}
		else
		{
			tmp = 1<<4;
			code = tmp+(cn+5)%tmp;
			len = 5+((cn+5)>>4);
		}
		break;
	case 10:
		code = cn+1;
		len = 1+2*log2i(code);
		break;
	case 11:
		len = cn < 2 ? cn + 1 : cn/2 + 3;
		code = cn < 2 ? 1 : 2 + (cn&1);
		break;
	case 12:
		len = min(4,cn+1);
		code = cn != 4;
		break;
	case 13:
		len = min(6,cn+1);
		code = cn != 6;
		break;
	default:
		fatalerror("No such VLC table, only 0-13 allowed.");
	}

	putbits(len,code,str);

	return len;
}

int quote_vlc(unsigned int n, unsigned int cn)
{
	unsigned int tmp;
	unsigned int len = 0;
	unsigned int code = 0;

	switch (n)
	{
	case 0:
	case 1:
	case 2:
	case 3:
	case 4:
	case 5:
		if ((int)cn < (6 * (1 << n)))
		{
			tmp = 1<<n;
			code = tmp+(cn & (tmp-1));
			len = 1+n+(cn>>n);
		}
		else
		{
			code = cn - (6 * (1 << n)) + (1 << n);
			len = (6-n)+1+2*log2i(code);
		}
		break;
	case 6:
	case 7:
		tmp = 1<<(n-4);
		code = tmp+cn%tmp;
		len = 1+(n-4)+(cn>>(n-4));
		break;
	case 8:
		if (cn == 0)
		{
			code = 1;
			len = 1;
		}
		else if (cn == 1)
		{
			code = 1;
			len = 2;
		}
		else if (cn == 2)
		{
			code = 0;
			len = 2;
		}
		else fatalerror("Code number too large for VLC8.");
		break;
	case 9:
		if (cn == 0)
		{
			code = 4;
			len = 3;
		}
		else if (cn == 1)
		{
			code = 10;
			len = 4;
		}
		else if (cn == 2)
		{
			code = 11;
			len = 4;
		}
		else if (cn < 11)
		{
			code = cn+21;
			len = 5;
		}
		else
		{
			tmp = 1<<4;
			code = tmp+(cn+5)%tmp;
			len = 5+((cn+5)>>4);
		}
		break;
	case 10:
		code = cn+1;
		len = 1+2*log2i(code);
		break;
	case 11:
		len = cn < 2 ? cn + 1 : cn/2 + 3;
		code = cn < 2 ? 1 : 2 + (cn&1);
		break;
	case 12:
		len = min(4,cn+1);
		code = cn==4 ? 0 : 1;
		break;
	case 13:
		len = min(6,cn+1);
		code = cn==6 ? 0 : 1;
		break;
	default:
		fatalerror("No such VLC table, only 0-10 allowed.");
	}

	return len;
}
