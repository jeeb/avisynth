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

#include "convert_yv12.h"
#include <emmintrin.h>


/* YV12 -> YUY2 conversion */


static inline void copy_yv12_line_to_yuy2_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  for (int x = 0; x < width / 2; ++x) {
    dstp[x*4] = srcY[x*2];
    dstp[x*4+2] = srcY[x*2+1];
    dstp[x*4+1] = srcU[x];
    dstp[x*4+3] = srcV[x];
  }
}

void convert_yv12_to_yuy2_progressive_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first two lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      //avg(avg(a, b)-1, b)
      dstp[x*4+1] = ((((srcU[x-src_pitch_uv] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch] = srcY[x*2 + src_pitch_y];
      dstp[x*4+2 + dst_pitch] = srcY[x*2+1 + src_pitch_y];

      dstp[x*4+1 + dst_pitch] = ((((srcU[x] + srcU[x+src_pitch_uv] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch] = ((((srcV[x] + srcV[x+src_pitch_uv] + 1) / 2) + srcV[x]) / 2);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}

void convert_yv12_to_yuy2_interlaced_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height) {
  //first four lines
  copy_yv12_line_to_yuy2_c(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_c(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_c(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; ++x) {
      dstp[x*4] = srcY[x*2];
      dstp[x*4+2] = srcY[x*2+1];

      dstp[x*4+1] = ((((srcU[x-src_pitch_uv*2] + srcU[x] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3] = ((((srcV[x-src_pitch_uv*2] + srcV[x] + 1) / 2) + srcV[x]) / 2);

      dstp[x*4 + dst_pitch*2] = srcY[x*2 + src_pitch_y*2];
      dstp[x*4+2 + dst_pitch*2] = srcY[x*2+1 + src_pitch_y*2];

      dstp[x*4+1 + dst_pitch*2] = ((((srcU[x] + srcU[x+src_pitch_uv*2] + 1) / 2) + srcU[x]) / 2);
      dstp[x*4+3 + dst_pitch*2] = ((((srcV[x] + srcV[x+src_pitch_uv*2] + 1) / 2) + srcV[x]) / 2);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}


#ifdef X86_32

#pragma warning(push)
#pragma warning(disable: 4799)
//75% of the first argument and 25% of the second one.
static __forceinline __m64 convert_yv12_to_yuy2_merge_chroma_isse(const __m64 &line75p, const __m64 &line25p, const __m64 &one) {
  __m64 avg_chroma_lo = _mm_avg_pu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_pu8(avg_chroma_lo, one);
  return _mm_avg_pu8(avg_chroma_lo, line75p);
}

// first parameter is 8 luma pixels
// second and third - 4 chroma bytes in low dwords
// last two params are OUT
static __forceinline void convert_yv12_pixels_to_yuy2_isse(const __m64 &y, const __m64 &u, const __m64 &v,  const __m64 &zero, __m64 &out_low, __m64 &out_high) {
  __m64 chroma = _mm_unpacklo_pi8(u, v);
  out_low = _mm_unpacklo_pi8(y, chroma);
  out_high = _mm_unpackhi_pi8(y, chroma);
}

static inline void copy_yv12_line_to_yuy2_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  __m64 zero = _mm_setzero_si64();
  for (int x = 0; x < width / 2; x+=4) {
    __m64 src_y = *reinterpret_cast<const __m64*>(srcY+x*2); //Y Y Y Y Y Y Y Y
    __m64 src_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU+x)); //0 0 0 0 U U U U
    __m64 src_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV+x)); //0 0 0 0 V V V V

    __m64 dst_lo, dst_hi;
    convert_yv12_pixels_to_yuy2_isse(src_y, src_u, src_v, zero, dst_lo, dst_hi);

    *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
    *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;
  }
}
#pragma warning(pop)

void convert_yv12_to_yuy2_interlaced_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first four lines
  copy_yv12_line_to_yuy2_isse(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_isse(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  __m64 one = _mm_set1_pi8(1);
  __m64 zero = _mm_setzero_si64();

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; x+=4) {

      __m64 luma_line = *reinterpret_cast<const __m64*>(srcY + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_current_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + x)); //0 0 0 0 U U U U
      __m64 src_current_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + x)); //0 0 0 0 V V V V
      __m64 src_prev_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU - src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m64 src_prev_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV - src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      __m64 src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_prev_u, one);
      __m64 src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_prev_v, one);

      __m64 dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;

      luma_line = *reinterpret_cast<const __m64*>(srcY + src_pitch_y *2+ x*2); //Y Y Y Y Y Y Y Y
      __m64 src_next_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m64 src_next_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + dst_pitch*2 + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + dst_pitch*2 + x*4 + 8) = dst_hi;
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
  _mm_empty();
}

