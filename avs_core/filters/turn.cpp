// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

/*
** Turn. version 0.1
** (c) 2003 - Ernst PechÚ
**
*/

#include "turn.h"
#include "resample.h"
#include "planeswap.h"
#include "../core/internal.h"
#include <tmmintrin.h>
#include <stdint.h>


extern const AVSFunction Turn_filters[] = {
  { "TurnLeft",  BUILTIN_FUNC_PREFIX, "c",Turn::create_turnleft },
  { "TurnRight", BUILTIN_FUNC_PREFIX, "c",Turn::create_turnright },
  { "Turn180",   BUILTIN_FUNC_PREFIX, "c",Turn::create_turn180 },
  { 0 }
};

enum TurnDirection
{
  DIRECTION_LEFT = 0,
  DIRECTION_RIGHT = 1,
  DIRECTION_180 = 2
};

static __forceinline void left_transpose_4_doublewords_sse2(__m128i &src1, __m128i &src2, __m128i& src3, __m128i &src4) {
  __m128i b0a0b1a1 = _mm_unpacklo_epi32(src2, src1);
  __m128i b2a2b3a3 = _mm_unpackhi_epi32(src2, src1);
  __m128i d0c0d1c1 = _mm_unpacklo_epi32(src4, src3);
  __m128i d2c2d3c3 = _mm_unpackhi_epi32(src4, src3);

  src1 = _mm_unpacklo_epi64(d0c0d1c1, b0a0b1a1); //d0c0b0a0
  src2 = _mm_unpackhi_epi64(d0c0d1c1, b0a0b1a1); //d1c1b1a1
  src3 = _mm_unpacklo_epi64(d2c2d3c3, b2a2b3a3); //d2c2b2a2
  src4 = _mm_unpackhi_epi64(d2c2d3c3, b2a2b3a3); //d3c3b3a3
}

static __forceinline void right_transpose_4_doublewords_sse2(__m128i &src1, __m128i &src2, __m128i& src3, __m128i &src4) {
  __m128i a0b0a1b1 = _mm_unpacklo_epi32(src1, src2);
  __m128i a2b2a3b3 = _mm_unpackhi_epi32(src1, src2);
  __m128i c0d0c1d1 = _mm_unpacklo_epi32(src3, src4);
  __m128i c2d2c3d3 = _mm_unpackhi_epi32(src3, src4);

  src1 = _mm_unpacklo_epi64(a0b0a1b1, c0d0c1d1); //a0b0c0d0
  src2 = _mm_unpackhi_epi64(a0b0a1b1, c0d0c1d1); //a1b1c1d1
  src3 = _mm_unpacklo_epi64(a2b2a3b3, c2d2c3d3); //a2b2c2d2
  src4 = _mm_unpackhi_epi64(a2b2a3b3, c2d2c3d3); //a3b3c3d3
}

void turn_left_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  int dstp_offset;
  for (int y = 0; y<height; y++) {
    dstp_offset = (height-1-y)*3;
    for (int x=0; x<width; x+=3) {	
      dstp[dstp_offset+0] = srcp[x+0];
      dstp[dstp_offset+1] = srcp[x+1];
      dstp[dstp_offset+2] = srcp[x+2];
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch;
  }
}

void turn_right_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  int dstp_offset;
  int dstp_base = (width/3-1) * dst_pitch;
  for (int y=0; y<height; y++) {
    dstp_offset = dstp_base + y*3;
    for (int x = 0; x<width; x+=3) {	
      dstp[dstp_offset+0] = srcp[x+0];
      dstp[dstp_offset+1] = srcp[x+1];
      dstp[dstp_offset+2] = srcp[x+2];
      dstp_offset -= dst_pitch;
    }
    srcp += src_pitch;
  }
}

static void turn_180_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  dstp += (height-1)*dst_pitch + (width-3);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x+=3) {	
      dstp[-x+0] = srcp[x+0];
      dstp[-x+1] = srcp[x+1];
      dstp[-x+2] = srcp[x+2];
    }
    dstp -= dst_pitch;
    srcp += src_pitch;
  }
}

