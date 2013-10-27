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

// ConvertPlanar (c) 2005 by Klaus Post


#include "convert_planar.h"
#include "../filters/planeswap.h"
#include "../filters/field.h"
#include <malloc.h>
#include <avs/win.h>
#include <avs/minmax.h>
#include "../core/internal.h"
#include <emmintrin.h>
#include <avs/alignment.h>


ConvertToY8::ConvertToY8(PClip src, int in_matrix, IScriptEnvironment* env) : GenericVideoFilter(src), matrix(NULL) {
  yuy2_input = blit_luma_only = rgb_input = false;

  if (vi.IsPlanar()) {
    blit_luma_only = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsYUY2()) {
    yuy2_input = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsRGB()) {
    rgb_input = true;
    pixel_step = vi.BytesFromPixels(1);
    vi.pixel_type = VideoInfo::CS_Y8;
    matrix = (signed short*)_aligned_malloc(sizeof(short)*4, 16);
    signed short* m = matrix;
    if (in_matrix == Rec601) {
      *m++ = (signed short)((219.0/255.0)*0.114*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.587*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.299*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_601) {
      *m++ = (signed short)(0.114*32768.0+0.5);  //B
      *m++ = (signed short)(0.587*32768.0+0.5);  //G
      *m++ = (signed short)(0.299*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == Rec709) {
      *m++ = (signed short)((219.0/255.0)*0.0722*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.7152*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.2126*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_709) {
      *m++ = (signed short)(0.0722*32768.0+0.5);  //B
      *m++ = (signed short)(0.7152*32768.0+0.5);  //G
      *m++ = (signed short)(0.2126*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == AVERAGE) {
      *m++ = (signed short)(32768.0/3 + 0.5);  //B
      *m++ = (signed short)(32768.0/3 + 0.5);  //G
      *m++ = (signed short)(32768.0/3 + 0.5);  //R
      offset_y = 0;
    } else {
      _aligned_free(matrix);
      matrix = 0;
      env->ThrowError("ConvertToY8: Unknown matrix.");
    }
    *m = 0;  // Alpha

    return;
  }

  env->ThrowError("ConvertToY8: Unknown input format");
}

ConvertToY8::~ConvertToY8() {
  _aligned_free(matrix);
  matrix = NULL;
}


//This is faster than mmx only sometimes. But will work on x64
static void convert_yuy2_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  size_t mod16_width = (width / 16) * 16;
  __m128i luma_mask = _mm_set1_epi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i src1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2));
      __m128i src2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+16));
      src1 = _mm_and_si128(src1, luma_mask);
      src2 = _mm_and_si128(src2, luma_mask);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), _mm_packus_epi16(src1, src2));
    }

    if (width != mod16_width) {
      __m128i src1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*2-32));
      __m128i src2 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*2-16));
      src1 = _mm_and_si128(src1, luma_mask);
      src2 = _mm_and_si128(src2, luma_mask);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+width-16), _mm_packus_epi16(src1, src2));
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

#ifdef X86_32
static void convert_yuy2_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  size_t mod8_width = (width / 8) * 8;
  __m64 luma_mask = _mm_set1_pi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 src1 = *reinterpret_cast<const __m64*>(srcp+x*2);
      __m64 src2 = *reinterpret_cast<const __m64*>(srcp+x*2+8);
      src1 = _mm_and_si64(src1, luma_mask);
      src2 = _mm_and_si64(src2, luma_mask);
      *reinterpret_cast<__m64*>(dstp+x) = _mm_packs_pu16(src1, src2);
    }

    if (width != mod8_width) {
        __m64 src1 = *reinterpret_cast<const __m64*>(srcp+width*2-16);
      __m64 src2 = *reinterpret_cast<const __m64*>(srcp+width*2-8);
      src1 = _mm_and_si64(src1, luma_mask);
      src2 = _mm_and_si64(src2, luma_mask);
      *reinterpret_cast<__m64*>(dstp+width-8) = _mm_packs_pu16(src1, src2);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm_empty();
}
#endif