void convert_yv12_to_yuy2_progressive_isse(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first two lines
  copy_yv12_line_to_yuy2_isse(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_isse(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_isse(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  __m64 one = _mm_set1_pi8(1);
  __m64 zero = _mm_setzero_si64();

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; x+=4) {
      __m64 luma_line = *reinterpret_cast<const __m64*>(srcY + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_current_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + x)); //0 0 0 0 U U U U
      __m64 src_current_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + x)); //0 0 0 0 V V V V
      __m64 src_prev_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU - src_pitch_uv + x)); //0 0 0 0 U U U U
      __m64 src_prev_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV - src_pitch_uv + x)); //0 0 0 0 V V V V

      __m64 src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_prev_u, one);
      __m64 src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_prev_v, one);

      __m64 dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + x*4 + 8) = dst_hi;

      luma_line = *reinterpret_cast<const __m64*>(srcY + src_pitch_y + x*2); //Y Y Y Y Y Y Y Y
      __m64 src_next_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU + src_pitch_uv + x)); //0 0 0 0 U U U U
      __m64 src_next_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV + src_pitch_uv + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_isse(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_isse(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_isse(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      *reinterpret_cast<__m64*>(dstp + dst_pitch + x*4) = dst_lo;
      *reinterpret_cast<__m64*>(dstp + dst_pitch + x*4 + 8) = dst_hi;
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
  _mm_empty();
}
#endif

#ifdef __SSE2__
//75% of the first argument and 25% of the second one.
static __forceinline __m128i convert_yv12_to_yuy2_merge_chroma_sse2(const __m128i &line75p, const __m128i &line25p, const __m128i &one) {
  __m128i avg_chroma_lo = _mm_avg_epu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_epu8(avg_chroma_lo, one);
  return _mm_avg_epu8(avg_chroma_lo, line75p);
}

// first parameter is 16 luma pixels
// second and third - 8 chroma bytes in low dwords
// last two params are OUT
static __forceinline void convert_yv12_pixels_to_yuy2_sse2(const __m128i &y, const __m128i &u, const __m128i &v,  const __m128i &zero, __m128i &out_low, __m128i &out_high) {
  AVS_UNUSED(zero);
  __m128i chroma = _mm_unpacklo_epi8(u, v); //...V3 U3 V2 U2 V1 U1 V0 U0
  out_low = _mm_unpacklo_epi8(y, chroma);
  out_high = _mm_unpackhi_epi8(y, chroma);
}

static inline void copy_yv12_line_to_yuy2_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int width) {
  __m128i zero = _mm_setzero_si128();
  for (int x = 0; x < width / 2; x+=8) {
    __m128i src_y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY+x*2)); //Y Y Y Y Y Y Y Y
    __m128i src_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU+x)); //0 0 0 0 U U U U
    __m128i src_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV+x)); //0 0 0 0 V V V V

    __m128i dst_lo, dst_hi;
    convert_yv12_pixels_to_yuy2_sse2(src_y, src_u, src_v, zero, dst_lo, dst_hi);

    _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
    _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);
  }
}

