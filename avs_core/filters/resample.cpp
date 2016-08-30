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

#include "resample.h"
#include <avs/config.h>
#include "../core/internal.h"

#include "transform.h"
#include "turn.h"
#include <avs/alignment.h>
#include <avs/minmax.h>
#include "../convert/convert_planar.h"
#include "../convert/convert_yuy2.h"

#include <type_traits>
// Intrinsics for SSE4.1, SSSE3, SSE3, SSE2, ISSE and MMX
#include <smmintrin.h>
#include <algorithm>

/***************************************
 ********* Templated SSE Loader ********
 ***************************************/

typedef __m128i (SSELoader)(const __m128i*);
typedef __m128 (SSELoader_ps)(const float*);

__forceinline __m128i simd_load_aligned(const __m128i* adr)
{
  return _mm_load_si128(adr);
}

__forceinline __m128i simd_load_unaligned(const __m128i* adr)
{
  return _mm_loadu_si128(adr);
}

__forceinline __m128i simd_load_unaligned_sse3(const __m128i* adr)
{
  return _mm_lddqu_si128(adr);
}

__forceinline __m128i simd_load_streaming(const __m128i* adr)
{
  return _mm_stream_load_si128(const_cast<__m128i*>(adr));
}

// float loaders
__forceinline __m128 simd_loadps_aligned(const float * adr)
{
  return _mm_load_ps(adr);
}

__forceinline __m128 simd_loadps_unaligned(const float* adr)
{
  return _mm_loadu_ps(adr);
}

// fake _mm_packus_epi32 (orig is SSE4.1 only)
static __forceinline __m128i _MM_PACKUS_EPI32( __m128i a, __m128i b )
{
  a = _mm_slli_epi32 (a, 16);
  a = _mm_srai_epi32 (a, 16);
  b = _mm_slli_epi32 (b, 16);
  b = _mm_srai_epi32 (b, 16);
  a = _mm_packs_epi32 (a, b);
  return a;
}

/***************************************
 ***** Vertical Resizer Assembly *******
 ***************************************/

template<typename pixel_size>
static void resize_v_planar_pointresize(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;

  pixel_size* src0 = (pixel_size *)src;
  pixel_size* dst0 = (pixel_size *)dst;
  dst_pitch = dst_pitch / sizeof(pixel_size);

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const pixel_size* src_ptr = src0 + pitch_table[offset]/sizeof(pixel_size);

    memcpy(dst0, src_ptr, width*sizeof(pixel_size));

    dst0 += dst_pitch;
  }
}

template<typename pixel_t>
static void resize_v_c_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;

  typedef typename std::conditional < std::is_floating_point<pixel_t>::value, float, short>::type coeff_t;
  coeff_t *current_coeff;

  if (!std::is_floating_point<pixel_t>::value)
    current_coeff = (coeff_t *)program->pixel_coefficient;
  else
    current_coeff = (coeff_t *)program->pixel_coefficient_float;

  pixel_t* src0 = (pixel_t *)src;
  pixel_t* dst0 = (pixel_t *)dst;
  dst_pitch = dst_pitch / sizeof(pixel_t);

  pixel_t limit = 0;
  if (!std::is_floating_point<pixel_t>::value) {  // floats are unscaled and uncapped
    if (sizeof(pixel_t) == 1) limit = 255;
    else if (sizeof(pixel_t) == 2) limit = pixel_t(65535);
  }

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const pixel_t* src_ptr = src0 + pitch_table[offset] / sizeof(pixel_t);

    for (int x = 0; x < width; x++) {
      // todo: check whether int result is enough for 16 bit samples (can an int overflow because of 16384 scale or really need __int64?)
      typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result;
      result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i] / sizeof(pixel_t))[x] * current_coeff[i];
      }
      if (!std::is_floating_point<pixel_t>::value) {  // floats are unscaled and uncapped
        result = ((result + 8192) / 16384);
        result = clamp(result, decltype(result)(0), decltype(result)(limit));
      }
      dst0[x] = (pixel_t)result;
    }

    dst0 += dst_pitch;
    current_coeff += filter_size;
  }
}

#ifdef X86_32
static void resize_v_mmx_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;

  int wMod8 = (width / 8) * 8;
  int sizeMod2 = (filter_size/2) * 2;
  bool notMod2 = sizeMod2 < filter_size;

  __m64 zero = _mm_setzero_si64();

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < wMod8; x += 8) {
      __m64 result_1 = _mm_set1_pi32(8192); // Init. with rounder (16384/2 = 8192)
      __m64 result_2 = result_1;
      __m64 result_3 = result_1;
      __m64 result_4 = result_1;

      for (int i = 0; i < sizeMod2; i += 2) {
        __m64 src_p1 = *(reinterpret_cast<const __m64*>(src_ptr+pitch_table[i]+x));   // For detailed explanation please see SSE2 version.
        __m64 src_p2 = *(reinterpret_cast<const __m64*>(src_ptr+pitch_table[i+1]+x));

        __m64 src_l = _mm_unpacklo_pi8(src_p1, src_p2);
        __m64 src_h = _mm_unpackhi_pi8(src_p1, src_p2);

        __m64 src_1 = _mm_unpacklo_pi8(src_l, zero);
        __m64 src_2 = _mm_unpackhi_pi8(src_l, zero);
        __m64 src_3 = _mm_unpacklo_pi8(src_h, zero);
        __m64 src_4 = _mm_unpackhi_pi8(src_h, zero);

        __m64 coeff = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(current_coeff+i));
        coeff = _mm_unpacklo_pi32(coeff, coeff);

        __m64 dst_1 = _mm_madd_pi16(src_1, coeff);
        __m64 dst_2 = _mm_madd_pi16(src_2, coeff);
        __m64 dst_3 = _mm_madd_pi16(src_3, coeff);
        __m64 dst_4 = _mm_madd_pi16(src_4, coeff);

        result_1 = _mm_add_pi32(result_1, dst_1);
        result_2 = _mm_add_pi32(result_2, dst_2);
        result_3 = _mm_add_pi32(result_3, dst_3);
        result_4 = _mm_add_pi32(result_4, dst_4);
      }

      if (notMod2) { // do last odd row
        __m64 src_p = *(reinterpret_cast<const __m64*>(src_ptr+pitch_table[sizeMod2]+x));

        __m64 src_l = _mm_unpacklo_pi8(src_p, zero);
        __m64 src_h = _mm_unpackhi_pi8(src_p, zero);

        __m64 coeff = _mm_set1_pi16(current_coeff[sizeMod2]);

        __m64 dst_ll = _mm_mullo_pi16(src_l, coeff);   // Multiply by coefficient
        __m64 dst_lh = _mm_mulhi_pi16(src_l, coeff);
        __m64 dst_hl = _mm_mullo_pi16(src_h, coeff);
        __m64 dst_hh = _mm_mulhi_pi16(src_h, coeff);

        __m64 dst_1 = _mm_unpacklo_pi16(dst_ll, dst_lh); // Unpack to 32-bit integer
        __m64 dst_2 = _mm_unpackhi_pi16(dst_ll, dst_lh);
        __m64 dst_3 = _mm_unpacklo_pi16(dst_hl, dst_hh);
        __m64 dst_4 = _mm_unpackhi_pi16(dst_hl, dst_hh);

        result_1 = _mm_add_pi32(result_1, dst_1);
        result_2 = _mm_add_pi32(result_2, dst_2);
        result_3 = _mm_add_pi32(result_3, dst_3);
        result_4 = _mm_add_pi32(result_4, dst_4);
      }

      // Divide by 16348 (FPRound)
      result_1  = _mm_srai_pi32(result_1, 14);
      result_2  = _mm_srai_pi32(result_2, 14);
      result_3  = _mm_srai_pi32(result_3, 14);
      result_4  = _mm_srai_pi32(result_4, 14);

      // Pack and store
      __m64 result_l = _mm_packs_pi32(result_1, result_2);
      __m64 result_h = _mm_packs_pi32(result_3, result_4);
      __m64 result   = _mm_packs_pu16(result_l, result_h);

      *(reinterpret_cast<__m64*>(dst+x)) = result;
    }

    // Leftover
    for (int x = wMod8; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }

  _mm_empty();
}
#endif