#pragma warning(disable: 4799)
static __forceinline __m128i convert_rgb_to_y8_sse2_core(__m128i &pixel01, __m128i &pixel23, __m128i &pixel45, __m128i &pixel67, __m128i& zero, __m128i &matrix, __m128i &round_mask, __m128i &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  // in general the algorithm is identical to MMX version, the only different part is getting r and g+b in appropriate registers. We use shuffling instead of unpacking here.
  pixel01 = _mm_madd_epi16(pixel01, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb | a0*0 + r0*cyr | g0*cyg + b0*cyb
  pixel23 = _mm_madd_epi16(pixel23, matrix); //a3*0 + r1*cyr | g3*cyg + b3*cyb | a2*0 + r2*cyr | g2*cyg + b2*cyb
  pixel45 = _mm_madd_epi16(pixel45, matrix); //a5*0 + r1*cyr | g5*cyg + b5*cyb | a4*0 + r4*cyr | g4*cyg + b4*cyb
  pixel67 = _mm_madd_epi16(pixel67, matrix); //a7*0 + r1*cyr | g7*cyg + b7*cyb | a6*0 + r6*cyr | g6*cyg + b6*cyb

  __m128i pixel_0123_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01), _mm_castsi128_ps(pixel23), _MM_SHUFFLE(3, 1, 3, 1))); // r3*cyr | r2*cyr | r1*cyr | r0*cyr
  __m128i pixel_4567_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45), _mm_castsi128_ps(pixel67), _MM_SHUFFLE(3, 1, 3, 1))); // r7*cyr | r6*cyr | r5*cyr | r4*cyr

  __m128i pixel_0123 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01), _mm_castsi128_ps(pixel23), _MM_SHUFFLE(2, 0, 2, 0)));
  __m128i pixel_4567 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45), _mm_castsi128_ps(pixel67), _MM_SHUFFLE(2, 0, 2, 0)));

  pixel_0123 = _mm_add_epi32(pixel_0123, pixel_0123_r); 
  pixel_4567 = _mm_add_epi32(pixel_4567, pixel_4567_r); 

  pixel_0123 = _mm_add_epi32(pixel_0123, round_mask);
  pixel_4567 = _mm_add_epi32(pixel_4567, round_mask);

  pixel_0123 = _mm_srai_epi32(pixel_0123, 15); 
  pixel_4567 = _mm_srai_epi32(pixel_4567, 15); 

  __m128i result = _mm_packs_epi32(pixel_0123, pixel_4567);

  result = _mm_packus_epi16(result, zero); 
  result = _mm_adds_epu8(result, offset);

  return result;
}
#pragma warning(default: 4799)

static void convert_rgb32_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi8(offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  size_t loop_limit;
  bool not_mod8 = false;
  //todo: simplify
  if (width % 8 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 8 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 8 * 8;
    not_mod8 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    if (not_mod8) {
      __m128i src0123 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-32)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width*4-16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width-8), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


static void convert_rgb24_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi8(offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  size_t loop_limit;
  bool not_mod8 = false;
  //todo: simplify
  if (width % 8 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 8 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 8 * 8;
    not_mod8 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+18)); //pixels 6 and 7

      
      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    if (not_mod8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-24)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-18)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-6)); //pixels 6 and 7

      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width-8), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


#ifdef X86_32

#pragma warning(disable: 4799)
static __forceinline int convert_rgb_to_y8_mmx_core(__m64 &pixel0, __m64 &pixel1, __m64 &pixel2, __m64 &pixel3, __m64& zero, __m64 &matrix, __m64 &round_mask, __m64 &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  
  pixel0 = _mm_madd_pi16(pixel0, matrix); //a0*0 + r0*cyr | g0*cyg + b0*cyb
  pixel1 = _mm_madd_pi16(pixel1, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb
  pixel2 = _mm_madd_pi16(pixel2, matrix); //a2*0 + r2*cyr | g2*cyg + b2*cyb
  pixel3 = _mm_madd_pi16(pixel3, matrix); //a3*0 + r3*cyr | g3*cyg + b3*cyb

  __m64 pixel_01_r = _mm_unpackhi_pi32(pixel0, pixel1); // r1*cyr | r0*cyr
  __m64 pixel_23_r = _mm_unpackhi_pi32(pixel2, pixel3); // r3*cyr | r2*cyr

  __m64 pixel_01 = _mm_unpacklo_pi32(pixel0, pixel1); //g1*cyg + b1*cyb | g0*cyg + b0*cyb
  __m64 pixel_23 = _mm_unpacklo_pi32(pixel2, pixel3); //g3*cyg + b3*cyb | g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, pixel_01_r); // r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  pixel_23 = _mm_add_pi32(pixel_23, pixel_23_r); // r3*cyr + g3*cyg + b3*cyb | r2*cyr + g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, round_mask); //r1*cyr + g1*cyg + b1*cyb + 16384 | r0*cyr + g0*cyg + b0*cyb + 16384
  pixel_23 = _mm_add_pi32(pixel_23, round_mask); //r3*cyr + g3*cyg + b3*cyb + 16384 | r2*cyr + g2*cyg + b2*cyb + 16384

  pixel_01 = _mm_srai_pi32(pixel_01, 15); //0 | p1 | 0 | p0
  pixel_23 = _mm_srai_pi32(pixel_23, 15); //0 | p3 | 0 | p2

  __m64 result = _mm_packs_pi32(pixel_01, pixel_23); //p3 | p2 | p1 | p0

  result = _mm_packs_pu16(result, zero); //0 0 0 0 p3 p2 p1 p0
  result = _mm_adds_pu8(result, offset);

  return _mm_cvtsi64_si32(result);
}
#pragma warning(default: 4799)

static void convert_rgb32_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi8(offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  size_t loop_limit;
  bool not_mod4 = false;
  //todo: simplify
  if (width % 4 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 4 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 4 * 4;
    not_mod4 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //pixels 0 and 1
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8);//pixels 2 and 3

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    if (not_mod4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+width*4-16);
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+width*4-8);

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstp+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
  _mm_empty();
}