void turn_left_rgb32_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  int dstp_offset;
  for (int y=0; y<height; y++) {
    dstp_offset = (height-1-y);
    for (int x=0; x<l_rowsize; x++) {	
      l_dstp[dstp_offset] = l_srcp[x];
      dstp_offset += l_dst_pitch;
    }
    l_srcp += l_src_pitch;
  }
}

void turn_left_rgb32_sse2(const BYTE *srcp, BYTE *dstp, int src_width_bytes, int src_height, int src_pitch, int dst_pitch)
{
  const BYTE* srcp2 = srcp;

  int src_width_mod16 = (src_width_bytes / 16) * 16;
  int src_height_mod4 = (src_height / 4) * 4;

  for(int y=0; y<src_height_mod4; y+=4)
  {
    int offset = (src_height*4)-16-(y*4);
    for (int x=0; x<src_width_mod16; x+=16)
    {
      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*0));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*1));
      __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*2));
      __m128i src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*3));

      left_transpose_4_doublewords_sse2(src1, src2, src3, src4);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*0), src1);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*1), src2);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*2), src3);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*3), src4);

      offset += dst_pitch*4;
    }

    if (src_width_mod16 != src_width_bytes) {
      offset = src_height*4 - 16 - (y*4) + ((src_width_bytes/4)-4)*dst_pitch;

      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*0));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*1));
      __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*2));
      __m128i src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*3));

      left_transpose_4_doublewords_sse2(src1, src2, src3, src4);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + offset + dst_pitch * 0), src1);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + offset + dst_pitch * 1), src2);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + offset + dst_pitch * 2), src3);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + offset + dst_pitch * 3), src4);
    }

    srcp += src_pitch * 4;
  }

  if (src_height_mod4 != src_height) {
    turn_left_rgb32_c(srcp2 + src_height_mod4 * src_pitch, dstp, src_width_bytes, src_height - src_height_mod4, src_pitch, dst_pitch);
  }
}

void turn_right_rgb32_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  int dstp_offset;
  int dstp_base = (l_rowsize-1) * l_dst_pitch;
  for (int y = 0; y<height; y++) {
    dstp_offset = dstp_base + y;
    for (int x = 0; x<l_rowsize; x++) {	
      l_dstp[dstp_offset] = l_srcp[x];
      dstp_offset -= l_dst_pitch;
    }
    l_srcp += l_src_pitch;
  }
}

void turn_right_rgb32_sse2(const BYTE *srcp, BYTE *dstp, int src_width_bytes, int src_height, int src_pitch, int dst_pitch)
{
  const BYTE* srcp2 = srcp;

  int src_width_mod16 = (src_width_bytes / 16) * 16;
  int src_height_mod4 = (src_height / 4) * 4;

  for(int y=0; y<src_height_mod4; y+=4)
  {
    int offset = (src_width_bytes / 4 - 4) * dst_pitch + (y*4);
    for (int x=0; x<src_width_mod16; x+=16)
    {
      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*0));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*1));
      __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*2));
      __m128i src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x+src_pitch*3));

      right_transpose_4_doublewords_sse2(src1, src2, src3, src4);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*0), src4);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*1), src3);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*2), src2);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+offset+dst_pitch*3), src1);

      offset -= dst_pitch*4;
    }

    if (src_width_mod16 != src_width_bytes) {
      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*0));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*1));
      __m128i src3 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*2));
      __m128i src4 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+src_width_bytes-16+src_pitch*3));

      right_transpose_4_doublewords_sse2(src1, src2, src3, src4);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + y*4 + dst_pitch * 0), src4);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + y*4 + dst_pitch * 1), src3);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + y*4 + dst_pitch * 2), src2);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + y*4 + dst_pitch * 3), src1);
    }

    srcp += src_pitch * 4;
  }

  if (src_height_mod4 != src_height) {
    turn_right_rgb32_c(srcp2 + src_height_mod4 * src_pitch, dstp + src_height_mod4*4, src_width_bytes, src_height - src_height_mod4, src_pitch, dst_pitch);
  }
}

