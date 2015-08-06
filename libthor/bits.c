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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <string.h>
#include <memory.h>
#include <assert.h>

#include "thor.h"
#include "bits.h"
#include "strings.h"
#include "snr.h"
#include "vlc.h"
#include "transform.h"
#include "block.h"
#include "inter.h"

/* getbits */

/* to mask the n least significant bits of an integer */
static const unsigned int msk[33] =
{
  0x00000000,0x00000001,0x00000003,0x00000007,
  0x0000000f,0x0000001f,0x0000003f,0x0000007f,
  0x000000ff,0x000001ff,0x000003ff,0x000007ff,
  0x00000fff,0x00001fff,0x00003fff,0x00007fff,
  0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
  0x000fffff,0x001fffff,0x003fffff,0x007fffff,
  0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
  0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
  0xffffffff
};

int initbits_dec(FILE *infile, stream_t *str)
{
  fpos_t fpos[1];
  long pos1,pos2;

  str->incnt = 0;
  str->rdptr = str->rdbfr + 2048;
  str->bitcnt = 0;
  str->infile = infile;

  fgetpos(str->infile,fpos);
  pos1 = ftell(str->infile);
  fseek(str->infile,0,SEEK_END);
  pos2 = ftell(str->infile);
  fsetpos(str->infile,fpos);
  str->length = pos2 - pos1;
  return 0;
}

int fillbfr(stream_t *str)
{
    //int l;

  while (str->incnt <= 24 && (str->rdptr < str->rdbfr + 2048))
  {
    str->inbfr = (str->inbfr << 8) | *str->rdptr++;
    str->incnt += 8;
  }

  if (str->rdptr >= str->rdbfr + 2048)
  {
    //l = (int)fread(str->rdbfr,sizeof(unsigned char),2048,str->infile);
    fread(str->rdbfr,sizeof(unsigned char),2048,str->infile);
    str->rdptr = str->rdbfr;

    while (str->incnt <= 24 && (str->rdptr < str->rdbfr + 2048))
    {
      str->inbfr = (str->inbfr << 8) | *str->rdptr++;
      str->incnt += 8;
    }
  }

  return 0;
}

unsigned int getbits(stream_t *str, int n)
{

  if (str->incnt < n)
  {
    fillbfr(str);
    if (str->incnt < n)
    {
      unsigned int l = str->inbfr;
      unsigned int k = *str->rdptr++;
      int shift = n-str->incnt;
      str->inbfr = (str->inbfr << 8) | k;
      str->incnt = str->incnt - n + 8;
      str->bitcnt += n;
      return (((l << shift) | (k >> (8-shift))) & msk[n]);
    }
  }

  str->incnt -= n;
  str->bitcnt += n;
  return ((str->inbfr >> str->incnt) & msk[n]);
}

unsigned int getbits1(stream_t *str)
{
  if (str->incnt < 1)
  {
    fillbfr(str);
  }
  str->incnt--;
  str->bitcnt++;
  return ((str->inbfr >> str->incnt) & 1);
}

unsigned int showbits(stream_t *str, int n)
{
  if (str->incnt < n)
  {
    fillbfr(str);
    if (str->incnt < n)
    {
      int shift = n-str->incnt;
      return (((str->inbfr << shift) | (str->rdptr[0] >> (8-shift))) & msk[n]);
    }
  }

  return ((str->inbfr >> (str->incnt-n)) & msk[n]);
}

int flushbits(stream_t *str, int n)
{
  str->incnt -= n;
  str->bitcnt += n;
  return 0;
}

/* putbits */

static unsigned int mask[33] = {
    0x00000000,0x00000001,0x00000003,0x00000007,
    0x0000000f,0x0000001f,0x0000003f,0x0000007f,
    0x000000ff,0x000001ff,0x000003ff,0x000007ff,
    0x00000fff,0x00001fff,0x00003fff,0x00007fff,
    0x0000ffff,0x0001ffff,0x0003ffff,0x0007ffff,
    0x000fffff,0x001fffff,0x003fffff,0x007fffff,
    0x00ffffff,0x01ffffff,0x03ffffff,0x07ffffff,
    0x0fffffff,0x1fffffff,0x3fffffff,0x7fffffff,
    0xffffffff};

void flush_bytebuf(stream_t *str, FILE *outfile)
{
  if (outfile)
  {
    if (fwrite(str->bitstream,sizeof(unsigned char),str->bytepos,outfile) != str->bytepos)
    {
      fatalerror("Problem writing bitstream to file.");
    }
  }
  str->bytepos = 0;
}


void flush_all_bits(stream_t *str, FILE *outfile)
{
  int i;
  int bytes = 4 - str->bitrest/8;

  printf("final flush: bytes=%4d\n",bytes);
  if ((str->bytepos+bytes) > str->bytesize)
  {
    flush_bytebuf(str,outfile);
  }
  for (i = 0; i < bytes; i++)
  {
    str->bitstream[str->bytepos++] = (str->bitbuf >> (24-i*8)) & 0xff;
  }

  if (outfile)
  {
    if (fwrite(str->bitstream,sizeof(unsigned char),str->bytepos,outfile) != str->bytepos)
    {
      fatalerror("Problem writing bitstream to file.");
    }
  }
  str->bytepos = 0;
}


void flush_bitbuf(stream_t *str)
{
  if ((str->bytepos+4) > str->bytesize)
  {
    fatalerror("Run out of bits in stream buffer.");
  }
  str->bitstream[str->bytepos++] = (str->bitbuf >> 24) & 0xff;
  str->bitstream[str->bytepos++] = (str->bitbuf >> 16) & 0xff;
  str->bitstream[str->bytepos++] = (str->bitbuf >> 8) & 0xff;
  str->bitstream[str->bytepos++] = str->bitbuf & 0xff;
  str->bitbuf = 0;
  str->bitrest = 32;
}

void putbits(unsigned int n, unsigned int val, stream_t *str)
{
  unsigned int rest;

  if (n <= str->bitrest)
  {
    str->bitbuf |= ((val & mask[n]) << (str->bitrest-n));
    str->bitrest -= n;
  }
  else
  {
    rest = n-str->bitrest;
    str->bitbuf |= (val >> rest) & mask[n-rest];
    flush_bitbuf(str);
    str->bitbuf |= (val & mask[rest]) << (32-rest);
    str->bitrest -= rest;
  }
}

int get_bit_pos(stream_t *str){
  int bitpos = 8*str->bytepos + (32 - str->bitrest);
  return bitpos;
}

void write_stream_pos(stream_t *stream, stream_pos_t *stream_pos){
  stream->bitrest = stream_pos->bitrest;
  stream->bytepos = stream_pos->bytepos;
  stream->bitbuf = stream_pos->bitbuf;
}

void read_stream_pos(stream_pos_t *stream_pos, stream_t *stream){
  stream_pos->bitrest = stream->bitrest;
  stream_pos->bytepos = stream->bytepos;
  stream_pos->bitbuf = stream->bitbuf;
}