void convert_yv12_to_yuy2_interlaced_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first four lines
  copy_yv12_line_to_yuy2_sse2(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y*2, srcU, srcV, dstp + dst_pitch*2, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY + src_pitch_y*3, srcU + src_pitch_uv, srcV + src_pitch_uv, dstp + dst_pitch*3, src_width);

  //last four lines. Easier to do them here
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-4),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-4),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-2),
    srcV + src_pitch_uv * ((height/2)-2),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-3),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-3),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y * 4;
  srcU += src_pitch_uv * 2;
  srcV += src_pitch_uv * 2;
  dstp += dst_pitch * 4;

  __m128i one = _mm_set1_epi8(1);
  __m128i zero = _mm_setzero_si128();

  for (int y = 4; y < height-4; y+= 2) {
    for (int x = 0; x < src_width / 2; x+=8) {

      __m128i luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_current_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + x)); //0 0 0 0 U U U U
      __m128i src_current_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + x)); //0 0 0 0 V V V V
      __m128i src_prev_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU - src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m128i src_prev_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV - src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      __m128i src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_prev_u, one);
      __m128i src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_prev_v, one);

      __m128i dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);

      luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + src_pitch_y*2+ x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_next_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + src_pitch_uv*2 + x)); //0 0 0 0 U U U U
      __m128i src_next_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + src_pitch_uv*2 + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch*2 + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch*2 + x*4 + 16), dst_hi);
    }

    if (y % 4 == 0) {
      //top field processed, jumb to the bottom
      srcY += src_pitch_y;
      dstp += dst_pitch;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    } else {
      //bottom field processed, jump to the next top
      srcY += src_pitch_y*3;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      dstp += dst_pitch*3;
    }
  }
}

void convert_yv12_to_yuy2_progressive_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, int src_width, int src_pitch_y, int src_pitch_uv, BYTE *dstp, int dst_pitch, int height)
{
  //first two lines
  copy_yv12_line_to_yuy2_sse2(srcY, srcU, srcV, dstp, src_width);
  copy_yv12_line_to_yuy2_sse2(srcY+src_pitch_y, srcU, srcV, dstp+dst_pitch, src_width);

  //last two lines. Easier to do them here
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-2),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-2),
    src_width
    );
  copy_yv12_line_to_yuy2_sse2(
    srcY + src_pitch_y * (height-1),
    srcU + src_pitch_uv * ((height/2)-1),
    srcV + src_pitch_uv * ((height/2)-1),
    dstp + dst_pitch * (height-1),
    src_width
    );

  srcY += src_pitch_y*2;
  srcU += src_pitch_uv;
  srcV += src_pitch_uv;
  dstp += dst_pitch*2;

  __m128i one = _mm_set1_epi8(1);
  __m128i zero = _mm_setzero_si128();

  for (int y = 2; y < height-2; y+=2) {
    for (int x = 0; x < src_width / 2; x+=8) {
      __m128i luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_current_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + x)); //0 0 0 0 U U U U
      __m128i src_current_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + x)); //0 0 0 0 V V V V
      __m128i src_prev_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU - src_pitch_uv + x)); //0 0 0 0 U U U U
      __m128i src_prev_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV - src_pitch_uv + x)); //0 0 0 0 V V V V

      __m128i src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_prev_u, one);
      __m128i src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_prev_v, one);

      __m128i dst_lo, dst_hi;
      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), dst_hi);

      luma_line = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + src_pitch_y + x*2)); //Y Y Y Y Y Y Y Y
      __m128i src_next_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU + src_pitch_uv + x)); //0 0 0 0 U U U U
      __m128i src_next_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV + src_pitch_uv + x)); //0 0 0 0 V V V V

      src_u = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_u, src_next_u, one);
      src_v = convert_yv12_to_yuy2_merge_chroma_sse2(src_current_v, src_next_v, one);

      convert_yv12_pixels_to_yuy2_sse2(luma_line, src_u, src_v, zero, dst_lo, dst_hi);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch + x*4), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp + dst_pitch + x*4 + 16), dst_hi);
    }
    srcY += src_pitch_y*2;
    dstp += dst_pitch*2;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
}
#endif

/* YUY2 -> YV12 conversion */


void convert_yuy2_to_yv12_progressive_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  //src_width is twice the luma width of yv12 frame
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }


  for (int y = 0; y < height / 2; ++y) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = (src[x*4+1] + src[x*4+1+src_pitch] + 1) / 2;
      dstV[x] = (src[x*4+3] + src[x*4+3+src_pitch] + 1) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch * 2;
  }
}