static void turn_180_rgb32_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  l_dstp += (height-1)*l_dst_pitch + (l_rowsize-1);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<l_rowsize; x++) {	
      l_dstp[-x] = l_srcp[x];
    }
    l_dstp -= l_dst_pitch;
    l_srcp += l_src_pitch;
  }
}

static void turn_180_rgb32_sse2(const BYTE *srcp, BYTE *dstp, int src_width, int src_height, int src_pitch, int dst_pitch)
{
  BYTE* dstp2 = dstp;
  const BYTE* srcp2 = srcp;
  int src_width_mod16 = (src_width / 16) * 16;

  __m128i zero = _mm_setzero_si128();

  dstp += dst_pitch * (src_height-1) + src_width - 16;
  for(int y = 0; y < src_height; ++y)
  {
    for (int x = 0; x < src_width_mod16; x+=16)
    {
      __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x));
      src = _mm_shuffle_epi32(src, _MM_SHUFFLE(0, 1, 2, 3));
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp-x), src);
    }

    if (src_width_mod16 != src_width) {
      __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp + src_width-16));
      src = _mm_shuffle_epi32(src, _MM_SHUFFLE(0, 1, 2, 3));
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp-src_width+16), src);
    }

    srcp += src_pitch;
    dstp -= dst_pitch;
  }
}

static void turn_right_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  BYTE u,v;
  int dstp_offset;

  for (int y=0; y<height; y+=2)
  {
    dstp_offset = ((height-2-y)<<1);
    for (int x=0; x<width; x+=4)
    {
      u = (srcp[x+1] + srcp[x+1+src_pitch] + 1) >> 1;
      v = (srcp[x+3] + srcp[x+3+src_pitch] + 1) >> 1;
      dstp[dstp_offset+0] = srcp[x+src_pitch];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[x];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
      dstp[dstp_offset+0] = srcp[x+src_pitch+2];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[x+2];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch<<1;
  }
}

static void turn_left_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  BYTE u,v;
  int dstp_offset;
 
  srcp += width-4;
  for (int y=0; y<height; y+=2)
  {
    dstp_offset = (y<<1);
    for (int x=0; x<width; x+=4)
    {
      u = (srcp[-x+1] + srcp[-x+1+src_pitch] + 1) >> 1;
      v = (srcp[-x+3] + srcp[-x+3+src_pitch] + 1) >> 1;
      dstp[dstp_offset+0] = srcp[-x+2];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[-x+2+src_pitch];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
      dstp[dstp_offset+0] = srcp[-x];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[-x+src_pitch];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch<<1;
  }
}

static void turn_180_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  dstp += (height-1)*dst_pitch + (width-4);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x+=4) {	
      dstp[-x+2] = srcp[x+0];
      dstp[-x+1] = srcp[x+1];
      dstp[-x+0] = srcp[x+2];
      dstp[-x+3] = srcp[x+3];
    }
    dstp -= dst_pitch;
    srcp += src_pitch;
  }
}


static __forceinline __m128i mm_movehl_si128(const __m128i &a, const __m128i &b) {
  return _mm_castps_si128(_mm_movehl_ps(_mm_castsi128_ps(a), _mm_castsi128_ps(b)));
}

