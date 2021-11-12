// Avisynth v2.5.  Copyright 2002-2009 Ben Rudiak-Gould et al.
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


#include <avs/alignment.h>
#include <avs/minmax.h>

#ifdef AVS_WINDOWS
    #include <intrin.h>
#else
    #include <x86intrin.h>
#endif

#ifndef _mm256_set_m128i
#define _mm256_set_m128i(v0, v1) _mm256_insertf128_si256(_mm256_castsi128_si256(v1), (v0), 1)
#endif

#ifndef _mm256_set_m128
#define _mm256_set_m128(v0, v1) _mm256_insertf128_ps(_mm256_castps128_ps256(v1), (v0), 1)
#endif

#include "convert_bits_avx2.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4305 4309)
#endif

template<typename pixel_t, bool chroma, bool fulls, bool fulld>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
void convert_32_to_uintN_avx2(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const float *srcp = reinterpret_cast<const float *>(srcp8);
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);

  src_pitch = src_pitch / sizeof(float);
  dst_pitch = dst_pitch / sizeof(pixel_t);

  int src_width = src_rowsize / sizeof(float);

  const int max_pixel_value = (1 << target_bitdepth) - 1;

  const int limit_lo_d = (fulld ? 0 : 16) << (target_bitdepth - 8);
  const int limit_hi_d = fulld ? ((1 << target_bitdepth) - 1) : ((chroma ? 240 : 235) << (target_bitdepth - 8));
  const float range_diff_d = (float)limit_hi_d - limit_lo_d;

  const int limit_lo_s = fulls ? 0 : 16;
  const int limit_hi_s = fulls ? 255 : (chroma ? 240 : 235);
  const float range_diff_s = (limit_hi_s - limit_lo_s) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240       Y = Y * ((235-16) << (bpp-8)) + 16, Chroma= Chroma * ((240-16) << (bpp-8)) + 16
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255
  const float factor = range_diff_d / range_diff_s;

  const float half_i = (float)(1 << (target_bitdepth - 1));
  const __m256 halfint_plus_rounder_ps = _mm256_set1_ps(half_i + 0.5f);
  const __m256 limit_lo_s_ps = _mm256_set1_ps(limit_lo_s / 255.0f);
  const __m256 limit_lo_plus_rounder_ps = _mm256_set1_ps(limit_lo_d + 0.5f);
  const __m256 max_dst_pixelvalue = _mm256_set1_ps((float)max_pixel_value); // 255, 1023, 4095, 16383, 65535.0
  const __m256 zero = _mm256_setzero_ps();

  __m256 factor_ps = _mm256_set1_ps(factor);

  for (int y = 0; y < src_height; y++)
  {
    for (int x = 0; x < src_width; x += 16) // 16 pixels at a time (64 byte - alignment is OK)
    {
      __m256i result;
      __m256i result_0, result_1;
      __m256 src_0 = _mm256_load_ps(reinterpret_cast<const float *>(srcp + x));
      __m256 src_1 = _mm256_load_ps(reinterpret_cast<const float *>(srcp + x + 8));
      if (chroma) {
        src_0 = _mm256_fmadd_ps(src_0, factor_ps, halfint_plus_rounder_ps);
        src_1 = _mm256_fmadd_ps(src_1, factor_ps, halfint_plus_rounder_ps);
      }
      else {
        if constexpr(!fulls) {
          src_0 = _mm256_sub_ps(src_0, limit_lo_s_ps);
          src_1 = _mm256_sub_ps(src_1, limit_lo_s_ps);
        }
        src_0 = _mm256_fmadd_ps(src_0, factor_ps, limit_lo_plus_rounder_ps);
        src_1 = _mm256_fmadd_ps(src_1, factor_ps, limit_lo_plus_rounder_ps);
        //pixel = (srcp0[x] - limit_lo_s_ps) * factor + half + limit_lo + 0.5f;
      }

      src_0 = _mm256_max_ps(_mm256_min_ps(src_0, max_dst_pixelvalue), zero);
      src_1 = _mm256_max_ps(_mm256_min_ps(src_1, max_dst_pixelvalue), zero);
      result_0 = _mm256_cvttps_epi32(src_0); // truncate
      result_1 = _mm256_cvttps_epi32(src_1);
      if constexpr(sizeof(pixel_t) == 2) {
        result = _mm256_packus_epi32(result_0, result_1);
        result = _mm256_permute4x64_epi64(result, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp + x), result);
      }
      else {
        result = _mm256_packs_epi32(result_0, result_1);
        result = _mm256_permute4x64_epi64(result, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
        __m128i result128_lo = _mm256_castsi256_si128(result);
        __m128i result128_hi = _mm256_extractf128_si256(result, 1);
        __m128i result128 = _mm_packus_epi16(result128_lo, result128_hi);
        _mm_store_si128(reinterpret_cast<__m128i *>(dstp + x), result128);
      }
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm256_zeroupper();
}

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#define convert_32_to_uintN_avx2_functions(type) \
template void convert_32_to_uintN_avx2<type, false, true, true>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, true, true, true>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, false, true, false>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, true, true, false>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, false, false, true>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, true, false, true>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, false, false, false>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_32_to_uintN_avx2<type, true, false, false>(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth);

convert_32_to_uintN_avx2_functions(uint8_t)
convert_32_to_uintN_avx2_functions(uint16_t)

#undef convert_32_to_uintN_avx2_functions

// YUV: bit shift 10-12-14-16 <=> 10-12-14-16 bits
// shift right or left, depending on expandrange
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
void convert_uint16_to_uint16_c_avx2(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint16_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    const int src_width = src_rowsize / sizeof(uint16_t);
    if (target_bitdepth > source_bitdepth) // expandrange
    {
      const int shift_bits = target_bitdepth - source_bitdepth;
      for (int y = 0; y < src_height; y++)
      {
        for (int x = 0; x < src_width; x++) {
          dstp0[x] = srcp0[x] << shift_bits;  // expand range. No clamp before, source is assumed to have valid range
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
      }
    }
    else
    {
      // reduce range
      const int shift_bits = source_bitdepth - target_bitdepth;
      const int round = 1 << (shift_bits - 1);
      for (int y = 0; y < src_height; y++)
      {
        for (int x = 0; x < src_width; x++) {
            dstp0[x] = (srcp0[x] + round) >> shift_bits;  // reduce range
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
      }
    }
    _mm256_zeroupper();
}

// YUV: bit shift 8-16 <=> 8-16 bits
// shift right or left, depending on expandrange
template<typename pixel_t_s, typename pixel_t_d>
static void convert_uint_limited_avx2(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const pixel_t_s* srcp0 = reinterpret_cast<const pixel_t_s*>(srcp);
  pixel_t_d* dstp0 = reinterpret_cast<pixel_t_d*>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t_s);
  dst_pitch = dst_pitch / sizeof(pixel_t_d);

  const int src_width = src_rowsize / sizeof(pixel_t_s);
  int wmod = (src_width / 32) * 32;

  auto zero = _mm256_setzero_si256();

  if (target_bitdepth > source_bitdepth) // expandrange. pixel_t_d is always uint16_t
  {
    const int shift_bits = target_bitdepth - source_bitdepth;
    __m128i shift = _mm_set_epi32(0, 0, 0, shift_bits);
    for (int y = 0; y < src_height; y++)
    {
      for (int x = 0; x < src_width; x += 32)
      {
        __m256i src_lo, src_hi;
        if constexpr (sizeof(pixel_t_s) == 1) {
          auto src_lo_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x)); // 16* uint8
          auto src_hi_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x + 16)); // 16* uint8
          src_lo = _mm256_cvtepu8_epi16(src_lo_128); // 16* uint16
          src_hi = _mm256_cvtepu8_epi16(src_hi_128); // 16* uint16
        }
        else {
          src_lo = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x)); // 16* uint_16
          src_hi = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x + 16)); // 16* uint_16
        }
        src_lo = _mm256_sll_epi16(src_lo, shift);
        src_hi = _mm256_sll_epi16(src_hi, shift);
        if constexpr (sizeof(pixel_t_d) == 1) {
          // upconvert always to 2 bytes
          assert(0);
        }
        else {
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), src_lo);
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x + 16), src_hi);
        }
      }
      // rest
      for (int x = wmod; x < src_width; x++) {
        dstp0[x] = (pixel_t_d)(srcp0[x]) << shift_bits;  // expand range. No clamp before, source is assumed to have valid range
      }
      dstp0 += dst_pitch;
      srcp0 += src_pitch;
    }
  }
  else
  {
    // reduce range
    const int shift_bits = source_bitdepth - target_bitdepth;
    const int round = 1 << (shift_bits - 1);
    __m128i shift = _mm_set_epi32(0, 0, 0, shift_bits);
    const auto round_simd = _mm256_set1_epi16(round);

    auto zero = _mm256_setzero_si256();

    for (int y = 0; y < src_height; y++)
    {
      for (int x = 0; x < src_width; x += 32)
      {
        if constexpr (sizeof(pixel_t_s) == 1)
          assert(0);
        // downconvert always from 2 bytes
        auto src_lo = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x)); // 16* uint_16
        auto src_hi = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x + 16)); // 16* uint_16
        src_lo = _mm256_srl_epi16(_mm256_adds_epu16(src_lo, round_simd), shift);
        src_hi = _mm256_srl_epi16(_mm256_adds_epu16(src_hi, round_simd), shift);
        if constexpr (sizeof(pixel_t_d) == 1) {
          // to 8 bits
          auto dst = _mm256_packus_epi16(src_lo, src_hi);
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), dst);
        }
        else {
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), src_lo);
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x + 16), src_hi);
        }
      }
      // rest
      for (int x = wmod; x < src_width; x++) {
        dstp0[x] = (srcp0[x] + round) >> shift_bits;  // reduce range
      }
      dstp0 += dst_pitch;
      srcp0 += src_pitch;
    }
  }
}

