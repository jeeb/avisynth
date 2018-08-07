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


#include "convert.h"
#include "convert_planar.h"
#include "../filters/resample.h"
#include "../filters/planeswap.h"
#include "../filters/field.h"
#include <avs/win.h>
#include <avs/alignment.h>
#include <tmmintrin.h>
#include <emmintrin.h>
#include <smmintrin.h>
#include <algorithm>
#include <string>

enum   {PLACEMENT_MPEG2, PLACEMENT_MPEG1, PLACEMENT_DV } ;

static int getPlacement( const AVSValue& _placement, IScriptEnvironment* env);
static ResamplingFunction* getResampler( const char* resampler, IScriptEnvironment* env);


ConvertToY8::ConvertToY8(PClip src, int in_matrix, IScriptEnvironment* env) : GenericVideoFilter(src) {
  yuy2_input = blit_luma_only = packed_rgb_input = planar_rgb_input = false;

  int target_pixel_type;
  int bits_per_pixel = vi.BitsPerComponent();
  switch (bits_per_pixel)
  {
  case 8: target_pixel_type = VideoInfo::CS_Y8; break;
  case 10: target_pixel_type = VideoInfo::CS_Y10; break;
  case 12: target_pixel_type = VideoInfo::CS_Y12; break;
  case 14: target_pixel_type = VideoInfo::CS_Y14; break;
  case 16: target_pixel_type = VideoInfo::CS_Y16; break;
  case 32: target_pixel_type = VideoInfo::CS_Y32; break;
  default:
    env->ThrowError("ConvertToY does not support %d-bit formats.", vi.BitsPerComponent());
  }

  pixelsize = vi.ComponentSize();

  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) { // not for Planar RGB
    blit_luma_only = true;
    vi.pixel_type = target_pixel_type;
    return;
  }

  if (vi.IsYUY2()) {
    yuy2_input = true;
    vi.pixel_type = target_pixel_type;
    return;
  }

  if (vi.IsRGB()) { // also Planar RGB
    if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
      planar_rgb_input = true;
    else
      packed_rgb_input = true;
    pixel_step = vi.BytesFromPixels(1); // for packed RGB 3,4,6,8
    vi.pixel_type = target_pixel_type;

    const double range_max = bits_per_pixel == 32 ? 1.0 : (double)((1 << bits_per_pixel) - 1);
    const double luma_range = bits_per_pixel == 32 ? (235 - 16) / 255.0 : (double)((235 - 16) << (bits_per_pixel - 8));
    const int ymin = bits_per_pixel == 32 ? 16 : 16 << (bits_per_pixel - 8); // float: n/a
    const float ymin_f = 16.0f / 255.0f;

    // 32768: 15bits integer arithmetic precision
    if (in_matrix == Rec601) {
      matrix.b = (int16_t)((luma_range / range_max)*0.114*32768.0+0.5);  //B
      matrix.g = (int16_t)((luma_range / range_max)*0.587*32768.0+0.5);  //G
      matrix.r = (int16_t)((luma_range / range_max)*0.299*32768.0+0.5);  //R
      matrix.b_f = (float)((luma_range / range_max)*0.114);  //B
      matrix.g_f = (float)((luma_range / range_max)*0.587);  //G
      matrix.r_f = (float)((luma_range / range_max)*0.299);  //R
      matrix.offset_y = ymin;
      matrix.offset_y_f = ymin_f;
    } else if (in_matrix == PC_601) {
      matrix.b = (int16_t)(0.114*32768.0+0.5);  //B
      matrix.g = (int16_t)(0.587*32768.0+0.5);  //G
      matrix.r = (int16_t)(0.299*32768.0+0.5);  //R
      matrix.b_f = 0.114f;  //B
      matrix.g_f = 0.587f;  //G
      matrix.r_f = 0.299f;  //R
      matrix.offset_y = 0;
      matrix.offset_y_f = 0;
    } else if (in_matrix == Rec709) {
      matrix.b = (int16_t)((luma_range / range_max)*0.0722*32768.0+0.5);  //B
      matrix.g = (int16_t)((luma_range / range_max)*0.7152*32768.0+0.5);  //G
      matrix.r = (int16_t)((luma_range / range_max)*0.2126*32768.0+0.5);  //R
      matrix.b_f = (float)((luma_range / range_max)*0.0722);  //B
      matrix.g_f = (float)((luma_range / range_max)*0.7152);  //G
      matrix.r_f = (float)((luma_range / range_max)*0.2126);  //R
      matrix.offset_y = ymin;
      matrix.offset_y_f = ymin_f;
    } else if (in_matrix == Rec2020) {
      matrix.b = (int16_t)((luma_range / range_max)*0.0593*32768.0+0.5);  //B
      matrix.g = (int16_t)((luma_range / range_max)*0.6780*32768.0+0.5);  //G
      matrix.r = (int16_t)((luma_range / range_max)*0.2627*32768.0+0.5);  //R
      matrix.b_f = (float)((luma_range / range_max)*0.0593);  //B
      matrix.g_f = (float)((luma_range / range_max)*0.6780);  //G
      matrix.r_f = (float)((luma_range / range_max)*0.2627);  //R
      matrix.offset_y = ymin;
      matrix.offset_y_f = ymin_f;
    } else if (in_matrix == PC_709) {
      matrix.b = (int16_t)(0.0722*32768.0+0.5);  //B
      matrix.g = (int16_t)(0.7152*32768.0+0.5);  //G
      matrix.r = (int16_t)(0.2126*32768.0+0.5);  //R
      matrix.b_f = 0.0722f;  //B
      matrix.g_f = 0.7152f;  //G
      matrix.r_f = 0.2126f;  //R
      matrix.offset_y = 0;
      matrix.offset_y_f = 0;
    } else if (in_matrix == AVERAGE) {
      matrix.b = (int16_t)(32768.0/3 + 0.5);  //B
      matrix.g = (int16_t)(32768.0/3 + 0.5);  //G
      matrix.r = (int16_t)(32768.0/3 + 0.5);  //R
      matrix.b_f = (float)(1.0/3);  //B
      matrix.g_f = (float)(1.0/3);  //G
      matrix.r_f = (float)(1.0/3);  //R
      matrix.offset_y = 0;
      matrix.offset_y_f = 0;
    } else {
      env->ThrowError("ConvertToY: Unknown matrix.");
    }
    // Anti-Overflow correction
    if (matrix.offset_y == 0 && matrix.g + matrix.r + matrix.b != 32768)
      matrix.g = 32768 - (matrix.r + matrix.b);
    
    return;
  }

  env->ThrowError("ConvertToY: Unknown input format");
}


//This is faster than mmx only sometimes. But will work on x64
static void convert_yuy2_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  __m128i luma_mask = _mm_set1_epi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 16) {
      __m128i src1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2));
      __m128i src2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+16));
      src1 = _mm_and_si128(src1, luma_mask);
      src2 = _mm_and_si128(src2, luma_mask);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), _mm_packus_epi16(src1, src2));
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

#ifdef X86_32
static void convert_yuy2_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height)
{
  __m64 luma_mask = _mm_set1_pi16(0xFF);

  for(size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 8) {
      __m64 src1 = *reinterpret_cast<const __m64*>(srcp+x*2);
      __m64 src2 = *reinterpret_cast<const __m64*>(srcp+x*2+8);
      src1 = _mm_and_si64(src1, luma_mask);
      src2 = _mm_and_si64(src2, luma_mask);
      *reinterpret_cast<__m64*>(dstp+x) = _mm_packs_pu16(src1, src2);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm_empty();
}
#endif


static __forceinline __m128i convert_rgb_to_y8_sse2_core(const __m128i &pixel01, const __m128i &pixel23, const __m128i &pixel45, const __m128i &pixel67, __m128i& zero, __m128i &matrix, __m128i &round_mask, __m128i &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  // in general the algorithm is identical to MMX version, the only different part is getting r and g+b in appropriate registers. We use shuffling instead of unpacking here.
  __m128i pixel01m = _mm_madd_epi16(pixel01, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb | a0*0 + r0*cyr | g0*cyg + b0*cyb
  __m128i pixel23m = _mm_madd_epi16(pixel23, matrix); //a3*0 + r3*cyr | g3*cyg + b3*cyb | a2*0 + r2*cyr | g2*cyg + b2*cyb
  __m128i pixel45m = _mm_madd_epi16(pixel45, matrix); //a5*0 + r5*cyr | g5*cyg + b5*cyb | a4*0 + r4*cyr | g4*cyg + b4*cyb
  __m128i pixel67m = _mm_madd_epi16(pixel67, matrix); //a7*0 + r7*cyr | g7*cyg + b7*cyb | a6*0 + r6*cyr | g6*cyg + b6*cyb

  __m128i pixel_0123_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01m), _mm_castsi128_ps(pixel23m), _MM_SHUFFLE(3, 1, 3, 1))); // r3*cyr | r2*cyr | r1*cyr | r0*cyr
  __m128i pixel_4567_r = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45m), _mm_castsi128_ps(pixel67m), _MM_SHUFFLE(3, 1, 3, 1))); // r7*cyr | r6*cyr | r5*cyr | r4*cyr

  __m128i pixel_0123 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01m), _mm_castsi128_ps(pixel23m), _MM_SHUFFLE(2, 0, 2, 0)));
  __m128i pixel_4567 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel45m), _mm_castsi128_ps(pixel67m), _MM_SHUFFLE(2, 0, 2, 0)));

  pixel_0123 = _mm_add_epi32(pixel_0123, pixel_0123_r); 
  pixel_4567 = _mm_add_epi32(pixel_4567, pixel_4567_r); 

  pixel_0123 = _mm_add_epi32(pixel_0123, round_mask);
  pixel_4567 = _mm_add_epi32(pixel_4567, round_mask);

  pixel_0123 = _mm_srai_epi32(pixel_0123, 15); 
  pixel_4567 = _mm_srai_epi32(pixel_4567, 15); 

  __m128i result = _mm_packs_epi32(pixel_0123, pixel_4567);
  
  result = _mm_adds_epi16(result, offset);
  result = _mm_packus_epi16(result, zero); 

  return result;
}

static void convert_rgb32_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, const ChannelConversionMatrix &matrix) {
  __m128i matrix_v = _mm_set_epi16(0, matrix.r, matrix.g, matrix.b, 0, matrix.r, matrix.g, matrix.b);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x+=8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


static void convert_rgb24_to_y8_sse2(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, const ChannelConversionMatrix &matrix) {
  __m128i matrix_v = _mm_set_epi16(0, matrix.r, matrix.g, matrix.b, 0, matrix.r, matrix.g, matrix.b);
  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x+=8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+18)); //pixels 6 and 7

      
      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x), convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, offset));
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
}


#ifdef X86_32

#pragma warning(push)
#pragma warning(disable: 4799)
static __forceinline int convert_rgb_to_y8_mmx_core(const __m64 &pixel0, const __m64 &pixel1, const __m64 &pixel2, const __m64 &pixel3, __m64& zero, __m64 &matrix, __m64 &round_mask, __m64 &offset) {
  //int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
  
  __m64 pixel0m = _mm_madd_pi16(pixel0, matrix); //a0*0 + r0*cyr | g0*cyg + b0*cyb
  __m64 pixel1m = _mm_madd_pi16(pixel1, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb
  __m64 pixel2m = _mm_madd_pi16(pixel2, matrix); //a2*0 + r2*cyr | g2*cyg + b2*cyb
  __m64 pixel3m = _mm_madd_pi16(pixel3, matrix); //a3*0 + r3*cyr | g3*cyg + b3*cyb

  __m64 pixel_01_r = _mm_unpackhi_pi32(pixel0m, pixel1m); // r1*cyr | r0*cyr
  __m64 pixel_23_r = _mm_unpackhi_pi32(pixel2m, pixel3m); // r3*cyr | r2*cyr

  __m64 pixel_01 = _mm_unpacklo_pi32(pixel0m, pixel1m); //g1*cyg + b1*cyb | g0*cyg + b0*cyb
  __m64 pixel_23 = _mm_unpacklo_pi32(pixel2m, pixel3m); //g3*cyg + b3*cyb | g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, pixel_01_r); // r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  pixel_23 = _mm_add_pi32(pixel_23, pixel_23_r); // r3*cyr + g3*cyg + b3*cyb | r2*cyr + g2*cyg + b2*cyb

  pixel_01 = _mm_add_pi32(pixel_01, round_mask); //r1*cyr + g1*cyg + b1*cyb + 16384 | r0*cyr + g0*cyg + b0*cyb + 16384
  pixel_23 = _mm_add_pi32(pixel_23, round_mask); //r3*cyr + g3*cyg + b3*cyb + 16384 | r2*cyr + g2*cyg + b2*cyb + 16384

  pixel_01 = _mm_srai_pi32(pixel_01, 15); //0 | p1 | 0 | p0
  pixel_23 = _mm_srai_pi32(pixel_23, 15); //0 | p3 | 0 | p2

  __m64 result = _mm_packs_pi32(pixel_01, pixel_23); //p3 | p2 | p1 | p0

  result = _mm_adds_pi16(result, offset);
  result = _mm_packs_pu16(result, zero); //0 0 0 0 p3 p2 p1 p0

  return _mm_cvtsi64_si32(result);
}
#pragma warning(pop)

static void convert_rgb32_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, const ChannelConversionMatrix &matrix) {
  __m64 matrix_v = _mm_set_pi16(0, matrix.r, matrix.g, matrix.b);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x+=4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //pixels 0 and 1
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8);//pixels 2 and 3

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, offset);
    }

    srcp -= src_pitch;
    dstp += dst_pitch;
  }
  _mm_empty();
}


static void convert_rgb24_to_y8_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height, const ChannelConversionMatrix &matrix) {
  __m64 matrix_v = _mm_set_pi16(0, matrix.r, matrix.g, matrix.b);
  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+3)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+9)); //pixel 3
      
      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstp+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, offset);
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

  const BYTE* srcp = src->GetReadPtr();
  const int src_pitch = src->GetPitch();

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* dstp = dst->GetWritePtr(PLANAR_Y);
  const int dst_pitch = dst->GetPitch(PLANAR_Y);
  int rowsize = dst->GetRowSize(PLANAR_Y);
  int width = rowsize / pixelsize;
  int height = dst->GetHeight(PLANAR_Y);

  if (yuy2_input) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      convert_yuy2_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, width, height);
    } else
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX) {
      convert_yuy2_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, width, height);
    } else