static void convert_rgb24_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, BYTE offset_y, short cyr, short cyg, short cyb ) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi8(offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  size_t loop_limit;
  bool not_mod4 = false;
  //todo: simplify
  if (width % 4 == 0) {
    loop_limit = width;
  } else if (dst_pitch % 4 == 0) {
    loop_limit = dst_pitch; //we'll just write some garbage
  } else {
    loop_limit = width / 4 * 4;
    not_mod4 = true;
  }

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+3)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+9)); //pixel 3
      
      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    if (not_mod4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-12)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-9)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-3)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstp+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix, round_mask, offset);
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
  _mm_empty();
}

#endif // X86_32


PVideoFrame __stdcall ConvertToY8::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (blit_luma_only) {
    // Abuse Subframe to snatch the Y plane
    return env->Subframe(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (yuy2_input) {

    const BYTE* srcP = src->GetReadPtr();
    const int srcPitch = src->GetPitch();
    int width = dst->GetRowSize(PLANAR_Y);
    int height = dst->GetHeight(PLANAR_Y);

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);
    

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcP, 16)) {
      convert_yuy2_to_y8_sse2(srcP, dstY, srcPitch, dstPitch, width, height);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      convert_yuy2_to_y8_mmx(srcP, dstY, srcPitch, dstPitch, width, height);
    } else
#endif 
    {
      for (int y=0; y<height; y++) {
        for (int x=0; x<width; x++) {
          dstY[x] = srcP[x*2];
        }
        srcP+=srcPitch;
        dstY+=dstPitch;
      }
    }
    return dst;
  }

  if (rgb_input) {
    const int src_pitch = src->GetPitch();
    const BYTE* srcp = src->GetReadPtr() + src_pitch * (vi.height-1);  // We start at last line

    BYTE* dstp = dst->GetWritePtr(PLANAR_Y);
    const int dst_pitch = dst->GetPitch(PLANAR_Y);

    const int m0 = matrix[0];
    const int m1 = matrix[1];
    const int m2 = matrix[2];

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } else if (pixel_step == 3) {
        convert_rgb24_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      }
    }

#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } else if (pixel_step == 3) {
        convert_rgb24_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, offset_y, m2, m1, m0);
        return dst;
      } 
    }
#endif

    const int srcMod = src_pitch + (vi.width * pixel_step);
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<vi.width; x++) {
        const int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
        dstp[x] = PixelClip(Y);  // All the safety we can wish for.
        srcp += pixel_step;
      }
      srcp -= srcMod;
      dstp += dst_pitch;
    }
  }
  return dst;
}

AVSValue __cdecl ConvertToY8::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsY8())
    return clip;
  return new ConvertToY8(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertRGBToYV24
 *
 * (c) Klaus Post, 2005
 ******************************************************/

// XMM_WORD ready
__declspec(align(16)) static const __int64 Post_Add00[2] = { 0x008000800000, 0x008000800000 };
__declspec(align(16)) static const __int64 Post_Add16[2] = { 0x008000800010, 0x008000800010 };


ConvertRGBToYV24::ConvertRGBToYV24(PClip src, int in_matrix, IScriptEnvironment* env)
  : GenericVideoFilter(src), matrix(0), unpckbuf(0)
{
  if (!vi.IsRGB())
    env->ThrowError("ConvertRGBToYV24: Only RGB data input accepted");

  pixel_step = vi.BytesFromPixels(1);
  vi.pixel_type = VideoInfo::CS_YV24;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);

  const int shift = 15;

  if (in_matrix == Rec601) {
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);
  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertRGBToYV24: Unknown matrix.");
  }

#ifdef X86_32
  // TODO: This is not thread safe
  unpckbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 32, 64);
  const __int64 *post_add = offset_y == 16 ? &Post_Add16[0] : &Post_Add00[0];
  this->GenerateAssembly(vi.width, shift, false, 0, post_add, pixel_step, 4, matrix, env);

  this->GenerateUnPacker(vi.width, env);
#endif
}

ConvertRGBToYV24::~ConvertRGBToYV24() {
  if (unpckbuf)
    _aligned_free(unpckbuf);
  unpckbuf = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

}


void ConvertRGBToYV24::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  R = r/Srgb                     // 0..1
  G = g/Srgb
  B = b*Srgb

  Y = Kr*R + Kg*G + Kb*B         // 0..1
  U = B - (Kr*R + Kg*G)/(1-Kb)   //-1..1
  V = R - (Kg*G + Kb*B)/(1-Kr)

  y = Y*Sy  + Oy                 // 16..235, 0..255
  u = U*Suv + Ouv                // 16..240, 1..255
  v = V*Suv + Ouv
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Sy  * Kb        * mulfac / Srgb + 0.5); //B
  *m++ = (signed short)(Sy  * Kg        * mulfac / Srgb + 0.5); //G
  *m++ = (signed short)(Sy  * Kr        * mulfac / Srgb + 0.5); //R
  *m++ = (signed short)(           -0.5 * mulfac             ); //Rounder, assumes target is -1, 0xffff
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kr/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)(Suv * Kb/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  offset_y = Oy;
}