static __forceinline void left_transpose_8_bytes_sse2(__m128i &src1, __m128i &src2, __m128i& src3, __m128i &src4, 
                              __m128i &src5, __m128i& src6, __m128i &src7, __m128i &src8, const __m128i &zero) {

  __m128i a07b07 = _mm_unpacklo_epi8(src1, src2); 
  __m128i c07d07 = _mm_unpacklo_epi8(src3, src4); 
  __m128i e07f07 = _mm_unpacklo_epi8(src5, src6); 
  __m128i g07h07 = _mm_unpacklo_epi8(src7, src8);  

  __m128i a03b03c03d03 = _mm_unpacklo_epi16(a07b07, c07d07);
  __m128i e03f03g03h03 = _mm_unpacklo_epi16(e07f07, g07h07);
  __m128i a47b47c47d47 = _mm_unpackhi_epi16(a07b07, c07d07);
  __m128i e47f47g47h47 = _mm_unpackhi_epi16(e07f07, g07h07);

  __m128i a01b01c01d01e01f01g01h01 = _mm_unpacklo_epi32(a03b03c03d03, e03f03g03h03); 
  __m128i a23b23c23d23e23f23g23h23 = _mm_unpackhi_epi32(a03b03c03d03, e03f03g03h03); 
  __m128i a45b45c45d45e45f45g45h45 = _mm_unpacklo_epi32(a47b47c47d47, e47f47g47h47); 
  __m128i a67b67c67d67e67f67g67h67 = _mm_unpackhi_epi32(a47b47c47d47, e47f47g47h47); 

  src1 = a01b01c01d01e01f01g01h01;
  src2 = mm_movehl_si128(zero, a01b01c01d01e01f01g01h01);
  src3 = a23b23c23d23e23f23g23h23;
  src4 = mm_movehl_si128(zero, a23b23c23d23e23f23g23h23);
  src5 = a45b45c45d45e45f45g45h45;
  src6 = mm_movehl_si128(zero, a45b45c45d45e45f45g45h45);
  src7 = a67b67c67d67e67f67g67h67;
  src8 = mm_movehl_si128(zero, a67b67c67d67e67f67g67h67);
}

static __forceinline void right_transpose_8_bytes_sse2(__m128i &src1, __m128i &src2, __m128i& src3, __m128i &src4,                                                __m128i &src5, __m128i& src6, __m128i &src7, __m128i &src8, const __m128i &zero) {

  __m128i b07a07 = _mm_unpacklo_epi8(src2, src1); 
  __m128i d07c07 = _mm_unpacklo_epi8(src4, src3); 
  __m128i f07e07 = _mm_unpacklo_epi8(src6, src5); 
  __m128i h07g07 = _mm_unpacklo_epi8(src8, src7);  

  __m128i d03c03b03a03 = _mm_unpacklo_epi16(d07c07, b07a07);
  __m128i h03g03f03e03 = _mm_unpacklo_epi16(h07g07, f07e07);
  __m128i d47c47b47a47 = _mm_unpackhi_epi16(d07c07, b07a07);
  __m128i h47g47f47e47 = _mm_unpackhi_epi16(h07g07, f07e07);

    __m128i h01g01f01e01d01c01b01a01 = _mm_unpacklo_epi32(h03g03f03e03, d03c03b03a03); 
    __m128i h23g23f23e23d23c23b23a23 = _mm_unpackhi_epi32(h03g03f03e03, d03c03b03a03); 
    __m128i h45g45f45e45d45c45b45a45 = _mm_unpacklo_epi32(h47g47f47e47, d47c47b47a47); 
    __m128i h67g67f67e67d67c67b67a67 = _mm_unpackhi_epi32(h47g47f47e47, d47c47b47a47); 

    src1 = h01g01f01e01d01c01b01a01;
    src2 = mm_movehl_si128(zero, h01g01f01e01d01c01b01a01);
    src3 = h23g23f23e23d23c23b23a23;
    src4 = mm_movehl_si128(zero, h23g23f23e23d23c23b23a23);
    src5 = h45g45f45e45d45c45b45a45;
    src6 = mm_movehl_si128(zero, h45g45f45e45d45c45b45a45);
    src7 = h67g67f67e67d67c67b67a67;
    src8 = mm_movehl_si128(zero, h67g67f67e67d67c67b67a67);
}