void convert_yuy2_to_yv12_interlaced_c(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  const BYTE* srcp = src;
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < src_width / 2 ; ++x) {
      dstY[x] = srcp[x*2];
    }
    dstY += dst_pitchY;
    srcp += src_pitch;
  }

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; ++x) {
      dstU[x] = ((src[x*4+1] + src[x*4+1+src_pitch*2] + 1) / 2 + src[x*4+1+src_pitch*2]) / 2;
      dstV[x] = ((src[x*4+3] + src[x*4+3+src_pitch*2] + 1) / 2 + src[x*4+3+src_pitch*2]) / 2;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    src += src_pitch*3;
  }
}

#ifdef X86_32

void convert_yuy2_to_yv12_progressive_isse(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height)
{
  __m64 luma_mask = _mm_set1_pi16(0x00FF);
  for (int y = 0; y < height/2; ++y) {
    for (int x = 0; x < (src_width+3) / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch+8);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY) = src_luma_line1;

      __m64 avg_chroma_lo = _mm_avg_pu8(src_lo_line0, src_lo_line1);
      __m64 avg_chroma_hi = _mm_avg_pu8(src_hi_line0, src_hi_line1);

      __m64 chroma_lo = _mm_srli_si64(avg_chroma_lo, 8);
      __m64 chroma_hi = _mm_srli_si64(avg_chroma_hi, 8);

      chroma_lo = _mm_and_si64(luma_mask, chroma_lo); //0V0U 0V0U
      chroma_hi = _mm_and_si64(luma_mask, chroma_hi); //0V0U 0V0U

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V

      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);
    }

    src += src_pitch*2;
    dstY += dst_pitchY * 2;
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
  }
  _mm_empty();
}

//75% of the first argument and 25% of the second one.
static __forceinline __m64 convert_yuy2_to_yv12_merge_chroma_isse(const __m64 &line75p, const __m64 &line25p, const __m64 &one, const __m64 &luma_mask) {
  __m64 avg_chroma_lo = _mm_avg_pu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_pu8(avg_chroma_lo, one);
  avg_chroma_lo = _mm_avg_pu8(avg_chroma_lo, line75p);
  __m64 chroma_lo = _mm_srli_si64(avg_chroma_lo, 8);
  return _mm_and_si64(luma_mask, chroma_lo); //0V0U 0V0U
}

void convert_yuy2_to_yv12_interlaced_isse(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  __m64 one = _mm_set1_pi8(1);
  __m64 luma_mask = _mm_set1_pi16(0x00FF);

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2+8);

      __m64 chroma_lo = convert_yuy2_to_yv12_merge_chroma_isse(src_lo_line0, src_lo_line1, one, luma_mask);
      __m64 chroma_hi = convert_yuy2_to_yv12_merge_chroma_isse(src_hi_line0, src_hi_line1, one, luma_mask);

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V

      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY*2) = src_luma_line1;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; x+=4) {
      __m64 src_lo_line0 = *reinterpret_cast<const __m64*>(src+x*4); //VYUY VYUY
      __m64 src_lo_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2);

      __m64 src_hi_line0 = *reinterpret_cast<const __m64*>(src+x*4+8);
      __m64 src_hi_line1 = *reinterpret_cast<const __m64*>(src+x*4+src_pitch*2+8);

      __m64 chroma_lo = convert_yuy2_to_yv12_merge_chroma_isse(src_lo_line1, src_lo_line0, one, luma_mask);
      __m64 chroma_hi = convert_yuy2_to_yv12_merge_chroma_isse(src_hi_line1, src_hi_line0, one, luma_mask);

      __m64 chroma = _mm_packs_pu16(chroma_lo, chroma_hi); //VUVU VUVU

      __m64 chroma_u = _mm_and_si64(luma_mask, chroma); //0U0U 0U0U
      __m64 chroma_v = _mm_andnot_si64(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si64(chroma_v, 8); //0V0V 0V0V

      chroma_u = _mm_packs_pu16(chroma_u, luma_mask);
      chroma_v = _mm_packs_pu16(chroma_v, luma_mask);

      *reinterpret_cast<int*>(dstU+x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstV+x) = _mm_cvtsi64_si32(chroma_v);

      __m64 src_lo_line0_luma = _mm_and_si64(src_lo_line0, luma_mask);
      __m64 src_lo_line1_luma = _mm_and_si64(src_lo_line1, luma_mask);
      __m64 src_hi_line0_luma = _mm_and_si64(src_hi_line0, luma_mask);
      __m64 src_hi_line1_luma = _mm_and_si64(src_hi_line1, luma_mask);

      __m64 src_luma_line0 = _mm_packs_pu16(src_lo_line0_luma, src_hi_line0_luma);
      __m64 src_luma_line1 = _mm_packs_pu16(src_lo_line1_luma, src_hi_line1_luma);

      *reinterpret_cast<__m64*>(dstY + x*2) = src_luma_line0;
      *reinterpret_cast<__m64*>(dstY + x*2 + dst_pitchY*2) = src_luma_line1;
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY*3;
    src += src_pitch*3;
  }
  _mm_empty();
}

