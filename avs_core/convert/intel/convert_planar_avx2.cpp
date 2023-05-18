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


#include <avs/alignment.h>
#ifdef AVS_WINDOWS
    #include <intrin.h>
#else
    #include <x86intrin.h>
#endif

#include "convert_planar_avx2.h"

#ifndef _mm256_set_m128i
#define _mm256_set_m128i(v0, v1) _mm256_insertf128_si256(_mm256_castsi128_si256(v1), (v0), 1)
#endif

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4309)
#endif

// packed rgb helper
static AVS_FORCEINLINE __m256i convert_yuv_to_rgb_avx2_core(const __m256i& px0189, const __m256i& px23AB, const __m256i& px45CD, const __m256i& px67EF, const __m256i& zero, const __m256i& matrix, const __m256i& round_mask) {
  //int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);

  //px01 - xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0

  auto low_lo = _mm256_madd_epi16(px0189, matrix); //xx*0 + v1*m2 | u1*m1 + y1*m0 | xx*0 + v0*m2 | u0*m1 + y0*m0
  auto low_hi = _mm256_madd_epi16(px23AB, matrix); //xx*0 + v3*m2 | u3*m1 + y3*m0 | xx*0 + v2*m2 | u2*m1 + y2*m0
  auto high_lo = _mm256_madd_epi16(px45CD, matrix);
  auto high_hi = _mm256_madd_epi16(px67EF, matrix);

  auto low_v = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(low_lo), _mm256_castsi256_ps(low_hi), _MM_SHUFFLE(3, 1, 3, 1))); // v3*m2 | v2*m2 | v1*m2 | v0*m2
  auto high_v = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(high_lo), _mm256_castsi256_ps(high_hi), _MM_SHUFFLE(3, 1, 3, 1)));

  auto low_yu = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(low_lo), _mm256_castsi256_ps(low_hi), _MM_SHUFFLE(2, 0, 2, 0))); // u3*m1 + y3*m0 | u2*m1 + y2*m0 | u1*m1 + y1*m0 | u0*m1 + y0*m0
  auto high_yu = _mm256_castps_si256(_mm256_shuffle_ps(_mm256_castsi256_ps(high_lo), _mm256_castsi256_ps(high_hi), _MM_SHUFFLE(2, 0, 2, 0)));

  auto t_lo = _mm256_add_epi32(low_v, low_yu); // v3*m2 + u3*m1 + y3*m0...
  auto t_hi = _mm256_add_epi32(high_v, high_yu);

  t_lo = _mm256_add_epi32(t_lo, round_mask); // v3*m2 + u3*m1 + y3*m0 + 4096...
  t_hi = _mm256_add_epi32(t_hi, round_mask);

  t_lo = _mm256_srai_epi32(t_lo, 13); // (v3*m2 + u3*m1 + y3*m0 + 4096) >> 13...
  t_hi = _mm256_srai_epi32(t_hi, 13);

  auto result = _mm256_packs_epi32(t_lo, t_hi);
  result = _mm256_packus_epi16(result, zero); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
  return result;
}