template<SSELoader load>
static void resize_v_sse2_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;

  int wMod16 = (width / 16) * 16;
  int sizeMod2 = (filter_size/2) * 2;
  bool notMod2 = sizeMod2 < filter_size;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < wMod16; x += 16) {
      __m128i result_1 = _mm_set1_epi32(8192); // Init. with rounder (16384/2 = 8192)
      __m128i result_2 = result_1;
      __m128i result_3 = result_1;
      __m128i result_4 = result_1;

      for (int i = 0; i < sizeMod2; i += 2) {
        __m128i src_p1 = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[i]+x));   // p|o|n|m|l|k|j|i|h|g|f|e|d|c|b|a
        __m128i src_p2 = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[i+1]+x)); // P|O|N|M|L|K|J|I|H|G|F|E|D|C|B|A

        __m128i src_l = _mm_unpacklo_epi8(src_p1, src_p2);                                   // Hh|Gg|Ff|Ee|Dd|Cc|Bb|Aa
        __m128i src_h = _mm_unpackhi_epi8(src_p1, src_p2);                                   // Pp|Oo|Nn|Mm|Ll|Kk|Jj|Ii

        __m128i src_1 = _mm_unpacklo_epi8(src_l, zero);                                      // .D|.d|.C|.c|.B|.b|.A|.a
        __m128i src_2 = _mm_unpackhi_epi8(src_l, zero);                                      // .H|.h|.G|.g|.F|.f|.E|.e
        __m128i src_3 = _mm_unpacklo_epi8(src_h, zero);                                      // etc.
        __m128i src_4 = _mm_unpackhi_epi8(src_h, zero);                                      // etc.

        __m128i coeff = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(current_coeff+i));   // XX|XX|XX|XX|XX|XX|CO|co
        coeff = _mm_shuffle_epi32(coeff, 0);                                                 // CO|co|CO|co|CO|co|CO|co

        __m128i dst_1 = _mm_madd_epi16(src_1, coeff);                                         // CO*D+co*d | CO*C+co*c | CO*B+co*b | CO*A+co*a
        __m128i dst_2 = _mm_madd_epi16(src_2, coeff);                                         // etc.
        __m128i dst_3 = _mm_madd_epi16(src_3, coeff);
        __m128i dst_4 = _mm_madd_epi16(src_4, coeff);

        result_1 = _mm_add_epi32(result_1, dst_1);
        result_2 = _mm_add_epi32(result_2, dst_2);
        result_3 = _mm_add_epi32(result_3, dst_3);
        result_4 = _mm_add_epi32(result_4, dst_4);
      }

      if (notMod2) { // do last odd row
        __m128i src_p = load(reinterpret_cast<const __m128i*>(src_ptr+pitch_table[sizeMod2]+x));

        __m128i src_l = _mm_unpacklo_epi8(src_p, zero);
        __m128i src_h = _mm_unpackhi_epi8(src_p, zero);

        __m128i coeff = _mm_set1_epi16(current_coeff[sizeMod2]);

        __m128i dst_ll = _mm_mullo_epi16(src_l, coeff);   // Multiply by coefficient
        __m128i dst_lh = _mm_mulhi_epi16(src_l, coeff);
        __m128i dst_hl = _mm_mullo_epi16(src_h, coeff);
        __m128i dst_hh = _mm_mulhi_epi16(src_h, coeff);

        __m128i dst_1 = _mm_unpacklo_epi16(dst_ll, dst_lh); // Unpack to 32-bit integer
        __m128i dst_2 = _mm_unpackhi_epi16(dst_ll, dst_lh);
        __m128i dst_3 = _mm_unpacklo_epi16(dst_hl, dst_hh);
        __m128i dst_4 = _mm_unpackhi_epi16(dst_hl, dst_hh);

        result_1 = _mm_add_epi32(result_1, dst_1);
        result_2 = _mm_add_epi32(result_2, dst_2);
        result_3 = _mm_add_epi32(result_3, dst_3);
        result_4 = _mm_add_epi32(result_4, dst_4);
      }

      // Divide by 16348 (FPRound)
      result_1  = _mm_srai_epi32(result_1, 14);
      result_2  = _mm_srai_epi32(result_2, 14);
      result_3  = _mm_srai_epi32(result_3, 14);
      result_4  = _mm_srai_epi32(result_4, 14);

      // Pack and store
      __m128i result_l = _mm_packs_epi32(result_1, result_2);
      __m128i result_h = _mm_packs_epi32(result_3, result_4);
      __m128i result   = _mm_packus_epi16(result_l, result_h);

      _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), result);
    }

    // Leftover
    for (int x = wMod16; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }
}