void turn_right_plane_sse2(const BYTE* pSrc, BYTE* pDst, int srcWidth, int srcHeight, int srcPitch, int dstPitch) {
  const BYTE* pSrc2 = pSrc;

  __m128i zero = _mm_setzero_si128();

  int srcWidthMod8 = (srcWidth / 8) * 8;
  int srcHeightMod8 = (srcHeight / 8) * 8;
  for(int y=0; y<srcHeightMod8; y+=8)
  {
    int offset = srcHeight-8-y;
    for (int x=0; x<srcWidthMod8; x+=8)
    {
      __m128i src1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*0));
      __m128i src2 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*1));
      __m128i src3 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*2));
      __m128i src4 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*3));
      __m128i src5 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*4));
      __m128i src6 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*5));
      __m128i src7 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*6));
      __m128i src8 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc+x+srcPitch*7));

      right_transpose_8_bytes_sse2(src1, src2, src3, src4, src5, src6, src7, src8, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*0), src1);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*1), src2);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*2), src3);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*3), src4);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*4), src5);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*5), src6);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*6), src7);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*7), src8);

      offset += dstPitch*8;
    }
    pSrc += srcPitch * 8;
  }

  if (srcHeightMod8 != srcHeight) {
    pSrc = pSrc2 + srcPitch*srcHeightMod8;
    for(int y=srcHeightMod8; y<srcHeight; ++y)
    {
      int offset = srcHeight-1-y;
      for (int x=0; x<srcWidth; ++x)
      {
        pDst[offset] = pSrc[x];
        offset += dstPitch;
      }
      pSrc += srcPitch;
    }
  }

  if (srcWidthMod8 != srcWidth) {
    pSrc = pSrc2;
    for(int y=0; y<srcHeight; ++y)
    {
      int offset = (srcWidthMod8)*dstPitch + srcHeight - 1 - y;
      for (int x=srcWidthMod8; x<srcWidth; ++x)
      {
        pDst[offset] = pSrc[x];
        offset += dstPitch;
      }
      pSrc += srcPitch;
    }
  }
}

void turn_left_plane_sse2(const BYTE* pSrc, BYTE* pDst, int srcWidth, int srcHeight, int srcPitch, int dstPitch) {
  const BYTE* pSrc2 = pSrc;
  int srcWidthMod8 = (srcWidth / 8) * 8;
  int srcHeightMod8 = (srcHeight / 8) * 8;

  pSrc += srcWidth-8;

  __m128i zero = _mm_setzero_si128();

  for(int y=0; y<srcHeightMod8; y+=8)
  {
    int offset = y;
    for (int x=0; x<srcWidthMod8; x+=8)
    {
      __m128i src1 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*0));
      __m128i src2 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*1));
      __m128i src3 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*2));
      __m128i src4 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*3));
      __m128i src5 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*4));
      __m128i src6 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*5));
      __m128i src7 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*6));
      __m128i src8 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(pSrc-x+srcPitch*7));

      left_transpose_8_bytes_sse2(src1, src2, src3, src4, src5, src6, src7, src8, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*0), src8);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*1), src7);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*2), src6);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*3), src5);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*4), src4);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*5), src3);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*6), src2);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(pDst+offset+dstPitch*7), src1);

      offset += dstPitch*8;
    }
    pSrc += srcPitch * 8;
  }

  if (srcHeightMod8 != srcHeight) {
    pSrc = pSrc2;

    pSrc += srcWidth-1 + srcPitch*srcHeightMod8;
    for(int y=srcHeightMod8; y<srcHeight; ++y)
    {
      int offset = y;
      for (int x=0; x<srcWidth; ++x)
      {
        pDst[offset] = pSrc[-x];
        offset += dstPitch;
      }
      pSrc += srcPitch;
    }
  }

  if (srcWidthMod8 != srcWidth) {
    pSrc = pSrc2;

    pSrc += srcWidth-1;
    for(int y=0; y<srcHeight; ++y)
    {
      int offset = y+dstPitch*srcWidthMod8;
      for (int x=srcWidthMod8; x<srcWidth; ++x)
      {
        pDst[offset] = pSrc[-x];
        offset += dstPitch;
      }
      pSrc += srcPitch;
    }
  }
}