void copy_stream(stream_t *str1, stream_t *str2){
  str1->bitrest = str2->bitrest;
  str1->bytepos = str2->bytepos;
  str1->bitbuf = str2->bitbuf;
  memcpy(&(str1->bitstream[0]),&(str2->bitstream[0]),str2->bytepos*sizeof(uint8_t));
}

/* read_bits */

extern int zigzag16[16];
extern int zigzag64[64];
extern int zigzag256[256];
extern int super_table[8][20];

void read_mv(stream_t *stream,mv_t *mv,mv_t *mvp)
{
    mv_t mvd;
    int code;

    code = get_vlc(10,stream);
    mvd.x = code&1 ? -((code+1)/2) : code/2;
    mv->x = mvp->x + mvd.x;

    code = get_vlc(10,stream);
    mvd.y = code&1 ? -((code+1)/2) : code/2;
    mv->y = mvp->y + mvd.y;
}

int YPOS,XPOS;


int find_index(int code, int maxrun, int type){

  int index;
  int maxrun2 = max(4,maxrun);

  if (type){
    if (code==0)
      index = -1;
    else if (code<=5)
      index = code-1;
    else if (code==6)
      index = maxrun2+1;
    else if (code==7)
      index = maxrun2+2;
    else if (code<=(maxrun2+3))
      index = code-3;
    else
      index = code-1;
  }
  else{
    if (code<=1)
      index = code;
    else if (code==2)
      index = -1;
    else if (code<=5)
      index = code-1;
    else if (code==6)
      index = maxrun2+1;
    else if (code==7)
      index = maxrun2+2;
    else if (code<=(maxrun2+3))
      index = code-3;
    else
      index = code-1;
  }
  return index;
}

void read_coeff(stream_t *stream,int16_t *coeff,int size,int type){

  int16_t scoeff[MAX_QUANT_SIZE*MAX_QUANT_SIZE];
  int i,j,levelFlag,sign,level,pos,index,run,tmp,vlc,maxrun,code,vlc_adaptive;
  int qsize = min(size,MAX_QUANT_SIZE);
  int N = qsize*qsize;
  int level_mode;

  /* Initialize arrays */
  memset(scoeff,0,N*sizeof(int16_t));
  memset(coeff,0,size*size*sizeof(int16_t));

  pos = 0;
  /* Use one bit to signal chroma/last_pos=1/level=1 */
  if (type==1){
    int tmp = getbits1(stream);
    if (tmp){
      sign = getbits1(stream);
      scoeff[pos] = sign ? -1 : 1;
      pos = N;
    }
  }

  /* Initiate forward scan */
  level_mode = 1;
  level = 1;
  vlc_adaptive = 0;
  while (pos < N){
    if (level_mode){
      /* Level-mode */
      //vlc_adaptive = (level > 3 && type==0) ? 1 : 0;
      while (pos < N && level > 0){
        level = get_vlc(vlc_adaptive,stream);
        if (level){
          sign = getbits1(stream);
        }
        else{
          sign = 1;
        }
        scoeff[pos] = sign ? -level : level;
        if (type==0)
          vlc_adaptive = level > 3;
        pos++;
      }
    }
    if (pos >= N){
      break;
    }

    /* Run-mode (run-level coding) */
    maxrun = N - pos - 1;

    /* Decode levelFlag (level > 1) and run */
    if (type && size <= 8){
      vlc = 10;
      code = get_vlc(vlc,stream);
    }
    else{
      vlc = 2;
      if (showbits(stream,2)==2){
        code = getbits(stream,2) - 2;
      }
      else{
        code = get_vlc(vlc,stream) - 1;
      }
    }

    index = find_index(code,maxrun,type);
    if (index == -1){
      break;
    }

    /* Extract levelFlag (level > 1) and run */
    int maxrun2 = max(4,maxrun);
    levelFlag =  index/(maxrun2+1);
    run = index%(maxrun2+1);
    pos += run;

    /* Decode level and sign */
    if (levelFlag){
      tmp = get_vlc(0,stream);
      sign = tmp&1;
      level = (tmp>>1)+2;
    }
    else{
      level = 1;
      sign = getbits1(stream);
    }
    scoeff[pos] = sign ? -level : level;

    level_mode = level > 1; //Set level_mode
    pos++;
  } //while pos < N

  /* Perform inverse zigzag scan */
  int *zigzagptr = zigzag64;
  if (qsize==4)
    zigzagptr = zigzag16;
  else if (qsize==8)
    zigzagptr = zigzag64;
  else if (qsize==16)
    zigzagptr = zigzag256;
  for (i=0;i<qsize;i++){
    for (j=0;j<qsize;j++){
      coeff[i*size+j] = scoeff[zigzagptr[i*qsize+j]];
    }
  }
}

int read_delta_qp(stream_t *stream){
  int abs_delta_qp,sign_delta_qp,delta_qp;
  sign_delta_qp = 0;
  abs_delta_qp = get_vlc(0,stream);
  if (abs_delta_qp > 0)
    sign_delta_qp = getbits(stream,1);
  delta_qp = sign_delta_qp ? -abs_delta_qp : abs_delta_qp;
  return delta_qp;
}