// for uint16_t and float. Both uses float arithmetic and coefficients
template<SSELoader load, SSELoader_ps loadps, bool sse41, typename pixel_t>
static void resize_v_sseX_planar_16or32(BYTE* dst0, const BYTE* src0, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;
  //short* current_coeff = program->pixel_coefficient;
  float* current_coeff_float = program->pixel_coefficient_float;

  int wMod8 = (width / 8) * 8; // uint16/float: 8 at a time (byte was 16 byte at a time)

  __m128i zero = _mm_setzero_si128();

  const pixel_t* src = (pixel_t *)src0;
  pixel_t* dst = (pixel_t *)dst0;
  dst_pitch = dst_pitch / sizeof(pixel_t);
  src_pitch = src_pitch / sizeof(pixel_t);

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const pixel_t* src_ptr = src + pitch_table[offset]/sizeof(pixel_t);

    for (int x = 0; x < wMod8; x+=8) {
      __m128 result_l_single = _mm_set1_ps(0.0f);
      __m128 result_h_single = result_l_single;

      const pixel_t* src2_ptr = src_ptr+x;

      for (int i = 0; i < filter_size; i++) {
        __m128 src_l_single;
        __m128 src_h_single;
        if(sizeof(pixel_t)==2) // word
        {
          // load is template-dependent
          __m128i src_p = load(reinterpret_cast<const __m128i*>(src2_ptr)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero); // spread higher 4*uint16_t pixel value -> 4*32 bit
          src_l_single = _mm_cvtepi32_ps (src_l); // Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
          src_h_single = _mm_cvtepi32_ps (src_h);
        }
        else { // float
          // _mm_load_ps or _mm_loadu_ps template dependent
          src_l_single = loadps(reinterpret_cast<const float*>(src2_ptr)); // float  4*32=128 4 pixels at a time
          src_h_single = loadps(reinterpret_cast<const float*>(src2_ptr+4));
        }
        __m128 coeff = _mm_load1_ps(reinterpret_cast<const float*>(current_coeff_float+i)); // loads 1, fills all 4 floats
        __m128 dst_l = _mm_mul_ps(src_l_single, coeff); // Multiply by coefficient
        __m128 dst_h = _mm_mul_ps(src_h_single, coeff); // 4*(32bit*32bit=32bit)
        result_l_single = _mm_add_ps(result_l_single, dst_l); // accumulate result.
        result_h_single = _mm_add_ps(result_h_single, dst_h);

        src2_ptr += src_pitch;
      }

      if(sizeof(pixel_t)==2) // word
      {
        // Converts the four single-precision, floating-point values of a to signed 32-bit integer values.
        __m128i result_l  = _mm_cvtps_epi32(result_l_single);
        __m128i result_h  = _mm_cvtps_epi32(result_h_single);
        // Pack and store
        // SIMD Extensions 4 (SSE4) packus or simulation
        __m128i result = sse41 ? _mm_packus_epi32(result_l, result_h) : (_MM_PACKUS_EPI32(result_l, result_h)) ; // 4*32+4*32 = 8*16
        _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), result);
      }
      else { // float
        _mm_store_ps(reinterpret_cast<float*>(dst+x), result_l_single);
        _mm_store_ps(reinterpret_cast<float*>(dst+x+4), result_h_single);
      }
    }

    // Leftover
    for (int x = wMod8; x < width; x++) {
      float result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i]/sizeof(pixel_t))[x] * current_coeff_float[i];
      }
      if (!std::is_floating_point<pixel_t>::value) {  // floats are unscaled and uncapped
        result = clamp(result, 0.0f, 65535.0f);
      }
      dst[x] = (pixel_t) result;
    }

    dst += dst_pitch;
    current_coeff_float += filter_size;
  }
}


template<SSELoader load>
static void resize_v_ssse3_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int target_height, const int* pitch_table, const void* storage)
{
  int filter_size = program->filter_size;
  short* current_coeff = program->pixel_coefficient;

  int wMod16 = (width / 16) * 16;

  __m128i zero = _mm_setzero_si128();
  __m128i coeff_unpacker = _mm_set_epi8(1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0, 1, 0);

  for (int y = 0; y < target_height; y++) {
    int offset = program->pixel_offset[y];
    const BYTE* src_ptr = src + pitch_table[offset];

    for (int x = 0; x < wMod16; x+=16) {
      __m128i result_l = _mm_set1_epi16(32); // Init. with rounder ((1 << 6)/2 = 32)
      __m128i result_h = result_l;

      const BYTE* src2_ptr = src_ptr+x;

      for (int i = 0; i < filter_size; i++) {
        __m128i src_p = load(reinterpret_cast<const __m128i*>(src2_ptr));

        __m128i src_l = _mm_unpacklo_epi8(src_p, zero);
        __m128i src_h = _mm_unpackhi_epi8(src_p, zero);

        src_l = _mm_slli_epi16(src_l, 7);
        src_h = _mm_slli_epi16(src_h, 7);

        __m128i coeff = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(current_coeff+i));
                coeff = _mm_shuffle_epi8(coeff, coeff_unpacker);

        __m128i dst_l = _mm_mulhrs_epi16(src_l, coeff);   // Multiply by coefficient (SSSE3)
        __m128i dst_h = _mm_mulhrs_epi16(src_h, coeff);

        result_l = _mm_add_epi16(result_l, dst_l);
        result_h = _mm_add_epi16(result_h, dst_h);

        src2_ptr += src_pitch;
      }

      // Divide by 64
      result_l  = _mm_srai_epi16(result_l, 6);
      result_h  = _mm_srai_epi16(result_h, 6);

      // Pack and store
      __m128i result   = _mm_packus_epi16(result_l, result_h);

      _mm_store_si128(reinterpret_cast<__m128i*>(dst+x), result);
    }

    // Leftover
    for (int x = wMod16; x < width; x++) {
      int result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src_ptr+pitch_table[i])[x] * current_coeff[i];
      }
      result = ((result+8192)/16384);
      result = result > 255 ? 255 : result < 0 ? 0 : result;
      dst[x] = (BYTE) result;
    }

    dst += dst_pitch;
    current_coeff += filter_size;
  }
}

__forceinline static void resize_v_create_pitch_table(int* table, int pitch, int height) {
  table[0] = 0;
  for (int i = 1; i < height; i++) {
    table[i] = table[i-1]+pitch;
  }
}



/***************************************
 ********* Horizontal Resizer** ********
 ***************************************/

static void resize_h_pointresize(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height) {
  int wMod4 = width/4 * 4;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod4; x+=4) {
#define pixel(a) src[program->pixel_offset[x+a]]
      unsigned int data = (pixel(3) << 24) + (pixel(2) << 16) + (pixel(1) << 8) + pixel(0);
#undef pixel
      *((unsigned int *)(dst+x)) = data;
    }

    for (int x = wMod4; x < width; x++) {
      dst[x] = src[program->pixel_offset[x]];
    }

    dst += dst_pitch;
    src += src_pitch;
  }
}

// make the resampling coefficient array mod8 friendly for simd, padding non-used coeffs with zeros
static void resize_h_prepare_coeff_8(ResamplingProgram* p, IScriptEnvironment2* env) {
  int filter_size = AlignNumber(p->filter_size, 8);
  short* new_coeff = (short*) env->Allocate(sizeof(short) * p->target_size * filter_size, 64, AVS_NORMAL_ALLOC);
  float* new_coeff_float = (float*) env->Allocate(sizeof(float) * p->target_size * filter_size, 64, AVS_NORMAL_ALLOC);
  if (!new_coeff || !new_coeff_float) {
    env->Free(new_coeff);
    env->Free(new_coeff_float);
    env->ThrowError("Could not reserve memory in a resampler.");
  }

  memset(new_coeff, 0, sizeof(short) * p->target_size * filter_size);
  std::fill_n(new_coeff_float, p->target_size * filter_size, 0.0f);

  // Copy existing coeff
  short *dst = new_coeff, *src = p->pixel_coefficient;
  float *dst_f = new_coeff_float, *src_f = p->pixel_coefficient_float;
  for (int i = 0; i < p->target_size; i++) {
    for (int j = 0; j < p->filter_size; j++) {
      dst[j] = src[j];
      dst_f[j] = src_f[j];
    }

    dst += filter_size;
    src += p->filter_size;

    dst_f += filter_size;
    src_f += p->filter_size;
  }

  env->Free(p->pixel_coefficient);
  env->Free(p->pixel_coefficient_float);
  p->pixel_coefficient = new_coeff;
  p->pixel_coefficient_float = new_coeff_float;
}