template<int instruction_set>
static void turn_180_plane_xsse(const BYTE* pSrc, BYTE* pDst, int srcWidth, int srcHeight, int srcPitch, int dstPitch) {
  BYTE* pDst2 = pDst;
  const BYTE* pSrc2 = pSrc;
  int srcWidthMod16 = (srcWidth / 16) * 16;

  __m128i zero = _mm_setzero_si128();
  __m128i pshufbMask = _mm_set_epi8(0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15);

  pDst += dstPitch * (srcHeight-1) + srcWidth - 16;
  for(int y = 0; y < srcHeight; ++y)
  {
    for (int x = 0; x < srcWidthMod16; x+=16)
    {
      __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(pSrc+x));

      if (instruction_set == CPUF_SSE2) {
        //15 14 13 12 11 10 9 8 7 6 5 4 3 2 1 0
        src = _mm_shuffle_epi32(src, _MM_SHUFFLE(0, 1, 2, 3)); //3 2 1 0 7 6 5 4 11 10 9 8 15 14 13 12 
        src = _mm_shufflelo_epi16(src, _MM_SHUFFLE(2, 3, 0, 1));
        src = _mm_shufflehi_epi16(src, _MM_SHUFFLE(2, 3, 0, 1)); //1 0 3 2 5 4 7 6 9 8 11 10 13 12 15 14
        __m128i t1 = _mm_slli_epi16(src, 8); 
        __m128i t2 = _mm_srli_epi16(src, 8); 
        src = _mm_or_si128(t1, t2);
      } else { 
        src = _mm_shuffle_epi8(src, pshufbMask);
      }

      _mm_storeu_si128(reinterpret_cast<__m128i*>(pDst-x), src);
    }
    pSrc += srcPitch;
    pDst -= dstPitch;
  }
  if (srcWidthMod16 != srcWidth) {
    pSrc = pSrc2;
    pDst = pDst2 + dstPitch * (srcHeight-1) + srcWidth - 1;

    for (int y = 0; y < srcHeight; ++y) {
      for (int x = srcWidthMod16; x < srcWidth; ++x) {
        pDst[-x] = pSrc[x];
      }
      pSrc += srcPitch;
      pDst -= dstPitch;
    }
  }
}

TurnFuncPtr turn_180_plane_sse2 = &turn_180_plane_xsse<CPUF_SSE2>;
TurnFuncPtr turn_180_plane_ssse3 = &turn_180_plane_xsse<CPUF_SSSE3>;

template<typename pixel_size>
void turn_right_plane_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  const pixel_size *_srcp = reinterpret_cast<const pixel_size *>(srcp);
  pixel_size *_dstp = reinterpret_cast<pixel_size *>(dstp);
  src_pitch = src_pitch / sizeof(pixel_size); // AVS16
  dst_pitch = dst_pitch / sizeof(pixel_size);
  width = width / sizeof(pixel_size); // width was GetRowSize()
  for(int y=0; y<height; y++)
  {
    int offset = height-1-y;
    for (int x=0; x<width; x++)
    {
      _dstp[offset] = _srcp[x];
      offset += dst_pitch;
    }
    _srcp += src_pitch;
  }
}

template<typename pixel_size>
void turn_left_plane_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  const pixel_size *_srcp = reinterpret_cast<const pixel_size *>(srcp);
  pixel_size *_dstp = reinterpret_cast<pixel_size *>(dstp);
  src_pitch = src_pitch / sizeof(pixel_size); // AVS16
  dst_pitch = dst_pitch / sizeof(pixel_size);
  width = width / sizeof(pixel_size); // width was GetRowSize()
  _srcp += width-1;
  for(int y=0; y<height; y++)
  {
    int offset = y;
    for (int x=0; x<width; x++)
    {
      _dstp[offset] = _srcp[-x];
      offset += dst_pitch;
    }
    _srcp += src_pitch;
  }
}