template<int rgb_pixel_step, bool hasAlpha>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
void convert_yv24_to_rgb_avx2(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, const BYTE* srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix& matrix) {
  dstp += dst_pitch * (height - 1);  // We start at last line

  size_t mod16_width = rgb_pixel_step == 3 ? width / 16 * 16 : width;
  // for rgb32 target we can process pixels beyond width, but we have 64bit alignment at target 16 pixels to 16*4=64 rgb pixels
  // if alignment would only be 32 bytes, we'd do the last cycle to process only 8 source pixels

  auto matrix_b = _mm256_set_epi16(0, matrix.v_b, matrix.u_b, matrix.y_b, 0, matrix.v_b, matrix.u_b, matrix.y_b, 0, matrix.v_b, matrix.u_b, matrix.y_b, 0, matrix.v_b, matrix.u_b, matrix.y_b);
  auto matrix_g = _mm256_set_epi16(0, matrix.v_g, matrix.u_g, matrix.y_g, 0, matrix.v_g, matrix.u_g, matrix.y_g, 0, matrix.v_g, matrix.u_g, matrix.y_g, 0, matrix.v_g, matrix.u_g, matrix.y_g);
  auto matrix_r = _mm256_set_epi16(0, matrix.v_r, matrix.u_r, matrix.y_r, 0, matrix.v_r, matrix.u_r, matrix.y_r, 0, matrix.v_r, matrix.u_r, matrix.y_r, 0, matrix.v_r, matrix.u_r, matrix.y_r);

  auto zero128 = _mm_setzero_si128();
  auto zero = _mm256_setzero_si256();
  auto round_mask = _mm256_set1_epi32(4096);
  auto offset = _mm256_set_epi16(0, -128, -128, matrix.offset_y, 0, -128, -128, matrix.offset_y, 0, -128, -128, matrix.offset_y, 0, -128, -128, matrix.offset_y);

  // 16 YUV(A) pixels --> 4x16 RGB quads = 64 bytes. Avisynth's alignment is 64 fortunately.

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x += 16) {
      __m128i src_y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY + x)); //Y15 .. Y7 Y6 Y5 Y4 Y3 Y2 Y1 Y0
      __m128i src_u = _mm_load_si128(reinterpret_cast<const __m128i*>(srcU + x)); //U15 .. U7 U6 U5 U4 U3 U2 U1 U0
      __m128i src_v = _mm_load_si128(reinterpret_cast<const __m128i*>(srcV + x)); //V15 .. V7 V6 V5 V4 V3 V2 V1 V0
      __m128i src_a;
      if constexpr(hasAlpha)
        src_a = _mm_load_si128(reinterpret_cast<const __m128i*>(srcA + x)); //A15 .. A7 A6 A5 A4 A3 A2 A1 A0

      __m128i t1_lo = _mm_unpacklo_epi8(src_y, src_u); //U7 Y7 U6 Y6 U5 Y5 U4 Y4 U3 Y3 U2 Y2 U1 Y1 U0 Y0
      __m128i t1_hi = _mm_unpackhi_epi8(src_y, src_u); //U15 Y15 U14 Y14 U13 Y13 U12 Y12 U11 Y11 U10 Y10 U9 Y9 U8 Y8
      __m128i t2_lo = _mm_unpacklo_epi8(src_v, zero128);  //00 V7 00 V6 00 V5 00 V4 00 V3 00 V2 00 V1 00 V0
      __m128i t2_hi = _mm_unpackhi_epi8(src_v, zero128);  //00 V15 00 V14 00 V13 00 V12 00 V11 00 V10 00 V9 00 V8

      __m256i t1 = _mm256_set_m128i(t1_hi, t1_lo);
      __m256i t2 = _mm256_set_m128i(t2_hi, t2_lo);
      t1 = _mm256_permute4x64_epi64(t1, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
      t2 = _mm256_permute4x64_epi64(t2, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));

      __m256i low = _mm256_unpacklo_epi16(t1, t2); //xx V11 U11 Y11 xx V10 U10 Y10 xx V9 U9 Y9 xx V8 U8 Y8    xx V3 U3 Y3 xx V2 U2 Y2 xx V1 U1 Y1 xx V0 U0 Y0
      __m256i high = _mm256_unpackhi_epi16(t1, t2); //xx V15 U15 Y15 xx V14 U14 Y14 xx V13 U13 Y13 xx V12 U12 Y12    xx V7 U7 Y7 xx V6 U6 Y6 xx V5 U5 Y5 xx V4 U4 Y4

      __m256i px0189 = _mm256_unpacklo_epi8(low, zero);  //xx xx 00 V1 00 U1 00 Y1 xx xx 00 V0 00 U0 00 Y0
      __m256i px23AB = _mm256_unpackhi_epi8(low, zero);  //xx xx 00 V3 00 U3 00 Y3 xx xx 00 V2 00 U2 00 Y2
      __m256i px45CD = _mm256_unpacklo_epi8(high, zero); //xx xx 00 V5 00 U5 00 Y5 xx xx 00 V4 00 U4 00 Y4
      __m256i px67EF = _mm256_unpackhi_epi8(high, zero); //xx xx 00 V7 00 U7 00 Y7 xx xx 00 V6 00 U6 00 Y6

      px0189 = _mm256_add_epi16(px0189, offset);
      px23AB = _mm256_add_epi16(px23AB, offset);
      px45CD = _mm256_add_epi16(px45CD, offset);
      px67EF = _mm256_add_epi16(px67EF, offset);

      __m256i result_b = convert_yuv_to_rgb_avx2_core(px0189, px23AB, px45CD, px67EF, zero, matrix_b, round_mask); //00 00 00 00 00 00 00 00 b7 b6 b5 b4 b3 b2 b1 b0
      __m256i result_g = convert_yuv_to_rgb_avx2_core(px0189, px23AB, px45CD, px67EF, zero, matrix_g, round_mask); //00 00 00 00 00 00 00 00 g7 g6 g5 g4 g3 g2 g1 g0
      __m256i result_r = convert_yuv_to_rgb_avx2_core(px0189, px23AB, px45CD, px67EF, zero, matrix_r, round_mask); //00 00 00 00 00 00 00 00 r7 r6 r5 r4 r3 r2 r1 r0

      __m256i result_bg = _mm256_unpacklo_epi8(result_b, result_g); //g7 b7 g6 b6 g5 b5 g4 b4 g3 b3 g2 b2 g1 b1 g0 b0
      __m256i alpha;
      if (hasAlpha) {
        __m128i a_lo = _mm_unpacklo_epi8(src_a, zero128);  //00 A7 00 A6 00 A5 00 A4 00 A3 00 A2 00 A1 00 A0
        __m128i a_hi = _mm_unpackhi_epi8(src_a, zero128);  //00 A15 00 A14 00 A13 00 A12 00 A11 00 A10 00 A9 00 A8
        alpha = _mm256_set_m128i(a_hi, a_lo); // a15 .. a0  at low of each m128i part
        alpha = _mm256_permute4x64_epi64(alpha, (0 << 0) | (2 << 2) | (1 << 4) | (3 << 6));
      }
      else
        alpha = _mm256_cmpeq_epi32(result_r, result_r); // FF FF FF FF ... default alpha transparent

      __m256i result_ra = _mm256_unpacklo_epi8(result_r, alpha);       //a7 r7 a6 r6 a5 r5 a4 r4 a3 r3 a2 r2 a1 r1 a0 r0

      __m256i result_lo = _mm256_unpacklo_epi16(result_bg, result_ra);
      __m256i result_hi = _mm256_unpackhi_epi16(result_bg, result_ra);

      if constexpr (rgb_pixel_step == 4) {
        //rgb32
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp + x * 4), result_lo);
        _mm256_store_si256(reinterpret_cast<__m256i*>(dstp + x * 4 + 32), result_hi);
      }
      else {
        // rgb24
        // 16*4 bytes to 16*3 bytes
        __m256i perm10 = _mm256_set_epi32(0, 0, 6, 5, 4, 2, 1, 0);
        __m256i shuffle_lo = _mm256_set_epi8(
            0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0,
            0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0
        );
        __m128i shuffle_hi_lo = _mm_set_epi8(9, 8, 6, 5,  4,  2,  1,  0, 0, 0, 0, 0, 0, 0, 0, 0);

        __m128i result_hi_lo = _mm256_extracti128_si256(result_hi, 0); // aBbBgBrB aAbAgArA a9b9g9r9 a8b8g8r8
        __m128i result_hi_hi = _mm256_extracti128_si256(result_hi, 1); // aFbFgFrF aEbEgErE aDbDgDrD aCbCgCrC
        
        __m256i result_lo_reorg = _mm256_shuffle_epi8(result_lo, shuffle_lo);
        // x  x  x  x  b7g7r7 b6g6r6 b5g5r5 b4g4r4 x  x  x  x  b3g3r3 b2g2r2 b1g1r1 b0g0r0
        result_lo_reorg = _mm256_permutevar8x32_epi32(result_lo_reorg, perm10);
        // x  x  x  x  x x x x b7g7r7 b6g6r6 b5g5r5 b4g4r4 b3g3r3 b2g2r2 b1g1r1 b0g0r0

        __m128i result_hi_lo_reorg = _mm_shuffle_epi8(result_hi_lo, shuffle_hi_lo);
        // gA rA b9 g9 r9 b8 g8 r8 x x x x x x x x

        __m256i dummy_y0 = _mm256_undefined_si256();
        auto result_hi_lo_reorg_2 = _mm256_inserti128_si256(dummy_y0, result_hi_lo_reorg, 1);
        //                                                                      
        // gA rA b9 g9|r9 b8 g8 r8|x x x  x|x x  x x|x  x x x |x x x  x|x x  x x|x  x x x   // result_hi_lo_reorg_2
        // x  x  x  x  x  x  x  x  b7g7r7 b6g6r6 b5g5r5 b4g4r4 b3g3r3 b2g2r2 b1g1r1 b0g0r0  // result_lo_reorg
        auto result_0_15 = _mm256_blend_epi32(result_lo_reorg, result_hi_lo_reorg_2, 0xC0); // 11000000
        _mm256_storeu_si256(reinterpret_cast<__m256i*>(dstp + x * 3), result_0_15); // not necessarily 32 bytes aligned

        // => x  x  x  x  x  x  x  x  x  x  x  x  bB gB rB bA
        __m128i shuffle_hi_lo_2 = _mm_set_epi8(0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 14, 13, 12, 10);
        __m128i xmm4 = _mm_shuffle_epi8(result_hi_lo, shuffle_hi_lo_2);

        // => bF gF rF bE gE rE bD gD rD bC gC rC x  x  x  x
        __m128i shuffle_hi_hi = _mm_set_epi8(14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0, 0x80, 0x80, 0x80, 0x80);
        __m128i xmm5 = _mm_shuffle_epi8(result_hi_hi, shuffle_hi_hi);

        // => bF gF rF bE gE rE bD gD rD bC gC rC bB gB rB bA
        __m128i result_16_23 = _mm_or_si128(xmm4, xmm5);
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp + x * 3 + 32), result_16_23);
#if 0
        // Intel compiler can cope with it 100% optimized. No store, just shuffles and blends as above.
        alignas(32) BYTE temp[64];
        _mm256_store_si256(reinterpret_cast<__m256i*>(temp), result_lo);
        _mm256_store_si256(reinterpret_cast<__m256i*>(temp + 32), result_hi);
        for (int i = 0; i < 16; ++i) {
          dstp[(x + i) * 3 + 0] = temp[i * 4 + 0];
          dstp[(x + i) * 3 + 1] = temp[i * 4 + 1];
          dstp[(x + i) * 3 + 2] = temp[i * 4 + 2];
        }