PVideoFrame __stdcall ConvertRGBToYV24::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcp = src->GetReadPtr();

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  const int Spitch = src->GetPitch();

  const int Ypitch = dst->GetPitch(PLANAR_Y);
  const int UVpitch = dst->GetPitch(PLANAR_U);

  const int awidth = dst->GetRowSize(PLANAR_Y_ALIGNED);

#ifdef X86_32
  // TODO: 64bit version
  srcp += (vi.height-1)*Spitch;
//  BYTE* unpckbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 32, 64);

  if (awidth & 7) {  // Should never happend, as all planar formats should have mod16 pitch
    int* iunpckbuf = (int*)unpckbuf;
    for (int y = 0; y < vi.height; y++) {

      assembly.Call(srcp, unpckbuf);

      for (int x = 0; x < vi.width; x++) {
        const int p = iunpckbuf[x];
        dstY[x] = p&0xff;
        dstU[x] = (p>>8)&0xff;
        dstV[x] = (p>>16)&0xff;
      }

      srcp -= Spitch;

      dstY += Ypitch;
      dstU += UVpitch;
      dstV += UVpitch;
    }
  }
  else {
    for (int y = 0; y < vi.height; y++) {

      assembly.Call(srcp, unpckbuf);

      this->unpacker.Call(unpckbuf, dstY, dstU, dstV);

      srcp -= Spitch;

      dstY += Ypitch;
      dstU += UVpitch;
      dstV += UVpitch;
    }
  }
//  _aligned_free(unpckbuf);
  return dst;
#endif

  //Slow C-code.

  signed short* m = (signed short*)matrix;
  srcp += Spitch * (vi.height-1);  // We start at last line
  const int Sstep = Spitch + (vi.width * pixel_step);
  for (int y = 0; y < vi.height; y++) {
    for (int x = 0; x < vi.width; x++) {
      int b = srcp[0];
      int g = srcp[1];
      int r = srcp[2];
      int Y = offset_y + (((int)m[0] * b + (int)m[1] * g + (int)m[2] * r + 16384)>>15);
      int U = 128+(((int)m[4] * b + (int)m[5] * g + (int)m[6] * r + 16384)>>15);
      int V = 128+(((int)m[8] * b + (int)m[9] * g + (int)m[10] * r + 16384)>>15);
      *dstY++ = PixelClip(Y);  // All the safety we can wish for.
      *dstU++ = PixelClip(U);
      *dstV++ = PixelClip(V);
      srcp += pixel_step;
    }
    srcp -= Sstep;
    dstY += Ypitch - vi.width;
    dstU += UVpitch - vi.width;
    dstV += UVpitch - vi.width;
  }
  return dst;
}

AVSValue __cdecl ConvertRGBToYV24::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV24())
    return clip;
  return new ConvertRGBToYV24(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertYV24ToRGB
 *
 * (c) Klaus Post, 2005
 ******************************************************/

// XMM_WORD ready
__declspec(align(16)) static const __int64 Pre_Add00[2]  = { 0xff80ff800000, 0xff80ff800000 };
__declspec(align(16)) static const __int64 Pre_Add16[2]  = { 0xff80ff80fff0, 0xff80ff80fff0 };


ConvertYV24ToRGB::ConvertYV24ToRGB(PClip src, int in_matrix, int _pixel_step, IScriptEnvironment* env)
 : GenericVideoFilter(src), pixel_step(_pixel_step), matrix(0), packbuf(0)
{

  if (!vi.IsYV24())
    env->ThrowError("ConvertYV24ToRGB: Only YV24 data input accepted");

  vi.pixel_type = (pixel_step == 3) ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);
  const int shift = 13;

  if (in_matrix == Rec601) {
/*
    B'= Y' + 1.772*U' + 0.000*V'
    G'= Y' - 0.344*U' - 0.714*V'
    R'= Y' + 0.000*U' + 1.402*V'
*/
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);

  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
/*
    B'= Y' + 1.8558*Cb + 0.0000*Cr
    G'= Y' - 0.1870*Cb - 0.4678*Cr
    R'= Y' + 0.0000*Cb + 1.5750*Cr
*/
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertYV24ToRGB: Unknown matrix.");
  }
#ifdef X86_32
  // TODO: This is not thread safe!
  packbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 60, 64);
  const __int64 *pre_add = offset_y == -16 ? &Pre_Add16[0] : &Pre_Add00[0];
  this->GenerateAssembly(vi.width, shift, true, pre_add, 0, 4, pixel_step, matrix, env);

  this->GeneratePacker(vi.width, env);
#endif
}

ConvertYV24ToRGB::~ConvertYV24ToRGB() {
  if (packbuf)
    _aligned_free(packbuf);
  packbuf = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

}