#endif 
    {
      for (int y=0; y < height; y++) {
        for (int x=0; x < width; x++) {
          dstp[x] = srcp[x*2];
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
    }
    return dst;
  }

  if (packed_rgb_input) {
    srcp += src_pitch * (vi.height-1);  // We start at last line

    if ((pixelsize==1) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      } else if(pixel_step == 3) {
        convert_rgb24_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      } else if(pixel_step == 8) {
        //todo
        //convert_rgb64_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      } else if(pixel_step == 6) {
        // todo
        //convert_rgb48_to_y8_sse2(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      }
      return dst;
    }

#ifdef X86_32
    if ((pixelsize==1) && (env->GetCPUFlags() & CPUF_MMX)) {
      if (pixel_step == 4) {
        convert_rgb32_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      } else {
        convert_rgb24_to_y8_mmx(srcp, dstp, src_pitch, dst_pitch, width, height, matrix);
      }
      return dst;
    }
#endif

    // Slow C
    const int srcMod = src_pitch + width * pixel_step;
    if(pixelsize==1) {
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<vi.width; x++) {
          const int Y = matrix.offset_y + ((matrix.b * srcp[0] + matrix.g * srcp[1] + matrix.r * srcp[2] + 16384) >> 15);
          dstp[x] = PixelClip(Y);  // All the safety we can wish for.
          srcp += pixel_step; // 3,4
        }
        srcp -= srcMod;
        dstp += dst_pitch;
      }
    }
    else { // pixelsize==2
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<vi.width; x++) {
          const uint16_t *srcp16 = reinterpret_cast<const uint16_t *>(srcp);
          // int overflows!
          const int Y = matrix.offset_y + (int)(((__int64)(matrix.b * srcp16[0] + matrix.g * srcp16[1]) + (__int64)matrix.r * srcp16[2] + 16384) >> 15);
          reinterpret_cast<uint16_t *>(dstp)[x] = clamp(Y,0,65535);  // All the safety we can wish for. packed RGB 65535

          // __int64 version is a bit faster
          //const float Y = matrix.offset_y_f + (matrix.b_f * srcp16[0] + matrix.g_f * srcp16[1] + matrix.r_f * srcp16[2]);
          //reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)clamp((int)Y,0,65535);  // All the safety we can wish for.
          srcp += pixel_step; // 6,8
        }
        srcp -= srcMod;
        dstp += dst_pitch;
      }
    }
  }

  if (planar_rgb_input)
  {
    // todo: SSE2, like convert_planarrgb_to_yuv_uint8_14_sse2 and convert_planarrgb_to_yuv_uint16_float_sse2
    const BYTE *srcpG = src->GetReadPtr(PLANAR_G);
    const BYTE *srcpB = src->GetReadPtr(PLANAR_B);
    const BYTE *srcpR = src->GetReadPtr(PLANAR_R);
    const int pitchG = src->GetPitch(PLANAR_G);
    const int pitchB = src->GetPitch(PLANAR_B);
    const int pitchR = src->GetPitch(PLANAR_R);
    if(pixelsize==1) {
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<vi.width; x++) {
          const int Y = matrix.offset_y + ((matrix.b * srcpB[x] + matrix.g * srcpG[x] + matrix.r * srcpR[x] + 16384) >> 15);
          dstp[x] = PixelClip(Y);  // All the safety we can wish for.
        }
        srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
        dstp += dst_pitch;
      }
    } else if(pixelsize==2) {
      int max_pixel_value = (1 << vi.BitsPerComponent()) - 1;
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<vi.width; x++) {
          // int overflows!
          const int Y = matrix.offset_y +
            (((__int64)matrix.b * reinterpret_cast<const uint16_t *>(srcpB)[x] +
              (__int64)matrix.g * reinterpret_cast<const uint16_t *>(srcpG)[x] +
              (__int64)matrix.r * reinterpret_cast<const uint16_t *>(srcpR)[x] +
              16384) >> 15);
          reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)clamp(Y,0,max_pixel_value);  // All the safety we can wish for.
        }
        srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
        dstp += dst_pitch;
      }
    }
    else if (pixelsize==4) {
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<vi.width; x++) {
          const float Y = matrix.offset_y_f +
            (matrix.b_f * reinterpret_cast<const float *>(srcpB)[x] +
             matrix.g_f * reinterpret_cast<const float *>(srcpG)[x] +
             matrix.r_f * reinterpret_cast<const float *>(srcpR)[x]
            );
          reinterpret_cast<float *>(dstp)[x] = Y;  // no clamping
        }
        srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
        dstp += dst_pitch;
      }
    }
  }
  return dst;
}

AVSValue __cdecl ConvertToY8::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().NumComponents() == 1)
    return clip;
  return new ConvertToY8(clip, getMatrix(args[1].AsString(0), env), env);
}

/*****************************************************
 * ConvertRGBToYUV444
 ******************************************************/

ConvertRGBToYUV444::ConvertRGBToYUV444(PClip src, int in_matrix, IScriptEnvironment* env)
  : GenericVideoFilter(src)
{
  if (!vi.IsRGB())
    env->ThrowError("ConvertRGBToYV24/YUV444: Only RGB data input accepted");

  isPlanarRGBfamily = vi.IsPlanarRGB() || vi.IsPlanarRGBA();
  hasAlpha = vi.IsPlanarRGBA(); // for packed RGB always false (no YUVA target option)
  if (isPlanarRGBfamily)
  {
    pixel_step = hasAlpha ? -2 : -1;
    switch (vi.BitsPerComponent())
    {
    case 8: vi.pixel_type  = hasAlpha ? VideoInfo::CS_YUVA444    : VideoInfo::CS_YV24; break;
    case 10: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P10 : VideoInfo::CS_YUV444P10; break;
    case 12: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P12 : VideoInfo::CS_YUV444P12; break;
    case 14: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P14 : VideoInfo::CS_YUV444P14; break;
    case 16: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P16 : VideoInfo::CS_YUV444P16; break;
    case 32: vi.pixel_type = hasAlpha ? VideoInfo::CS_YUVA444PS  : VideoInfo::CS_YUV444PS; break;
    }
  } else { // packed RGB24/32/48/64
    // for compatibility reasons target is not YUVA even if original has alpha, such as RGB32
    pixel_step = vi.BytesFromPixels(1); // 3,4 for packed 8 bit, 6,8 for
    switch(vi.ComponentSize())
    {
    case 1: vi.pixel_type = VideoInfo::CS_YV24; break;
    case 2: vi.pixel_type = VideoInfo::CS_YUV444P16; break;
    case 4: vi.pixel_type = VideoInfo::CS_YUV444PS; break; // planar RGB
    }
  }


  const int shift = 15; // internally 15 bits precision, still no overflow in calculations
  
  int bits_per_pixel = vi.BitsPerComponent();

  if (in_matrix == Rec601) {
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    BuildMatrix(0.299,  /* 0.587  */ 0.114, shift, false, bits_per_pixel); // false: limited range
  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114, shift, true, bits_per_pixel); // true: full scale
  }
  else if (in_matrix == Rec709) {
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, shift, false, bits_per_pixel); // false: limited range
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, shift, true, bits_per_pixel); // true: full scale
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, shift, true, bits_per_pixel); // true: full scale
  }
  else if (in_matrix == Rec2020) {
    BuildMatrix(0.2627, /* 0.6780 */ 0.0593, shift, false, bits_per_pixel); // false: limited range
  }
  else {
    env->ThrowError("ConvertRGBToYV24/YUV444: Unknown matrix.");
  }
}

void ConvertRGBToYUV444::BuildMatrix(double Kr, double Kb, int shift, bool full_scale, int bits_per_pixel)
{
  int Sy, Suv, Oy;
  float Sy_f, Suv_f, Oy_f;

  if (bits_per_pixel <= 16) {
    Oy = full_scale ? 0 : (16 << (bits_per_pixel - 8));
    Oy_f = (float)Oy; // for 16 bits

    int ymin = (full_scale ? 0 : 16) << (bits_per_pixel - 8);
    int max_pixel_value = (1 << bits_per_pixel) - 1;
    int ymax = full_scale ? max_pixel_value : (235 << (bits_per_pixel - 8));
    Sy = ymax - ymin;
    Sy_f = (float)Sy;

    int cmin = full_scale ? 0 : (16 << (bits_per_pixel - 8));
    int cmax = full_scale ? max_pixel_value : (240 << (bits_per_pixel - 8));
    Suv = (cmax - cmin) / 2;
    Suv_f = (cmax - cmin) / 2.0f;

  }
  else {
    Oy_f = full_scale ? 0.0f : (16.0f / 255.0f);
    Oy = full_scale ? 0 : 16; // n/a

    Sy_f = full_scale ? c8tof(255) : (c8tof(235) - c8tof(16));
    Suv_f = full_scale ? uv8tof(128) : (uv8tof(240) - uv8tof(16)) / 2;
  }


/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}   // { 235-16, 255-0 }
  Suv  = {112, 127}   // { (240-16)/2, (255-0)/2 }
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
  const double mulfac = double(1<<shift); // integer aritmetic precision scale

  const double Kg = 1.- Kr - Kb;

  if (bits_per_pixel <= 16) {
    const int Srgb = (1 << bits_per_pixel) - 1;  // 255;
    matrix.y_b = (int16_t)(Sy  * Kb        * mulfac / Srgb + 0.5); //B
    matrix.y_g = (int16_t)(Sy  * Kg        * mulfac / Srgb + 0.5); //G
    matrix.y_r = (int16_t)(Sy  * Kr        * mulfac / Srgb + 0.5); //R
    matrix.u_b = (int16_t)(Suv             * mulfac / Srgb + 0.5);
    matrix.u_g = (int16_t)(Suv * Kg / (Kb - 1) * mulfac / Srgb + 0.5);
    matrix.u_r = (int16_t)(Suv * Kr / (Kb - 1) * mulfac / Srgb + 0.5);
    matrix.v_b = (int16_t)(Suv * Kb / (Kr - 1) * mulfac / Srgb + 0.5);
    matrix.v_g = (int16_t)(Suv * Kg / (Kr - 1) * mulfac / Srgb + 0.5);
    matrix.v_r = (int16_t)(Suv             * mulfac / Srgb + 0.5);
    matrix.offset_y = Oy;
  }

  // for 16 bits, float is used, no unsigned 16 bit arithmetic
  double Srgb_f = bits_per_pixel == 32 ? 1.0 : ((1 << bits_per_pixel) - 1);
  matrix.y_b_f  = (float)(Sy_f  * Kb /Srgb_f); //B
  matrix.y_g_f  = (float)(Sy_f  * Kg / Srgb_f); //G
  matrix.y_r_f  = (float)(Sy_f  * Kr / Srgb_f); //R
  matrix.u_b_f  = (float)(Suv_f / Srgb_f);
  matrix.u_g_f  = (float)(Suv_f * Kg/(Kb-1) / Srgb_f);
  matrix.u_r_f  = (float)(Suv_f * Kr/(Kb-1) / Srgb_f);
  matrix.v_b_f  = (float)(Suv_f * Kb/(Kr-1) / Srgb_f);
  matrix.v_g_f  = (float)(Suv_f * Kg/(Kr-1) / Srgb_f);
  matrix.v_r_f  = (float)(Suv_f / Srgb_f);
  matrix.offset_y_f = Oy_f;

}

static void convert_rgb32_to_yv24_sse2(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  __m128i matrix_y = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i matrix_u = _mm_set_epi16(0, matrix.u_r, matrix.u_g, matrix.u_b, 0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m128i matrix_v = _mm_set_epi16(0, matrix.v_r, matrix.v_g, matrix.v_b, 0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i v128 = _mm_set1_epi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //pixels 0, 1, 2 and 3
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16));//pixels 4, 5, 6 and 7

      __m128i pixel01 = _mm_unpacklo_epi8(src0123, zero); 
      __m128i pixel23 = _mm_unpackhi_epi8(src0123, zero); 
      __m128i pixel45 = _mm_unpacklo_epi8(src4567, zero); 
      __m128i pixel67 = _mm_unpackhi_epi8(src4567, zero); 

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+x), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), result_v);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
}

static void convert_rgb24_to_yv24_sse2(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  size_t mod8_width = width / 8 * 8;

  __m128i matrix_y = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i matrix_u = _mm_set_epi16(0, matrix.u_r, matrix.u_g, matrix.u_b, 0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m128i matrix_v = _mm_set_epi16(0, matrix.v_r, matrix.v_g, matrix.v_b, 0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m128i zero = _mm_setzero_si128();
  __m128i offset = _mm_set1_epi16(matrix.offset_y);
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i v128 = _mm_set1_epi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+18)); //pixels 6 and 7

      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+x), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+x), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+x), result_v);
    }

    if (mod8_width != width) {
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-24)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-18)); //pixels 2 and 3
      __m128i pixel45 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-12)); //pixels 4 and 5
      __m128i pixel67 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*3-6)); //pixels 6 and 7

      pixel01 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel23 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel45 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel45, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      pixel67 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel67, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1)); 

      __m128i result_y = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_y, round_mask, offset);
      __m128i result_u = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_u, round_mask, v128);
      __m128i result_v = convert_rgb_to_y8_sse2_core(pixel01, pixel23, pixel45, pixel67, zero, matrix_v, round_mask, v128);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstY+width-8), result_y);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstU+width-8), result_u);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstV+width-8), result_v);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
}

#ifdef X86_32

static void convert_rgb32_to_yv24_mmx(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix& matrix) {
  srcp += src_pitch * (height-1);

  __m64 matrix_y = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 matrix_u = _mm_set_pi16(0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m64 matrix_v = _mm_set_pi16(0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 v128 = _mm_set1_pi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //pixels 0 and 1
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8);//pixels 2 and 3

      __m64 pixel0 = _mm_unpacklo_pi8(src01, zero); //a0 r0 g0 b0
      __m64 pixel1 = _mm_unpackhi_pi8(src01, zero); //a1 r1 g1 b1
      __m64 pixel2 = _mm_unpacklo_pi8(src23, zero); //a2 r2 g2 b2
      __m64 pixel3 = _mm_unpackhi_pi8(src23, zero); //a3 r3 g3 b3

      *reinterpret_cast<int*>(dstY+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
  _mm_empty();
}

static void convert_rgb24_to_yv24_mmx(BYTE* dstY, BYTE* dstU, BYTE* dstV, const BYTE*srcp, size_t dst_pitch_y, size_t UVpitch, size_t src_pitch, size_t width, size_t height, const ConversionMatrix &matrix) {
  srcp += src_pitch * (height-1);

  size_t mod4_width = width / 4 * 4;

  __m64 matrix_y = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 matrix_u = _mm_set_pi16(0, matrix.u_r, matrix.u_g, matrix.u_b);
  __m64 matrix_v = _mm_set_pi16(0, matrix.v_r, matrix.v_g, matrix.v_b);

  __m64 zero = _mm_setzero_si64();
  __m64 offset = _mm_set1_pi16(matrix.offset_y);
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 v128 = _mm_set1_pi16(128);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod4_width; x+=4) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+3)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*3+9)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstY+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+x) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    if (mod4_width != width) {
      __m64 pixel0 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-12)); //pixel 0
      __m64 pixel1 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-9)); //pixel 1
      __m64 pixel2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-6)); //pixel 2
      __m64 pixel3 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*3-3)); //pixel 3

      pixel0 = _mm_unpacklo_pi8(pixel0, zero);
      pixel1 = _mm_unpacklo_pi8(pixel1, zero);
      pixel2 = _mm_unpacklo_pi8(pixel2, zero);
      pixel3 = _mm_unpacklo_pi8(pixel3, zero);

      *reinterpret_cast<int*>(dstY+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_y, round_mask, offset);
      *reinterpret_cast<int*>(dstU+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_u, round_mask, v128);
      *reinterpret_cast<int*>(dstV+width-4) = convert_rgb_to_y8_mmx_core(pixel0, pixel1, pixel2, pixel3, zero, matrix_v, round_mask, v128);
    }

    srcp -= src_pitch;
    dstY += dst_pitch_y;
    dstU += UVpitch;
    dstV += UVpitch;
  }
  _mm_empty();
}

#endif