#endif
      }
    }

    if constexpr (rgb_pixel_step == 3) {
      // for rgb32 (pixel_step == 4) we processed full width and more, including padded bytes
      for (size_t x = mod16_width; x < width; ++x) {
        int Y = srcY[x] + matrix.offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)matrix.y_b * Y + (int)matrix.u_b * U + (int)matrix.v_b * V + 4096) >> 13);
        int g = (((int)matrix.y_g * Y + (int)matrix.u_g * U + (int)matrix.v_g * V + 4096) >> 13);
        int r = (((int)matrix.y_r * Y + (int)matrix.u_r * U + (int)matrix.v_r * V + 4096) >> 13);
        dstp[x * rgb_pixel_step + 0] = PixelClip(b);
        dstp[x * rgb_pixel_step + 1] = PixelClip(g);
        dstp[x * rgb_pixel_step + 2] = PixelClip(r);
        if constexpr (rgb_pixel_step == 4) { // n/a
          dstp[x * 4 + 3] = 255;
        }
      }
    }
    dstp -= dst_pitch;
    srcY += src_pitch_y;
    srcU += src_pitch_uv;
    srcV += src_pitch_uv;
    if constexpr(hasAlpha)
      srcA += src_pitch_a;
  }
}

//instantiate
//template<int rgb_pixel_step, bool hasAlpha>
template void convert_yv24_to_rgb_avx2<3, false>(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, const BYTE* srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix& matrix);
template void convert_yv24_to_rgb_avx2<4, false>(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, const BYTE* srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix& matrix);
template void convert_yv24_to_rgb_avx2<3, true>(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, const BYTE* srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix& matrix);
template void convert_yv24_to_rgb_avx2<4, true>(BYTE* dstp, const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, const BYTE* srcA, size_t dst_pitch, size_t src_pitch_y, size_t src_pitch_uv, size_t src_pitch_a, size_t width, size_t height, const ConversionMatrix& matrix);