template<typename pixel_t>
static void resize_h_c_planar(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height) {
  int filter_size = program->filter_size;

  typedef typename std::conditional < std::is_floating_point<pixel_t>::value, float, short>::type coeff_t;
  coeff_t *current_coeff;

  pixel_t limit = 0;
  if (!std::is_floating_point<pixel_t>::value) {  // floats are unscaled and uncapped
    if (sizeof(pixel_t) == 1) limit = 255;
    else if (sizeof(pixel_t) == 2) limit = pixel_t(65535);
  }

  src_pitch = src_pitch / sizeof(pixel_t);
  dst_pitch = dst_pitch / sizeof(pixel_t);

  pixel_t* src0 = (pixel_t*)src;
  pixel_t* dst0 = (pixel_t*)dst;

  // external loop y is much faster
  for (int y = 0; y < height; y++) {
    if (!std::is_floating_point<pixel_t>::value)
      current_coeff = (coeff_t *)program->pixel_coefficient;
    else
      current_coeff = (coeff_t *)program->pixel_coefficient_float;
    for (int x = 0; x < width; x++) {
      int begin = program->pixel_offset[x];
      // todo: check whether int result is enough for 16 bit samples (can an int overflow because of 16384 scale or really need __int64?)
      typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result;
      result = 0;
      for (int i = 0; i < filter_size; i++) {
        result += (src0+y*src_pitch)[(begin+i)] * current_coeff[i];
      }
      if (!std::is_floating_point<pixel_t>::value) {  // floats are unscaled and uncapped
        result = ((result + 8192) / 16384);
        result = clamp(result, decltype(result)(0), decltype(result)(limit));
      }
      (dst0 + y*dst_pitch)[x] = (pixel_t)result;
      current_coeff += filter_size;
    }
  }
}

static void resizer_h_ssse3_generic(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height) {
  int filter_size = AlignNumber(program->filter_size, 8) / 8;
  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < height; y++) {
    short* current_coeff = program->pixel_coefficient;
    for (int x = 0; x < width; x+=4) {
      __m128i result1 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result2 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result3 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result4 = _mm_setr_epi32(8192, 0, 0, 0);

      int begin1 = program->pixel_offset[x+0];
      int begin2 = program->pixel_offset[x+1];
      int begin3 = program->pixel_offset[x+2];
      int begin4 = program->pixel_offset[x+3];

      for (int i = 0; i < filter_size; i++) {
        __m128i data, coeff, current_result;
        data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin1+i*8));
        data = _mm_unpacklo_epi8(data, zero);
        coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
        current_result = _mm_madd_epi16(data, coeff);
        result1 = _mm_add_epi32(result1, current_result);

        current_coeff += 8;
      }

      for (int i = 0; i < filter_size; i++) {
        __m128i data, coeff, current_result;
        data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin2+i*8));
        data = _mm_unpacklo_epi8(data, zero);
        coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
        current_result = _mm_madd_epi16(data, coeff);
        result2 = _mm_add_epi32(result2, current_result);

        current_coeff += 8;
      }

      for (int i = 0; i < filter_size; i++) {
        __m128i data, coeff, current_result;
        data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin3+i*8));
        data = _mm_unpacklo_epi8(data, zero);
        coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
        current_result = _mm_madd_epi16(data, coeff);
        result3 = _mm_add_epi32(result3, current_result);

        current_coeff += 8;
      }

      for (int i = 0; i < filter_size; i++) {
        __m128i data, coeff, current_result;
        data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin4+i*8));
        data = _mm_unpacklo_epi8(data, zero);
        coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
        current_result = _mm_madd_epi16(data, coeff);
        result4 = _mm_add_epi32(result4, current_result);

        current_coeff += 8;
      }

      __m128i result12 = _mm_hadd_epi32(result1, result2);
      __m128i result34 = _mm_hadd_epi32(result3, result4);
      __m128i result = _mm_hadd_epi32(result12, result34);

      result = _mm_srai_epi32(result, 14);

      result = _mm_packs_epi32(result, zero);
      result = _mm_packus_epi16(result, zero);

      *((int*)(dst+x)) = _mm_cvtsi128_si32(result);
    }

    dst += dst_pitch;
    src += src_pitch;
  }
}