void ConvertYV24ToRGB::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  Y =(y-Oy)  / Sy                         // 0..1
  U =(u-Ouv) / Suv                        //-1..1
  V =(v-Ouv) / Suv

  R = Y                  + V*(1-Kr)       // 0..1
  G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
  B = Y + U*(1-Kb)

  r = R*Srgb                              // 0..255
  g = G*Srgb
  b = B*Srgb
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5); //Y
  *m++ = (signed short)(Srgb * (1-Kb)       * mulfac / Suv + 0.5); //U
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5); //V
  *m++ = (signed short)(                0.5 * mulfac            ); //Rounder assumes target is +1, 0x0001
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * (Kb-1)*Kb/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (Kr-1)*Kr/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (1-Kr)       * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  offset_y = -Oy;
}

PVideoFrame __stdcall ConvertYV24ToRGB::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 8);


  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  BYTE* dstp = dst->GetWritePtr();

  int awidth = src->GetRowSize(PLANAR_Y_ALIGNED);

  const int Ypitch = src->GetPitch(PLANAR_Y);
  const int UVpitch = src->GetPitch(PLANAR_U);

  const int Dpitch = dst->GetPitch();

#ifdef X86_32
  // TODO: 64-bit version
  dstp += (vi.height-1)*Dpitch;
//  BYTE* packbuf = (BYTE*)_aligned_malloc(vi.width * 4 + 60, 64);

  if (awidth & 15) { // This should be very safe to assume to never happend
    int* ipackbuf = (int*)packbuf;

    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        ipackbuf[x] = srcY[x] | (srcU[x] << 8 ) | (srcV[x] << 16) | (1 << 24);
      }

      assembly.Call(packbuf, dstp);

      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
      dstp -= Dpitch;
    }
  }
  else {
    for (int y = 0; y < vi.height; y++) {

      this->packer.Call(srcY, srcU, srcV, packbuf);

      assembly.Call(packbuf, dstp);

      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
      dstp -= Dpitch;
    }
  }
//  _aligned_free(packbuf);
  return dst;
#endif

  //Slow C-code.

  signed short* m = (signed short*)matrix;
  dstp += Dpitch * (vi.height-1);  // We start at last line
  if (pixel_step == 4) {
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[x*4+0] = PixelClip(b);  // All the safety we can wish for.
        dstp[x*4+1] = PixelClip(g);  // Probably needed here.
        dstp[x*4+2] = PixelClip(r);
        dstp[x*4+3] = 255; // alpha
      }
      dstp -= Dpitch;
      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
    }
  } else {
    const int Dstep = Dpitch + (vi.width * pixel_step);
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[0] = PixelClip(b);  // All the safety we can wish for.
        dstp[1] = PixelClip(g);  // Probably needed here.
        dstp[2] = PixelClip(r);
        dstp += pixel_step;
      }
      dstp -= Dstep;
      srcY += Ypitch;
      srcU += UVpitch;
      srcV += UVpitch;
    }
  }
  return dst;
}

AVSValue __cdecl ConvertYV24ToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString(0), env), 4, env);
}

AVSValue __cdecl ConvertYV24ToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString(0), env), 3, env);
}

/************************************
 * YUY2 to YV16
 ************************************/

ConvertYUY2ToYV16::ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYUY2())
    env->ThrowError("ConvertYUY2ToYV16: Only YUY2 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YV16;

}

void convert_yuy2_to_yv16_sse2(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_u, size_t dst_pitch_v, size_t width, size_t height)
{
  __m128i low_byte_mask = _mm_set1_epi16(0x00FF);
  size_t half_width = width / 2;
  size_t mod8 = half_width / 8 * 8;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod8; x+=8) {
      __m128i p0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x*4));
      __m128i p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x*4 + 16));

      __m128i p0_luma = _mm_and_si128(p0, low_byte_mask);
      __m128i p1_luma = _mm_and_si128(p1, low_byte_mask); 
      __m128i luma = _mm_packus_epi16(p0_luma, p1_luma);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp_y + x*2), luma);

      __m128i p0_chroma = _mm_and_si128(_mm_srli_epi16(p0, 8), low_byte_mask); //00 V3 00 U3 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0
      __m128i p1_chroma = _mm_and_si128(_mm_srli_epi16(p1, 8), low_byte_mask); //00 V7 00 U7 00 V6 00 U6 00 V5 00 U5 00 V4 00 U4

      __m128i tmp_chroma = _mm_packus_epi16(p0_chroma, p1_chroma); //V U V U V U V U V U

      __m128i chroma_u16 = _mm_and_si128(tmp_chroma, low_byte_mask);
      __m128i chroma_v16 = _mm_srli_epi16(tmp_chroma, 8);

      __m128i chroma_u = _mm_packus_epi16(chroma_u16, chroma_u16);
      __m128i chroma_v = _mm_packus_epi16(chroma_v16, chroma_v16);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_u + x), chroma_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_v + x), chroma_v);
    }

    for (size_t x=mod8; x<half_width; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_u;
    dstp_v += dst_pitch_v;
  }
}