template<int bits_per_pixel>
#if defined(GCC) || defined(CLANG)
__attribute__((__target__("avx2")))
#endif
void convert_planarrgb_to_yuv_uint16_avx2(BYTE* (&dstp)[3], int(&dstPitch)[3], const BYTE* (&srcp)[3], const int(&srcPitch)[3], int width, int height, const ConversionMatrix& m)
{
  // generic for 10-16 bit uint16 but only used for 16 bits where unsigned 16 arithmetic makes things difficult
  // 16 bit uint16_t (unsigned range)
  __m256  half_f = _mm256_set1_ps((float)(1u << (bits_per_pixel - 1)));
  __m128i limit = _mm_set1_epi16((short)((1 << bits_per_pixel) - 1)); // 255
  __m256 offset_f = _mm256_set1_ps(m.offset_y_f);

  //__m128i zero = _mm_setzero_si128();

  const int rowsize = width * sizeof(uint16_t);
  for (int yy = 0; yy < height; yy++) {
    for (int x = 0; x < rowsize; x += 8 * sizeof(uint16_t)) {
      __m256 g, b, r;
      // uint16_t: load 16 bytes: 8 pixels

      __m128i gi = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp[0] + x));
      __m128i bi = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp[1] + x));
      __m128i ri = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp[2] + x));
      g = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(gi));
      b = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(bi));
      r = _mm256_cvtepi32_ps(_mm256_cvtepu16_epi32(ri));
      /*
      int Y = m.offset_y + (int)(((sum_t)m.y_b * b + (sum_t)m.y_g * g + (sum_t)m.y_r * r + 16384)>>15);
      int U = half + (int)(((sum_t)m.u_b * b + (sum_t)m.u_g * g + (sum_t)m.u_r * r + 16384) >> 15);
      int V = half + (int)(((sum_t)m.v_b * b + (sum_t)m.v_g * g + (sum_t)m.v_r * r + 16384) >> 15);
      */
      // *Y*
      {
        auto mat_r = _mm256_set1_ps(m.y_r_f);
        auto mat_g = _mm256_set1_ps(m.y_g_f);
        auto mat_b = _mm256_set1_ps(m.y_b_f);
        __m256 y = _mm256_fmadd_ps(r, mat_r, _mm256_fmadd_ps(g, mat_g, _mm256_fmadd_ps(b, mat_b, offset_f)));
        __m256i yi = _mm256_cvtps_epi32(y);
        yi = _mm256_packus_epi32(yi, _mm256_setzero_si256()); // 16x uint16_t
        __m128i res = _mm256_castsi256_si128(_mm256_permute4x64_epi64(yi, 0xD8));
        if constexpr (bits_per_pixel < 16) // albeit 10-14 bit have another function, make this general
          res = _mm_min_epi16(res, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp[0] + x), res);
      }
      // *U*
      {
        auto mat_r = _mm256_set1_ps(m.u_r_f);
        auto mat_g = _mm256_set1_ps(m.u_g_f);
        auto mat_b = _mm256_set1_ps(m.u_b_f);
        __m256 y = _mm256_fmadd_ps(r, mat_r, _mm256_fmadd_ps(g, mat_g, _mm256_fmadd_ps(b, mat_b, half_f)));
        __m256i yi = _mm256_cvtps_epi32(y);
        yi = _mm256_packus_epi32(yi, _mm256_setzero_si256()); // 16x uint16_t
        __m128i res = _mm256_castsi256_si128(_mm256_permute4x64_epi64(yi, 0xD8));
        if constexpr (bits_per_pixel < 16) // albeit 10-14 bit have another function, make this general
          res = _mm_min_epi16(res, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp[1] + x), res);
      }
      // *V*
      {
        auto mat_r = _mm256_set1_ps(m.v_r_f);
        auto mat_g = _mm256_set1_ps(m.v_g_f);
        auto mat_b = _mm256_set1_ps(m.v_b_f);
        __m256 y = _mm256_fmadd_ps(r, mat_r, _mm256_fmadd_ps(g, mat_g, _mm256_fmadd_ps(b, mat_b, half_f)));
        __m256i yi = _mm256_cvtps_epi32(y);
        yi = _mm256_packus_epi32(yi, _mm256_setzero_si256()); // 16x uint16_t
        __m128i res = _mm256_castsi256_si128(_mm256_permute4x64_epi64(yi, 0xD8));
        if constexpr (bits_per_pixel < 16) // albeit 10-14 bit have another function, make this general
          res = _mm_min_epi16(res, limit); // clamp 10,12,14 bit
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp[2] + x), res);
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

// Instantiate them
template void convert_planarrgb_to_yuv_uint16_avx2<10>(BYTE* (&dstp)[3], int(&dstPitch)[3], const BYTE* (&srcp)[3], const int(&srcPitch)[3], int width, int height, const ConversionMatrix& m);
template void convert_planarrgb_to_yuv_uint16_avx2<12>(BYTE* (&dstp)[3], int(&dstPitch)[3], const BYTE* (&srcp)[3], const int(&srcPitch)[3], int width, int height, const ConversionMatrix& m);
template void convert_planarrgb_to_yuv_uint16_avx2<14>(BYTE* (&dstp)[3], int(&dstPitch)[3], const BYTE* (&srcp)[3], const int(&srcPitch)[3], int width, int height, const ConversionMatrix& m);
template void convert_planarrgb_to_yuv_uint16_avx2<16>(BYTE* (&dstp)[3], int(&dstPitch)[3], const BYTE* (&srcp)[3], const int(&srcPitch)[3], int width, int height, const ConversionMatrix& m);

#ifdef _MSC_VER
#pragma warning(pop)
#endif