template<typename pixel_t_s, typename pixel_t_d, bool chroma, bool fulls, bool fulld>
void convert_uint_avx2(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  // limited to limited is bitshift, see in other function
  if constexpr (!fulls && !fulld) {
    convert_uint_limited_avx2<pixel_t_s, pixel_t_d>(srcp, dstp, src_rowsize, src_height, src_pitch, dst_pitch, source_bitdepth, target_bitdepth);
    return;
  }

  const pixel_t_s* srcp0 = reinterpret_cast<const pixel_t_s*>(srcp);
  pixel_t_d* dstp0 = reinterpret_cast<pixel_t_d*>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t_s);
  dst_pitch = dst_pitch / sizeof(pixel_t_d);

  int src_width = src_rowsize / sizeof(pixel_t_s);
  int wmod32 = (src_width / 32) * 32;

  if constexpr (sizeof(pixel_t_s) == 1 && sizeof(pixel_t_d) == 2) {
    if (fulls && fulld && !chroma && source_bitdepth == 8 && target_bitdepth == 16) {
      // special case 8->16 bit full scale: * 65535 / 255 = *257
      auto zero = _mm256_setzero_si256();
      __m256i multiplier = _mm256_set1_epi16(257);

      for (int y = 0; y < src_height; y++)
      {
        for (int x = 0; x < src_width; x += 32)
        {
          auto src_lo_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x)); // 16* uint8
          auto src_hi_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x + 16)); // 16* uint8
          auto src_lo = _mm256_cvtepu8_epi16(src_lo_128); // 16* uint16
          auto src_hi = _mm256_cvtepu8_epi16(src_hi_128); // 16* uint16

            // *257 mullo is faster than x*257 = (x<<8 + x) add/or solution (i7)
          auto res_lo = _mm256_mullo_epi16(src_lo, multiplier); // lower 16 bit of multiplication is enough
          auto res_hi = _mm256_mullo_epi16(src_hi, multiplier);
          // dstp[x] = srcp0[x] * 257; // RGB: full range 0..255 <-> 0..65535 (257 = 65535 / 255)
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), res_lo);
          _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x + 16), res_hi);
        } // for x
        // rest
        for (int x = wmod32; x < src_width; x++)
        {
          dstp0[x] = (pixel_t_d)(srcp0[x] * 257);
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
      } // for y
      return;
    }
  }

  const int target_max = (1 << target_bitdepth) - 1;

  float mul_factor;
  int src_offset, dst_offset;

  if constexpr (chroma) {
    // chroma: go into signed world, factor, then go back to biased range
    src_offset = 1 << (source_bitdepth - 1); // chroma center of source
    dst_offset = 1 << (target_bitdepth - 1); // chroma center of target
    mul_factor =
      fulls && fulld ? (float)(dst_offset - 1) / (src_offset - 1) :
      fulld ? (float)(dst_offset - 1) / (112 << (source_bitdepth - 8)) : // +-127 ==> +-112 (240-16)/2
      /*fulld*/ (float)(112 << (target_bitdepth - 8)) / (src_offset - 1); // +-112 (240-16)/2 ==> +-127
  }
  else {
    // luma/limited: subtract offset, convert, add offset
    src_offset = fulls ? 0 : 16 << (source_bitdepth - 8); // full/limited range low limit
    dst_offset = fulld ? 0 : 16 << (target_bitdepth - 8); // // full/limited range low limit
    const int source_max = (1 << source_bitdepth) - 1;
    mul_factor =
      fulls && fulld ? (float)target_max / source_max :
      fulld ? (float)target_max / (219 << (source_bitdepth - 8)) : // 0..255 ==> 16-235 (219)
      /*fulld*/ (float)(219 << (target_bitdepth - 8)) / source_max; // 16-235 ==> 0..255
  }

  auto vect_mul_factor = _mm256_set1_ps(mul_factor);
  auto vect_src_offset = _mm256_set1_epi32(src_offset);
  auto dst_offset_plus_round = dst_offset + 0.5f;
  auto vect_dst_offset_plus_round = _mm256_set1_ps(dst_offset_plus_round);

  auto vect_target_max = _mm256_set1_epi16(target_max);
  constexpr auto target_min = chroma && fulld ? 1 : 0;
  auto vect_target_min = _mm256_set1_epi32(target_min); // chroma-only. 0 is not valid in full case. We leave limited as is.

  auto zero = _mm256_setzero_si256();

  for (int y = 0; y < src_height; y++)
  {
    for (int x = 0; x < src_width; x += 32)
    {
      __m256i src_lo, src_hi;
      if constexpr (sizeof(pixel_t_s) == 1) {
        auto src_lo_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x)); // 16* uint8
        auto src_hi_128 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp0 + x + 16)); // 16* uint8
        src_lo = _mm256_cvtepu8_epi16(src_lo_128); // 16* uint16
        src_hi = _mm256_cvtepu8_epi16(src_hi_128); // 16* uint16
      }
      else {
        src_lo = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x)); // 8* uint_16
        src_hi = _mm256_load_si256(reinterpret_cast<const __m256i*>(srcp0 + x + 16)); // 8* uint_16
      }

      __m256i result1, result2;
      {
        // src: 8*uint16
        // convert to int32, bias, to float
        auto res_lo_i = _mm256_unpacklo_epi16(src_lo, zero);
        auto res_hi_i = _mm256_unpackhi_epi16(src_lo, zero);
        if constexpr (chroma || !fulls) {
          // src_offset is not zero
          res_lo_i = _mm256_sub_epi32(res_lo_i, vect_src_offset);
          res_hi_i = _mm256_sub_epi32(res_hi_i, vect_src_offset);
        }
        auto res_lo = _mm256_cvtepi32_ps(res_lo_i);
        auto res_hi = _mm256_cvtepi32_ps(res_hi_i);
        // multiply, bias back+round
        // avx2 mode: smart fma instruction inserted even from msvc!
        res_lo = _mm256_add_ps(_mm256_mul_ps(res_lo, vect_mul_factor), vect_dst_offset_plus_round);
        res_hi = _mm256_add_ps(_mm256_mul_ps(res_hi, vect_mul_factor), vect_dst_offset_plus_round);
        // convert back w/ truncate
        auto result_l = _mm256_cvttps_epi32(res_lo); // no banker's rounding
        auto result_h = _mm256_cvttps_epi32(res_hi);
        if constexpr (chroma) {
          result_l = _mm256_max_epi32(result_l, vect_target_min);
          result_h = _mm256_max_epi32(result_h, vect_target_min);
        }
        // back to 16 bit
        result1 = _mm256_packus_epi32(result_l, result_h); // 8 * 16 bit pixels
        result1 = _mm256_min_epu16(result1, vect_target_max);
      }

      // byte target: not yet
      if constexpr (sizeof(pixel_t_d) == 2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), result1);

      {
        // src: 8*uint16
        // convert to int32, bias, to float
        auto res_lo_i = _mm256_unpacklo_epi16(src_hi, zero);
        auto res_hi_i = _mm256_unpackhi_epi16(src_hi, zero);
        if constexpr (chroma || !fulls) {
          // src_offset is not zero
          res_lo_i = _mm256_sub_epi32(res_lo_i, vect_src_offset);
          res_hi_i = _mm256_sub_epi32(res_hi_i, vect_src_offset);
        }
        auto res_lo = _mm256_cvtepi32_ps(res_lo_i);
        auto res_hi = _mm256_cvtepi32_ps(res_hi_i);
        // multiply, bias back+round
        res_lo = _mm256_add_ps(_mm256_mul_ps(res_lo, vect_mul_factor), vect_dst_offset_plus_round);
        res_hi = _mm256_add_ps(_mm256_mul_ps(res_hi, vect_mul_factor), vect_dst_offset_plus_round);
        // convert back w/ truncate
        auto result_l = _mm256_cvttps_epi32(res_lo);
        auto result_h = _mm256_cvttps_epi32(res_hi);
        if constexpr (chroma) {
          result_l = _mm256_max_epi32(result_l, vect_target_min);
          result_h = _mm256_max_epi32(result_h, vect_target_min);
        }
        // back to 16 bit
        result2 = _mm256_packus_epi32(result_l, result_h); // 8 * 16 bit pixels
        result2 = _mm256_min_epu16(result2, vect_target_max);
      }

      if constexpr (sizeof(pixel_t_d) == 2)
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x + 16), result2);
      else {
        // byte target: store both
        auto result12 = _mm256_packus_epi16(result1, result2);
        result12 = _mm256_permute4x64_epi64(result12, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp0 + x), result12);
      }
    } // for x

    // rest
    for (int x = wmod32; x < src_width; x++)
    {
      const float val = (srcp0[x] - src_offset) * mul_factor + dst_offset_plus_round;
      dstp0[x] = clamp((int)val, target_min, target_max);
    }

    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

// instantiate them all
#define convert_uint_avx2_functions(uint_X_t, uint_X_dest_t) \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, false, false, false>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, false, false, true>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, false, true, false>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, false, true, true>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, true, false, false>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, true, false, true>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, true, true, false>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth); \
template void convert_uint_avx2<uint_X_t, uint_X_dest_t, true, true, true>(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth);

convert_uint_avx2_functions(uint8_t, uint8_t)
convert_uint_avx2_functions(uint8_t, uint16_t)
convert_uint_avx2_functions(uint16_t, uint8_t)
convert_uint_avx2_functions(uint16_t, uint16_t)

#undef convert_uint_avx2_functions