#ifdef X86_32

void convert_yuy2_to_yv16_mmx(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_u, size_t dst_pitch_v, size_t width, size_t height)
{
  __m64 low_byte_mask = _mm_set1_pi16(0x00FF);
  size_t half_width = width / 2;
  size_t mod4 = half_width / 4 * 4;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<mod4; x+=4) {
      __m64 p0 = *reinterpret_cast<const __m64*>(srcp + x*4);
      __m64 p1 = *reinterpret_cast<const __m64*>(srcp + x*4 + 8);

      __m64 p0_luma = _mm_and_si64(p0, low_byte_mask);
      __m64 p0_chroma = _mm_and_si64(_mm_srli_pi16(p0, 8), low_byte_mask); //0 V 0 U 0 V 0 U
      __m64 p1_luma = _mm_and_si64(p1, low_byte_mask); 
      __m64 p1_chroma = _mm_and_si64(_mm_srli_pi16(p1, 8), low_byte_mask);

      __m64 luma = _mm_packs_pu16(p0_luma, p1_luma);
      *reinterpret_cast<__m64*>(dstp_y + x*2) = luma;

      __m64 tmp_chroma = _mm_packs_pu16(p0_chroma, p1_chroma); //V U V U V U V U V U

      __m64 chroma_u16 = _mm_and_si64(tmp_chroma, low_byte_mask);
      __m64 chroma_v16 = _mm_srli_pi16(tmp_chroma, 8);

      __m64 chroma_u = _mm_packs_pu16(chroma_u16, chroma_u16);
      __m64 chroma_v = _mm_packs_pu16(chroma_v16, chroma_v16);

      
      *reinterpret_cast<int*>(dstp_u + x) = _mm_cvtsi64_si32(chroma_u);
      *reinterpret_cast<int*>(dstp_v + x) = _mm_cvtsi64_si32(chroma_v);
    }

    for (size_t x=mod4; x<half_width; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_u;
    dstp_v += dst_pitch_v;
  }
  _mm_empty();
}

#endif

void convert_yuy2_to_yv16_c(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_u, size_t dst_pitch_v, size_t width, size_t height)
{
  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<width/2; x++) {
      dstp_y[x*2]   = srcp[x*4+0];
      dstp_y[x*2+1] = srcp[x*4+2];
      dstp_u[x]     = srcp[x*4+1];
      dstp_v[x]     = srcp[x*4+3];
    }
    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_u;
    dstp_v += dst_pitch_v;
  }
}

PVideoFrame __stdcall ConvertYUY2ToYV16::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcP = src->GetReadPtr();
  const int awidth = min(src->GetPitch()>>1, (vi.width+7) & -8);

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcP, 16)) {
    convert_yuy2_to_yv16_sse2(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_V),  vi.width, vi.height);
  } else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) { 
    convert_yuy2_to_yv16_mmx(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_V),  vi.width, vi.height);
  } else
#endif
  {
    convert_yuy2_to_yv16_c(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_V),  vi.width, vi.height);
  }
  
  return dst;
}




AVSValue __cdecl ConvertYUY2ToYV16::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV16())
    return clip;
  return new ConvertYUY2ToYV16(clip, env);
}

/************************************
 * YV16 to YUY2
 ************************************/

ConvertYV16ToYUY2::ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYV16())
    env->ThrowError("ConvertYV16ToYUY2: Only YV16 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YUY2;

}

PVideoFrame __stdcall ConvertYV16ToYUY2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 16);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  const int awidth = min(src->GetPitch(PLANAR_Y), (vi.width+7) & -8);

  BYTE* dstp = dst->GetWritePtr();

#ifdef X86_32
  if (!(awidth&7)) {  // Use MMX
    this->conv422toYUV422(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
      dst->GetPitch(), awidth, vi.height);
  }
#endif

  const int w = vi.width/2;

  for (int y=0; y<vi.height; y++) { // ASM will probably not be faster here.
    for (int x=0; x<w; x++) {
      dstp[x*4+0] = srcY[x*2];
      dstp[x*4+1] = srcU[x];
      dstp[x*4+2] = srcY[x*2+1];
      dstp[x*4+3] = srcV[x];
    }
    srcY += src->GetPitch(PLANAR_Y);
    srcU += src->GetPitch(PLANAR_U);
    srcV += src->GetPitch(PLANAR_V);
    dstp += dst->GetPitch();
  }
  return dst;
}

#ifdef X86_32
void ConvertYV16ToYUY2::conv422toYUV422(const unsigned char *py, const unsigned char *pu, const unsigned char *pv,
                                        unsigned char *dst,
                                        int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	__asm
	{
        push ebx
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov ecx,width
		mov edi,[dst]
        shr ecx,1
yloop:
		xor eax,eax
		align 16
xloop:
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm0,mm1      ;VYUYVYUY
		add eax,4
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4-16],mm0 ;store
		cmp eax,ecx
		movq [edi+eax*4-8],mm3   ;store
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
        pop ebx
	}
}
#endif