template<typename pixel_t, int bits_per_pixel>
static void convert_planarrgb_to_yuv_uint8_14_sse2(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  // 8 bit        uint8_t
  // 10,12,14 bit uint16_t (signed range)
  __m128i half = _mm_set1_epi16((short)(1 << (bits_per_pixel - 1)));  // 128
  __m128i limit = _mm_set1_epi16((short)((1 << bits_per_pixel) - 1)); // 255
  __m128i offset = _mm_set1_epi16((short)m.offset_y);

  __m128i zero = _mm_setzero_si128();

  const int rowsize = width * sizeof(pixel_t);
  int wmod = (rowsize / 8) * 8;
  for (int yy = 0; yy < height; yy++) {
    for (int x = 0; x < wmod; x += 8 * sizeof(pixel_t)) {
      __m128i res1, res2;
      __m128i m_bg, m_rR;
      __m128i bg0123, bg4567;
      __m128i ar0123, ar4567;
      __m128i g, b, r;
      // cant handle 16 at a time, only 2x4 8bits pixels (4x32_mul_result=128 bit)
      if constexpr(sizeof(pixel_t) == 1) {
        g = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[0] + x)), zero);
        b = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[1] + x)), zero);
        r = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[2] + x)), zero);
      }
      else { // uint16_t pixels, 14 bits OK, but 16 bit pixels are unsigned, cannot madd
        g = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[0] + x));
        b = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[1] + x));
        r = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[2] + x));
      }
      // Need1:  (m.y_b   m.y_g)     (m.y_b   m.y_g)     (m.y_b   m.y_g)     (m.y_b   m.y_g)   8x16 bit
      //         (  b3     g3  )     (  b2      g2 )     (  b1      g1 )     (   b0     g0 )   8x16 bit
      // res1=  (y_b*b3 + y_g*g3)   (y_b*b2 + y_g*g2)   (y_b*b1 + y_g*g1)   (y_b*b0 + y_g*g0)  4x32 bit
      // Need2:  (m.y_r   round)     (m.y_r   round)     (m.y_r   round)     (m.y_r   round)
      //         (  r3      1  )     (  r2      1  )     (  r1      1  )     (  r0      1  )
      // res2=  (y_r*r3 + round )   (y_r*r2 + round )   (y_r*r1 + round )   (y_r*r0 + round )
      // Y result 4x32 bit = offset + ((res1 + res2) >> 15)
      // UV result 4x32 bit = half + ((res1_u_or_v + res2_u_or_v) >> 15)
      // *Y* ----------------
      m_bg = _mm_set1_epi32(int(((uint16_t)(m.y_g) << 16) | (uint16_t)m.y_b)); // green and blue
      m_rR = _mm_set1_epi32(int(((uint16_t)(16384) << 16) | (uint16_t)m.y_r)); // rounding 15 bit >> 1   and red

      bg0123 = _mm_unpacklo_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg0123);
      ar0123 = _mm_unpacklo_epi16(r, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_rR, ar0123);
      __m128i y_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2), 15);

      bg4567 = _mm_unpackhi_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg4567);
      ar4567 = _mm_unpackhi_epi16(r, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_rR, ar4567);
      __m128i y_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2), 15);

      __m128i y = _mm_add_epi16(_mm_packs_epi32(y_lo, y_hi), offset); // 2x4x32 -> 2x4xuint16_t
      if constexpr(sizeof(pixel_t) == 1) {
        y = _mm_packus_epi16(y, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[0]+x), y);
      }
      else {
        y = _mm_min_epi16(y, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[0]+x), y);
      }

      // *U* ----------------
      m_bg = _mm_set1_epi32(int(((uint16_t)(m.u_g) << 16) | (uint16_t)m.u_b)); // green and blue
      m_rR = _mm_set1_epi32(int(((uint16_t)(16384) << 16) | (uint16_t)m.u_r)); // rounding 15 bit >> 1   and red

      bg0123 = _mm_unpacklo_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg0123);
      ar0123 = _mm_unpacklo_epi16(r, _mm_set1_epi16(1));
      res2   = _mm_madd_epi16(m_rR, ar0123);
      __m128i u_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2),15);

      bg4567 = _mm_unpackhi_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg4567);
      ar4567 = _mm_unpackhi_epi16(r, _mm_set1_epi16(1));
      res2   = _mm_madd_epi16(m_rR, ar4567);
      __m128i u_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2),15);

      __m128i u = _mm_add_epi16(_mm_packs_epi32(u_lo, u_hi), half); // 2x4x32 -> 2x4xuint16_t

      if constexpr(sizeof(pixel_t) == 1) {
        u = _mm_packus_epi16(u, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[1]+x), u);
      }
      else {
        u = _mm_min_epi16(u, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[1]+x), u);
      }
      // *V* ----------------
      m_bg = _mm_set1_epi32(int(((uint16_t)(m.v_g) << 16) | (uint16_t)m.v_b)); // green and blue
      m_rR = _mm_set1_epi32(int(((uint16_t)(16384 << 16)) | (uint16_t)m.v_r)); // rounding 15 bit >> 1   and red

      bg0123 = _mm_unpacklo_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg0123);
      ar0123 = _mm_unpacklo_epi16(r, _mm_set1_epi16(1));
      res2   = _mm_madd_epi16(m_rR, ar0123);
      __m128i v_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2),15);

      bg4567 = _mm_unpackhi_epi16(b, g);
      res1 = _mm_madd_epi16(m_bg, bg4567);
      ar4567 = _mm_unpackhi_epi16(r, _mm_set1_epi16(1));
      res2   = _mm_madd_epi16(m_rR, ar4567);
      __m128i v_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2),15);

      __m128i v = _mm_add_epi16(_mm_packs_epi32(v_lo, v_hi), half); // 2x4x32 -> 2x4xuint16_t

      if constexpr(sizeof(pixel_t) == 1) {
        v = _mm_packus_epi16(v, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[2]+x), v);
      }
      else {
        v = _mm_min_epi16(v, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[2]+x), v);
      }
      /*
      int Y = (m.offset_y + (int)(((sum_t)m.y_b * b + (sum_t)m.y_g * g + (sum_t)m.y_r * r + 16384)>>15);
      int U = half + (int)(((sum_t)m.u_b * b + (sum_t)m.u_g * g + (sum_t)m.u_r * r + 16384) >> 15);
      int V = half + (int)(((sum_t)m.v_b * b + (sum_t)m.v_g * g + (sum_t)m.v_r * r + 16384) >> 15);
      reinterpret_cast<pixel_t *>(dstp[0])[x] = (pixel_t)clamp(Y, 0, limit);
      reinterpret_cast<pixel_t *>(dstp[1])[x] = (pixel_t)clamp(U, 0, limit);
      reinterpret_cast<pixel_t *>(dstp[2])[x] = (pixel_t)clamp(V, 0, limit);
      */
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

// todo FMA?
template<typename pixel_t, int bits_per_pixel, bool hasSSE4>
static void convert_planarrgb_to_yuv_uint16_float_sse2(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  // 16 bit uint16_t (unsigned range)
  // 32 bit float
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float shift = 0.5f;
#else
  const float shift = 0.0f;
#endif

  __m128  half_f = _mm_set1_ps(sizeof(pixel_t) == 4 ? shift : (float)(1u << (bits_per_pixel - 1)));
  __m128i limit  = _mm_set1_epi16((short)((1 << bits_per_pixel) - 1)); // 255
  __m128 offset_f = _mm_set1_ps(m.offset_y_f);

  __m128i zero = _mm_setzero_si128();

  const int rowsize = width * sizeof(pixel_t);
  for (int yy = 0; yy < height; yy++) {
    for (int x = 0; x < rowsize; x += 4 * sizeof(pixel_t)) {
      __m128 sum1, sum2;
      __m128 mul_r, mul_g, mul_b;
      __m128 mat_r, mat_g, mat_b;
      __m128 g, b, r;
      if constexpr(sizeof(pixel_t) == 4) {
        // float: load 16 bytes: 4 pixels
        g = _mm_load_ps(reinterpret_cast<const float *>(srcp[0] + x));
        b = _mm_load_ps(reinterpret_cast<const float *>(srcp[1] + x));
        r = _mm_load_ps(reinterpret_cast<const float *>(srcp[2] + x));
      }
      else {
        // uint16_t: load 8 bytes: 4 pixels
        __m128i gi = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[0] + x));
        __m128i bi = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[1] + x));
        __m128i ri = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[2] + x));
        g = _mm_cvtepi32_ps(_mm_unpacklo_epi16(gi,zero));
        b = _mm_cvtepi32_ps(_mm_unpacklo_epi16(bi,zero));
        r = _mm_cvtepi32_ps(_mm_unpacklo_epi16(ri,zero));
      }
      /*
      int Y = m.offset_y + (int)(((sum_t)m.y_b * b + (sum_t)m.y_g * g + (sum_t)m.y_r * r + 16384)>>15);
      int U = half + (int)(((sum_t)m.u_b * b + (sum_t)m.u_g * g + (sum_t)m.u_r * r + 16384) >> 15);
      int V = half + (int)(((sum_t)m.v_b * b + (sum_t)m.v_g * g + (sum_t)m.v_r * r + 16384) >> 15);
      */
      // *Y*
      mat_r = _mm_set1_ps(m.y_r_f);
      mat_g = _mm_set1_ps(m.y_g_f);
      mat_b = _mm_set1_ps(m.y_b_f);
      mul_r = _mm_mul_ps(r, mat_r);
      mul_g = _mm_mul_ps(g, mat_g);
      mul_b = _mm_mul_ps(b, mat_b);
      sum1 = _mm_add_ps(mul_r, mul_g);
      sum2 = _mm_add_ps(mul_b, offset_f);
      __m128 y = _mm_add_ps(sum1, sum2);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[0] + x), y);
      }
      else {
        __m128i yi = _mm_cvtps_epi32(y); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          yi = _mm_packus_epi32(yi, zero);
        else
          yi = _MM_PACKUS_EPI32(yi, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          yi = _mm_min_epi16(yi, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[0] + x), yi);
      }
      // *U*
      mat_r = _mm_set1_ps(m.u_r_f);
      mat_g = _mm_set1_ps(m.u_g_f);
      mat_b = _mm_set1_ps(m.u_b_f);
      mul_r = _mm_mul_ps(r, mat_r);
      mul_g = _mm_mul_ps(g, mat_g);
      mul_b = _mm_mul_ps(b, mat_b);
      sum1 = _mm_add_ps(mul_r, mul_g);
      sum2 = _mm_add_ps(mul_b, half_f);
      __m128 u = _mm_add_ps(sum1, sum2);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[1] + x), u);
      }
      else {
        __m128i ui = _mm_cvtps_epi32(u); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          ui = _mm_packus_epi32(ui, zero);
        else
          ui = _MM_PACKUS_EPI32(ui, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          ui = _mm_min_epi16(ui, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[1] + x), ui);
      }
      // *V*
      mat_r = _mm_set1_ps(m.v_r_f);
      mat_g = _mm_set1_ps(m.v_g_f);
      mat_b = _mm_set1_ps(m.v_b_f);
      mul_r = _mm_mul_ps(r, mat_r);
      mul_g = _mm_mul_ps(g, mat_g);
      mul_b = _mm_mul_ps(b, mat_b);
      sum1 = _mm_add_ps(mul_r, mul_g);
      sum2 = _mm_add_ps(mul_b, half_f);
      __m128 v = _mm_add_ps(sum1, sum2);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[2] + x), v);
      }
      else {
        __m128i vi = _mm_cvtps_epi32(v); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          vi = _mm_packus_epi32(vi, zero);
        else
          vi = _MM_PACKUS_EPI32(vi, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          vi = _mm_min_epi16(vi, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[2] + x), vi);
      }
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

template<typename pixel_t, int bits_per_pixel>
static void convert_planarrgb_to_yuv_int_c(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  const pixel_t half = 1 << (bits_per_pixel - 1 );
  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type sum_t;
  const int limit = (1 << bits_per_pixel) - 1;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixel_t g = reinterpret_cast<const pixel_t *>(srcp[0])[x];
      pixel_t b = reinterpret_cast<const pixel_t *>(srcp[1])[x];
      pixel_t r = reinterpret_cast<const pixel_t *>(srcp[2])[x];
      int Y = m.offset_y + (int)(((sum_t)m.y_b * b + (sum_t)m.y_g * g + (sum_t)m.y_r * r + 16384)>>15);
      int U = half + (int)(((sum_t)m.u_b * b + (sum_t)m.u_g * g + (sum_t)m.u_r * r + 16384) >> 15);
      int V = half + (int)(((sum_t)m.v_b * b + (sum_t)m.v_g * g + (sum_t)m.v_r * r + 16384) >> 15);
      reinterpret_cast<pixel_t *>(dstp[0])[x] = (pixel_t)clamp(Y, 0, limit);
      reinterpret_cast<pixel_t *>(dstp[1])[x] = (pixel_t)clamp(U, 0, limit);
      reinterpret_cast<pixel_t *>(dstp[2])[x] = (pixel_t)clamp(V, 0, limit);
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

static void convert_planarrgb_to_yuv_float_c(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  const float limit = 1.0f; // we clamp on RGB conversions for float
  const float limit_lo_chroma = -0.5f; // checked before shift
  const float limit_hi_chroma = 0.5f;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float half = 0.5f;
#else
  const float half = 0.0f;
#endif
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float g = reinterpret_cast<const float *>(srcp[0])[x];
      float b = reinterpret_cast<const float *>(srcp[1])[x];
      float r = reinterpret_cast<const float *>(srcp[2])[x];
      float Y = m.offset_y_f + (m.y_b_f * b + m.y_g_f * g + m.y_r_f * r);
      float U = half + (m.u_b_f * b + m.u_g_f * g + m.u_r_f * r);
      float V = half + (m.v_b_f * b + m.v_g_f * g + m.v_r_f * r);
      // All the safety we can wish for.
      // theoretical question: should we clamp here?
      reinterpret_cast<float *>(dstp[0])[x] = clamp(Y, 0.0f, limit);
      reinterpret_cast<float *>(dstp[1])[x] = clamp(U, limit_lo_chroma, limit_hi_chroma) + half;
      reinterpret_cast<float *>(dstp[2])[x] = clamp(V, limit_lo_chroma, limit_hi_chroma) + half;
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

PVideoFrame __stdcall ConvertRGBToYUV444::GetFrame(int n, IScriptEnvironment* env)
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

  if (pixel_step != 4 && pixel_step != 3 && pixel_step != 8 && pixel_step != 6 && pixel_step != -1 && pixel_step != -2) {
    env->ThrowError("Invalid pixel step. This is a bug.");
  }

  // sse2 for 8 bit only (pixel_step==3,4), todo
  if (((pixel_step == 3) || (pixel_step == 4)) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
    if (pixel_step == 4) {
      convert_rgb32_to_yv24_sse2(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    } else {
      convert_rgb24_to_yv24_sse2(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    }
    return dst;
  }

#ifdef X86_32
  if (((pixel_step == 3) || (pixel_step == 4)) && (env->GetCPUFlags() & CPUF_MMX)) {
    if (pixel_step == 4) {
      convert_rgb32_to_yv24_mmx(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    } else {
      convert_rgb24_to_yv24_mmx(dstY, dstU, dstV, srcp, Ypitch, UVpitch, Spitch, vi.width, vi.height, matrix);
    }
    return dst;
  }
#endif

  //Slow C-code.

  ConversionMatrix &m = matrix;
  srcp += Spitch * (vi.height-1);  // We start at last line
  const int Sstep = Spitch + (vi.width * pixel_step);

  if(pixel_step==3 || pixel_step==4)
  {
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int b = srcp[0];
        int g = srcp[1];
        int r = srcp[2];
        int Y = m.offset_y + (((int)m.y_b * b + (int)m.y_g * g + (int)m.y_r * r + 16384)>>15);
        int U = 128+(((int)m.u_b * b + (int)m.u_g * g + (int)m.u_r * r + 16384)>>15);
        int V = 128+(((int)m.v_b * b + (int)m.v_g * g + (int)m.v_r * r + 16384)>>15);
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
  }
  else if(pixel_step==6 || pixel_step==8){
    // uint16: pixel_step==6,8
    uint16_t *dstY16 = reinterpret_cast<uint16_t *>(dstY);
    uint16_t *dstU16 = reinterpret_cast<uint16_t *>(dstU);
    uint16_t *dstV16 = reinterpret_cast<uint16_t *>(dstV);
    int Ypitch16 = Ypitch / sizeof(uint16_t);
    int UVpitch16 = UVpitch / sizeof(uint16_t);
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int b = reinterpret_cast<const uint16_t *>(srcp)[0];
        int g = reinterpret_cast<const uint16_t *>(srcp)[1];
        int r = reinterpret_cast<const uint16_t *>(srcp)[2];
        int Y = m.offset_y + (((__int64)m.y_b * b + (__int64)m.y_g * g + (__int64)m.y_r * r + 16384)>>15);
        int U = 32768+(((__int64)m.u_b * b + (__int64)m.u_g * g + (__int64)m.u_r * r + 16384)>>15);
        int V = 32768+(((__int64)m.v_b * b + (__int64)m.v_g * g + (__int64)m.v_r * r + 16384)>>15);
        *dstY16++ = (uint16_t)clamp(Y, 0, 65535);// PixelClip(Y);  // All the safety we can wish for.
        *dstU16++ = (uint16_t)clamp(U, 0, 65535);
        *dstV16++ = (uint16_t)clamp(V, 0, 65535);
        srcp += pixel_step;
      }
      srcp -= Sstep;
      dstY16 += Ypitch16 - vi.width;
      dstU16 += UVpitch16 - vi.width;
      dstV16 += UVpitch16 - vi.width;
    }
  }
  else {
    // isPlanarRGBfamily
    if(hasAlpha) {
      // simple copy
      BYTE* dstA = dst->GetWritePtr(PLANAR_A);
      const int Apitch = dst->GetPitch(PLANAR_A);
      env->BitBlt(dstA, Apitch, src->GetReadPtr(PLANAR_A), src->GetPitch(PLANAR_A), src->GetRowSize(PLANAR_A_ALIGNED), src->GetHeight(PLANAR_A));
    }
    int pixelsize = vi.ComponentSize();
    int bits_per_pixel = vi.BitsPerComponent();

    const BYTE *srcp[3] = { src->GetReadPtr(PLANAR_G), src->GetReadPtr(PLANAR_B), src->GetReadPtr(PLANAR_R) };
    const int srcPitch[3] = { src->GetPitch(PLANAR_G), src->GetPitch(PLANAR_B), src->GetPitch(PLANAR_R) };

    BYTE *dstp[3] = { dstY, dstU, dstV };
    int dstPitch[3] = { Ypitch, UVpitch, UVpitch };
    if (bits_per_pixel < 16 && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp[0], 16) && IsPtrAligned(dstp[0], 16))
    {
      switch (bits_per_pixel) {
      case 8: convert_planarrgb_to_yuv_uint8_14_sse2<uint8_t, 8>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 10: convert_planarrgb_to_yuv_uint8_14_sse2<uint16_t, 10>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 12: convert_planarrgb_to_yuv_uint8_14_sse2<uint16_t, 12>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 14: convert_planarrgb_to_yuv_uint8_14_sse2<uint16_t, 14>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      }
      return dst;
    }
    if (bits_per_pixel >= 16 && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp[0], 16) && IsPtrAligned(dstp[0], 16)) {
      if (pixelsize == 4) // float 32 bit
        convert_planarrgb_to_yuv_uint16_float_sse2<float, 8 /*n/a*/, false>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      else if (env->GetCPUFlags() & CPUF_SSE4)
        convert_planarrgb_to_yuv_uint16_float_sse2<uint16_t, 16, true>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      else
        convert_planarrgb_to_yuv_uint16_float_sse2<uint16_t, 16, false>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      return dst;
    }

    switch(bits_per_pixel) {
    case 8: convert_planarrgb_to_yuv_int_c<uint8_t, 8>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    case 10: convert_planarrgb_to_yuv_int_c<uint16_t, 10>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    case 12: convert_planarrgb_to_yuv_int_c<uint16_t, 12>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    case 14: convert_planarrgb_to_yuv_int_c<uint16_t, 14>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    case 16: convert_planarrgb_to_yuv_int_c<uint16_t, 16>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    case 32: convert_planarrgb_to_yuv_float_c(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
    }
  }
  return dst;
}

AVSValue __cdecl ConvertRGBToYUV444::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().Is444())
    return clip;
  return new ConvertRGBToYUV444(clip, getMatrix(args[1].AsString(0), env), env);
}