static void resizer_h_ssse3_8(BYTE* dst, const BYTE* src, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height) {
  int filter_size = AlignNumber(program->filter_size, 8) / 8;

  __m128i zero = _mm_setzero_si128();

  for (int y = 0; y < height; y++) {
    short* current_coeff = program->pixel_coefficient;
    for (int x = 0; x < width; x+=4) {
      __m128i result1 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result2 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result3 = _mm_setr_epi32(8192, 0, 0, 0);
      __m128i result4 = _mm_setr_epi32(8192, 0, 0, 0);

      int begin1 = program->pixel_offset[x+0];
      int begin2 = program->pixel_offset[x+1];
      int begin3 = program->pixel_offset[x+2];
      int begin4 = program->pixel_offset[x+3];

      __m128i data, coeff, current_result;

      // Unroll 1
      data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin1));
      data = _mm_unpacklo_epi8(data, zero);
      coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
      current_result = _mm_madd_epi16(data, coeff);
      result1 = _mm_add_epi32(result1, current_result);

      current_coeff += 8;

      // Unroll 2
      data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin2));
      data = _mm_unpacklo_epi8(data, zero);
      coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
      current_result = _mm_madd_epi16(data, coeff);
      result2 = _mm_add_epi32(result2, current_result);

      current_coeff += 8;

      // Unroll 3
      data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin3));
      data = _mm_unpacklo_epi8(data, zero);
      coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
      current_result = _mm_madd_epi16(data, coeff);
      result3 = _mm_add_epi32(result3, current_result);

      current_coeff += 8;

      // Unroll 4
      data = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(src+begin4));
      data = _mm_unpacklo_epi8(data, zero);
      coeff = _mm_load_si128(reinterpret_cast<const __m128i*>(current_coeff));
      current_result = _mm_madd_epi16(data, coeff);
      result4 = _mm_add_epi32(result4, current_result);

      current_coeff += 8;

      // Combine
      __m128i result12 = _mm_hadd_epi32(result1, result2);
      __m128i result34 = _mm_hadd_epi32(result3, result4);
      __m128i result = _mm_hadd_epi32(result12, result34);

      result = _mm_srai_epi32(result, 14);

      result = _mm_packs_epi32(result, zero);
      result = _mm_packus_epi16(result, zero);

      *((int*)(dst+x)) = _mm_cvtsi128_si32(result);
    }

    dst += dst_pitch;
    src += src_pitch;
  }
}

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Resample_filters[] = {
  { "PointResize",    BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_PointResize },
  { "BilinearResize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_BilinearResize },
  { "BicubicResize",  BUILTIN_FUNC_PREFIX, "cii[b]f[c]f[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_BicubicResize },
  { "LanczosResize",  BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_LanczosResize},
  { "Lanczos4Resize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Lanczos4Resize},
  { "BlackmanResize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_BlackmanResize},
  { "Spline16Resize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline16Resize},
  { "Spline36Resize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline36Resize},
  { "Spline64Resize", BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f", FilteredResize::Create_Spline64Resize},
  { "GaussResize",    BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f[p]f", FilteredResize::Create_GaussianResize},
  { "SincResize",     BUILTIN_FUNC_PREFIX, "cii[src_left]f[src_top]f[src_width]f[src_height]f[taps]i", FilteredResize::Create_SincResize},
  /**
    * Resize(PClip clip, dst_width, dst_height [src_left, src_top, src_width, int src_height,] )
    *
    * src_left et al.   =  when these optional arguments are given, the filter acts just like
    *                      a Crop was performed with those parameters before resizing, only faster
   **/

  { 0 }
};


FilteredResizeH::FilteredResizeH( PClip _child, double subrange_left, double subrange_width,
                                  int target_width, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child),
  resampling_program_luma(0), resampling_program_chroma(0),
  src_pitch_table_luma(0),
  src_pitch_luma(-1),
  filter_storage_luma(0), filter_storage_chroma(0)
{
  src_width  = vi.width;
  src_height = vi.height;
  dst_width  = target_width;
  dst_height = vi.height;

  pixelsize = vi.ComponentSize(); // AVS16
  grey = vi.IsY();

  bool isRGBPfamily = vi.IsPlanarRGB() || vi.IsPlanarRGBA();

  if (target_width <= 0) {
    env->ThrowError("Resize: Width must be greater than 0.");
  }

  if (vi.IsPlanar() && !grey && !isRGBPfamily) {
    const int mask = (1 << vi.GetPlaneWidthSubsampling(PLANAR_U)) - 1;

    if (target_width & mask)
      env->ThrowError("Resize: Planar destination height must be a multiple of %d.", mask+1);
  }

  auto env2 = static_cast<IScriptEnvironment2*>(env);

  // Main resampling program
  resampling_program_luma = func->GetResamplingProgram(vi.width, subrange_left, subrange_width, target_width, env2);
  if (vi.IsPlanar() && !grey && !isRGBPfamily) {
    const int shift = vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int shift_h = vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int div   = 1 << shift;


    resampling_program_chroma = func->GetResamplingProgram(
      vi.width       >> shift,
      subrange_left   / div,
      subrange_width  / div,
      target_width   >> shift,
      env2);
  }

  fast_resize = (env->GetCPUFlags() & CPUF_SSSE3) == CPUF_SSSE3 && vi.IsPlanar() && target_width%4 == 0;
  if (fast_resize /*&& vi.IsYUV()*/ && !grey && !isRGBPfamily) {
    const int shift = vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int dst_chroma_width = dst_width >> shift;

    if (dst_chroma_width%4 != 0) {
      fast_resize = false;
    }
  }

  if (false && resampling_program_luma->filter_size == 1 && vi.IsPlanar()) {
    // dead code?
    fast_resize = true;
    resampler_h_luma = resize_h_pointresize;
    resampler_h_chroma = resize_h_pointresize;
  } else if (!fast_resize) {
    // Create resampling program and pitch table
    src_pitch_table_luma     = new int[vi.width];

    resampler_luma   = FilteredResizeV::GetResampler(env->GetCPUFlags(), true, pixelsize, filter_storage_luma, resampling_program_luma);
    if (vi.IsPlanar() && !grey && !isRGBPfamily) {
      resampler_chroma = FilteredResizeV::GetResampler(env->GetCPUFlags(), true, pixelsize, filter_storage_chroma, resampling_program_chroma);
    }

    // Temporary buffer size
    temp_1_pitch = AlignNumber(vi.BytesFromPixels(src_height), 64);
    temp_2_pitch = AlignNumber(vi.BytesFromPixels(dst_height), 64);

    resize_v_create_pitch_table(src_pitch_table_luma, temp_1_pitch, src_width);

    // Initialize Turn function
    // see turn.cpp
    bool has_sse2 = (env->GetCPUFlags() & CPUF_SSE2) != 0;
    if (vi.IsRGB24()) {
      turn_left = turn_left_rgb24;
      turn_right = turn_right_rgb24;
    } else if (vi.IsRGB32()) {
      if (has_sse2) {
        turn_left = turn_left_rgb32_sse2;
        turn_right = turn_right_rgb32_sse2;
      } else {
        turn_left = turn_left_rgb32_c;
        turn_right = turn_right_rgb32_c;
      }
    } else if (vi.IsRGB48()) {
        turn_left = turn_left_rgb48; // todo: _c suffix
        turn_right = turn_right_rgb48; // todo: _c suffix
    } else if (vi.IsRGB64()) {
      if (has_sse2) {
        turn_left = turn_left_rgb64_sse2;
        turn_right = turn_right_rgb64_sse2;
      } else {
        turn_left = turn_left_rgb64_c;
        turn_right = turn_right_rgb64_c;
      }
    } else {
      switch (vi.ComponentSize()) {// AVS16
      case 1: // 8 bit
        if (has_sse2) {
          turn_left = turn_left_plane_8_sse2;
          turn_right = turn_right_plane_8_sse2;
        } else {
          turn_left = turn_left_plane_8_c;
          turn_right = turn_right_plane_8_c;
        }
        break;
      case 2: // 16 bit
        if (has_sse2) {
          turn_left = turn_left_plane_16_sse2;
          turn_right = turn_right_plane_16_sse2;
        } else {
          turn_left = turn_left_plane_16_c;
          turn_right = turn_right_plane_16_c;
        }
        break;
      default: // 32 bit
        if (has_sse2) {
          turn_left = turn_left_plane_32_sse2;
          turn_right = turn_right_plane_32_sse2;
        } else {
          turn_left = turn_left_plane_32_c;
          turn_right = turn_right_plane_32_c;
        }
      }
    }
  } else { // Plannar + SSSE3 = use new horizontal resizer routines
    resampler_h_luma = GetResampler(env->GetCPUFlags(), true, pixelsize, resampling_program_luma, env2);

    if (!grey && !isRGBPfamily) {
      resampler_h_chroma = GetResampler(env->GetCPUFlags(), true, pixelsize, resampling_program_chroma, env2);
    }
  }

  // Change target video info size
  vi.width = target_width;
}

PVideoFrame __stdcall FilteredResizeH::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  auto env2 = static_cast<IScriptEnvironment2*>(env);

  bool isRGBPfamily = vi.IsPlanarRGB() || vi.IsPlanarRGBA();

  if (!fast_resize) {
    // e.g. not aligned, not mod4
    // temp_1_pitch and temp_2_pitch is pixelsize-aware
    BYTE* temp_1 = static_cast<BYTE*>(env2->Allocate(temp_1_pitch * src_width, 64, AVS_POOLED_ALLOC));
    BYTE* temp_2 = static_cast<BYTE*>(env2->Allocate(temp_2_pitch * dst_width, 64, AVS_POOLED_ALLOC));
    if (!temp_1 || !temp_2) {
      env2->Free(temp_1);
      env2->Free(temp_2);
      env->ThrowError("Could not reserve memory in a resampler.");
    }

    if (!vi.IsRGB() || isRGBPfamily) {
      // Y/G Plane
      turn_right(src->GetReadPtr(), temp_1, src_width * pixelsize, src_height, src->GetPitch(), temp_1_pitch); // * pixelsize: turn_right needs GetPlaneWidth full size
      resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_luma, src_height, dst_width, src_pitch_table_luma, filter_storage_luma);
      turn_left(temp_2, dst->GetWritePtr(), dst_height * pixelsize, dst_width, temp_2_pitch, dst->GetPitch());

      if (isRGBPfamily)
      {
        turn_right(src->GetReadPtr(PLANAR_B), temp_1, src_width * pixelsize, src_height, src->GetPitch(PLANAR_B), temp_1_pitch); // * pixelsize: turn_right needs GetPlaneWidth full size
        resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_luma, src_height, dst_width, src_pitch_table_luma, filter_storage_luma);
        turn_left(temp_2, dst->GetWritePtr(PLANAR_B), dst_height * pixelsize, dst_width, temp_2_pitch, dst->GetPitch(PLANAR_B));

        turn_right(src->GetReadPtr(PLANAR_R), temp_1, src_width * pixelsize, src_height, src->GetPitch(PLANAR_R), temp_1_pitch); // * pixelsize: turn_right needs GetPlaneWidth full size
        resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_luma, src_height, dst_width, src_pitch_table_luma, filter_storage_luma);
        turn_left(temp_2, dst->GetWritePtr(PLANAR_R), dst_height * pixelsize, dst_width, temp_2_pitch, dst->GetPitch(PLANAR_R));
      }
      else if(!grey) {
        const int shift = vi.GetPlaneWidthSubsampling(PLANAR_U);
        const int shift_h = vi.GetPlaneHeightSubsampling(PLANAR_U);

        const int src_chroma_width = src_width >> shift;
        const int dst_chroma_width = dst_width >> shift;
        const int src_chroma_height = src_height >> shift_h;
        const int dst_chroma_height = dst_height >> shift_h;

        // turn_xxx: width * pixelsize: needs GetPlaneWidth-like full size
        // U Plane
        turn_right(src->GetReadPtr(PLANAR_U), temp_1, src_chroma_width * pixelsize, src_chroma_height, src->GetPitch(PLANAR_U), temp_1_pitch);
        resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_chroma, src_chroma_height, dst_chroma_width, src_pitch_table_luma, filter_storage_chroma);
        turn_left(temp_2, dst->GetWritePtr(PLANAR_U), dst_chroma_height * pixelsize, dst_chroma_width, temp_2_pitch, dst->GetPitch(PLANAR_U));

        // V Plane
        turn_right(src->GetReadPtr(PLANAR_V), temp_1, src_chroma_width * pixelsize, src_chroma_height, src->GetPitch(PLANAR_V), temp_1_pitch);
        resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_chroma, src_chroma_height, dst_chroma_width, src_pitch_table_luma, filter_storage_chroma);
        turn_left(temp_2, dst->GetWritePtr(PLANAR_V), dst_chroma_height * pixelsize, dst_chroma_width, temp_2_pitch, dst->GetPitch(PLANAR_V));
      }
      if (vi.IsYUVA() || vi.IsPlanarRGBA())
      {
        turn_right(src->GetReadPtr(PLANAR_A), temp_1, src_width * pixelsize, src_height, src->GetPitch(PLANAR_A), temp_1_pitch); // * pixelsize: turn_right needs GetPlaneWidth full size
        resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_luma, src_height, dst_width, src_pitch_table_luma, filter_storage_luma);
        turn_left(temp_2, dst->GetWritePtr(PLANAR_A), dst_height * pixelsize, dst_width, temp_2_pitch, dst->GetPitch(PLANAR_A));
      }

    } else {
      // packed RGB
      // First left, then right. Reason: packed RGB bottom to top. Right+left shifts RGB24/RGB32 image to the opposite horizontal direction
      turn_left(src->GetReadPtr(), temp_1, vi.BytesFromPixels(src_width), src_height, src->GetPitch(), temp_1_pitch);
      resampler_luma(temp_2, temp_1, temp_2_pitch, temp_1_pitch, resampling_program_luma, vi.BytesFromPixels(src_height) / pixelsize, dst_width, src_pitch_table_luma, filter_storage_luma);
      turn_right(temp_2, dst->GetWritePtr(), vi.BytesFromPixels(dst_height), dst_width, temp_2_pitch, dst->GetPitch());
    }

    env2->Free(temp_1);
    env2->Free(temp_2);
  } else {

    // Y Plane
    resampler_h_luma(dst->GetWritePtr(), src->GetReadPtr(), dst->GetPitch(), src->GetPitch(), resampling_program_luma, dst_width, dst_height);

    if (isRGBPfamily) {
      resampler_h_luma(dst->GetWritePtr(PLANAR_B), src->GetReadPtr(PLANAR_B), dst->GetPitch(PLANAR_B), src->GetPitch(PLANAR_B), resampling_program_luma, dst_width, dst_height);
      resampler_h_luma(dst->GetWritePtr(PLANAR_R), src->GetReadPtr(PLANAR_R), dst->GetPitch(PLANAR_R), src->GetPitch(PLANAR_R), resampling_program_luma, dst_width, dst_height);
    }
    else if (!grey) {
      const int dst_chroma_width = dst_width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
      const int dst_chroma_height = dst_height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

      // U Plane
      resampler_h_chroma(dst->GetWritePtr(PLANAR_U), src->GetReadPtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetPitch(PLANAR_U), resampling_program_chroma, dst_chroma_width, dst_chroma_height);

      // V Plane
      resampler_h_chroma(dst->GetWritePtr(PLANAR_V), src->GetReadPtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetPitch(PLANAR_V), resampling_program_chroma, dst_chroma_width, dst_chroma_height);
    }
    if (vi.IsYUVA() || vi.IsPlanarRGBA())
    {
      resampler_h_luma(dst->GetWritePtr(PLANAR_A), src->GetReadPtr(PLANAR_A), dst->GetPitch(PLANAR_A), src->GetPitch(PLANAR_A), resampling_program_luma, dst_width, dst_height);
    }

  }

  return dst;
}

ResamplerH FilteredResizeH::GetResampler(int CPU, bool aligned, int pixelsize, ResamplingProgram* program, IScriptEnvironment2* env)
{
  if (pixelsize == 1)
  {
  if (CPU & CPUF_SSSE3) {
    // make the resampling coefficient array mod8 friendly for simd, padding non-used coeffs with zeros
    resize_h_prepare_coeff_8(program, env);
    if (program->filter_size > 8)
      return resizer_h_ssse3_generic;
    else
      return resizer_h_ssse3_8;
  }
    else { // C version
      return resize_h_c_planar<uint8_t>;
}
  }
  else if (pixelsize == 2) { // todo: non_c
    return resize_h_c_planar<uint16_t>;
  } else { //if (pixelsize == 4)
    return resize_h_c_planar<float>;
  }
}

FilteredResizeH::~FilteredResizeH(void)
{
  if (resampling_program_luma)   { delete resampling_program_luma; }
  if (resampling_program_chroma) { delete resampling_program_chroma; }
  if (src_pitch_table_luma)    { delete[] src_pitch_table_luma; }
}

/***************************************
 ***** Filtered Resize - Vertical ******
 ***************************************/

FilteredResizeV::FilteredResizeV( PClip _child, double subrange_top, double subrange_height,
                                  int target_height, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child),
    resampling_program_luma(0), resampling_program_chroma(0),
    filter_storage_luma_aligned(0), filter_storage_luma_unaligned(0),
    filter_storage_chroma_aligned(0), filter_storage_chroma_unaligned(0)
{
  if (target_height <= 0)
    env->ThrowError("Resize: Height must be greater than 0.");

  pixelsize = vi.ComponentSize(); // AVS16
  grey = vi.IsY();
  bool isRGBPfamily = vi.IsPlanarRGB() || vi.IsPlanarRGBA();

  if (vi.IsPlanar() && !grey && !isRGBPfamily) {
    const int mask = (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1;

    if (target_height & mask)
      env->ThrowError("Resize: Planar destination height must be a multiple of %d.", mask+1);
  }

  auto env2 = static_cast<IScriptEnvironment2*>(env);

  if (vi.IsRGB() && !isRGBPfamily)
    subrange_top = vi.height - subrange_top - subrange_height; // why?


  // Create resampling program and pitch table
  resampling_program_luma  = func->GetResamplingProgram(vi.height, subrange_top, subrange_height, target_height, env2);
  resampler_luma_aligned   = GetResampler(env->GetCPUFlags(), true , pixelsize, filter_storage_luma_aligned,   resampling_program_luma);
  resampler_luma_unaligned = GetResampler(env->GetCPUFlags(), false, pixelsize, filter_storage_luma_unaligned, resampling_program_luma);

  if (vi.IsPlanar() && !grey && !isRGBPfamily) {
    const int shift = vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int div   = 1 << shift;

    resampling_program_chroma = func->GetResamplingProgram(
                                  vi.height      >> shift,
                                  subrange_top    / div,
                                  subrange_height / div,
                                  target_height  >> shift,
                                  env2);

    resampler_chroma_aligned   = GetResampler(env->GetCPUFlags(), true , pixelsize, filter_storage_chroma_aligned,   resampling_program_chroma);
    resampler_chroma_unaligned = GetResampler(env->GetCPUFlags(), false, pixelsize, filter_storage_chroma_unaligned, resampling_program_chroma);
  }

  // Change target video info size
  vi.height = target_height;
}

PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  const BYTE* srcp = src->GetReadPtr();
        BYTE* dstp = dst->GetWritePtr();

  auto env2 = static_cast<IScriptEnvironment2*>(env);

  bool isRGBPfamily = vi.IsPlanarRGB() || vi.IsPlanarRGBA();

  // Create pitch table
  int* src_pitch_table_luma = static_cast<int*>(env2->Allocate(sizeof(int) * src->GetHeight(), 16, AVS_POOLED_ALLOC));
  if (!src_pitch_table_luma) {
    env->ThrowError("Could not reserve memory in a resampler.");
  }

  resize_v_create_pitch_table(src_pitch_table_luma, src->GetPitch(), src->GetHeight());

  int* src_pitch_table_chromaU = NULL;
  int* src_pitch_table_chromaV = NULL;
  if ((!grey && vi.IsPlanar() && !isRGBPfamily)) {
    src_pitch_table_chromaU = static_cast<int*>(env2->Allocate(sizeof(int) * src->GetHeight(PLANAR_U), 16, AVS_POOLED_ALLOC));
    src_pitch_table_chromaV = static_cast<int*>(env2->Allocate(sizeof(int) * src->GetHeight(PLANAR_V), 16, AVS_POOLED_ALLOC));
    if (!src_pitch_table_chromaU || !src_pitch_table_chromaV) {
      env2->Free(src_pitch_table_chromaU);
      env2->Free(src_pitch_table_chromaV);
      env->ThrowError("Could not reserve memory in a resampler.");
    }

    resize_v_create_pitch_table(src_pitch_table_chromaU, src->GetPitch(PLANAR_U), src->GetHeight(PLANAR_U));
    resize_v_create_pitch_table(src_pitch_table_chromaV, src->GetPitch(PLANAR_V), src->GetHeight(PLANAR_V));
  }

  // Do resizing
  int work_width = vi.IsPlanar() ? vi.width : vi.BytesFromPixels(vi.width) / pixelsize; // packed RGB: or vi.width * vi.NumComponent()
  if (IsPtrAligned(srcp, 16) && (src_pitch & 15) == 0)
    resampler_luma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_aligned);
  else
    resampler_luma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_unaligned);

  if(isRGBPfamily)
  {
    src_pitch = src->GetPitch(PLANAR_B);
    dst_pitch = dst->GetPitch(PLANAR_B);
    srcp = src->GetReadPtr(PLANAR_B);
    dstp = dst->GetWritePtr(PLANAR_B);
    if (IsPtrAligned(srcp, 16) && (src_pitch & 15) == 0)
      resampler_luma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_aligned);
    else
      resampler_luma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_unaligned);

    src_pitch = src->GetPitch(PLANAR_R);
    dst_pitch = dst->GetPitch(PLANAR_R);
    srcp = src->GetReadPtr(PLANAR_R);
    dstp = dst->GetWritePtr(PLANAR_R);
    if (IsPtrAligned(srcp, 16) && (src_pitch & 15) == 0)
      resampler_luma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_aligned);
    else
      resampler_luma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_luma, work_width, vi.height, src_pitch_table_luma, filter_storage_luma_unaligned);
  }
  else if (!grey && vi.IsPlanar()) {
    int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
    int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

    // Plane U resizing
    src_pitch = src->GetPitch(PLANAR_U);
    dst_pitch = dst->GetPitch(PLANAR_U);
    srcp = src->GetReadPtr(PLANAR_U);
    dstp = dst->GetWritePtr(PLANAR_U);

    if (IsPtrAligned(srcp, 16) && (src_pitch & 15) == 0)
      resampler_chroma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaU, filter_storage_chroma_unaligned);
    else
      resampler_chroma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaU, filter_storage_chroma_unaligned);

    // Plane V resizing
    src_pitch = src->GetPitch(PLANAR_V);
    dst_pitch = dst->GetPitch(PLANAR_V);
    srcp = src->GetReadPtr(PLANAR_V);
    dstp = dst->GetWritePtr(PLANAR_V);

    if (IsPtrAligned(srcp, 16) && (src_pitch & 15) == 0)
      resampler_chroma_aligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaV, filter_storage_chroma_unaligned);
    else
      resampler_chroma_unaligned(dstp, srcp, dst_pitch, src_pitch, resampling_program_chroma, width, height, src_pitch_table_chromaV, filter_storage_chroma_unaligned);
  }

  // Free pitch table
  env2->Free(src_pitch_table_luma);
  env2->Free(src_pitch_table_chromaU);
  env2->Free(src_pitch_table_chromaV);

  return dst;
}