AVSValue __cdecl ConvertYV16ToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  return new ConvertYV16ToYUY2(clip, env);
}

/**********************************************
 * Converter between arbitrary planar formats
 *
 * This uses plane copy for luma, and the
 * bicubic resizer for chroma (could be
 * customizable later)
 *
 * (c) Klaus Post, 2005
 * (c) Ian Brabham, 2011
 **********************************************/

inline float ChrOffset(bool point, int sIn, float dIn, int sOut, float dOut) {
      //     (1 - sOut/sIn)/2 + (dOut-dIn)/sIn; // Gavino Jan 2011
      return point ? (dOut-dIn)/sIn : 0.5f + (dOut-dIn - 0.5f*sOut)/sIn;
}

ConvertToPlanarGeneric::ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                                               const AVSValue& InPlacement, const AVSValue& chromaResampler,
                                               const AVSValue& OutPlacement, IScriptEnvironment* env) : GenericVideoFilter(src) {
  Y8input = vi.IsY8();

  if (!Y8input) {

    if (! (vi.IsYV12() || dst_space == VideoInfo::CS_YV12))
      interlaced = false;  // Ignore, if YV12 is not involved.

    // Describe input pixel positioning
    float xdInU = 0.0f, txdInU = 0.0f, bxdInU = 0.0f;
    float ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.0f;
    float xdInV = 0.0f, txdInV = 0.0f, bxdInV = 0.0f;
    float ydInV = 0.0f, tydInV = 0.0f, bydInV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(InPlacement, env)) {
        case PLACEMENT_DV:
          ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.5f;
          ydInV = 1.0f, tydInV = 0.5f, bydInV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdInU = 0.5f, txdInU = 0.5f, bxdInU = 0.5f;
          xdInV = 0.5f, txdInV = 0.5f, bxdInV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydInU = 0.5f, tydInU = 0.25f, bydInU = 0.75f;
          ydInV = 0.5f, tydInV = 0.25f, bydInV = 0.75f;
          break;
      }
    }
    else if (InPlacement.Defined())
      env->ThrowError("Convert: Input ChromaPlacement only available with YV12 source.");

    const int xsIn = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int ysIn = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);

    vi.pixel_type = dst_space;

    // Describe output pixel positioning
    float xdOutU = 0.0f, txdOutU = 0.0f, bxdOutU = 0.0f;
    float ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.0f;
    float xdOutV = 0.0f, txdOutV = 0.0f, bxdOutV = 0.0f;
    float ydOutV = 0.0f, tydOutV = 0.0f, bydOutV = 0.0f;

    if (vi.IsYV12()) {
      switch (getPlacement(OutPlacement, env)) {
        case PLACEMENT_DV:
          ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.5f;
          ydOutV = 1.0f, tydOutV = 0.5f, bydOutV = 1.0f;
          break;
        case PLACEMENT_MPEG1:
          xdOutU = 0.5f, txdOutU = 0.5f, bxdOutU = 0.5f;
          xdOutV = 0.5f, txdOutV = 0.5f, bxdOutV = 0.5f;
          // fall thru
        case PLACEMENT_MPEG2:
          ydOutU = 0.5f, tydOutU = 0.25f, bydOutU = 0.75f;
          ydOutV = 0.5f, tydOutV = 0.25f, bydOutV = 0.75f;
          break;
      }
    }
    else if (OutPlacement.Defined())
      env->ThrowError("Convert: Output ChromaPlacement only available with YV12 output.");

    const int xsOut = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xsOut - 1;
    if (vi.width & xmask)
      env->ThrowError("Convert: Cannot convert if width isn't mod%d!", xsOut);

    const int ysOut = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ysOut - 1;
    if (vi.height & ymask)
      env->ThrowError("Convert: Cannot convert if height isn't mod%d!", ysOut);

    int uv_width  = vi.width  >> vi.GetPlaneWidthSubsampling(PLANAR_U);
    int uv_height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

    ResamplingFunction *filter = getResampler(chromaResampler.AsString("bicubic"), env);

    bool P = !lstrcmpi(chromaResampler.AsString(""), "point");

    if (interlaced) {
      uv_height /=  2;

      AVSValue tUsubSampling[4] = { ChrOffset(P, xsIn, txdInU, xsOut, txdOutU), ChrOffset(P, ysIn, tydInU, ysOut, tydOutU), AVSValue(), AVSValue() };
      AVSValue bUsubSampling[4] = { ChrOffset(P, xsIn, bxdInU, xsOut, bxdOutU), ChrOffset(P, ysIn, bydInU, ysOut, bydOutU), AVSValue(), AVSValue() };
      AVSValue tVsubSampling[4] = { ChrOffset(P, xsIn, txdInV, xsOut, txdOutV), ChrOffset(P, ysIn, tydInV, ysOut, tydOutV), AVSValue(), AVSValue() };
      AVSValue bVsubSampling[4] = { ChrOffset(P, xsIn, bxdInV, xsOut, bxdOutV), ChrOffset(P, ysIn, bydInV, ysOut, bydOutV), AVSValue(), AVSValue() };

      Usource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::UToY8, env), true), env);
      Vsource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::VToY8, env), true), env);

      PClip *tbUsource = new PClip[2]; // Interleave()::~Interleave() will delete these
      PClip *tbVsource = new PClip[2];

      tbUsource[0] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 0), uv_width, uv_height, tUsubSampling, filter, env);
      tbUsource[1] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 1), uv_width, uv_height, bUsubSampling, filter, env);
      tbVsource[0] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 0), uv_width, uv_height, tVsubSampling, filter, env);
      tbVsource[1] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 1), uv_width, uv_height, bVsubSampling, filter, env);

      Usource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbUsource, env)), 2, 0);
      Vsource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbVsource, env)), 2, 0);
    }
    else {
      AVSValue UsubSampling[4] = { ChrOffset(P, xsIn, xdInU, xsOut, xdOutU), ChrOffset(P, ysIn, ydInU, ysOut, ydOutU), AVSValue(), AVSValue() };
      AVSValue VsubSampling[4] = { ChrOffset(P, xsIn, xdInV, xsOut, xdOutV), ChrOffset(P, ysIn, ydInV, ysOut, ydOutV), AVSValue(), AVSValue() };

      Usource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::UToY8, env), uv_width, uv_height, UsubSampling, filter, env);
      Vsource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::VToY8, env), uv_width, uv_height, VsubSampling, filter, env);
    }
    delete filter;
  }
  else {
    vi.pixel_type = dst_space;
  }
}