/*****************************************************
 * ConvertYV24ToRGB
 *
 * (c) Klaus Post, 2005
 * Generic 4:4:4(:4), 16 bit and Planar RGB(A) support 2016 by PF
 ******************************************************/


ConvertYUV444ToRGB::ConvertYUV444ToRGB(PClip src, int in_matrix, int _pixel_step, IScriptEnvironment* env)
 : GenericVideoFilter(src), pixel_step(_pixel_step)
{

  if (!vi.Is444())
    env->ThrowError("ConvertYUV444ToRGB: Only 4:4:4 data input accepted");

  switch (pixel_step)
  {
  case -1: case -2:
    switch (vi.BitsPerComponent())
    {
    case 8:  vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAP : VideoInfo::CS_RGBP; break;
    case 10: vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAP10 : VideoInfo::CS_RGBP10; break;
    case 12: vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAP12 : VideoInfo::CS_RGBP12; break;
    case 14: vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAP14 : VideoInfo::CS_RGBP14; break;
    case 16: vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAP16 : VideoInfo::CS_RGBP16; break;
    case 32: vi.pixel_type = pixel_step == -2 ? VideoInfo::CS_RGBAPS : VideoInfo::CS_RGBPS; break;
    default:
      env->ThrowError("ConvertYUV444ToRGB: invalid vi.BitsPerComponent(): %d", vi.BitsPerComponent());
    }
    break;
  case 3: vi.pixel_type = VideoInfo::CS_BGR24; break;
  case 4: vi.pixel_type = VideoInfo::CS_BGR32; break;
  case 6: vi.pixel_type = VideoInfo::CS_BGR48; break;
  case 8: vi.pixel_type = VideoInfo::CS_BGR64; break;
  default:
    env->ThrowError("ConvertYUV444ToRGB: invalid pixel step: %d", pixel_step);
  }

  const int shift = 13; // for integer arithmetic, over 13 bits would overflow the internal calculation

  const int bits_per_pixel = vi.BitsPerComponent();

  if (in_matrix == Rec601) {
/*
    B'= Y' + 1.772*U' + 0.000*V'
    G'= Y' - 0.344*U' - 0.714*V'
    R'= Y' + 0.000*U' + 1.402*V'
*/
    BuildMatrix(0.299,  /* 0.587  */ 0.114, shift, false, bits_per_pixel); // false: limited range

  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114, shift, true, bits_per_pixel); // true: full scale
  }
  else if (in_matrix == Rec709) {
/*
    B'= Y' + 1.8558*Cb + 0.0000*Cr
    G'= Y' - 0.1870*Cb - 0.4678*Cr
    R'= Y' + 0.0000*Cb + 1.5750*Cr
*/
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, shift, false, bits_per_pixel); // false: limited range
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, shift, true, bits_per_pixel); // true: full scale
  }
  else if (in_matrix == Rec2020) {
    BuildMatrix(0.2627, /* 0.6780 */ 0.0593, shift, false, bits_per_pixel); // false: limited range
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, shift, true, bits_per_pixel); // true: full scale
  }
  else {
    env->ThrowError("ConvertYV24ToRGB: Unknown matrix.");
  }
}

void ConvertYUV444ToRGB::BuildMatrix(double Kr, double Kb, int shift, bool full_scale, int bits_per_pixel)
{
  int Sy, Suv, Oy;
  float Sy_f, Suv_f, Oy_f;

  if (bits_per_pixel <= 16) {
    Oy = full_scale ? 0 : (16 << (bits_per_pixel - 8));
    Oy_f = (float)Oy; // for 16 bits

    int ymin = (full_scale ? 0 : 16) << (bits_per_pixel - 8);
    int max_pixel_value = (1 << bits_per_pixel) - 1;
    int ymax = full_scale ? max_pixel_value : (235 << (bits_per_pixel - 8));
    Sy = ymax - ymin;
    Sy_f = (float)Sy;

    int cmin = full_scale ? 0 : (16 << (bits_per_pixel - 8));
    int cmax = full_scale ? max_pixel_value : (240 << (bits_per_pixel - 8));
    Suv = (cmax - cmin) / 2;
    Suv_f = (cmax - cmin) / 2.0f;

  }
  else {
    Oy_f = full_scale ? 0.0f : (16.0f / 255.0f);
    Oy = full_scale ? 0 : 16; // n/a

    Sy_f = full_scale ? c8tof(255) : (c8tof(235) - c8tof(16));
    Suv_f = full_scale ? uv8tof(128) : (uv8tof(240) - uv8tof(16)) / 2;
  }


/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}   // { 235-16, 255-0 }
  Suv  = {112, 127}   // { (240-16)/2, (255-0)/2 }
  Oy   = {16, 0}
  Ouv  = 128

  Y =(y-Oy)  / Sy                         // 0..1
  U =(u-Ouv) / Suv                        //-1..1
  V =(v-Ouv) / Suv

  R = Y                  + V*(1-Kr)       // 0..1
  G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
  B = Y + U*(1-Kb)

  r = R*Srgb                              // 0..255   0..65535
  g = G*Srgb
  b = B*Srgb
*/

  const double mulfac = double(1 << shift); // integer aritmetic precision scale

  const double Kg = 1. - Kr - Kb;

  if (bits_per_pixel <= 16) {
    const int Srgb = (1 << bits_per_pixel) - 1;  // 255;
    matrix.y_b = (int16_t)(Srgb * 1.000        * mulfac / Sy + 0.5); //Y
    matrix.u_b = (int16_t)(Srgb * (1 - Kb)       * mulfac / Suv + 0.5); //U
    matrix.v_b = (int16_t)(Srgb * 0.000        * mulfac / Suv + 0.5); //V
    matrix.y_g = (int16_t)(Srgb * 1.000        * mulfac / Sy + 0.5);
    matrix.u_g = (int16_t)(Srgb * (Kb - 1)*Kb / Kg * mulfac / Suv + 0.5);
    matrix.v_g = (int16_t)(Srgb * (Kr - 1)*Kr / Kg * mulfac / Suv + 0.5);
    matrix.y_r = (int16_t)(Srgb * 1.000        * mulfac / Sy + 0.5);
    matrix.u_r = (int16_t)(Srgb * 0.000        * mulfac / Suv + 0.5);
    matrix.v_r = (int16_t)(Srgb * (1 - Kr)       * mulfac / Suv + 0.5);
    matrix.offset_y = -Oy;
  }

  double Srgb_f = bits_per_pixel == 32 ? 1.0 : ((1 << bits_per_pixel) - 1);
  matrix.y_b_f = (float)(Srgb_f * 1.000 / Sy_f); //Y
  matrix.u_b_f = (float)(Srgb_f * (1-Kb)       / Suv_f); //U
  matrix.v_b_f = (float)(Srgb_f * 0.000        / Suv_f); //V
  matrix.y_g_f = (float)(Srgb_f * 1.000        / Sy_f);
  matrix.u_g_f = (float)(Srgb_f * (Kb-1)*Kb/Kg / Suv_f);
  matrix.v_g_f = (float)(Srgb_f * (Kr-1)*Kr/Kg / Suv_f);
  matrix.y_r_f = (float)(Srgb_f * 1.000        / Sy_f);
  matrix.u_r_f = (float)(Srgb_f * 0.000        / Suv_f);
  matrix.v_r_f = (float)(Srgb_f * (1-Kr)       / Suv_f);
  matrix.offset_y_f = -Oy_f;
}

template<typename pixel_t, int bits_per_pixel>
static void convert_yuv_to_planarrgb_uint8_14_sse2(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  // 8 bit        uint8_t
  // 10,12,14 bit uint16_t (signed range)
  __m128i half = _mm_set1_epi16((short)(1 << (bits_per_pixel - 1)));  // 128
  __m128i limit = _mm_set1_epi16((short)((1 << bits_per_pixel) - 1)); // 255
  __m128i offset = _mm_set1_epi16((short)(m.offset_y));

  __m128i zero = _mm_setzero_si128();

  const int rowsize = width * sizeof(pixel_t);
  for (int yy = 0; yy < height; yy++) {
    // if not mod16 then still no trouble, process non-visible pixels, we have 32 byte aligned in avs+
    for (int x = 0; x < rowsize; x += 8 * sizeof(pixel_t)) {
      __m128i res1, res2;
      __m128i m_uy, m_vR; // bg=uy rR=vR, g=y b=u r=v R=rounding
      __m128i uy0123, uy4567;
      __m128i xv0123, xv4567;
      __m128i y, u, v;
      // cant handle 16 at a time, only 2x4 8bits pixels (4x32_mul_result=128 bit)
      if constexpr(sizeof(pixel_t) == 1) {
        y = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[0] + x)), zero);
        u = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[1] + x)), zero);
        v = _mm_unpacklo_epi8(_mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[2] + x)), zero);
      }
      else { // uint16_t pixels, 14 bits OK, but 16 bit pixels are unsigned, cannot madd
        y = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[0] + x));
        u = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[1] + x));
        v = _mm_load_si128(reinterpret_cast<const __m128i *>(srcp[2] + x));
      }
      y = _mm_adds_epi16(y, offset); // offset is negative
      u = _mm_subs_epi16(u, half);
      v = _mm_subs_epi16(v, half);
      /*
      int b = (((__int64)matrix.y_b * Y + (__int64)matrix.u_b * U + (__int64)matrix.v_b * V + 4096)>>13);
      int g = (((__int64)matrix.y_g * Y + (__int64)matrix.u_g * U + (__int64)matrix.v_g * V + 4096)>>13);
      int r = (((__int64)matrix.y_r * Y + (__int64)matrix.u_r * U + (__int64)matrix.v_r * V + 4096)>>13);
      */
      // Need1:  (m.y_b   m.u_b )     (m.y_b   m.u_b)     (m.y_b   m.u_b)     (m.y_b   m.u_b)   8x16 bit
      //         (  y3      u3  )     (  y2      u2 )     (  y1      u1 )     (   y0     u0 )   8x16 bit
      // res1=  (y_b*y3 + u_b*u3)   ...                                                         4x32 bit
      // Need2:  (m.v_b   round )     (m.y_b   round)     (m.y_b   round)     (m.y_b   round)
      //         (  v3      1   )     (  v2      1  )     (  v1      1  )     (  v0      1  )
      // res2=  (yv_b*v3 + round )  ...
      // *G* ----------------
      m_uy = _mm_set1_epi32(int((static_cast<uint16_t>(m.y_g) << 16) | static_cast<uint16_t>(m.u_g))); // y and u
      m_vR = _mm_set1_epi32(int((static_cast<uint16_t>(4096) << 16) | static_cast<uint16_t>(m.v_g))); // rounding 13 bit >> 1 and v

      uy0123 = _mm_unpacklo_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy0123);
      xv0123 = _mm_unpacklo_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv0123);
      __m128i g_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      uy4567 = _mm_unpackhi_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy4567);
      xv4567 = _mm_unpackhi_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv4567);
      __m128i g_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      __m128i g = _mm_packs_epi32(g_lo, g_hi); // 2x4x32 -> 2x4xuint16_t
      if constexpr(sizeof(pixel_t) == 1) {
        g = _mm_packus_epi16(g, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[0]+x), g);
      }
      else {
        g = _mm_max_epi16(_mm_min_epi16(g, limit), zero); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[0]+x), g);
      }
      // *B* ----------------
      m_uy = _mm_set1_epi32(int((static_cast<uint16_t>(m.y_b) << 16) | static_cast<uint16_t>(m.u_b))); // y and u
      m_vR = _mm_set1_epi32(int((static_cast<uint16_t>(4096) << 16) | static_cast<uint16_t>(m.v_b))); // rounding 13 bit >> 1 and v

      uy0123 = _mm_unpacklo_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy0123);
      xv0123 = _mm_unpacklo_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv0123);
      __m128i b_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      uy4567 = _mm_unpackhi_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy4567);
      xv4567 = _mm_unpackhi_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv4567);
      __m128i b_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      __m128i b = _mm_packs_epi32(b_lo, b_hi); // 2x4x32 -> 2x4xuint16_t
      if constexpr(sizeof(pixel_t) == 1) {
        b = _mm_packus_epi16(b, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[1]+x), b);
      }
      else {
        b = _mm_max_epi16(_mm_min_epi16(b, limit), zero); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[1]+x), b);
      }
      // *R* ----------------
      m_uy = _mm_set1_epi32(int((static_cast<uint16_t>(m.y_r) << 16) | static_cast<uint16_t>(m.u_r))); // y and u
      m_vR = _mm_set1_epi32(int((static_cast<uint16_t>(4096) << 16) | static_cast<uint16_t>(m.v_r))); // rounding 13 bit >> 1 and v

      uy0123 = _mm_unpacklo_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy0123);
      xv0123 = _mm_unpacklo_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv0123);
      __m128i r_lo = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      uy4567 = _mm_unpackhi_epi16(u, y);
      res1 = _mm_madd_epi16(m_uy, uy4567);
      xv4567 = _mm_unpackhi_epi16(v, _mm_set1_epi16(1));
      res2 = _mm_madd_epi16(m_vR, xv4567);
      __m128i r_hi = _mm_srai_epi32(_mm_add_epi32(res1, res2), 13);

      __m128i r = _mm_packs_epi32(r_lo, r_hi); // 2x4x32 -> 2x4xuint16_t
      if constexpr(sizeof(pixel_t) == 1) {
        r = _mm_packus_epi16(r, zero);   // 8x uint16_t -> 8x uint_8
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[2]+x), r);
      }
      else {
        r = _mm_max_epi16(_mm_min_epi16(r, limit), zero); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp[2]+x), r);
      }
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