template<typename pixel_size>
static void turn_180_plane_c(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  const pixel_size *_srcp = reinterpret_cast<const pixel_size *>(srcp);
  pixel_size *_dstp = reinterpret_cast<pixel_size *>(dstp);
  src_pitch = src_pitch / sizeof(pixel_size); // AVS16
  dst_pitch = dst_pitch / sizeof(pixel_size);
  width = width / sizeof(pixel_size); // width was GetRowSize()
  _dstp += (height-1)*dst_pitch + (width-1);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x++) {
      _dstp[-x] = _srcp[x];
    }
    _dstp -= src_pitch;
    _srcp += dst_pitch;
  }
}


Turn::Turn(PClip _child, int _direction, IScriptEnvironment* env) : GenericVideoFilter(_child), u_source(0), v_source(0)
{
  if (_direction == DIRECTION_LEFT || _direction == DIRECTION_RIGHT) {
    const int src_height = vi.height;
    vi.height = vi.width;
    vi.width = src_height;
  }
  direction = _direction;

  if (vi.IsRGB())
  {
    if (vi.BitsPerPixel() == 32) { 
      if (env->GetCPUFlags() & CPUF_SSE2) {
        TurnFuncPtr functions[3] = {turn_left_rgb32_sse2, turn_right_rgb32_sse2, turn_180_rgb32_sse2};
        turn_function = functions[direction]; 
      } else {
        TurnFuncPtr functions[3] = {turn_left_rgb32_c, turn_right_rgb32_c, turn_180_rgb32_c};
        turn_function = functions[direction]; 
      }
    }
    else if (vi.BitsPerPixel() == 24) {
      TurnFuncPtr functions[3] = {turn_left_rgb24, turn_right_rgb24, turn_180_rgb24};
      turn_function = functions[direction]; 
    }
    else env->ThrowError("Turn: Unsupported RGB bit depth");
  }
  else if (vi.IsYUY2())
  {
    if ((vi.width % 2) != 0) {
      env->ThrowError("Turn: YUY2 data must have MOD2 height");
    }
    TurnFuncPtr functions[3] = {turn_left_yuy2, turn_right_yuy2, turn_180_yuy2};
    turn_function = functions[direction]; 
  }
  else if (vi.IsPlanar())
  {
    switch (vi.BytesFromPixels(1)) // AVS16
    {
    case 1: // 8 bit
      if (env->GetCPUFlags() & CPUF_SSSE3) {
        TurnFuncPtr functions[3] = { turn_left_plane_sse2, turn_right_plane_sse2, turn_180_plane_ssse3 };
        turn_function = functions[direction];
      }
      else if (env->GetCPUFlags() & CPUF_SSE2) {
        TurnFuncPtr functions[3] = { turn_left_plane_sse2, turn_right_plane_sse2, turn_180_plane_sse2 };
        turn_function = functions[direction];
      }
      else {
        TurnFuncPtr functions[3] = { turn_left_plane_c<uint8_t>, turn_right_plane_c<uint8_t>, turn_180_plane_c<uint8_t> };
        turn_function = functions[direction];
      }
      break;
    case 2: // 16 bit todo SSE
    {
      TurnFuncPtr functions[3] = { turn_left_plane_c<uint16_t>, turn_right_plane_c<uint16_t>, turn_180_plane_c<uint16_t> };
      turn_function = functions[direction];
      break;
    }
    default: // 32 bit todo SSE
    {
      TurnFuncPtr functions[3] = { turn_left_plane_c<float>, turn_right_plane_c<float>, turn_180_plane_c<float> };
      turn_function = functions[direction];
    }
    }
      // rectangular formats?
    if ((_direction == DIRECTION_LEFT || _direction == DIRECTION_RIGHT) && 
      !vi.IsY8() && !vi.IsColorSpace(VideoInfo::CS_Y16) && !vi.IsColorSpace(VideoInfo::CS_Y32) &&
      (vi.GetPlaneWidthSubsampling(PLANAR_U) != vi.GetPlaneHeightSubsampling(PLANAR_U)))
    {
      if (vi.width % (1<<vi.GetPlaneWidthSubsampling(PLANAR_U))) // YV16 & YV411
        env->ThrowError("Turn: Planar data must have MOD %d height",
        1<<vi.GetPlaneWidthSubsampling(PLANAR_U));

      if (vi.height % (1<<vi.GetPlaneHeightSubsampling(PLANAR_U))) // No current formats
        env->ThrowError("Turn: Planar data must have MOD %d width",
        1<<vi.GetPlaneHeightSubsampling(PLANAR_U));

      MitchellNetravaliFilter filter(1./3., 1./3.);
      AVSValue subs[4] = { 0.0, 0.0, 0.0, 0.0 }; 

      u_source = new SwapUVToY(child, SwapUVToY::UToY8, env); // Y16 and Y32 capable
      v_source = new SwapUVToY(child, SwapUVToY::VToY8, env);

      const VideoInfo vi_u = u_source->GetVideoInfo();

      const int uv_height = (vi_u.height << vi.GetPlaneHeightSubsampling(PLANAR_U)) >> vi.GetPlaneWidthSubsampling(PLANAR_U);
      const int uv_width  = (vi_u.width  << vi.GetPlaneWidthSubsampling(PLANAR_U))  >> vi.GetPlaneHeightSubsampling(PLANAR_U);

      u_source = FilteredResize::CreateResize(u_source, uv_width, uv_height, subs, &filter, env);
      v_source = FilteredResize::CreateResize(v_source, uv_width, uv_height, subs, &filter, env);
    }
  }
  else
  {
    env->ThrowError("Turn: Image format not supported!");
  }
}