PVideoFrame __stdcall ConvertToPlanarGeneric::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
              src->GetRowSize(PLANAR_Y_ALIGNED), src->GetHeight(PLANAR_Y));
  if (Y8input) {
    memset(dst->GetWritePtr(PLANAR_U), 0x80, dst->GetHeight(PLANAR_U)*dst->GetPitch(PLANAR_U));
    memset(dst->GetWritePtr(PLANAR_V), 0x80, dst->GetHeight(PLANAR_V)*dst->GetPitch(PLANAR_V));
  } else {
    src = Usource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_U));
    src = Vsource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_V));
  }
  return dst;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV12(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV12()) {
    if (getPlacement(args[3], env) == getPlacement(args[5], env))
      return clip;
  }
  else if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);
  else if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV12: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV12, args[1].AsBool(false), args[3], args[4], args[5], env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV16(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV16())
    return clip;

  if (clip->GetVideoInfo().IsYUY2())
    return new ConvertYUY2ToYV16(clip,  env);

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV16: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV16, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV24() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    return new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV24: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV24, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV411(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV411() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString(0), env), env);
  else if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV411: Can only convert from Planar YUV.");

  // ConvertToPlanarGeneric will invoke 3 chains upon clip, cache it!
  clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
  return new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV411, args[1].AsBool(false), args[3], args[4], AVSValue(), env);
}


static int getPlacement(const AVSValue& _placement, IScriptEnvironment* env) {
  const char* placement = _placement.AsString(0);

  if (placement) {
    if (!lstrcmpi(placement, "mpeg2"))
      return PLACEMENT_MPEG2;

    if (!lstrcmpi(placement, "mpeg1"))
      return PLACEMENT_MPEG1;

    if (!lstrcmpi(placement, "dv"))
      return PLACEMENT_DV;

    env->ThrowError("Convert: Unknown chromaplacement");
  }
  return PLACEMENT_MPEG2;
}


static ResamplingFunction* getResampler( const char* resampler, IScriptEnvironment* env) {
  if (resampler) {
    if      (!lstrcmpi(resampler, "point"))
      return new PointFilter();
    else if (!lstrcmpi(resampler, "bilinear"))
      return new TriangleFilter();
    else if (!lstrcmpi(resampler, "bicubic"))
      return new MitchellNetravaliFilter(1./3,1./3); // Parse out optional B= and C= from string
    else if (!lstrcmpi(resampler, "lanczos"))
      return new LanczosFilter(3); // Parse out optional Taps= from string
    else if (!lstrcmpi(resampler, "lanczos4"))
      return new LanczosFilter(4);
    else if (!lstrcmpi(resampler, "blackman"))
      return new BlackmanFilter(4);
    else if (!lstrcmpi(resampler, "spline16"))
      return new Spline16Filter();
    else if (!lstrcmpi(resampler, "spline36"))
      return new Spline36Filter();
    else if (!lstrcmpi(resampler, "spline64"))
      return new Spline64Filter();
    else if (!lstrcmpi(resampler, "gauss"))
      return new GaussianFilter(30.0); // Parse out optional P= from string
    else if (!lstrcmpi(resampler, "sinc"))
      return new SincFilter(4); // Parse out optional Taps= from string
    else
      env->ThrowError("Convert: Unknown chroma resampler, '%s'", resampler);
  }
  return new MitchellNetravaliFilter(1./3,1./3); // Default colorspace conversion for AviSynth
}