template<typename pixel_t, int bits_per_pixel, bool hasSSE4>
static void convert_yuv_to_planarrgb_uint16_float_sse2(BYTE *(&dstp)[3], int (&dstPitch)[3], const BYTE *(&srcp)[3], const int (&srcPitch)[3], int width, int height, const ConversionMatrix &m)
{
  // 16 bit uint16_t (unsigned range)
  // 32 bit float
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float shift = 0.5f;
#else
  const float shift = 0.0f;
#endif

  __m128  half_f = _mm_set1_ps(sizeof(pixel_t) == 4 ? shift : (float)(1u << (bits_per_pixel - 1)));
  __m128i limit  = _mm_set1_epi16((short)((1 << bits_per_pixel) - 1)); // 255
  __m128  offset_f = _mm_set1_ps(m.offset_y_f);

  __m128i zero = _mm_setzero_si128();

  const int rowsize = width * sizeof(pixel_t);
  for (int yy = 0; yy < height; yy++) {
    for (int x = 0; x < rowsize; x += 4 * sizeof(pixel_t)) {
      __m128 sum1, res;
      __m128 mul_y, mul_u, mul_v;
      __m128 mat_y, mat_u, mat_v;
      __m128 y, u, v;
      if constexpr(sizeof(pixel_t) == 4) {
        // float: load 16 bytes: 4 pixels
        y = _mm_load_ps(reinterpret_cast<const float *>(srcp[0] + x));
        u = _mm_load_ps(reinterpret_cast<const float *>(srcp[1] + x));
        v = _mm_load_ps(reinterpret_cast<const float *>(srcp[2] + x));
      }
      else {
        // uint16_t: load 8 bytes: 4 pixels
        __m128i yi = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[0] + x));
        __m128i ui = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[1] + x));
        __m128i vi = _mm_loadl_epi64(reinterpret_cast<const __m128i *>(srcp[2] + x));
        y = _mm_cvtepi32_ps(_mm_unpacklo_epi16(yi,zero));
        u = _mm_cvtepi32_ps(_mm_unpacklo_epi16(ui,zero));
        v = _mm_cvtepi32_ps(_mm_unpacklo_epi16(vi,zero));
      }
      y = _mm_add_ps(y, offset_f); // offset is negative
      u = _mm_sub_ps(u, half_f);
      v = _mm_sub_ps(v, half_f);
      // *G*
      mat_y = _mm_set1_ps(m.y_g_f);  mat_u = _mm_set1_ps(m.u_g_f); mat_v = _mm_set1_ps(m.v_g_f);
      mul_y = _mm_mul_ps(y, mat_y);
      mul_u = _mm_mul_ps(u, mat_u);
      mul_v = _mm_mul_ps(v, mat_v);
      sum1 = _mm_add_ps(mul_y, mul_u);
      res = _mm_add_ps(sum1, mul_v);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[0] + x), res);
      }
      else {
        __m128i resi = _mm_cvtps_epi32(res); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          resi = _mm_packus_epi32(resi, zero);
        else
          resi = _MM_PACKUS_EPI32(resi, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          resi = _mm_min_epi16(resi, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[0] + x), resi);
      }
      // *B*
      mat_y = _mm_set1_ps(m.y_b_f);  mat_u = _mm_set1_ps(m.u_b_f); mat_v = _mm_set1_ps(m.v_b_f);
      mul_y = _mm_mul_ps(y, mat_y);
      mul_u = _mm_mul_ps(u, mat_u);
      mul_v = _mm_mul_ps(v, mat_v);
      sum1 = _mm_add_ps(mul_y, mul_u);
      res = _mm_add_ps(sum1, mul_v);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[1] + x), res);
      }
      else {
        __m128i resi = _mm_cvtps_epi32(res); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          resi = _mm_packus_epi32(resi, zero);
        else
          resi = _MM_PACKUS_EPI32(resi, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          resi = _mm_min_epi16(resi, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[1] + x), resi);
      }
      // *R*
      mat_y = _mm_set1_ps(m.y_r_f);  mat_u = _mm_set1_ps(m.u_r_f); mat_v = _mm_set1_ps(m.v_r_f);
      mul_y = _mm_mul_ps(y, mat_y);
      mul_u = _mm_mul_ps(u, mat_u);
      mul_v = _mm_mul_ps(v, mat_v);
      sum1 = _mm_add_ps(mul_y, mul_u);
      res = _mm_add_ps(sum1, mul_v);
      if constexpr(sizeof(pixel_t) == 4) {
        // no clamp
        _mm_store_ps(reinterpret_cast<float *>(dstp[2] + x), res);
      }
      else {
        __m128i resi = _mm_cvtps_epi32(res); // no extra rounding, cvtps rounds to nearest
        if (hasSSE4)
          resi = _mm_packus_epi32(resi, zero);
        else
          resi = _MM_PACKUS_EPI32(resi, zero); // simulation
        if constexpr(bits_per_pixel<16) // albeit 10-14 bit have another function, make this general
          resi = _mm_min_epi16(resi, limit); // clamp 10,12,14 bit
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dstp[2] + x), resi);
      }
    }
    srcp[0] += srcPitch[0];
    srcp[1] += srcPitch[1];
    srcp[2] += srcPitch[2];
    dstp[0] += dstPitch[0];
    dstp[1] += dstPitch[1];
    dstp[2] += dstPitch[2];
  }
}

// packed rgb helper
static __forceinline __m128i convert_yuv_to_rgb_sse2_core(const __m128i &px01, const __m128i &px23, const __m128i &px45, const __m128i &px67, const __m128i& zero, const __m128i &matrix, const __m128i &round_mask) {
  //int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);

  //px01 - xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0

  __m128i low_lo  = _mm_madd_epi16(px01, matrix); //xx*0 + v1*m2 | u1*m1 + y1*m0 | xx*0 + v0*m2 | u0*m1 + y0*m0
  __m128i low_hi  = _mm_madd_epi16(px23, matrix); //xx*0 + v3*m2 | u3*m1 + y3*m0 | xx*0 + v2*m2 | u2*m1 + y2*m0
  __m128i high_lo = _mm_madd_epi16(px45, matrix); 
  __m128i high_hi = _mm_madd_epi16(px67, matrix); 

  __m128i low_v  = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(low_lo), _mm_castsi128_ps(low_hi), _MM_SHUFFLE(3, 1, 3, 1))); // v3*m2 | v2*m2 | v1*m2 | v0*m2
  __m128i high_v = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(high_lo), _mm_castsi128_ps(high_hi), _MM_SHUFFLE(3, 1, 3, 1))); 

  __m128i low_yu  = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(low_lo), _mm_castsi128_ps(low_hi), _MM_SHUFFLE(2, 0, 2, 0))); // u3*m1 + y3*m0 | u2*m1 + y2*m0 | u1*m1 + y1*m0 | u0*m1 + y0*m0
  __m128i high_yu = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(high_lo), _mm_castsi128_ps(high_hi), _MM_SHUFFLE(2, 0, 2, 0)));

  __m128i t_lo = _mm_add_epi32(low_v, low_yu); // v3*m2 + u3*m1 + y3*m0...
  __m128i t_hi = _mm_add_epi32(high_v, high_yu); 

  t_lo = _mm_add_epi32(t_lo, round_mask); // v3*m2 + u3*m1 + y3*m0 + 4096...
  t_hi = _mm_add_epi32(t_hi, round_mask);

  t_lo = _mm_srai_epi32(t_lo, 13); // (v3*m2 + u3*m1 + y3*m0 + 4096) >> 13...
  t_hi = _mm_srai_epi32(t_hi, 13); 

  __m128i result = _mm_packs_epi32(t_lo, t_hi); 
  result = _mm_packus_epi16(result, zero); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
  return result;
}

//todo: consider rewriting
template<int rgb_pixel_step, int instruction_set, bool hasAlpha>
static void convert_yv24_to_rgb_ssex(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE*srcV, const BYTE*srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix &matrix) {
  dstp += dst_pitch * (height-1);  // We start at last line

  size_t mod8_width = rgb_pixel_step == 3 ? width / 8 * 8 : width; // for rgb32 target we may process pixels beyond width, but we have 32bit alignment at target

  __m128i matrix_b = _mm_set_epi16(0, matrix.v_b, matrix.u_b, matrix.y_b, 0, matrix.v_b, matrix.u_b, matrix.y_b);
  __m128i matrix_g = _mm_set_epi16(0, matrix.v_g, matrix.u_g, matrix.y_g, 0, matrix.v_g, matrix.u_g, matrix.y_g);
  __m128i matrix_r = _mm_set_epi16(0, matrix.v_r, matrix.u_r, matrix.y_r, 0, matrix.v_r, matrix.u_r, matrix.y_r);

  __m128i zero = _mm_setzero_si128();
  __m128i round_mask = _mm_set1_epi32(4096);
  __m128i offset = _mm_set_epi16(0, -128, -128, matrix.offset_y, 0, -128, -128, matrix.offset_y);
  __m128i pixels0123_mask = _mm_set_epi8(0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
  __m128i pixels4567_mask = _mm_set_epi8(4, 2, 1, 0, 0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5);
  __m128i ssse3_merge_mask = _mm_set_epi32(0xFFFFFFFF, 0, 0, 0);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m128i src_y = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcY+x)); //0 0 0 0 0 0 0 0 Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
      __m128i src_u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcU+x)); //0 0 0 0 0 0 0 0 U7 U6 U5 U4 U3 U2 U1 U0
      __m128i src_v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcV+x)); //0 0 0 0 0 0 0 0 V7 V6 V5 V4 V3 V2 V1 V0
      __m128i src_a;
      if(hasAlpha)
        src_a = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcA+x)); //0 0 0 0 0 0 0 0 A7 A6 A5 A4 A3 A2 A1 A0

      __m128i t1 = _mm_unpacklo_epi8(src_y, src_u); //U7 Y7 U6 Y6 U5 Y5 U4 Y4 U3 Y3 U2 Y2 U1 Y1 U0 Y0
      __m128i t2 = _mm_unpacklo_epi8(src_v, zero);  //00 V7 00 V6 00 V5 00 V4 00 V3 00 V2 00 V1 00 V0

      __m128i low  = _mm_unpacklo_epi16(t1, t2); //xx V3 U3 Y3 xx V2 U2 Y2 xx V1 U1 Y1 xx V0 U0 Y0
      __m128i high = _mm_unpackhi_epi16(t1, t2); //xx V7 U7 Y7 xx V6 U6 Y6 xx V5 U5 Y5 xx V4 U4 Y4

      __m128i px01 = _mm_unpacklo_epi8(low, zero);  //xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0
      __m128i px23 = _mm_unpackhi_epi8(low, zero);  //xx xx 00 V3 00 U3 00 Y3 xx xx 00 V2 00 U2 00 Y2
      __m128i px45 = _mm_unpacklo_epi8(high, zero); //xx xx 00 V5 00 U5 00 Y5 xx xx 00 V4 00 U4 00 Y4
      __m128i px67 = _mm_unpackhi_epi8(high, zero); //xx xx 00 V7 00 U7 00 Y7 xx xx 00 V6 00 U6 00 Y6
      
      px01 = _mm_add_epi16(px01, offset); 
      px23 = _mm_add_epi16(px23, offset); 
      px45 = _mm_add_epi16(px45, offset);
      px67 = _mm_add_epi16(px67, offset);
      
      __m128i result_b = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_b, round_mask); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
      __m128i result_g = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_g, round_mask); //00 00 00 00 00 00 00 00 g7 g6 g5 g4 g3 g2 g1 g0
      __m128i result_r = convert_yuv_to_rgb_sse2_core(px01, px23, px45, px67, zero, matrix_r, round_mask); //00 00 00 00 00 00 00 00 r7 r6 r5 r4 r3 r2 r1 r0

      __m128i result_bg = _mm_unpacklo_epi8(result_b, result_g); //g7 b7 g6 b6 g5 b5 g4 b4 g3 b3 g2 b2 g1 b1 g0 b0
      __m128i alpha;
      if(hasAlpha)
        alpha = src_a; // a7 .. a0
      else
        alpha = _mm_cmpeq_epi32(result_r, result_r); // FF FF FF FF ... default alpha transparent

      __m128i result_ra = _mm_unpacklo_epi8(result_r, alpha);       //a7 r7 a6 r6 a5 r5 a4 r4 a3 r3 a2 r2 a1 r1 a0 r0

      __m128i result_lo = _mm_unpacklo_epi16(result_bg, result_ra);
      __m128i result_hi = _mm_unpackhi_epi16(result_bg, result_ra);

      if constexpr(rgb_pixel_step == 4) {
        //rgb32
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*4),    result_lo);
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*4+16), result_hi);
      } else {
        //rgb24
        if constexpr(instruction_set == CPUF_SSSE3) {
          //"fast" SSSE3 version
          __m128i px0123 = _mm_shuffle_epi8(result_lo, pixels0123_mask); //xxxx xxxx b3g3 r3b2 g2r2 b1g1 r1b0 g0r0
          __m128i dst567 = _mm_shuffle_epi8(result_hi, pixels4567_mask); //r5b4 g4r4 xxxx xxxx b7g7 r7b6 g6r6 b5g5

          __m128i dst012345 = _mm_or_si128(
            _mm_andnot_si128(ssse3_merge_mask, px0123),
            _mm_and_si128(ssse3_merge_mask, dst567)
            ); //r5b4 g4r4 b3g3 r3b2 g2r2 b1g1 r1b0 g0r0

          _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x*3), dst012345);
          _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*3+16), dst567);

        } else {
          alignas(16) BYTE temp[32];
          //slow SSE2 version
          _mm_store_si128(reinterpret_cast<__m128i*>(temp),    result_lo);
          _mm_store_si128(reinterpret_cast<__m128i*>(temp+16), result_hi);

          for (int i = 0; i < 8; ++i) {
            *reinterpret_cast<int*>(dstp + (x+i)*3) = *reinterpret_cast<int*>(temp+i*4);
          }
          //last pixel
          dstp[(x+7)*3+0] = temp[7*4+0];
          dstp[(x+7)*3+1] = temp[7*4+1];
          dstp[(x+7)*3+2] = temp[7*4+2];
        }
      }
    }

    if constexpr(rgb_pixel_step == 3) {
      // for rgb32 (pixel_step == 4) we processed full width and more, including padded 8 bytes
      for (size_t x = mod8_width; x < width; ++x) {
        int Y = srcY[x] + matrix.offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096) >> 13);
        int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096) >> 13);
        int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096) >> 13);
        dstp[x*rgb_pixel_step + 0] = PixelClip(b);
        dstp[x*rgb_pixel_step + 1] = PixelClip(g);
        dstp[x*rgb_pixel_step + 2] = PixelClip(r);
        if constexpr(rgb_pixel_step == 4) { // n/a
          dstp[x * 4 + 3] = 255;
        }
      }
    }
    dstp -= dst_pitch;
    srcY += src_pitch_y;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
    if(hasAlpha)
      srcA += src_pitch_a;
  }
}