#endif

void convert_yuy2_to_yv12_progressive_sse2(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height)
{
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
  for (int y = 0; y < height/2; ++y) {
    for (int x = 0; x < (src_width+3) / 4; x+=8) {
      __m128i src_lo_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4)); //VYUY VYUY
      __m128i src_lo_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch));

      __m128i src_hi_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+16));
      __m128i src_hi_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch+16));

      __m128i src_lo_line0_luma = _mm_and_si128(src_lo_line0, luma_mask);
      __m128i src_lo_line1_luma = _mm_and_si128(src_lo_line1, luma_mask);
      __m128i src_hi_line0_luma = _mm_and_si128(src_hi_line0, luma_mask);
      __m128i src_hi_line1_luma = _mm_and_si128(src_hi_line1, luma_mask);

      __m128i src_luma_line0 = _mm_packus_epi16(src_lo_line0_luma, src_hi_line0_luma);
      __m128i src_luma_line1 = _mm_packus_epi16(src_lo_line1_luma, src_hi_line1_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2), src_luma_line0);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2 + dst_pitchY), src_luma_line1);

      __m128i avg_chroma_lo = _mm_avg_epu8(src_lo_line0, src_lo_line1);
      __m128i avg_chroma_hi = _mm_avg_epu8(src_hi_line0, src_hi_line1);

      __m128i chroma_lo = _mm_srli_si128(avg_chroma_lo, 1);
      __m128i chroma_hi = _mm_srli_si128(avg_chroma_hi, 1);

      chroma_lo = _mm_and_si128(luma_mask, chroma_lo); //0V0U 0V0U
      chroma_hi = _mm_and_si128(luma_mask, chroma_hi); //0V0U 0V0U

      __m128i chroma = _mm_packus_epi16(chroma_lo, chroma_hi); //VUVU VUVU

      __m128i chroma_u = _mm_and_si128(luma_mask, chroma); //0U0U 0U0U
      __m128i chroma_v = _mm_andnot_si128(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si128(chroma_v, 1); //0V0V 0V0V

      chroma_u = _mm_packus_epi16(chroma_u, luma_mask);
      chroma_v = _mm_packus_epi16(chroma_v, luma_mask);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), chroma_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), chroma_v);
    }

    src += src_pitch*2;
    dstY += dst_pitchY * 2;
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
  }
}

//75% of the first argument and 25% of the second one.
static __forceinline __m128i convert_yuy2_to_yv12_merge_chroma_sse2(const __m128i &line75p, const __m128i &line25p, const __m128i &one, const __m128i &luma_mask) {
  __m128i avg_chroma_lo = _mm_avg_epu8(line75p, line25p);
  avg_chroma_lo = _mm_subs_epu8(avg_chroma_lo, one);
  avg_chroma_lo = _mm_avg_epu8(avg_chroma_lo, line75p);
  __m128i chroma_lo = _mm_srli_si128(avg_chroma_lo, 1);
  return _mm_and_si128(luma_mask, chroma_lo); //0V0U 0V0U
}