ResamplerV FilteredResizeV::GetResampler(int CPU, bool aligned, int pixelsize, void*& storage, ResamplingProgram* program)
{
  if (program->filter_size == 1) {
    // Fast pointresize
    switch (pixelsize) // AVS16
    {
    case 1: return resize_v_planar_pointresize<uint8_t>;
    case 2: return resize_v_planar_pointresize<uint16_t>;
    default: // case 4:
      return resize_v_planar_pointresize<float>;
    }
  }
  else {
    // Other resizers
    if (pixelsize == 1)
    {
      if (CPU & CPUF_SSSE3) {
        if (aligned && CPU & CPUF_SSE4_1) {
          return resize_v_ssse3_planar<simd_load_streaming>;
        }
        else if (aligned) { // SSSE3 aligned
          return resize_v_ssse3_planar<simd_load_aligned>;
        }
        else if (CPU & CPUF_SSE3) { // SSE3 lddqu
          return resize_v_ssse3_planar<simd_load_unaligned_sse3>;
        }
        else { // SSSE3 unaligned
          return resize_v_ssse3_planar<simd_load_unaligned>;
        }
      }
      else if (CPU & CPUF_SSE2) {
        if (aligned && CPU & CPUF_SSE4_1) { // SSE4.1 movntdqa constantly provide ~2% performance increase in my testing
          return resize_v_sse2_planar<simd_load_streaming>;
        }
        else if (aligned) { // SSE2 aligned
          return resize_v_sse2_planar<simd_load_aligned>;
        }
        else if (CPU & CPUF_SSE3) { // SSE2 lddqu
          return resize_v_sse2_planar<simd_load_unaligned_sse3>;
        }
        else { // SSE2 unaligned
          return resize_v_sse2_planar<simd_load_unaligned>;
        }
#ifdef X86_32
      }
      else if (CPU & CPUF_MMX) {
        return resize_v_mmx_planar;
#endif
      }
      else { // C version
        return resize_v_c_planar<uint8_t>;
      }
    }
    else if (pixelsize == 2) {
      if (CPU & CPUF_SSE4_1) {
        if (aligned) {
          return resize_v_sseX_planar_16or32<simd_load_streaming, simd_loadps_aligned, true, uint16_t>;
        }
        else if (CPU & CPUF_SSE3) { // SSE3 lddqu
          return resize_v_sseX_planar_16or32<simd_load_unaligned_sse3, simd_loadps_unaligned, true, uint16_t>;
        }
        else { // unaligned
          return resize_v_sseX_planar_16or32<simd_load_unaligned, simd_loadps_unaligned, true, uint16_t>;
        }
      }
      else if (CPU & CPUF_SSE2) {
        if (aligned) {
          return resize_v_sseX_planar_16or32<simd_load_aligned, simd_loadps_aligned, false, uint16_t>;
        }
        else {
          return resize_v_sseX_planar_16or32<simd_load_unaligned, simd_loadps_unaligned, false, uint16_t>;
        }
      } else { // C version
        return resize_v_c_planar<uint16_t>;
      }
    }
    else { // pixelsize== 4
      // no special integer loading difference, no special sse4 case
      if (CPU & CPUF_SSE2) {
        if (aligned) {
          return resize_v_sseX_planar_16or32<simd_load_aligned, simd_loadps_aligned, false, float>;
        }
        else {
          return resize_v_sseX_planar_16or32<simd_load_unaligned, simd_loadps_unaligned, false, float>;
        }
      } else {
        return resize_v_c_planar<float>;
      }
    }
  }
}