#ifdef X86_32

static __forceinline __m64 convert_yuv_to_rgb_mmx_core(const __m64 &px0, const __m64 &px1, const __m64 &px2, const __m64 &px3, const __m64& zero, const __m64 &matrix, const __m64 &round_mask) {
  //int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);

  //px01 - xx xx 00 V0 00 U0 00 Y0

  __m64 low_lo  = _mm_madd_pi16(px0, matrix); //xx*0 + v1*m2 | u1*m1 + y1*m0 | xx*0 + v0*m2 | u0*m1 + y0*m0
  __m64 low_hi  = _mm_madd_pi16(px1, matrix); //xx*0 + v3*m2 | u3*m1 + y3*m0 | xx*0 + v2*m2 | u2*m1 + y2*m0
  __m64 high_lo = _mm_madd_pi16(px2, matrix); 
  __m64 high_hi = _mm_madd_pi16(px3, matrix); 

  __m64 low_v = _mm_unpackhi_pi32(low_lo, low_hi); // v1*m2 | v0*m2
  __m64 high_v = _mm_unpackhi_pi32(high_lo, high_hi);

  __m64 low_yu = _mm_unpacklo_pi32(low_lo, low_hi); // u1*m1 + y1*m0 | u0*m1 + y0*m0
  __m64 high_yu = _mm_unpacklo_pi32(high_lo, high_hi); 

  __m64 t_lo = _mm_add_pi32(low_v, low_yu); // v3*m2 + u3*m1 + y3*m0...
  __m64 t_hi = _mm_add_pi32(high_v, high_yu); 

  t_lo = _mm_add_pi32(t_lo, round_mask); // v3*m2 + u3*m1 + y3*m0 + 4096...
  t_hi = _mm_add_pi32(t_hi, round_mask);

  t_lo = _mm_srai_pi32(t_lo, 13); // (v3*m2 + u3*m1 + y3*m0 + 4096) >> 13...
  t_hi = _mm_srai_pi32(t_hi, 13); 

  __m64 result = _mm_packs_pi32(t_lo, t_hi); 
  result = _mm_packs_pu16(result, zero); //00 00 00 00 b3 b2 b1 b0
  return result;
}

template<int rgb_pixel_step>
static void convert_yv24_to_rgb_mmx(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE*srcV, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t width, size_t height, const ConversionMatrix &matrix) {
  dstp += dst_pitch * (height-1);  // We start at last line

  size_t mod4_width = rgb_pixel_step == 3 ? width / 4 * 4 : width;

  __m64 matrix_b = _mm_set_pi16(0, matrix.v_b, matrix.u_b, matrix.y_b);
  __m64 matrix_g = _mm_set_pi16(0, matrix.v_g, matrix.u_g, matrix.y_g);
  __m64 matrix_r = _mm_set_pi16(0, matrix.v_r, matrix.u_r, matrix.y_r);

  __m64 zero = _mm_setzero_si64();
  __m64 round_mask = _mm_set1_pi32(4096);
  __m64 ff = _mm_set1_pi32(0xFFFFFFFF);
  __m64 offset = _mm_set_pi16(0, -128, -128, matrix.offset_y);
  __m64 low_pixel_mask = _mm_set_pi32(0, 0x00FFFFFF);
  __m64 high_pixel_mask = _mm_set_pi32(0x00FFFFFF, 0);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod4_width; x+=4) {
      __m64 src_y = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcY+x)); //0 0 0 0 Y3 Y2 Y1 Y0
      __m64 src_u = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcU+x)); //0 0 0 0 U3 U2 U1 U0
      __m64 src_v = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcV+x)); //0 0 0 0 V3 V2 V1 V0

      __m64 t1 = _mm_unpacklo_pi8(src_y, src_u); //U3 Y3 U2 Y2 U1 Y1 U0 Y0
      __m64 t2 = _mm_unpacklo_pi8(src_v, zero);  //00 V3 00 V2 00 V1 00 V0

      __m64 low  = _mm_unpacklo_pi16(t1, t2); //xx V1 U1 Y1 xx V0 U0 Y0
      __m64 high = _mm_unpackhi_pi16(t1, t2); //xx V3 U3 Y3 xx V2 U2 Y2

      __m64 px0 = _mm_unpacklo_pi8(low, zero);  //xx xx 00 V0 00 U0 00 Y0
      __m64 px1 = _mm_unpackhi_pi8(low, zero);  //xx xx 00 V1 00 U1 00 Y1
      __m64 px2 = _mm_unpacklo_pi8(high, zero); //xx xx 00 V2 00 U2 00 Y2
      __m64 px3 = _mm_unpackhi_pi8(high, zero); //xx xx 00 V3 00 U3 00 Y3

      px0 = _mm_add_pi16(px0, offset); 
      px1 = _mm_add_pi16(px1, offset); 
      px2 = _mm_add_pi16(px2, offset);
      px3 = _mm_add_pi16(px3, offset);

      __m64 result_b = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_b, round_mask); //00 00 00 00 b3 b2 b1 b0
      __m64 result_g = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_g, round_mask); //00 00 00 00 g3 g2 g1 g0
      __m64 result_r = convert_yuv_to_rgb_mmx_core(px0, px1, px2, px3, zero, matrix_r, round_mask); //00 00 00 00 r3 r2 r1 r0

      __m64 result_bg = _mm_unpacklo_pi8(result_b, result_g); //g3 b3 g2 b2 g1 b1 g0 b0
      __m64 result_ra = _mm_unpacklo_pi8(result_r, ff);       //a3 r3 a2 r2 a1 r1 a0 r0

      __m64 result_lo = _mm_unpacklo_pi16(result_bg, result_ra);
      __m64 result_hi = _mm_unpackhi_pi16(result_bg, result_ra);

      if (rgb_pixel_step == 4) {
        //rgb32
        *reinterpret_cast<__m64*>(dstp+x*4) = result_lo;
        *reinterpret_cast<__m64*>(dstp+x*4+8) = result_hi;
      } else {
        __m64 p0 = _mm_and_si64(result_lo, low_pixel_mask); //0000 0000 00r0 g0b0
        __m64 p1 = _mm_and_si64(result_lo, high_pixel_mask); //00r1 g1b1 0000 0000
        __m64 p2 = _mm_and_si64(result_hi, low_pixel_mask); //0000 0000 00r2 g2b2
        __m64 p3 = _mm_and_si64(result_hi, high_pixel_mask); //00r3 g3b3 0000 0000

        __m64 dst01 = _mm_or_si64(p0, _mm_srli_si64(p1, 8)); //0000 r1g1 b1r0 g0b0
        p3 = _mm_srli_si64(p3, 24); //0000 0000 r3g3 b300

        __m64 dst012 = _mm_or_si64(dst01, _mm_slli_si64(p2, 48));  //g2b2 r1g1 b1r0 g0b0
        __m64 dst23 = _mm_or_si64(p3, _mm_srli_si64(p2, 16)); //0000 0000 r3g3 b3r2

        *reinterpret_cast<__m64*>(dstp+x*3) = dst012;
        *reinterpret_cast<int*>(dstp+x*3+8) = _mm_cvtsi64_si32(dst23);
      }
    }

    if (rgb_pixel_step == 3) {
      for (size_t x = mod4_width; x < width; ++x) {
        int Y = srcY[x] + matrix.offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096) >> 13);
        int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096) >> 13);
        int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096) >> 13);
        dstp[x*rgb_pixel_step + 0] = PixelClip(b);
        dstp[x*rgb_pixel_step + 1] = PixelClip(g);
        dstp[x*rgb_pixel_step + 2] = PixelClip(r);
        if (rgb_pixel_step == 4) {
          dstp[x * 4 + 3] = 255;
        }
      }
    }

    dstp -= dst_pitch;
    srcY += src_pitch_y;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
  }
  _mm_empty();
}

#endif

PVideoFrame __stdcall ConvertYUV444ToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  //PVideoFrame dst = env->NewVideoFrame(vi, 8); // PF: why 8? A larger default Avisynth align should work fine
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  const BYTE* srcA = src->GetReadPtr(PLANAR_A);

  BYTE* dstp = dst->GetWritePtr();

  const int src_pitch_y = src->GetPitch(PLANAR_Y);
  const int src_pitch_uv = src->GetPitch(PLANAR_U);
  const int src_pitch_a = src->GetPitch(PLANAR_A); // zero if no Alpha

  const int dst_pitch = dst->GetPitch();

  /*if (pixel_step != 4 && pixel_step != 3 && pixel_step != 8 && pixel_step != 6 && pixel_step != -1 && pixel_step != -2) {
    env->ThrowError("Invalid pixel step. This is a bug.");
  }*/

  // todo: SSE for not only 8 bit RGB
  // packed RGB24 and RGB32
  if ((env->GetCPUFlags() & CPUF_SSE2) && (pixel_step==3 || pixel_step==4)) {
    //we load using movq so no need to check for alignment
    if (pixel_step == 4) {
      if(src_pitch_a) // move alpha channel from YUVA
        convert_yv24_to_rgb_ssex<4, CPUF_SSE2, true>(dstp, srcY, srcU, srcV, srcA, dst_pitch, src_pitch_y, src_pitch_uv, src_pitch_a, vi.width, vi.height, matrix);
      else
        convert_yv24_to_rgb_ssex<4, CPUF_SSE2, false>(dstp, srcY, srcU, srcV, srcA, dst_pitch, src_pitch_y, src_pitch_uv, src_pitch_a, vi.width, vi.height, matrix);
    } else {
      if (env->GetCPUFlags() & CPUF_SSSE3) {
        convert_yv24_to_rgb_ssex<3, CPUF_SSSE3, false>(dstp, srcY, srcU, srcV, srcA, dst_pitch, src_pitch_y, src_pitch_uv, src_pitch_a, vi.width, vi.height, matrix);
      } else {
        convert_yv24_to_rgb_ssex<3, CPUF_SSE2, false>(dstp, srcY, srcU, srcV, srcA, dst_pitch, src_pitch_y, src_pitch_uv, src_pitch_a, vi.width, vi.height, matrix);
      }
    }
    return dst;
  }

#ifdef X86_32
  // packed RGB24 and RGB32
  if ((src_pitch_a==0) && (env->GetCPUFlags() & CPUF_MMX) && (pixel_step==3 || pixel_step==4)) {
    if (pixel_step == 4) {
      convert_yv24_to_rgb_mmx<4>(dstp, srcY, srcU, srcV, dst_pitch, src_pitch_y, src_pitch_uv, vi.width, vi.height, matrix);
    } else {
      convert_yv24_to_rgb_mmx<3>(dstp, srcY, srcU, srcV, dst_pitch, src_pitch_y, src_pitch_uv, vi.width, vi.height, matrix);
    }
    return dst;
  }