void convert_yuy2_to_yv12_interlaced_sse2(const BYTE* src, int src_width, int src_pitch, BYTE* dstY, BYTE* dstU, BYTE* dstV, int dst_pitchY, int dst_pitchUV, int height) {
  __m128i one = _mm_set1_epi8(1);
  __m128i luma_mask = _mm_set1_epi16(0x00FF);

  for (int y = 0; y < height / 2; y+=2) {
    for (int x = 0; x < src_width / 4; x+=8) {
      __m128i src_lo_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4)); //VYUY VYUY
      __m128i src_lo_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch*2));

      __m128i src_hi_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+16));
      __m128i src_hi_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch*2+16));

      __m128i chroma_lo = convert_yuy2_to_yv12_merge_chroma_sse2(src_lo_line0, src_lo_line1, one, luma_mask);
      __m128i chroma_hi = convert_yuy2_to_yv12_merge_chroma_sse2(src_hi_line0, src_hi_line1, one, luma_mask);

      __m128i chroma = _mm_packus_epi16(chroma_lo, chroma_hi); //VUVU VUVU

      __m128i chroma_u = _mm_and_si128(luma_mask, chroma); //0U0U 0U0U
      __m128i chroma_v = _mm_andnot_si128(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si128(chroma_v, 1); //0V0V 0V0V

      chroma_u = _mm_packus_epi16(chroma_u, luma_mask);
      chroma_v = _mm_packus_epi16(chroma_v, luma_mask);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), chroma_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), chroma_v);

      __m128i src_lo_line0_luma = _mm_and_si128(src_lo_line0, luma_mask);
      __m128i src_lo_line1_luma = _mm_and_si128(src_lo_line1, luma_mask);
      __m128i src_hi_line0_luma = _mm_and_si128(src_hi_line0, luma_mask);
      __m128i src_hi_line1_luma = _mm_and_si128(src_hi_line1, luma_mask);

      __m128i src_luma_line0 = _mm_packus_epi16(src_lo_line0_luma, src_hi_line0_luma);
      __m128i src_luma_line1 = _mm_packus_epi16(src_lo_line1_luma, src_hi_line1_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2), src_luma_line0);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2 + dst_pitchY*2), src_luma_line1);
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY;
    src += src_pitch;

    for (int x = 0; x < src_width / 4; x+=8) {
      __m128i src_lo_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4)); //VYUY VYUY
      __m128i src_lo_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch*2));

      __m128i src_hi_line0 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+16));
      __m128i src_hi_line1 = _mm_load_si128(reinterpret_cast<const __m128i*>(src+x*4+src_pitch*2+16));

      __m128i chroma_lo = convert_yuy2_to_yv12_merge_chroma_sse2(src_lo_line1, src_lo_line0, one, luma_mask);
      __m128i chroma_hi = convert_yuy2_to_yv12_merge_chroma_sse2(src_hi_line1, src_hi_line0, one, luma_mask);

      __m128i chroma = _mm_packus_epi16(chroma_lo, chroma_hi); //VUVU VUVU

      __m128i chroma_u = _mm_and_si128(luma_mask, chroma); //0U0U 0U0U
      __m128i chroma_v = _mm_andnot_si128(luma_mask, chroma); //V0V0 V0V0
      chroma_v = _mm_srli_si128(chroma_v, 1); //0V0V 0V0V

      chroma_u = _mm_packus_epi16(chroma_u, luma_mask);
      chroma_v = _mm_packus_epi16(chroma_v, luma_mask);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), chroma_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), chroma_v);

      __m128i src_lo_line0_luma = _mm_and_si128(src_lo_line0, luma_mask);
      __m128i src_lo_line1_luma = _mm_and_si128(src_lo_line1, luma_mask);
      __m128i src_hi_line0_luma = _mm_and_si128(src_hi_line0, luma_mask);
      __m128i src_hi_line1_luma = _mm_and_si128(src_hi_line1, luma_mask);

      __m128i src_luma_line0 = _mm_packus_epi16(src_lo_line0_luma, src_hi_line0_luma);
      __m128i src_luma_line1 = _mm_packus_epi16(src_lo_line1_luma, src_hi_line1_luma);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2), src_luma_line0);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstY + x*2 + dst_pitchY*2), src_luma_line1);
    }
    dstU += dst_pitchUV;
    dstV += dst_pitchUV;
    dstY += dst_pitchY*3;
    src += src_pitch*3;
  }
}