FilteredResizeV::~FilteredResizeV(void)
{
  if (resampling_program_luma)   { delete resampling_program_luma; }
  if (resampling_program_chroma) { delete resampling_program_chroma; }
}


/**********************************************
 *******   Resampling Factory Methods   *******
 **********************************************/

PClip FilteredResize::CreateResizeH(PClip clip, double subrange_left, double subrange_width, int target_width,
                    ResamplingFunction* func, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_left == 0 && subrange_width == target_width && subrange_width == vi.width) {
    return clip;
  }

  if (subrange_left == int(subrange_left) && subrange_width == target_width
   && subrange_left >= 0 && subrange_left + subrange_width <= vi.width) {
    const int mask = ((vi.IsYUV() || vi.IsYUVA()) && !vi.IsY()) ? (1 << vi.GetPlaneWidthSubsampling(PLANAR_U)) - 1 : 0;

    if (((int(subrange_left) | int(subrange_width)) & mask) == 0)
      return new Crop(int(subrange_left), 0, int(subrange_width), vi.height, 0, clip, env);
  }

  // Convert interleaved yuv to planar yuv
  PClip result = clip;
  if (vi.IsYUY2()) {
    result = new ConvertYUY2ToYV16(result,  env);
  }
  result = new FilteredResizeH(result, subrange_left, subrange_width, target_width, func, env);
  if (vi.IsYUY2()) {
    result = new ConvertYV16ToYUY2(result,  env);
  }

  return result;
}