#endif

  //Slow C-code.

  dstp += dst_pitch * (vi.height-1);  // We start at last line. Not for Planar RGB
  bool srcHasAlpha = (src_pitch_a != 0);
  if (pixel_step == 4) { // RGB32
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + matrix.offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        uint8_t a = srcHasAlpha ? srcA[x] : 255; // YUVA aware
        int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096)>>13);
        int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096)>>13);
        int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096)>>13);
        dstp[x*4+0] = PixelClip(b);  // All the safety we can wish for.
        dstp[x*4+1] = PixelClip(g);  // Probably needed here.
        dstp[x*4+2] = PixelClip(r);
        dstp[x*4+3] = a; // alpha
      }
      dstp -= dst_pitch;
      srcY += src_pitch_y;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
      srcA += src_pitch_a;
    }
  } else if (pixel_step == 3) { // RGB24
    const int Dstep = dst_pitch + (vi.width * pixel_step);
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + matrix.offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096)>>13);
        int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096)>>13);
        int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096)>>13);
        dstp[0] = PixelClip(b);  // All the safety we can wish for.
        dstp[1] = PixelClip(g);  // Probably needed here.
        dstp[2] = PixelClip(r);
        dstp += pixel_step;
      }
      dstp -= Dstep;
      srcY += src_pitch_y;
      srcU += src_pitch_uv;
      srcV += src_pitch_uv;
    }
  } else if (pixel_step == 8) { // RGB64
    for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
            int Y = reinterpret_cast<const uint16_t *>(srcY)[x] + matrix.offset_y;
            int U = reinterpret_cast<const uint16_t *>(srcU)[x] - 32768;
            int V = reinterpret_cast<const uint16_t *>(srcV)[x] - 32768;
            uint16_t a = srcHasAlpha ? reinterpret_cast<const uint16_t *>(srcA)[x] : 65535; // YUVA aware
            int b = (((__int64)matrix.y_b * Y + (__int64)matrix.u_b * U + (__int64)matrix.v_b * V + 4096)>>13);
            int g = (((__int64)matrix.y_g * Y + (__int64)matrix.u_g * U + (__int64)matrix.v_g * V + 4096)>>13);
            int r = (((__int64)matrix.y_r * Y + (__int64)matrix.u_r * U + (__int64)matrix.v_r * V + 4096)>>13);
            reinterpret_cast<uint16_t *>(dstp)[x*4+0] = clamp(b,0,65535);  // All the safety we can wish for.
            reinterpret_cast<uint16_t *>(dstp)[x*4+1] = clamp(g,0,65535);  // Probably needed here.
            reinterpret_cast<uint16_t *>(dstp)[x*4+2] = clamp(r,0,65535);
            reinterpret_cast<uint16_t *>(dstp)[x*4+3] = a; // alpha
        }
        dstp -= dst_pitch;
        srcY += src_pitch_y;
        srcU += src_pitch_uv;
        srcV += src_pitch_uv;
        srcA += src_pitch_a;
    }
  } else if (pixel_step == 6) { // RGB48
    const int Dstep = dst_pitch + (vi.width * pixel_step);
    for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
            int Y = reinterpret_cast<const uint16_t *>(srcY)[x] + matrix.offset_y;
            int U = reinterpret_cast<const uint16_t *>(srcU)[x] - 32768;
            int V = reinterpret_cast<const uint16_t *>(srcV)[x] - 32768;
            int b = (((__int64)matrix.y_b * Y + (__int64)matrix.u_b * U + (__int64)matrix.v_b * V + 4096)>>13);
            int g = (((__int64)matrix.y_g * Y + (__int64)matrix.u_g * U + (__int64)matrix.v_g * V + 4096)>>13);
            int r = (((__int64)matrix.y_r * Y + (__int64)matrix.u_r * U + (__int64)matrix.v_r * V + 4096)>>13);
            reinterpret_cast<uint16_t *>(dstp)[0] = clamp(b,0,65535);  // All the safety we can wish for.
            reinterpret_cast<uint16_t *>(dstp)[1] = clamp(g,0,65535);  // Probably needed here.
            reinterpret_cast<uint16_t *>(dstp)[2] = clamp(r,0,65535);
            dstp += pixel_step;
        }
        dstp -= Dstep;
        srcY += src_pitch_y;
        srcU += src_pitch_uv;
        srcV += src_pitch_uv;
    }
  } else if(pixel_step < 0) // -1: RGBP  -2:RGBAP
  {
      // YUV444 -> PlanarRGB
      // YUVA444 -> PlanarRGBA
    bool targetHasAlpha = pixel_step == -2;

    BYTE *dstpG = dst->GetWritePtr(PLANAR_G);
    BYTE *dstpB = dst->GetWritePtr(PLANAR_B);
    BYTE *dstpR = dst->GetWritePtr(PLANAR_R);

    // copy or fill alpha
    BYTE *dstpA;
    if (targetHasAlpha) {
      dstpA = dst->GetWritePtr(PLANAR_A);
      int heightA = dst->GetHeight(PLANAR_A);
      int dst_pitchA = dst->GetPitch(PLANAR_A);
        // simple copy
      if(src->GetRowSize(PLANAR_A)) // vi.IsYUVA() no-no! vi is already the target video type
        env->BitBlt(dstpA, dst_pitchA, src->GetReadPtr(PLANAR_A), src->GetPitch(PLANAR_A), src->GetRowSize(PLANAR_A_ALIGNED), src->GetHeight(PLANAR_A));
      else {
        // fill default transparency
        switch (vi.ComponentSize())
        {
        case 1:
          fill_plane<BYTE>(dstpA, heightA, dst_pitchA, 255);
          break;
        case 2:
          fill_plane<uint16_t>(dstpA, heightA, dst_pitchA, (1 << vi.BitsPerComponent()) - 1);
          break;
        case 4:
          fill_plane<float>(dstpA, heightA, dst_pitchA, 1.0f);
          break;
        }
      }
    }

    int dst_pitchG = dst->GetPitch(PLANAR_G);
    int dst_pitchB = dst->GetPitch(PLANAR_B);
    int dst_pitchR = dst->GetPitch(PLANAR_R);
    int dst_pitchA = dst->GetPitch(PLANAR_A);

    int pixelsize = vi.ComponentSize();
    int bits_per_pixel = vi.BitsPerComponent();

    const BYTE *srcp[3] = { src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V) };
    const int srcPitch[3] = { src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), src->GetPitch(PLANAR_V) };

    BYTE *dstp[3] = { dstpG, dstpB, dstpR };
    int dstPitch[3] = { dst_pitchG, dst_pitchB, dst_pitchR };
    if (bits_per_pixel < 16 && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp[0], 16) && IsPtrAligned(dstp[0], 16))
    {
      switch (bits_per_pixel) {
      case 8: convert_yuv_to_planarrgb_uint8_14_sse2<uint8_t, 8>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 10: convert_yuv_to_planarrgb_uint8_14_sse2<uint16_t, 10>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 12: convert_yuv_to_planarrgb_uint8_14_sse2<uint16_t, 12>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      case 14: convert_yuv_to_planarrgb_uint8_14_sse2<uint16_t, 14>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix); break;
      }
      return dst;
    }
    if (bits_per_pixel >= 16 && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp[0], 16) && IsPtrAligned(dstp[0], 16)) {
      if (pixelsize == 4) // float 32 bit
        convert_yuv_to_planarrgb_uint16_float_sse2<float, 8 /*n/a*/, false>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      else if (env->GetCPUFlags() & CPUF_SSE4)
        convert_yuv_to_planarrgb_uint16_float_sse2<uint16_t, 16, true>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      else
        convert_yuv_to_planarrgb_uint16_float_sse2<uint16_t, 16, false>(dstp, dstPitch, srcp, srcPitch, vi.width, vi.height, matrix);
      return dst;
    }

    // todo: template for integers
    if(pixelsize==1)
    {
      for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
          int Y = reinterpret_cast<const uint8_t *>(srcY)[x] + matrix.offset_y;
          int U = reinterpret_cast<const uint8_t *>(srcU)[x] - 128;
          int V = reinterpret_cast<const uint8_t *>(srcV)[x] - 128;
          int A = 0;
          if(targetHasAlpha)
            A = srcHasAlpha ? reinterpret_cast<const uint8_t *>(srcA)[x] : 255;
          int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096)>>13);
          int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096)>>13);
          int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096)>>13);
          reinterpret_cast<uint8_t *>(dstpB)[x] = clamp(b,0,255);  // All the safety we can wish for.
          reinterpret_cast<uint8_t *>(dstpG)[x] = clamp(g,0,255);  // Probably needed here.
          reinterpret_cast<uint8_t *>(dstpR)[x] = clamp(r,0,255);
          if(targetHasAlpha)
            reinterpret_cast<uint8_t *>(dstpA)[x] = A;
        }
        dstpG += dst_pitchG;
        dstpB += dst_pitchB;
        dstpR += dst_pitchR;
        if(targetHasAlpha)
          dstpA += dst_pitchA;
        srcY += src_pitch_y;
        srcU += src_pitch_uv;
        srcV += src_pitch_uv;
      }
    } else if (pixelsize==2) {
      int bits_per_pixel = vi.BitsPerComponent();
      int half_pixel_value = 1 << (bits_per_pixel - 1);
      int max_pixel_value = (1 << bits_per_pixel) - 1;
      for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
          int Y = reinterpret_cast<const uint16_t *>(srcY)[x] + matrix.offset_y;
          int U = reinterpret_cast<const uint16_t *>(srcU)[x] - half_pixel_value;
          int V = reinterpret_cast<const uint16_t *>(srcV)[x] - half_pixel_value;
          int A;
          if(targetHasAlpha)
            A = srcHasAlpha ? reinterpret_cast<const uint16_t *>(srcA)[x] : max_pixel_value;
          // __int64 needed for 16 bit pixels
          int b = (((__int64)matrix.y_b * Y + (__int64)matrix.u_b * U + (__int64)matrix.v_b * V + 4096)>>13);
          int g = (((__int64)matrix.y_g * Y + (__int64)matrix.u_g * U + (__int64)matrix.v_g * V + 4096)>>13);
          int r = (((__int64)matrix.y_r * Y + (__int64)matrix.u_r * U + (__int64)matrix.v_r * V + 4096)>>13);
          reinterpret_cast<uint16_t *>(dstpB)[x] = clamp(b,0,max_pixel_value);  // All the safety we can wish for.
          reinterpret_cast<uint16_t *>(dstpG)[x] = clamp(g,0,max_pixel_value);  // Probably needed here.
          reinterpret_cast<uint16_t *>(dstpR)[x] = clamp(r,0,max_pixel_value);
          if(targetHasAlpha)
            reinterpret_cast<uint16_t *>(dstpA)[x] = A;
        }
        dstpG += dst_pitchG;
        dstpB += dst_pitchB;
        dstpR += dst_pitchR;
        if(targetHasAlpha)
          dstpA += dst_pitchA;
        srcY += src_pitch_y;
        srcU += src_pitch_uv;
        srcV += src_pitch_uv;
      }
    } else { // pixelsize==4 float
      for (int y = 0; y < vi.height; y++) {
        for (int x = 0; x < vi.width; x++) {
          float Y = reinterpret_cast<const float *>(srcY)[x] + matrix.offset_y_f;
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
          const float shift = 0.5f;
#else
          const float shift = 0.0f;
#endif
          float U = reinterpret_cast<const float *>(srcU)[x] - shift;
          float V = reinterpret_cast<const float *>(srcV)[x] - shift;
          float A;
          if(targetHasAlpha)
            A = srcHasAlpha ? reinterpret_cast<const float *>(srcA)[x] : 1.0f;
          float b = matrix.y_b_f * Y + matrix.u_b_f * U + matrix.v_b_f * V;
          float g = matrix.y_g_f * Y + matrix.u_g_f * U + matrix.v_g_f * V;
          float r = matrix.y_r_f * Y + matrix.u_r_f * U + matrix.v_r_f * V;
          reinterpret_cast<float *>(dstpB)[x] = clamp(b, 0.0f, 1.0f);  // All the safety we can wish for.
          reinterpret_cast<float *>(dstpG)[x] = clamp(g, 0.0f, 1.0f);  // Probably needed here.
          reinterpret_cast<float *>(dstpR)[x] = clamp(r, 0.0f, 1.0f);
          if(targetHasAlpha)
            reinterpret_cast<float *>(dstpA)[x] = A;
        }
        dstpG += dst_pitchG;
        dstpB += dst_pitchB;
        dstpR += dst_pitchR;
        if(targetHasAlpha)
          dstpA += dst_pitchA;
        srcY += src_pitch_y;
        srcU += src_pitch_uv;
        srcV += src_pitch_uv;
      }
    }
  }
  return dst;
}

/************************************
 * YUY2 to YV16
 ************************************/

ConvertYUY2ToYV16::ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYUY2())
    env->ThrowError("ConvertYUY2ToYV16: Only YUY2 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YV16;

}


static void convert_yuy2_to_yv16_sse2(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  width /= 2;

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x += 8) {
      __m128i p0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x * 4));      // V3 Y7 U3 Y6 V2 Y5 U2 Y4 V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m128i p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x * 4 + 16)); // V7 Yf U7 Ye V6 Yd U6 Yc V5 Yb U5 Ya V4 Y9 U4 Y8

      __m128i p2 = _mm_unpacklo_epi8(p0, p1); // V5 V1 Yb Y3 U5 U1 Ya Y2 V4 V0 Y9 Y1 U4 U0 Y8 Y0
      __m128i p3 = _mm_unpackhi_epi8(p0, p1); // V7 V3 Yf Y7 U7 U3 Ye Y6 V6 V2 Yd Y5 U6 U2 Yc Y4

      p0 = _mm_unpacklo_epi8(p2, p3); // V6 V4 V2 V0 Yd Y9 Y5 Y1 U6 U4 U2 U0 Yc Y8 Y4 Y0
      p1 = _mm_unpackhi_epi8(p2, p3); // V7 V5 V3 V1 Yf Yb Y7 Y3 U7 U5 U3 U1 Ye Ya Y6 Y2

      p2 = _mm_unpacklo_epi8(p0, p1); // U7 U6 U5 U4 U3 U2 U1 U0 Ye Yc Ya Y8 Y6 Y4 Y2 Y0
      p3 = _mm_unpackhi_epi8(p0, p1); // V7 V6 V5 V4 V3 V2 V1 V0 Yf Yd Yb Y9 Y7 Y5 Y3 Y1

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_u + x), _mm_srli_si128(p2, 8));
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp_v + x), _mm_srli_si128(p3, 8));
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp_y + x * 2), _mm_unpacklo_epi8(p2, p3));
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
}


#ifdef X86_32

static void convert_yuy2_to_yv16_mmx(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  width /= 2;

  for (size_t y = 0; y < height; ++y) { 
    for (size_t x = 0; x < width; x += 4) {
      __m64 p0 = *reinterpret_cast<const __m64*>(srcp + x * 4);     // V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 p1 = *reinterpret_cast<const __m64*>(srcp + x * 4 + 8); // V3 Y7 U3 Y6 V2 Y5 U2 Y4

      __m64 p2 = _mm_unpacklo_pi8(p0, p1); // V2 V0 Y5 Y1 U2 U0 Y4 Y0
      __m64 p3 = _mm_unpackhi_pi8(p0, p1); // V3 V1 Y7 Y3 U3 U1 Y6 Y2

      p0 = _mm_unpacklo_pi8(p2, p3); // U3 U2 U1 U0 Y6 Y4 Y2 Y0
      p1 = _mm_unpackhi_pi8(p2, p3); // V3 V2 V1 V0 Y7 Y5 Y3 Y1

      *reinterpret_cast<int*>(dstp_u + x) = _mm_cvtsi64_si32(_mm_srli_si64(p0, 4));
      *reinterpret_cast<int*>(dstp_v + x) = _mm_cvtsi64_si32(_mm_srli_si64(p1, 4));
      *reinterpret_cast<__m64*>(dstp_y + x * 2) = _mm_unpacklo_pi8(p0, p1);
    }

    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
  _mm_empty();
}

#endif

static void convert_yuy2_to_yv16_c(const BYTE *srcp, BYTE *dstp_y, BYTE *dstp_u, BYTE *dstp_v, size_t src_pitch, size_t dst_pitch_y, size_t dst_pitch_uv, size_t width, size_t height)
{
  width /= 2;

  for (size_t y = 0; y < height; ++y) { 
    for (size_t x = 0; x < width; ++x) {
      dstp_y[x * 2]     = srcp[x * 4 + 0];
      dstp_y[x * 2 + 1] = srcp[x * 4 + 2];
      dstp_u[x]         = srcp[x * 4 + 1];
      dstp_v[x]         = srcp[x * 4 + 3];
    }
    srcp += src_pitch;
    dstp_y += dst_pitch_y;
    dstp_u += dst_pitch_uv;
    dstp_v += dst_pitch_uv;
  }
}

PVideoFrame __stdcall ConvertYUY2ToYV16::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcP = src->GetReadPtr();

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcP, 16)) {
    convert_yuy2_to_yv16_sse2(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  } 
  else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) { 
    convert_yuy2_to_yv16_mmx(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
  } else
#endif
  {
    convert_yuy2_to_yv16_c(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), vi.width, vi.height);
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

void convert_yv16_to_yuy2_sse2(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height)
{
  width /= 2;

  for (size_t yy=0; yy<height; yy++) { 
    for (size_t x=0; x<width; x+=8) {
      
      __m128i y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp_y + x*2));
      __m128i u = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp_u + x));
      __m128i v = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp_v + x));

      __m128i uv = _mm_unpacklo_epi8(u, v);
      __m128i yuv_lo = _mm_unpacklo_epi8(y, uv);
      __m128i yuv_hi = _mm_unpackhi_epi8(y, uv);

      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x*4), yuv_lo);
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x*4 + 16), yuv_hi);
    }

    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
}

#ifdef X86_32
void convert_yv16_to_yuy2_mmx(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height)
{
  width /= 2;

  for (size_t y=0; y<height; y++) { 
    for (size_t x=0; x<width; x+=4) {
      __m64 y = *reinterpret_cast<const __m64*>(srcp_y + x*2);
      __m64 u = *reinterpret_cast<const __m64*>(srcp_u + x);
      __m64 v = *reinterpret_cast<const __m64*>(srcp_v + x);

      __m64 uv = _mm_unpacklo_pi8(u, v);
      __m64 yuv_lo = _mm_unpacklo_pi8(y, uv);
      __m64 yuv_hi = _mm_unpackhi_pi8(y, uv);

      *reinterpret_cast<__m64*>(dstp + x*4) = yuv_lo;
      *reinterpret_cast<__m64*>(dstp + x*4+8) = yuv_hi;
    }

    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
  _mm_empty();
}
#endif

void convert_yv16_to_yuy2_c(const BYTE *srcp_y, const BYTE *srcp_u, const BYTE *srcp_v, BYTE *dstp, size_t src_pitch_y, size_t src_pitch_uv, size_t dst_pitch, size_t width, size_t height) {
  for (size_t y=0; y < height; y++) {
    for (size_t x=0; x < width / 2; x++) {
      dstp[x*4+0] = srcp_y[x*2];
      dstp[x*4+1] = srcp_u[x];
      dstp[x*4+2] = srcp_y[x*2+1];
      dstp[x*4+3] = srcp_v[x];
    }
    srcp_y += src_pitch_y;
    srcp_u += src_pitch_uv;
    srcp_v += src_pitch_uv;
    dstp += dst_pitch;
  }
}

PVideoFrame __stdcall ConvertYV16ToYUY2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 32);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  BYTE* dstp = dst->GetWritePtr();

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcY, 16)) {
    //U and V don't have to be aligned since we user movq to read from those
    convert_yv16_to_yuy2_sse2(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), vi.width, vi.height);
  } else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) { 
    convert_yv16_to_yuy2_mmx(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), vi.width, vi.height);
  } else