int __stdcall Turn::SetCacheHints(int cachehints, int frame_range) {
  return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
}

PVideoFrame __stdcall Turn::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);

  PVideoFrame dst = env->NewVideoFrame(vi);

	if (u_source && v_source) 
  {
		PVideoFrame usrc = u_source->GetFrame(n, env);
		PVideoFrame vsrc = v_source->GetFrame(n, env);

    turn_function(src->GetReadPtr(PLANAR_Y),  dst->GetWritePtr(PLANAR_Y), src->GetRowSize(PLANAR_Y),  src->GetHeight(PLANAR_Y),  src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y));
    turn_function(usrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), usrc->GetRowSize(PLANAR_Y), usrc->GetHeight(PLANAR_Y), usrc->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U));
    turn_function(vsrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_V), usrc->GetRowSize(PLANAR_Y), usrc->GetHeight(PLANAR_Y), vsrc->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_V));
	}
  else if (vi.IsPlanar()) 
  {
    turn_function(src->GetReadPtr(PLANAR_Y),  dst->GetWritePtr(PLANAR_Y), src->GetRowSize(PLANAR_Y),  src->GetHeight(PLANAR_Y),  src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y));
    turn_function(src->GetReadPtr(PLANAR_U),  dst->GetWritePtr(PLANAR_U), src->GetRowSize(PLANAR_U),  src->GetHeight(PLANAR_U),  src->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_U));
    turn_function(src->GetReadPtr(PLANAR_V),  dst->GetWritePtr(PLANAR_V), src->GetRowSize(PLANAR_V),  src->GetHeight(PLANAR_V),  src->GetPitch(PLANAR_V), dst->GetPitch(PLANAR_V));
  } 
  else 
  {
		turn_function(src->GetReadPtr(),dst->GetWritePtr(),src->GetRowSize(), src->GetHeight(),src->GetPitch(),dst->GetPitch());
  }
  return dst;
}


AVSValue __cdecl Turn::create_turnleft(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_LEFT, env);
}

AVSValue __cdecl Turn::create_turnright(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_RIGHT, env);
}

AVSValue __cdecl Turn::create_turn180(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_180, env);
}