PClip FilteredResize::CreateResizeV(PClip clip, double subrange_top, double subrange_height, int target_height,
                    ResamplingFunction* func, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_top == 0 && subrange_height == target_height && subrange_height == vi.height) {
    return clip;
  }

  if (subrange_top == int(subrange_top) && subrange_height == target_height
   && subrange_top >= 0 && subrange_top + subrange_height <= vi.height) {
    const int mask = ((vi.IsYUV() || vi.IsYUVA()) && !vi.IsY()) ? (1 << vi.GetPlaneHeightSubsampling(PLANAR_U)) - 1 : 0;

    if (((int(subrange_top) | int(subrange_height)) & mask) == 0)
      return new Crop(0, int(subrange_top), vi.width, int(subrange_height), 0, clip, env);
  }
  return new FilteredResizeV(clip, subrange_top, subrange_height, target_height, func, env);
}


PClip FilteredResize::CreateResize(PClip clip, int target_width, int target_height, const AVSValue* args,
                   ResamplingFunction* f, IScriptEnvironment* env)
{
  const VideoInfo& vi = clip->GetVideoInfo();
  const double subrange_left = args[0].AsFloat(0), subrange_top = args[1].AsFloat(0);

  double subrange_width = args[2].AsDblDef(vi.width), subrange_height = args[3].AsDblDef(vi.height);
  // Crop style syntax
  if (subrange_width  <= 0.0) subrange_width  = vi.width  - subrange_left + subrange_width;
  if (subrange_height <= 0.0) subrange_height = vi.height - subrange_top  + subrange_height;

  PClip result;
  // ensure that the intermediate area is maximal
  const double area_FirstH = subrange_height * target_width;
  const double area_FirstV = subrange_width * target_height;
  if (area_FirstH < area_FirstV)
  {
      result = CreateResizeV(clip, subrange_top, subrange_height, target_height, f, env);
      result = CreateResizeH(result, subrange_left, subrange_width, target_width, f, env);
  }
  else
  {
      result = CreateResizeH(clip, subrange_left, subrange_width, target_width, f, env);
      result = CreateResizeV(result, subrange_top, subrange_height, target_height, f, env);
  }
  return result;
}

AVSValue __cdecl FilteredResize::Create_PointResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = PointFilter();
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}


AVSValue __cdecl FilteredResize::Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = TriangleFilter();
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}


AVSValue __cdecl FilteredResize::Create_BicubicResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = MitchellNetravaliFilter(args[3].AsDblDef(1. / 3.), args[4].AsDblDef(1. / 3.));
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[5], &f, env);
}

AVSValue __cdecl FilteredResize::Create_LanczosResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = LanczosFilter(args[7].AsInt(3));
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_Lanczos4Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = LanczosFilter(4);
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_BlackmanResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = BlackmanFilter(args[7].AsInt(4));
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_Spline16Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = Spline16Filter();
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_Spline36Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = Spline36Filter();
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_Spline64Resize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = Spline64Filter();
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_GaussianResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = GaussianFilter(args[7].AsFloat(30.0f));
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

AVSValue __cdecl FilteredResize::Create_SincResize(AVSValue args, void*, IScriptEnvironment* env)
{
  auto f = SincFilter(args[7].AsInt(4));
  return CreateResize(args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], &f, env);
}