#endif
  {
    convert_yv16_to_yuy2_c(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), dst->GetPitch(), vi.width, vi.height);
  }
  
  return dst;
}


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

ConvertToPlanarGeneric::ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                                               const AVSValue& InPlacement, const AVSValue& chromaResampler,
                                               const AVSValue& OutPlacement, IScriptEnvironment* env) : GenericVideoFilter(src) {
  Yinput = vi.NumComponents() == 1;
  pixelsize = vi.ComponentSize();

  if (Yinput) {
    vi.pixel_type = dst_space;
    if (vi.ComponentSize() != pixelsize)
      env->ThrowError("Convert: Conversion from %d to %d-byte format not supported.", pixelsize, vi.ComponentSize());
    return;
  }

  auto Is420 = [](int pix_type) {
    return pix_type == VideoInfo::CS_YV12 || pix_type == VideoInfo::CS_I420 ||
      pix_type == VideoInfo::CS_YUV420P10 || pix_type == VideoInfo::CS_YUV420P12 ||
      pix_type == VideoInfo::CS_YUV420P14 || pix_type == VideoInfo::CS_YUV420P16 ||
      pix_type == VideoInfo::CS_YUV420PS ||
        pix_type == VideoInfo::CS_YUVA420 ||
      pix_type == VideoInfo::CS_YUVA420P10 || pix_type == VideoInfo::CS_YUVA420P12 ||
      pix_type == VideoInfo::CS_YUVA420P14 || pix_type == VideoInfo::CS_YUVA420P16 ||
      pix_type == VideoInfo::CS_YUVA420PS;
  };

  if (!Is420(vi.pixel_type) && !Is420(dst_space))
    interlaced = false;  // Ignore, if YV12 is not involved.
  //if (interlaced) env->ThrowError("Convert: Interlaced only available with 4:2:0 color spaces.");

  // Describe input pixel positioning
  float xdInU = 0.0f, txdInU = 0.0f, bxdInU = 0.0f;
  float ydInU = 0.0f, tydInU = 0.0f, bydInU = 0.0f;
  float xdInV = 0.0f, txdInV = 0.0f, bxdInV = 0.0f;
  float ydInV = 0.0f, tydInV = 0.0f, bydInV = 0.0f;

  if (Is420(vi.pixel_type)) {
    switch (getPlacement(InPlacement, env)) {
      case PLACEMENT_DV:
        ydInU = 1.0f, tydInU = 1.0f, bydInU = 1.0f; // Cb
        ydInV = 0.0f, tydInV = 0.0f, bydInV = 0.0f; // Cr
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
    env->ThrowError("Convert: Input ChromaPlacement only available with 4:2:0 source.");

  const int xsIn = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
  const int ysIn = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);

  vi.pixel_type = dst_space;

  if (vi.ComponentSize() != pixelsize)
    env->ThrowError("Convert: Conversion from %d to %d-byte format not supported.", pixelsize, vi.ComponentSize());

  // Describe output pixel positioning
  float xdOutU = 0.0f, txdOutU = 0.0f, bxdOutU = 0.0f;
  float ydOutU = 0.0f, tydOutU = 0.0f, bydOutU = 0.0f;
  float xdOutV = 0.0f, txdOutV = 0.0f, bxdOutV = 0.0f;
  float ydOutV = 0.0f, tydOutV = 0.0f, bydOutV = 0.0f;

  if (Is420(vi.pixel_type)) {
    switch (getPlacement(OutPlacement, env)) {
      case PLACEMENT_DV:
        ydOutU = 1.0f, tydOutU = 1.0f, bydOutU = 1.0f; // Cb
        ydOutV = 0.0f, tydOutV = 0.0f, bydOutV = 0.0f; // Cr
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
    env->ThrowError("Convert: Output ChromaPlacement only available with 4:2:0 output.");

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

  auto ChrOffset = [P](int sIn, float dIn, int sOut, float dOut) {
    //     (1 - sOut/sIn)/2 + (dOut-dIn)/sIn; // Gavino Jan 2011
    return P ? (dOut - dIn) / sIn : 0.5f + (dOut - dIn - 0.5f*sOut) / sIn;
  };

  if (interlaced) {
    uv_height /=  2;

    AVSValue tUsubSampling[4] = { ChrOffset(xsIn, txdInU, xsOut, txdOutU), ChrOffset(ysIn, tydInU, ysOut, tydOutU), AVSValue(), AVSValue() };
    AVSValue bUsubSampling[4] = { ChrOffset(xsIn, bxdInU, xsOut, bxdOutU), ChrOffset(ysIn, bydInU, ysOut, bydOutU), AVSValue(), AVSValue() };
    AVSValue tVsubSampling[4] = { ChrOffset(xsIn, txdInV, xsOut, txdOutV), ChrOffset(ysIn, tydInV, ysOut, tydOutV), AVSValue(), AVSValue() };
    AVSValue bVsubSampling[4] = { ChrOffset(xsIn, bxdInV, xsOut, bxdOutV), ChrOffset(ysIn, bydInV, ysOut, bydOutV), AVSValue(), AVSValue() };

    Usource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::UToY8, env), true), env); // also works for Y16/Y32
    Vsource = new SeparateFields(new AssumeParity(new SwapUVToY(child, SwapUVToY::VToY8, env), true), env); // also works for Y16/Y32

    PClip *tbUsource = new PClip[2]; // Interleave()::~Interleave() will delete these
    PClip *tbVsource = new PClip[2];

  tbUsource[0] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 0, env), uv_width, uv_height, tUsubSampling, filter, env);
  tbUsource[1] = FilteredResize::CreateResize(new SelectEvery(Usource, 2, 1, env), uv_width, uv_height, bUsubSampling, filter, env);
  tbVsource[0] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 0, env), uv_width, uv_height, tVsubSampling, filter, env);
  tbVsource[1] = FilteredResize::CreateResize(new SelectEvery(Vsource, 2, 1, env), uv_width, uv_height, bVsubSampling, filter, env);

  Usource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbUsource, env)), 2, 0, env);
  Vsource = new SelectEvery(new DoubleWeaveFields(new Interleave(2, tbVsource, env)), 2, 0, env);
  }
  else {
    AVSValue UsubSampling[4] = { ChrOffset(xsIn, xdInU, xsOut, xdOutU), ChrOffset(ysIn, ydInU, ysOut, ydOutU), AVSValue(), AVSValue() };
    AVSValue VsubSampling[4] = { ChrOffset(xsIn, xdInV, xsOut, xdOutV), ChrOffset(ysIn, ydInV, ysOut, ydOutV), AVSValue(), AVSValue() };

    Usource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::UToY8, env), uv_width, uv_height, UsubSampling, filter, env);
    Vsource = FilteredResize::CreateResize(new SwapUVToY(child, SwapUVToY::VToY8, env), uv_width, uv_height, VsubSampling, filter, env);
  }
  delete filter;
}

template <typename pixel_t>
void fill_chroma(BYTE* dstp_u, BYTE* dstp_v, int height, int pitch, pixel_t val)
{
  size_t size = height * pitch / sizeof(pixel_t);
  std::fill_n(reinterpret_cast<pixel_t*>(dstp_u), size, val);
  std::fill_n(reinterpret_cast<pixel_t*>(dstp_v), size, val);
}

template <typename pixel_t>
void fill_plane(BYTE* dstp, int height, int pitch, pixel_t val)
{
  size_t size = height * pitch / sizeof(pixel_t);
  std::fill_n(reinterpret_cast<pixel_t*>(dstp), size, val);
}


PVideoFrame __stdcall ConvertToPlanarGeneric::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
              src->GetRowSize(PLANAR_Y_ALIGNED), src->GetHeight(PLANAR_Y));

  // alpha. if pitch is zero -> no alpha channel
  const int dst_pitchA = dst->GetPitch(PLANAR_A);
  BYTE* dstp_a = (dst_pitchA == 0) ? nullptr : dst->GetWritePtr(PLANAR_A);
  const int heightA = dst->GetHeight(PLANAR_A);

  if (dst_pitchA != 0)
  {
    if (src->GetPitch(PLANAR_A) != 0)
      env->BitBlt(dstp_a, dst_pitchA, src->GetReadPtr(PLANAR_A), src->GetPitch(PLANAR_A),
        src->GetRowSize(PLANAR_A_ALIGNED), src->GetHeight(PLANAR_A));
    else {
      switch (vi.ComponentSize())
      {
      case 1:
        fill_plane<BYTE>(dstp_a, heightA, dst_pitchA, 255);
        break;
      case 2:
        fill_plane<uint16_t>(dstp_a, heightA, dst_pitchA, (1 << vi.BitsPerComponent()) - 1);
        break;
      case 4:
        fill_plane<float>(dstp_a, heightA, dst_pitchA, 1.0f);
        break;
      }
    }
  }

  BYTE* dstp_u = dst->GetWritePtr(PLANAR_U);
  BYTE* dstp_v = dst->GetWritePtr(PLANAR_V);
  const int height = dst->GetHeight(PLANAR_U);
  const int dst_pitch = dst->GetPitch(PLANAR_U);

  if (Yinput) {
    switch (vi.ComponentSize())
    {
      case 1:
        fill_chroma<BYTE>(dstp_u, dstp_v, height, dst_pitch, 0x80);
        break;
      case 2:
        fill_chroma<uint16_t>(dstp_u, dstp_v, height, dst_pitch, 1 << (vi.BitsPerComponent() - 1));
        break;
      case 4:
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
        const float half = 0.5f;
#else
        const float half = 0.0f;
#endif
        fill_chroma<float>(dstp_u, dstp_v, height, dst_pitch, half);
        break;
    }
  } else {
    src = Usource->GetFrame(n, env);
    env->BitBlt(dstp_u, dst_pitch, src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y_ALIGNED), height);
    src = Vsource->GetFrame(n, env);
    env->BitBlt(dstp_v, dst_pitch, src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y_ALIGNED), height);
  }

  return dst;
}

AVSValue ConvertToPlanarGeneric::Create(AVSValue& args, const char* filter, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  VideoInfo vi = clip->GetVideoInfo();

  if (vi.IsRGB()) { // packed or planar
    if (vi.IsRGB48() || vi.IsRGB64()) {
      // we convert to intermediate PlanarRGB, RGB48/64->YUV444 is slow C, planarRGB  is fast
      AVSValue new_args[5] = { clip, AVSValue(), AVSValue(), AVSValue(), AVSValue() };
      clip = ConvertToRGB::Create(AVSValue(new_args, 5), (void *)-1, env).AsClip();
      vi = clip->GetVideoInfo();
    }

    clip = new ConvertRGBToYUV444(clip, getMatrix(args[2].AsString(0), env), env);
    vi = clip->GetVideoInfo();
  }
  else if (vi.IsYUY2()) { // 8 bit only
    clip = new ConvertYUY2ToYV16(clip, env);
    vi = clip->GetVideoInfo();
  }
  else if (!vi.IsPlanar())
    env->ThrowError("%s: Can only convert from Planar YUV.", filter);

  int pixel_type = VideoInfo::CS_UNKNOWN;
  AVSValue outplacement = AVSValue();

  bool hasAlpha = vi.NumComponents() == 4;

  if (strcmp(filter, "ConvertToYUV420") == 0) {
    if (vi.Is420())
      if (getPlacement(args[3], env) == getPlacement(args[5], env))
        return clip;
    outplacement = args[5];
    switch (vi.BitsPerComponent())
    {
    case 8 : pixel_type = hasAlpha ? VideoInfo::CS_YUVA420 : VideoInfo::CS_YV12; break;
    case 10: pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P10 : VideoInfo::CS_YUV420P10; break;
    case 12: pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P12 : VideoInfo::CS_YUV420P12; break;
    case 14: pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P14 : VideoInfo::CS_YUV420P14; break;
    case 16: pixel_type = hasAlpha ? VideoInfo::CS_YUVA420P16 : VideoInfo::CS_YUV420P16; break;
    case 32: pixel_type = hasAlpha ? VideoInfo::CS_YUVA420PS  : VideoInfo::CS_YUV420PS; break;
    }
  }
  else if (strcmp(filter, "ConvertToYUV422") == 0) {
    if (vi.Is422())
      return clip;
    switch (vi.BitsPerComponent())
    {
    case 8 : pixel_type = hasAlpha ? VideoInfo::CS_YUVA422 : VideoInfo::CS_YV16; break;
    case 10: pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P10 : VideoInfo::CS_YUV422P10; break;
    case 12: pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P12 : VideoInfo::CS_YUV422P12; break;
    case 14: pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P14 : VideoInfo::CS_YUV422P14; break;
    case 16: pixel_type = hasAlpha ? VideoInfo::CS_YUVA422P16 : VideoInfo::CS_YUV422P16; break;
    case 32: pixel_type = hasAlpha ? VideoInfo::CS_YUVA422PS  : VideoInfo::CS_YUV422PS; break;
    }
  }
  else if (strcmp(filter, "ConvertToYUV444") == 0) {
    if (vi.Is444())
      return clip;
    switch (vi.BitsPerComponent())
    {
    case 8 : pixel_type = hasAlpha ? VideoInfo::CS_YUVA444 : VideoInfo::CS_YV24; break;
    case 10: pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P10 : VideoInfo::CS_YUV444P10; break;
    case 12: pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P12 : VideoInfo::CS_YUV444P12; break;
    case 14: pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P14 : VideoInfo::CS_YUV444P14; break;
    case 16: pixel_type = hasAlpha ? VideoInfo::CS_YUVA444P16 : VideoInfo::CS_YUV444P16; break;
    case 32: pixel_type = hasAlpha ? VideoInfo::CS_YUVA444PS  : VideoInfo::CS_YUV444PS; break;
    }
  }
  else if (strcmp(filter, "ConvertToYV411") == 0) {
    if (vi.IsYV411()) return clip;
    if(vi.ComponentSize()!=1)
      env->ThrowError("%s: 8 bit only", filter);
    pixel_type = VideoInfo::CS_YV411;
  }
  else env->ThrowError("Convert: unknown filter '%s'.", filter);

  if (pixel_type == VideoInfo::CS_UNKNOWN)
    env->ThrowError("%s: unsupported bit depth", filter);

  return new ConvertToPlanarGeneric(clip, pixel_type, args[1].AsBool(false), args[3], args[4], outplacement, env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYUV420(AVSValue args, void*, IScriptEnvironment* env) {
  return Create(args, "ConvertToYUV420", env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYUV422(AVSValue args, void*, IScriptEnvironment* env) {
  return Create(args, "ConvertToYUV422", env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYUV444(AVSValue args, void*, IScriptEnvironment* env) {
  return Create(args, "ConvertToYUV444", env);
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV411(AVSValue args, void*, IScriptEnvironment* env) {
  return Create(args, "ConvertToYV411", env);
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