int read_block(decoder_info_t *decoder_info,stream_t *stream,block_info_dec_t *block_info, frame_type_t frame_type)
{
  int width = decoder_info->width;
  int height = decoder_info->height;
  int bit_start;
  int code,tmp,tb_split;
  int PBpart=0;
  cbp_t cbp;

  int size = block_info->block_pos.size;
  int ypos = block_info->block_pos.ypos;
  int xpos = block_info->block_pos.xpos;

  YPOS = ypos;
  XPOS = xpos;

  int sizeY = size;
  int sizeC = size/2;

  mv_t mv,zerovec;
  mv_t mvp;
  mv_t mv_arr[4]; //TODO: Use mv_arr0 instead
  mv_t mv_arr0[4];
  mv_t mv_arr1[4];

  block_mode_t mode;
  intra_mode_t intra_mode = MODE_DC;

  int16_t *coeff_y = block_info->coeffq_y;
  int16_t *coeff_u = block_info->coeffq_u;
  int16_t *coeff_v = block_info->coeffq_v;

  zerovec.y = zerovec.x = 0;
  bit_start = stream->bitcnt;

  mode = decoder_info->mode;

  /* Initialize bit counter for statistical purposes */
  bit_start = stream->bitcnt;

  if (mode == MODE_SKIP){
    /* Derive skip vector candidates and number of skip vector candidates from neighbour blocks */
    mv_t mv_skip[MAX_NUM_SKIP];
    int num_skip_vec,skip_idx;
    mvb_t tmp_mvb_skip[MAX_NUM_SKIP];
    num_skip_vec = get_mv_skip(ypos,xpos,width,height,size,decoder_info->deblock_data,tmp_mvb_skip);
    for (int idx=0;idx<num_skip_vec;idx++){
      mv_skip[idx].x = tmp_mvb_skip[idx].x0;
      mv_skip[idx].y = tmp_mvb_skip[idx].y0;
    }
    /* Decode skip index */
    if (num_skip_vec == 4)
      skip_idx = getbits(stream,2);
    else if (num_skip_vec == 3){
      tmp = getbits(stream,1);
      if (tmp)
        skip_idx = 0;
      else
        skip_idx = 1 + getbits(stream,1);
    }
    else if (num_skip_vec == 2){
      skip_idx = getbits(stream,1);
    }
    else
      skip_idx = 0;
    decoder_info->bit_count.skip_idx[frame_type] += (stream->bitcnt - bit_start);

    block_info->num_skip_vec = num_skip_vec;
    block_info->pred_data.skip_idx = skip_idx;

    if (skip_idx == num_skip_vec)
      mv = mv_skip[0];
    else
      mv = mv_skip[skip_idx];
    mv_arr[0] = mv;
    mv_arr[1] = mv;
    mv_arr[2] = mv;
    mv_arr[3] = mv;

    //int ref_idx = tmp_mvb_skip[skip_idx].ref_idx0;

    block_info->pred_data.ref_idx0 = tmp_mvb_skip[skip_idx].ref_idx0;
    block_info->pred_data.ref_idx1 = tmp_mvb_skip[skip_idx].ref_idx1;
    block_info->pred_data.dir = tmp_mvb_skip[skip_idx].dir;
    mv_arr0[0].x = tmp_mvb_skip[skip_idx].x0;
    mv_arr0[0].y = tmp_mvb_skip[skip_idx].y0;
    mv_arr0[1] = mv_arr0[0];
    mv_arr0[2] = mv_arr0[0];
    mv_arr0[3] = mv_arr0[0];

    mv_arr1[0].x = tmp_mvb_skip[skip_idx].x1;
    mv_arr1[0].y = tmp_mvb_skip[skip_idx].y1;
    mv_arr1[1] = mv_arr1[0];
    mv_arr1[2] = mv_arr1[0];
    mv_arr1[3] = mv_arr1[0];

  }
  else if (mode == MODE_MERGE){
    /* Derive skip vector candidates and number of skip vector candidates from neighbour blocks */
    mv_t mv_skip[MAX_NUM_SKIP];
    int num_skip_vec,skip_idx;
    mvb_t tmp_mvb_skip[MAX_NUM_SKIP];
    num_skip_vec = get_mv_merge(ypos,xpos,width,height,size,decoder_info->deblock_data,tmp_mvb_skip);
    for (int idx=0;idx<num_skip_vec;idx++){
      mv_skip[idx].x = tmp_mvb_skip[idx].x0;
      mv_skip[idx].y = tmp_mvb_skip[idx].y0;
    }
    /* Decode skip index */
    if (num_skip_vec == 4)
      skip_idx = getbits(stream,2);
    else if (num_skip_vec == 3){
      tmp = getbits(stream,1);
      if (tmp)
        skip_idx = 0;
      else
        skip_idx = 1 + getbits(stream,1);
    }
    else if (num_skip_vec == 2){
      skip_idx = getbits(stream,1);
    }
    else
      skip_idx = 0;
    decoder_info->bit_count.skip_idx[frame_type] += (stream->bitcnt - bit_start);

    block_info->num_skip_vec = num_skip_vec;
    block_info->pred_data.skip_idx = skip_idx;

    if (skip_idx == num_skip_vec)
      mv = mv_skip[0];
    else
      mv = mv_skip[skip_idx];
    mv_arr[0] = mv;
    mv_arr[1] = mv;
    mv_arr[2] = mv;
    mv_arr[3] = mv;

    //int ref_idx = tmp_mvb_skip[skip_idx].ref_idx0;

    block_info->pred_data.ref_idx0 = tmp_mvb_skip[skip_idx].ref_idx0;
    block_info->pred_data.ref_idx1 = tmp_mvb_skip[skip_idx].ref_idx1;
    block_info->pred_data.dir = tmp_mvb_skip[skip_idx].dir;
    mv_arr0[0].x = tmp_mvb_skip[skip_idx].x0;
    mv_arr0[0].y = tmp_mvb_skip[skip_idx].y0;
    mv_arr0[1] = mv_arr0[0];
    mv_arr0[2] = mv_arr0[0];
    mv_arr0[3] = mv_arr0[0];

    mv_arr1[0].x = tmp_mvb_skip[skip_idx].x1;
    mv_arr1[0].y = tmp_mvb_skip[skip_idx].y1;
    mv_arr1[1] = mv_arr1[0];
    mv_arr1[2] = mv_arr1[0];
    mv_arr1[3] = mv_arr1[0];
  }
  else if (mode==MODE_INTER){
    int ref_idx;

    if (decoder_info->pb_split){
      /* Decode PU partition */
      tmp = getbits(stream,1);
      if (tmp==1){
        code = 0;
      }
      else{
        tmp = getbits(stream,1);
        if (tmp==1){
          code = 1;
        }
        else{
          tmp = getbits(stream,1);
          code = 3 - tmp;
        }
      }
      PBpart = code;
    }
    else{
      PBpart = 0;
    }
    block_info->pred_data.PBpart = PBpart;
    if (decoder_info->frame_info.num_ref > 1){
      ref_idx = decoder_info->ref_idx;
    }
    else{
      ref_idx = 0;
    }

    if (mode==MODE_INTER)
      decoder_info->bit_count.size_and_ref_idx[log2i(size)-3][ref_idx] += 1;

#if TWO_MVP
    int num_skip_vec;
    mvb_t tmp_mvb_skip[MAX_NUM_SKIP];
    num_skip_vec = get_mv_skip(ypos,xpos,width,height,size,decoder_info->deblock_data,tmp_mvb_skip);
    if (num_skip_vec > 1){
      int idx = getbits(stream,1);
      mvp.y = tmp_mvb_skip[idx].y0;
      mvp.x = tmp_mvb_skip[idx].x0;
    }
    else{
      mvp.y = tmp_mvb_skip[0].y0;
      mvp.x = tmp_mvb_skip[0].x0;
    }
#else
    mvp = get_mv_pred(ypos,xpos,width,height,size,ref_idx,decoder_info->deblock_data);
#endif

    /* Deode motion vectors for each prediction block */
    mv_t mvp2 = mvp;

    if (PBpart==0){
      read_mv(stream,&mv_arr[0],&mvp2);
      mv_arr[1] = mv_arr[0];
      mv_arr[2] = mv_arr[0];
      mv_arr[3] = mv_arr[0];
    }
    else if(PBpart==1){ //HOR
      read_mv(stream,&mv_arr[0],&mvp2);
      mvp2 = mv_arr[0];
      read_mv(stream,&mv_arr[2],&mvp2);
      mv_arr[1] = mv_arr[0];
      mv_arr[3] = mv_arr[2];
    }
    else if(PBpart==2){ //VER
      read_mv(stream,&mv_arr[0],&mvp2);
      mvp2 = mv_arr[0];
      read_mv(stream,&mv_arr[1],&mvp2);
      mv_arr[2] = mv_arr[0];
      mv_arr[3] = mv_arr[1];
    }
    else{
      read_mv(stream,&mv_arr[0],&mvp2);
      mvp2 = mv_arr[0];
      read_mv(stream,&mv_arr[1],&mvp2);
      read_mv(stream,&mv_arr[2],&mvp2);
      read_mv(stream,&mv_arr[3],&mvp2);
    }
    decoder_info->bit_count.mv[frame_type] += (stream->bitcnt - bit_start);
    block_info->pred_data.ref_idx0 = ref_idx;
    block_info->pred_data.ref_idx1 = ref_idx;
    block_info->pred_data.dir = 0;
  }
  else if (mode==MODE_BIPRED){
#if TWO_MVP
    int num_skip_vec;
    mvb_t tmp_mvb_skip[MAX_NUM_SKIP];
    num_skip_vec = get_mv_skip(ypos,xpos,width,height,size,decoder_info->deblock_data,tmp_mvb_skip);
    if (num_skip_vec > 1){
      int idx = getbits(stream,1);
      mvp.y = tmp_mvb_skip[idx].y0;
      mvp.x = tmp_mvb_skip[idx].x0;
    }
    else{
      mvp.y = tmp_mvb_skip[0].y0;
      mvp.x = tmp_mvb_skip[0].x0;
    }
#else
    int ref_idx = 0;
    mvp = get_mv_pred(ypos,xpos,width,height,size,ref_idx,decoder_info->deblock_data);
#endif

    /* Deode motion vectors */
    mv_t mvp2 = mvp;

    read_mv(stream,&mv_arr0[0],&mvp2);
    mv_arr0[1] = mv_arr0[0];
    mv_arr0[2] = mv_arr0[0];
    mv_arr0[3] = mv_arr0[0];
    read_mv(stream,&mv_arr1[0],&mvp2);
    mv_arr1[1] = mv_arr1[0];
    mv_arr1[2] = mv_arr1[0];
    mv_arr1[3] = mv_arr1[0];

    if (decoder_info->frame_info.num_ref==2){
      int code = get_vlc0_limit(3,stream);
      block_info->pred_data.ref_idx0 = (code>>0)&1;
      block_info->pred_data.ref_idx1 = (code>>1)&1;
    }
    else{
      int code = get_vlc(10,stream);
      block_info->pred_data.ref_idx0 = (code>>0)&3;
      block_info->pred_data.ref_idx1 = (code>>2)&3;
    }
    block_info->pred_data.dir = 2;
    decoder_info->bit_count.bi_ref[block_info->pred_data.ref_idx0 * decoder_info->frame_info.num_ref + block_info->pred_data.ref_idx1] += 1;
    decoder_info->bit_count.mv[frame_type] += (stream->bitcnt - bit_start);
  }

  else if (mode==MODE_INTRA){
    /* Decode intra prediction mode */
    if (decoder_info->frame_info.num_intra_modes<=4){
      intra_mode = getbits(stream,2);
    }
    else if (decoder_info->frame_info.num_intra_modes<=8){
      intra_mode = getbits(stream,3);
    }
    else if (decoder_info->frame_info.num_intra_modes<=10){
#if LIMIT_INTRA_MODES
      int intra_mode_map_inv[MAX_NUM_INTRA_MODES] = {3,2,0,9,8,4,7,6,1,5};
      int tmp,code;
      tmp = getbits(stream,2);
      if (tmp<3){
        code = tmp;
      }
      else{
        tmp = getbits(stream,2);
        if (tmp<3){
          code = 3 + tmp;
        }
        else{
          tmp = getbits(stream,1);
          code = 6 + tmp;
        }
      }
      intra_mode = intra_mode_map_inv[code];
#else
      int intra_mode_map_inv[MAX_NUM_INTRA_MODES] = {3,2,0,1,9,8,4,7,6,5};
      int tmp,code;
      tmp = getbits(stream,1);
      if (tmp){
        code = getbits(stream,1);
      }
      else{
        tmp = getbits(stream,1);
        if (tmp){
          code = 2 + getbits(stream,1);
        }
        else{
          tmp = getbits(stream,1);
          if (tmp){
            code = 4 + getbits(stream,1);
          }
          else{
            code = 6 + getbits(stream,2);
          }
        }
      }
      intra_mode = intra_mode_map_inv[code];
#endif
    }

    decoder_info->bit_count.intra_mode[frame_type] += (stream->bitcnt - bit_start);
    decoder_info->bit_count.size_and_intra_mode[frame_type][log2i(size)-3][intra_mode] += 1;

    block_info->pred_data.intra_mode = intra_mode;
    for (int i=0;i<4;i++){
      mv_arr[i] = zerovec; //Note: This is necessary for derivation of mvp and mv_skip
    }
    block_info->pred_data.ref_idx0 = 0;
    block_info->pred_data.ref_idx1 = 0;
    block_info->pred_data.dir = -1;
  }


  if (mode!=MODE_SKIP){
    int tmp,cbp2;
    int cbp_table[8] = {1,0,5,2,6,3,7,4};

    bit_start = stream->bitcnt;
    code = get_vlc(0,stream);

#if NEW_BLOCK_STRUCTURE
    if (decoder_info->tb_split_enable && mode!=MODE_BIPRED){
#else
    if (decoder_info->tb_split_enable && (mode==MODE_INTRA || (mode==MODE_INTER && PBpart==0))){
#endif
      tb_split = code==2;
      if (code > 2) code -= 1;
      if (tb_split)
        decoder_info->bit_count.cbp2_stat[0][frame_type][mode-1][log2i(size)-3][8] += 1;
    }
    else{
      tb_split = 0;
    }
    block_info->tb_split = tb_split;
    decoder_info->bit_count.cbp[frame_type] += (stream->bitcnt - bit_start);

    if (tb_split == 0){
      tmp = 0;
#if NO_SUBBLOCK_SKIP
      if (0){
#else
      if (mode==MODE_MERGE){
#endif
        if (code==7)
          code = 1;
        else if (code>0)
          code = code+1;
      }
      while (tmp < 8 && code != cbp_table[tmp]) tmp++;
#if NO_SUBBLOCK_SKIP
      if (1){
#else
      if (mode!=MODE_MERGE){
#endif

        if (decoder_info->block_context->cbp==0 && tmp < 2){
          tmp = 1-tmp;
        }
      }
      cbp2 = tmp;
      decoder_info->bit_count.cbp2_stat[max(0,decoder_info->block_context->cbp)][frame_type][mode-1][log2i(size)-3][cbp2] += 1;

      cbp.y = ((cbp2>>0)&1);
      cbp.u = ((cbp2>>1)&1);
      cbp.v = ((cbp2>>2)&1);
      block_info->cbp = cbp;

      if (cbp.y){
        bit_start = stream->bitcnt;
        read_coeff(stream,coeff_y,sizeY,0);
        decoder_info->bit_count.coeff_y[frame_type] += (stream->bitcnt - bit_start);
      }
      else
        memset(coeff_y,0,sizeY*sizeY*sizeof(int16_t));

      if (cbp.u){
        bit_start = stream->bitcnt;
        read_coeff(stream,coeff_u,sizeC,1);
        decoder_info->bit_count.coeff_u[frame_type] += (stream->bitcnt - bit_start);
      }
      else
        memset(coeff_u,0,sizeC*sizeC*sizeof(int16_t));

      if (cbp.v){
        bit_start = stream->bitcnt;
        read_coeff(stream,coeff_v,size/2,1);
        decoder_info->bit_count.coeff_v[frame_type] += (stream->bitcnt - bit_start);
      }
      else
        memset(coeff_v,0,sizeC*sizeC*sizeof(int16_t));
    }
    else{
      if (size > 8){
        int index;
        int16_t *coeff;

        /* Loop over 4 TUs */
        for (index=0;index<4;index++){
          bit_start = stream->bitcnt;
          code = get_vlc(0,stream);
          int tmp = 0;
          while (code != cbp_table[tmp] && tmp < 8) tmp++;
          if (decoder_info->block_context->cbp==0 && tmp < 2)
            tmp = 1-tmp;
          cbp.y = ((tmp>>0)&1);
          cbp.u = ((tmp>>1)&1);
          cbp.v = ((tmp>>2)&1);

          /* Updating statistics for CBP */
          decoder_info->bit_count.cbp[frame_type] += (stream->bitcnt - bit_start);
          decoder_info->bit_count.cbp_stat[frame_type][cbp.y + (cbp.u<<1) + (cbp.v<<2)] += 1;

          /* Decode coefficients for this TU */

          /* Y */
          coeff = coeff_y + index*sizeY/2*sizeY/2;
          if (cbp.y){
            bit_start = stream->bitcnt;
            read_coeff(stream,coeff,sizeY/2,0);
            decoder_info->bit_count.coeff_y[frame_type] += (stream->bitcnt - bit_start);
          }
          else{
            memset(coeff,0,sizeY/2*sizeY/2*sizeof(int16_t));
          }

          /* U */
          coeff = coeff_u + index*sizeC/2*sizeC/2;
          if (cbp.u){
            bit_start = stream->bitcnt;
            read_coeff(stream,coeff,sizeC/2,1);
            decoder_info->bit_count.coeff_u[frame_type] += (stream->bitcnt - bit_start);
          }
          else{
            memset(coeff,0,sizeC/2*sizeC/2*sizeof(int16_t));
          }

          /* V */
          coeff = coeff_v + index*sizeC/2*sizeC/2;
          if (cbp.v){
            bit_start = stream->bitcnt;
            read_coeff(stream,coeff,sizeC/2,1);
            decoder_info->bit_count.coeff_v[frame_type] += (stream->bitcnt - bit_start);
          }
          else{
            memset(coeff,0,sizeC/2*sizeC/2*sizeof(int16_t));
          }
        }
        block_info->cbp.y = 1; //TODO: Do properly with respect to deblocking filter
        block_info->cbp.u = 1;
        block_info->cbp.u = 1;
      }
      else{
        int index;
        int16_t *coeff;

        /* Loop over 4 TUs */
        for (index=0;index<4;index++){
          bit_start = stream->bitcnt;
          cbp.y = getbits(stream,1);
          decoder_info->bit_count.cbp[frame_type] += (stream->bitcnt - bit_start);

          /* Y */
          coeff = coeff_y + index*sizeY/2*sizeY/2;
          if (cbp.y){
            bit_start = stream->bitcnt;
            read_coeff(stream,coeff,sizeY/2,0);
            decoder_info->bit_count.coeff_y[frame_type] += (stream->bitcnt - bit_start);
          }
          else{
            memset(coeff,0,sizeY/2*sizeY/2*sizeof(int16_t));
          }
        }

        bit_start = stream->bitcnt;
        int tmp;
        tmp = getbits(stream,1);
        if (tmp){
          cbp.u = cbp.v = 0;
        }
        else{
          tmp = getbits(stream,1);
          if (tmp){
            cbp.u = 1;
            cbp.v = 0;
          }
          else{
            tmp = getbits(stream,1);
            if (tmp){
              cbp.u = 0;
              cbp.v = 1;
            }
            else{
              cbp.u = 1;
              cbp.v = 1;
            }
          }
        }
        decoder_info->bit_count.cbp[frame_type] += (stream->bitcnt - bit_start);
        if (cbp.u){
          bit_start = stream->bitcnt;
          read_coeff(stream,coeff_u,sizeC,1);
          decoder_info->bit_count.coeff_u[frame_type] += (stream->bitcnt - bit_start);
        }
        else
          memset(coeff_u,0,sizeC*sizeC*sizeof(int16_t));
        if (cbp.v){
          bit_start = stream->bitcnt;
          read_coeff(stream,coeff_v,size/2,1);
          decoder_info->bit_count.coeff_v[frame_type] += (stream->bitcnt - bit_start);
        }
        else
          memset(coeff_v,0,sizeC*sizeC*sizeof(int16_t));

        block_info->cbp.y = 1; //TODO: Do properly with respect to deblocking filter
        block_info->cbp.u = 1;
        block_info->cbp.u = 1;
      } //if (size==8)
    } //if (tb_split==0)
  } //if (mode!=MODE_SKIP)
  else{
    tb_split = 0;
    block_info->cbp.y = 0;
    block_info->cbp.u = 0;
    block_info->cbp.v = 0;
  }

  /* Store block data */
  if (mode==MODE_BIPRED){
    memcpy(block_info->pred_data.mv_arr0,mv_arr0,4*sizeof(mv_t)); //Used for mv0 coding
    memcpy(block_info->pred_data.mv_arr1,mv_arr1,4*sizeof(mv_t)); //Used for mv1 coding
  }
  else if(mode==MODE_SKIP){
    memcpy(block_info->pred_data.mv_arr0,mv_arr0,4*sizeof(mv_t)); //Used for mv0 coding
    memcpy(block_info->pred_data.mv_arr1,mv_arr1,4*sizeof(mv_t)); //Used for mv1 coding
  }
  else if(mode==MODE_MERGE){
    memcpy(block_info->pred_data.mv_arr0,mv_arr0,4*sizeof(mv_t)); //Used for mv0 coding
    memcpy(block_info->pred_data.mv_arr1,mv_arr1,4*sizeof(mv_t)); //Used for mv1 coding
  }
  else{
    memcpy(block_info->pred_data.mv_arr0,mv_arr,4*sizeof(mv_t)); //Used for mv0 coding
    memcpy(block_info->pred_data.mv_arr1,mv_arr,4*sizeof(mv_t)); //Used for mv1 coding
  }
  block_info->pred_data.mode = mode;
  block_info->tb_split = tb_split;

  int bwidth = min(size,width - xpos);
  int bheight = min(size,height - ypos);

  /* Update mode and block size statistics */
  decoder_info->bit_count.mode[frame_type][mode] += (bwidth/MIN_BLOCK_SIZE * bheight/MIN_BLOCK_SIZE);
  decoder_info->bit_count.size[frame_type][log2i(size)-3] += (bwidth/MIN_BLOCK_SIZE * bheight/MIN_BLOCK_SIZE);
  if (frame_type != I_FRAME){
    decoder_info->bit_count.size_and_mode[log2i(size)-3][mode] += (bwidth/MIN_BLOCK_SIZE * bheight/MIN_BLOCK_SIZE);
  }
  return 0;
}

/* write_bits */

extern int zigzag16[16];
extern int zigzag64[64];
extern int zigzag256[256];
extern int super_table[8][20];
extern int YPOS,XPOS;

void write_mv(stream_t *stream,mv_t *mv,mv_t *mvp)
{
    uint16_t  mvabs,mvsign;
    mv_t mvd;
    mvd.x = mv->x - mvp->x;
    mvd.y = mv->y - mvp->y;

    int code;

    mvabs = abs(mvd.x);
    mvsign = mvd.x < 0 ? 1 : 0;
    code = 2*mvabs - mvsign;
    put_vlc(10,code,stream);

    mvabs = abs(mvd.y);
    mvsign = mvd.y < 0 ? 1 : 0;
    code = 2*mvabs - mvsign;
    put_vlc(10,code,stream);

}

int find_code(int run, int level, int maxrun, int type,int eob){

  int cn,index;
  int maxrun2 = max(4,maxrun);
  index = run + (level>1)*(maxrun2+1);

  if (type){
    if (eob)
      cn = 0;
    else if (index<=4)
      cn = index + 1;
    else if (index<=maxrun2)
      cn = index + 3;
    else if (index==(maxrun2+1))
      cn = 6;
    else if (index==(maxrun2+2))
      cn = 7;
    else
      cn = index+1;
  }
  else{
    if (eob)
      cn = 2;
    else if (index<2)
      cn = index;
    else if (index<=4)
      cn = index + 1;
    else if (index<=maxrun2)
      cn = index + 3;
    else if (index==(maxrun2+1))
      cn = 6;
    else if (index==(maxrun2+2))
      cn = 7;
    else
      cn = index+1;
  }
  return cn;
}

void write_coeff(stream_t *stream,int16_t *coeff,int size,int type)
{
  int16_t scoeff[MAX_QUANT_SIZE*MAX_QUANT_SIZE];
  int i,j,len,pos,c;
  int qsize = min(MAX_QUANT_SIZE,size);
  unsigned int cn;
  int level,vlc,sign,last_pos;
  int maxrun,run;
  int vlc_adaptive=0;
  int N = qsize*qsize;
  int level_mode;

  /* Zigzag scan */
  int *zigzagptr = zigzag64;
  if (qsize==4)
    zigzagptr = zigzag16;
  else if (qsize==8)
    zigzagptr = zigzag64;
  else if (qsize==16)
    zigzagptr = zigzag256;

  for(i=0;i<qsize;i++){
    for (j=0;j<qsize;j++){
      scoeff[zigzagptr[i*qsize+j]] = coeff[i*size+j];
    }
  }

  /* Find last_pos to determine when to send EOB */
  pos = N-1;
  while (scoeff[pos]==0 && pos>0) pos--;
  if (pos==0 && scoeff[0]==0)
    fatalerror("No coeffs even if cbp nonzero. Exiting.");
  last_pos = pos;

  /* Use one bit to signal chroma/last_pos=1/level=1 */
  pos = 0;
  if (type==1){
    if (last_pos==0 && abs(scoeff[0])==1){
      putbits(1,1,stream);
      sign = (scoeff[0] < 0) ? 1 : 0;
      putbits(1,sign,stream);
      pos = N;
    }
    else{
      putbits(1,0,stream);
    }
  }

  /* Initiate forward scan */
  level_mode = 1;
  level = 1;
  while (pos <= last_pos){ //Outer loop for forward scan
    if (level_mode){
      /* Level-mode */
      //vlc_adaptive = (level > 3 && type==0) ? 1 : 0;
      while (pos <= last_pos && level > 0){
        c = scoeff[pos];
        level = abs(c);
        len = put_vlc(vlc_adaptive,level,stream);
        if (level > 0){
          sign = (c < 0) ? 1 : 0;
          putbits(1,sign,stream);
          len += 1;
        }
        if (type==0)
          vlc_adaptive = level > 3;
        pos++;
      }
    }

    /* Run-mode (run-level coding) */
    maxrun = N - pos - 1;
    run = 0;
    c = 0;
    while (c==0 && pos <= last_pos){
      c = scoeff[pos];
      if (c==0){
        run++;
      }
      else{
        level = abs(c);
        sign = (c < 0) ? 1 : 0;

        /* Code combined event of run and (level>1) */
        cn = find_code(run, level, maxrun, type, 0);

        if (type && size <= 8){
          vlc = 10;
          len = put_vlc(vlc,cn,stream);
        }
        else{
          vlc = 2;
          if (cn == 0)
            putbits(2,2,stream);
          else
            put_vlc(vlc,cn+1,stream);
        }
        /* Code level and sign */
        if (level > 1){
          len += put_vlc(0,2*(level-2)+sign,stream);
        }
        else{
          putbits(1,sign,stream);
          len += 1;
        }
        run = 0;
      }
      pos++;
      //vlc_adaptive = (level > 3 && type==0) ? 1 : 0;
      level_mode = level > 1; //Set level_mode
    } //while (c==0 && pos < last_pos)
  } //while (pos <= last_pos){

  if (pos < N){
    /* If terminated in level mode, code one extra zero before an EOB can be sent */
    if (level_mode){
      c = scoeff[pos];
      level = abs(c);
      len = put_vlc(vlc_adaptive,level,stream);
      if (level > 0){
        sign = (c < 0) ? 1 : 0;
        putbits(1,sign,stream);
        len += 1;
      }
      pos++;
    }
  }

  /* EOB */
  if (pos < N){
    cn = find_code(0, 0, 0, type, 1);
    if (type && size <= 8){
      vlc = 0;
      put_vlc(vlc,cn,stream);
    }
    else{
      vlc = 2;
      if (cn == 0)
        putbits(2,2,stream);
      else
        put_vlc(vlc,cn+1,stream);
    }
  }
}

int write_delta_qp(stream_t *stream, int delta_qp){
  int len;
  int abs_delta_qp = abs(delta_qp);
  int sign_delta_qp = delta_qp < 0 ? 1 : 0;
  len = put_vlc(0,abs_delta_qp,stream);
  if (abs_delta_qp > 0){
    putbits(1,sign_delta_qp,stream);
    len += 1;
  }
  return len;
}

#if NEW_BLOCK_STRUCTURE
void write_super_mode(stream_t *stream,write_data_t *write_data){

  int size = write_data->size;
  block_mode_t mode = write_data->mode;
  frame_type_t frame_type = write_data->frame_type;
  if (frame_type!=I_FRAME && write_data->encode_rectangular_size==0){
    int num_split_codes = 2*(size==MAX_BLOCK_SIZE);
    int code=0,maxbit,mode_offset,ref_offset;
    mode_offset = 2 + num_split_codes;
    maxbit = write_data->num_ref + 2 + num_split_codes;
    if (write_data->num_ref>1 && write_data->enable_bipred) maxbit += 1;

    if (mode==MODE_SKIP){
      code = 0;
    }
    else if (mode==MODE_MERGE){
      code = mode_offset - 1;
    }
    else if (mode==MODE_INTRA){
      code = mode_offset + 1;
    }
    else if (mode==MODE_INTER){
      ref_offset = write_data->ref_idx==0 ? 0 : write_data->ref_idx + 1;
      code = mode_offset + ref_offset;
    }
    else if (mode==MODE_BIPRED){
      code = maxbit;
    }

    /* Switch MERGE and INTER-R0 */ //TODO: Integrate with code above
    if (code==2)
      code = 3;
    else if(code==3)
      code = 2;

    if (write_data->block_context->index==2 || write_data->block_context->index>3){
      if (size>MIN_BLOCK_SIZE && code<3)
        code = (code+2)%3;
    }

    if (code==maxbit)
      putbits(maxbit,0,stream);
    else
      putbits(code+1,1,stream);

  }
}
#else
void write_super_mode(stream_t *stream,write_data_t *write_data){

  int size = write_data->size;
  block_mode_t mode = write_data->mode;
  frame_type_t frame_type = write_data->frame_type;
  if (frame_type!=I_FRAME){
    int code=0,maxbit;
    maxbit = write_data->num_ref + 2 + (size>MIN_BLOCK_SIZE);
    if (write_data->num_ref>1 && write_data->enable_bipred) maxbit += 1;

    if (size>MIN_BLOCK_SIZE){
      if (mode==MODE_SKIP)
        code = 0;
      else if (mode==MODE_INTER && write_data->ref_idx==0)
        code = 2;
      else if (mode==MODE_MERGE)
        code = 3;
      else if (mode==MODE_INTRA)
        code = 4;
      else if (mode==MODE_INTER && write_data->ref_idx>0)
        code = 4 + write_data->ref_idx;
      else if (mode==MODE_BIPRED)
        code = 4 + write_data->num_ref;
#if NO_SUBBLOCK_SKIP
      if (size < MAX_BLOCK_SIZE){
        if (code==2) code = 3;
        else if (code==3) code = 2;
      }
#endif
    }
    else{
      if (mode==MODE_SKIP)
        code = 0;
#if 9
      else if (mode==MODE_INTER && write_data->ref_idx==0)
        code = 1;
      else if (mode==MODE_MERGE)
        code = 2;
      else if (mode==MODE_INTRA)
        code = 3;
#else
      else if (mode==MODE_INTER && write_data->ref_idx==0)
        code = 3;
      else if (mode==MODE_MERGE)
        code = 1;
      else if (mode==MODE_INTRA)
        code = 2;
#endif
      else if (mode==MODE_INTER && write_data->ref_idx>0)
        code = 3 + write_data->ref_idx;
      else if (mode==MODE_BIPRED)
        code = 3 + write_data->num_ref;
#if NO_SUBBLOCK_SKIP
      if (size < MAX_BLOCK_SIZE){
        if (code==1) code = 2;
        else if (code==2) code = 1;
      }
#endif
    }

    if (write_data->block_context->index==2 || write_data->block_context->index>3){
      if (size>MIN_BLOCK_SIZE && code<4)
        code = (code+3)%4;
    }

    if (code==maxbit)
      putbits(maxbit,0,stream);
    else
      putbits(code+1,1,stream);

  }
  else{
    putbits(1,0,stream); // To signal split_flag = 0
  }
}
#endif

int write_block(stream_t *stream,write_data_t *write_data){

  int start_bits,end_bits;
  int size = write_data->size;

  int tb_split = write_data->tb_part;

  uint8_t cbp_y = write_data->cbp->y;
  uint8_t cbp_u = write_data->cbp->u;
  uint8_t cbp_v = write_data->cbp->v;

  int16_t *coeffq_y = write_data->coeffq_y;
  int16_t *coeffq_u = write_data->coeffq_u;
  int16_t *coeffq_v = write_data->coeffq_v;

  block_mode_t mode = write_data->mode;
  intra_mode_t intra_mode = write_data->intra_mode;

  mv_t mvp = write_data->mvp;

  start_bits = get_bit_pos(stream);

  int code,cbp;
  int cbp_table[8] = {1,0,5,2,6,3,7,4};


  /* Write mode and ref_idx */
  write_super_mode(stream,write_data);

  if (size==MAX_BLOCK_SIZE && mode != MODE_SKIP && write_data->max_delta_qp){
    write_delta_qp(stream,write_data->delta_qp);
  }

  /* Code intra mode */
  if (mode==MODE_INTRA){
    if (write_data->num_intra_modes<=4){
      putbits(2,intra_mode,stream);
    }
    else if (write_data->num_intra_modes <= 8){
      putbits(3,intra_mode,stream);
    }
    else if (write_data->num_intra_modes <= 10){
#if LIMIT_INTRA_MODES
      int intra_mode_map[MAX_NUM_INTRA_MODES] = {2,8,1,0,5,9,7,6,4,3};
      int code = intra_mode_map[intra_mode];
      assert (code<8);
      if (code==0){
        putbits(2,0,stream);
      }
      else if(code==1){
         putbits(2,1,stream);
      }
      else if(code==2){
         putbits(2,2,stream);
      }
      else if(code==3){
         putbits(4,12,stream);
      }
      else if(code==4){
         putbits(4,13,stream);
      }
      else if(code==5){
         putbits(4,14,stream);
      }
      else if(code==6){
         putbits(5,30,stream);
      }
      else if(code==7){
         putbits(5,31,stream);
      }
#else
      int intra_mode_map[MAX_NUM_INTRA_MODES] = {2,3,1,0,6,9,8,7,5,4};
      int code = intra_mode_map[intra_mode];
      if (code==0){
        putbits(2,2,stream);
      }
      else if(code==1){
         putbits(2,3,stream);
      }
      else if(code==2){
         putbits(3,2,stream);
      }
      else if(code==3){
         putbits(3,3,stream);
      }
      else if(code==4){
         putbits(4,2,stream);
      }
      else if(code==5){
         putbits(4,3,stream);
      }
      else if(code==6){
         putbits(5,0,stream);
      }
      else if(code==7){
         putbits(5,1,stream);
      }
      else if(code==8){
         putbits(5,2,stream);
      }
      else if(code==9){
         putbits(5,3,stream);
      }
#endif
    }
  }
  else if (mode==MODE_INTER){
    /* Code PU partitions */
    if (write_data->max_num_pb_part > 1){
      if (write_data->pb_part==0)
        putbits(1,1,stream);
      else if(write_data->pb_part==1)
        putbits(2,1,stream);
      else if(write_data->pb_part==2)
        putbits(3,1,stream);
      else if(write_data->pb_part==3)
        putbits(3,0,stream);
    }
    /* Code motion vectors for each prediction block */
    mv_t mvp2 = mvp;
#if TWO_MVP
    if (write_data->mv_idx >= 0){
      putbits(1,write_data->mv_idx,stream);
    }
#endif
    if (write_data->pb_part==PART_NONE){ //NONE
      write_mv(stream,&write_data->mv_arr[0],&mvp2);
    }
    else if (write_data->pb_part==PART_HOR){ //HOR
      write_mv(stream,&write_data->mv_arr[0],&mvp2);
      mvp2 = write_data->mv_arr[0];
      write_mv(stream,&write_data->mv_arr[2],&mvp2);
    }
    else if (write_data->pb_part==PART_VER){ //VER
      write_mv(stream,&write_data->mv_arr[0],&mvp2);
      mvp2 = write_data->mv_arr[0];
      write_mv(stream,&write_data->mv_arr[1],&mvp2);
    }
    else{
      write_mv(stream,&write_data->mv_arr[0],&mvp2);
      mvp2 = write_data->mv_arr[0];
      write_mv(stream,&write_data->mv_arr[1],&mvp2);
      write_mv(stream,&write_data->mv_arr[2],&mvp2);
      write_mv(stream,&write_data->mv_arr[3],&mvp2);
    }
  }
  else if (mode==MODE_BIPRED){
#if TWO_MVP
    if (write_data->mv_idx >= 0){
      putbits(1,write_data->mv_idx,stream);
    }
#endif
    /* Code motion vectors for each prediction block */
    mv_t mvp2 = mvp; //TODO: Use different predictors for mv0 and mv1
    write_mv(stream,&write_data->mv_arr0[0],&mvp2); //TODO: Make bipred and pb-split combination work
    write_mv(stream,&write_data->mv_arr1[0],&mvp2);
    if (write_data->num_ref==2){
      int code = 2*write_data->ref_idx1 + write_data->ref_idx0;
      if (code==3)
        putbits(3,0,stream);
      else
        putbits(code+1,1,stream);
    }
    else{
      int code = 4*write_data->ref_idx1 + write_data->ref_idx0; //TODO: Optimize for num_ref != 4
      put_vlc(10,code,stream);
    }
  }
  else if (mode==MODE_SKIP){
    /* Code skip_idx */
    if (write_data->num_skip_vec == 4)
      putbits(2,write_data->skip_idx,stream);
    else if (write_data->num_skip_vec == 3){
      if (write_data->skip_idx == 0) putbits(1,1,stream);
      else if (write_data->skip_idx == 1) putbits(2,0,stream);
      else putbits(2,1,stream);
    }
    else if (write_data->num_skip_vec == 2){
      putbits(1,write_data->skip_idx,stream);
    }
  }
  else if (mode==MODE_MERGE){
    /* Code skip_idx */
    if (write_data->num_skip_vec == 4)
      putbits(2,write_data->skip_idx,stream);
    else if (write_data->num_skip_vec == 3){
      if (write_data->skip_idx == 0) putbits(1,1,stream);
      else if (write_data->skip_idx == 1) putbits(2,0,stream);
      else putbits(2,1,stream);
    }
    else if (write_data->num_skip_vec == 2){
      putbits(1,write_data->skip_idx,stream);
    }
  }

  if (mode != MODE_SKIP){
    if (write_data->max_num_tb_part>1){
      if (tb_split){
        code = 2;
      }
      else{
        cbp = cbp_y + (cbp_u<<1) + (cbp_v<<2);
        code = cbp_table[cbp];
        if (write_data->block_context->cbp==0 && code < 2)
          code = 1-code;
        if (code > 1) code++;
      }
    }
    else{
#if NO_SUBBLOCK_SKIP
      if (0){
#else
      if (mode==MODE_MERGE){
#endif
        cbp = cbp_y + (cbp_u<<1) + (cbp_v<<2);
        code = cbp_table[cbp];
        if (code==1)
          code = 7;
        else if (code>1)
          code = code-1;
      }
      else{
        cbp = cbp_y + (cbp_u<<1) + (cbp_v<<2);
        code = cbp_table[cbp];
        if (write_data->block_context->cbp==0 && code < 2){
          code = 1-code;
        }
      }
    }
    put_vlc(0,code,stream);

    if (tb_split==0){
      if (cbp_y){
        write_coeff(stream,coeffq_y,size,0);
      }
      if (cbp_u){
        write_coeff(stream,coeffq_u,size/2,1);
      }
      if (cbp_v){
        write_coeff(stream,coeffq_v,size/2,1);
      }
    }
    else{
      if (size > 8){
        int index;
        for (index=0;index<4;index++){
          cbp_y = ((write_data->cbp->y)>>(3-index))&1;
          cbp_u = ((write_data->cbp->u)>>(3-index))&1;
          cbp_v = ((write_data->cbp->v)>>(3-index))&1;

          /* Code cbp separately for each TU */
          int cbp = cbp_y + (cbp_u<<1) + (cbp_v<<2);
          code = cbp_table[cbp];
          if (write_data->block_context->cbp==0 && code < 2)
            code = 1-code;
          put_vlc(0,code,stream);

          /* Code coefficients for each TU separately */
          coeffq_y = write_data->coeffq_y + index*(size/2)*(size/2);
          coeffq_u = write_data->coeffq_u + index*(size/4)*(size/4);
          coeffq_v = write_data->coeffq_v + index*(size/4)*(size/4);
          if (cbp_y){
            write_coeff(stream,coeffq_y,size/2,0);
          }
          if (cbp_u){
            write_coeff(stream,coeffq_u,size/4,1);
          }
          if (cbp_v){
            write_coeff(stream,coeffq_v,size/4,1);
          }
        } //for index=
      } //if (size > 8)
      else{
        int index;
        for (index=0;index<4;index++){
          cbp_y = ((write_data->cbp->y)>>(3-index))&1;

          /* Code cbp_y separately for each TU */
          putbits(1,cbp_y,stream);

          /* Code coefficients for each TU separately */
          coeffq_y = write_data->coeffq_y + index*(size/2)*(size/2);
          if (cbp_y){
            write_coeff(stream,coeffq_y,size/2,0);
          }
        }
        cbp = cbp_u + 2*cbp_v;
        if (cbp==0)
          putbits(1,1,stream);
        else if(cbp==1)
          putbits(2,1,stream);
        else if(cbp==2)
          putbits(3,1,stream);
        else
          putbits(3,0,stream);
        if (cbp_u){
          write_coeff(stream,coeffq_u,size/2,1);
        }
        if (cbp_v){
          write_coeff(stream,coeffq_v,size/2,1);
        }
      } //if (size > 8)
    } //if (tb_split==0
  } //if (mode!= MODE_SKIP)

  end_bits = get_bit_pos(stream);

  return (end_bits - start_bits);
}

