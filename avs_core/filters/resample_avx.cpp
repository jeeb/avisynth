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
#include <emmintrin.h>
#include <immintrin.h>
#include <algorithm>
#include "resample_avx.h"

template<typename pixel_t, bool hasSSE41>
void resizer_h_ssse3_as_avx_generic_int16_float(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel) {
  int filter_size = AlignNumber(program->filter_size, 8) / 8;
  __m128i zero = _mm_setzero_si128();

  const pixel_t *src = reinterpret_cast<const pixel_t *>(src8);
  pixel_t *dst = reinterpret_cast<pixel_t *>(dst8);
  dst_pitch /= sizeof(pixel_t);
  src_pitch /= sizeof(pixel_t);

  __m128 clamp_limit;
  if (sizeof(pixel_t) == 2)
    clamp_limit = _mm_set1_ps((float)(((int)1 << bits_per_pixel) - 1)); // clamp limit

  for (int y = 0; y < height; y++) {
    float* current_coeff = program->pixel_coefficient_float;
    for (int x = 0; x < width; x+=4) {
      __m128 result1 = _mm_set1_ps(0.0f);
      __m128 result2 = result1;
      __m128 result3 = result1;
      __m128 result4 = result1;

      int begin1 = program->pixel_offset[x+0];
      int begin2 = program->pixel_offset[x+1];
      int begin3 = program->pixel_offset[x+2];
      int begin4 = program->pixel_offset[x+3];

      // begin1, result1
      for (int i = 0; i < filter_size; i++) {
        __m128 data_l_single, data_h_single;
        if(sizeof(pixel_t)==2) // word
        {
          // unaligned
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin1+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero); // spread higher 4*uint16_t pixel value -> 4*32 bit
          data_l_single = _mm_cvtepi32_ps (src_l); // Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
          data_h_single = _mm_cvtepi32_ps (src_h);
        }
        else { // float
               // unaligned
          data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin1+i*8)); // float  4*32=128 4 pixels at a time
          data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin1+i*8+4)); // float  4*32=128 4 pixels at a time
        }
        __m128 coeff_l = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m128 coeff_h = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
        __m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
        __m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
        result1 = _mm_add_ps(result1, dst_l); // accumulate result.
        result1 = _mm_add_ps(result1, dst_h);

        current_coeff += 8;
      }

      // begin2, result2
      for (int i = 0; i < filter_size; i++) {
        __m128 data_l_single, data_h_single;
        if(sizeof(pixel_t)==2) // word
        {
          // unaligned
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin2+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero); // spread higher 4*uint16_t pixel value -> 4*32 bit
          data_l_single = _mm_cvtepi32_ps (src_l); // Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
          data_h_single = _mm_cvtepi32_ps (src_h);
        }
        else { // float
               // unaligned
          data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin2+i*8)); // float  4*32=128 4 pixels at a time
          data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin2+i*8+4)); // float  4*32=128 4 pixels at a time
        }
        __m128 coeff_l = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m128 coeff_h = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
        __m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
        __m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
        result2 = _mm_add_ps(result2, dst_l); // accumulate result.
        result2 = _mm_add_ps(result2, dst_h);

        current_coeff += 8;
      }

      // begin3, result3
      for (int i = 0; i < filter_size; i++) {
        __m128 data_l_single, data_h_single;
        if(sizeof(pixel_t)==2) // word
        {
          // unaligned
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin3+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero); // spread higher 4*uint16_t pixel value -> 4*32 bit
          data_l_single = _mm_cvtepi32_ps (src_l); // Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
          data_h_single = _mm_cvtepi32_ps (src_h);
        }
        else { // float
               // unaligned
          data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin3+i*8)); // float  4*32=128 4 pixels at a time
          data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin3+i*8+4)); // float  4*32=128 4 pixels at a time
        }
        __m128 coeff_l = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m128 coeff_h = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
        __m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
        __m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
        result3 = _mm_add_ps(result3, dst_l); // accumulate result.
        result3 = _mm_add_ps(result3, dst_h);

        current_coeff += 8;
      }

      // begin4, result4
      for (int i = 0; i < filter_size; i++) {
        __m128 data_l_single, data_h_single;
        if(sizeof(pixel_t)==2) // word
        {
          // unaligned
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin4+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero); // spread higher 4*uint16_t pixel value -> 4*32 bit
          data_l_single = _mm_cvtepi32_ps (src_l); // Converts the four signed 32-bit integer values of a to single-precision, floating-point values.
          data_h_single = _mm_cvtepi32_ps (src_h);
        }
        else { // float
               // unaligned
          data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin4+i*8)); // float  4*32=128 4 pixels at a time
          data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin4+i*8+4)); // float  4*32=128 4 pixels at a time
        }
        __m128 coeff_l = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m128 coeff_h = /*loadps*/_mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
        __m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
        __m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
        result4 = _mm_add_ps(result4, dst_l); // accumulate result.
        result4 = _mm_add_ps(result4, dst_h);

        current_coeff += 8;
      }

      __m128 result;

      // this part needs ssse3
      __m128 result12 = _mm_hadd_ps(result1, result2);
      __m128 result34 = _mm_hadd_ps(result3, result4);
      result = _mm_hadd_ps(result12, result34);

      if (sizeof(pixel_t) == 2)
      {
        result = _mm_min_ps(result, clamp_limit); // mainly for 10-14 bit
                                                  // result = _mm_max_ps(result, zero); low limit through pack_us
      }

      if(sizeof(pixel_t)==2) // word
      {
        // Converts the four single-precision, floating-point values of a to signed 32-bit integer values.
        __m128i result_4x_int32  = _mm_cvtps_epi32(result);  // 4 * 32 bit integers
                                                             // SIMD Extensions 4 (SSE4) packus or simulation
        __m128i result_4x_uint16 = hasSSE41 ? _mm_packus_epi32(result_4x_int32, zero) : (_MM_PACKUS_EPI32(result_4x_int32, zero)) ; // 4*32+zeros = lower 4*16 OK
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst + x), result_4x_uint16);
      } else { // float
               // aligned
        _mm_store_ps(reinterpret_cast<float*>(dst+x), result); // 4 results at a time
      }

    }

    dst += dst_pitch;
    src += src_pitch;
  }
  _mm256_zeroupper();
}

template<typename pixel_t>
void resizer_h_avx_generic_int16_float(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel) {

  int filter_size = AlignNumber(program->filter_size, 8) / 8;
  const __m128 zero = _mm_setzero_ps();
  const __m128i zero128 = _mm_setzero_si128();

  const pixel_t *src = reinterpret_cast<const pixel_t *>(src8);
  pixel_t *dst = reinterpret_cast<pixel_t *>(dst8);
  dst_pitch /= sizeof(pixel_t);
  src_pitch /= sizeof(pixel_t);

  __m128 clamp_limit;
  if (sizeof(pixel_t) == 2)
    clamp_limit = _mm_set1_ps((float)(((int)1 << bits_per_pixel) - 1)); // clamp limit

  for (int y = 0; y < height; y++) {
    float* current_coeff = program->pixel_coefficient_float;

#if 0
    for (int x = 0; x < width; x+=4) {
      __m256 result1 = _mm256_setzero_ps();
      __m256 result2 = result1;
      __m256 result3 = result1;
      __m256 result4 = result1;
      __m256 coeff;

      int *begin = &program->pixel_offset[x]; // x+0..x+3

      for(int a = 0; a<4; a++)
      {
        result4 = _mm256_setzero_ps();
        // begin1, result1
        for (int i = 0; i < filter_size; i++) {
          __m256 data_single;
          if(sizeof(pixel_t)==2) // word
          {
            // AVX2 _mm256_cvtepu16_epi32
            //__m256i src256 = _mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + begin1 + i * 8))); // 8*16->8*32 bits
            __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src + *begin + i * 8)); // uint16_t  8*16=128 8 pixels at a time
            __m128i src_l = _mm_unpacklo_epi16(src_p, zero128); // spread lower  4*uint16_t pixel value -> 4*32 bit
            __m128i src_h = _mm_unpackhi_epi16(src_p, zero128); // spread higher 4*uint16_t pixel value -> 4*32 bit
            __m256i src256 = _mm256_set_m128i(src_h, src_l);
            data_single = _mm256_cvtepi32_ps (src256); // Converts the 8x signed 32-bit integer values of a to single-precision, floating-point values.
            coeff = /*loadps*/_mm256_loadu_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
          }
          else { // float
                 // unaligned
            /*
            data_single = _mm256_castsi256_ps(
              _mm256_lddqu_si256(reinterpret_cast<const __m256i*>(
                reinterpret_cast<const float*>(src + *begin + i * 8)))
              );
            */
            // float  8*32=256 8 pixels at a time
#ifndef __AVX2__
            data_single = _mm256_loadu_ps(reinterpret_cast<const float*>(src + *begin + i * 8));
            // for AVX2
            coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
#else
            data_single = _mm256_loadu2_m128(reinterpret_cast<const float*>(src+*begin+i*8+4), reinterpret_cast<const float*>(src+*begin+i*8  )); // float  4*32=128 4 pixels at a time
            //__m128i data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin+i*8  )); // float  4*32=128 4 pixels at a time
            //__m128i data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin+i*8+4)); // float  4*32=128 4 pixels at a time
            coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
            /*
            __m128 coeff_l = _mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
            __m128 coeff_h = _mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
            coeff = _mm256_set_m128(coeff_h, coeff_l);
            */
#endif
          }

          //__m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
          //__m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
          __m256 dst = _mm256_mul_ps(data_single, coeff); // Multiply by coefficient
                                                          //result1 = _mm_add_ps(result1, dst_l); // accumulate result.
                                                          //result1 = _mm_add_ps(result1, dst_h);
          result4 = _mm256_add_ps(result4, dst);

          current_coeff += 8;
        }
        switch (a) {
        case 0: result1 = result4; break;
        case 1: result2 = result4; break;
        case 2: result3 = result4; break;
          // result 4 ok
        }
        begin++;
      }


      __m128 result;

      // result1: A0 A1 A2 A3 A4 A5 A6 A7
      // result2: B0 B1 B2 B3 B4 B5 B6 B7
      /*
      ymm2 = _mm256_permute2f128_ps(ymm , ymm , 1);
      ymm = _mm256_add_ps(ymm, ymm2);
      ymm = _mm256_hadd_ps(ymm, ymm);
      ymm = _mm256_hadd_ps(ymm, ymm);
      */
      // hiQuad = ( x7, x6, x5, x4 )
      //const __m128 hiQuad = _mm256_extractf128_ps(result1, 1);
      // loQuad = ( x3, x2, x1, x0 )
      //const __m128 loQuad = _mm256_castps256_ps128(result);
      // sumQuad = ( x3 + x7, x2 + x6, x1 + x5, x0 + x4 )
      const __m128 sumQuad1 = _mm_add_ps(_mm256_castps256_ps128(result1), _mm256_extractf128_ps(result1, 1));
      const __m128 sumQuad2 = _mm_add_ps(_mm256_castps256_ps128(result2), _mm256_extractf128_ps(result2, 1));
      __m128 result12 = _mm_hadd_ps(sumQuad1, sumQuad2);
      const __m128 sumQuad3 = _mm_add_ps(_mm256_castps256_ps128(result3), _mm256_extractf128_ps(result3, 1));
      const __m128 sumQuad4 = _mm_add_ps(_mm256_castps256_ps128(result4), _mm256_extractf128_ps(result4, 1));
      __m128 result34 = _mm_hadd_ps(sumQuad3, sumQuad4);
      result = _mm_hadd_ps(result12, result34);

      if (sizeof(pixel_t) == 2)
      {
        result = _mm_min_ps(result, clamp_limit); // mainly for 10-14 bit
                                                  // result = _mm_max_ps(result, zero); low limit through pack_us
      }
      /*
      __m256 result12 = _mm256_hadd_ps(result1, result2);
      // result12: A0+A1, A2+A3, B0+B1, B2+B3, A4+A5, A6+A7, B4+B5, B6+B7
      __m256 result34 = _mm256_hadd_ps(result3, result4);
      result = _mm256_hadd_ps(result12, result34);
      */
      if(sizeof(pixel_t)==2) // word
      {
        // Converts the four single-precision, floating-point values of a to signed 32-bit integer values.
        __m128i result_4x_int32  = _mm_cvtps_epi32(result);  // 4 * 32 bit integers
        __m128i result_4x_uint16 = _mm_packus_epi32(result_4x_int32, zero128); // 4*32+zeros = lower 4*16 OK
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst + x), result_4x_uint16);
      }
      else { // float
             // aligned
        //_mm_store_ps(reinterpret_cast<float*>(dst+x), result); // 4 results at a time
        _mm_stream_ps(reinterpret_cast<float*>(dst+x), result); // 4 results at a time
      }

    }
#else
    for (int x = 0; x < width; x+=4) {
      __m256 result1 = _mm256_setzero_ps();
      __m256 result2 = result1;
      __m256 result3 = result1;
      __m256 result4 = result1;

      int begin1 = program->pixel_offset[x+0];
      int begin2 = program->pixel_offset[x+1];
      int begin3 = program->pixel_offset[x+2];
      int begin4 = program->pixel_offset[x+3];

      // begin1, result1
      for (int i = 0; i < filter_size; i++) {
        __m256 data_single;
        if(sizeof(pixel_t)==2) // word
        {
          // AVX2 _mm256_cvtepu16_epi32
          //__m256i src256 = _mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + begin1 + i * 8))); // 8*16->8*32 bits
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin1+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero128); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero128); // spread higher 4*uint16_t pixel value -> 4*32 bit
          __m256i src256 = _mm256_set_m128i(src_h, src_l);
          data_single = _mm256_cvtepi32_ps (src256); // Converts the 8x signed 32-bit integer values of a to single-precision, floating-point values.
        }
        else { // float
               // unaligned
          data_single = _mm256_loadu_ps(reinterpret_cast<const float*>(src+begin1+i*8)); // float  8*32=256 8 pixels at a time
          //data_l_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin1+i*8  )); // float  4*32=128 4 pixels at a time
          //data_h_single = _mm_loadu_ps(reinterpret_cast<const float*>(src+begin1+i*8+4)); // float  4*32=128 4 pixels at a time
        }
        //__m128 coeff_l = _mm_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        //__m128 coeff_h = _mm_load_ps(reinterpret_cast<const float*>(current_coeff+4));  // always aligned
        __m256 coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        //__m128 dst_l = _mm_mul_ps(data_l_single, coeff_l); // Multiply by coefficient
        //__m128 dst_h = _mm_mul_ps(data_h_single, coeff_h); // 4*(32bit*32bit=32bit)
        __m256 dst = _mm256_mul_ps(data_single, coeff); // Multiply by coefficient
        //result1 = _mm_add_ps(result1, dst_l); // accumulate result.
        //result1 = _mm_add_ps(result1, dst_h);
        result1 = _mm256_add_ps(result1, dst);

        current_coeff += 8;
      }

      // begin2, result2
      for (int i = 0; i < filter_size; i++) {
        __m256 data_single;
        if(sizeof(pixel_t)==2) // word
        {
          // AVX2 _mm256_cvtepu16_epi32
          //__m256i src256 = _mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + begin2 + i * 8))); // 8*16->8*32 bits
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin2+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero128); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero128); // spread higher 4*uint16_t pixel value -> 4*32 bit
          __m256i src256 = _mm256_set_m128i(src_h, src_l);
          data_single = _mm256_cvtepi32_ps (src256); // Converts the 8x signed 32-bit integer values of a to single-precision, floating-point values.
        }
        else { // float
          data_single = _mm256_loadu_ps(reinterpret_cast<const float*>(src+begin2+i*8)); // float  8*32=256 8 pixels at a time
        }
        __m256 coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m256 dst = _mm256_mul_ps(data_single, coeff); // Multiply by coefficient
        result2 = _mm256_add_ps(result2, dst);

        current_coeff += 8;
      }

      // begin3, result3
      for (int i = 0; i < filter_size; i++) {
        __m256 data_single;
        if(sizeof(pixel_t)==2) // word
        {
          // AVX2 _mm256_cvtepu16_epi32
          //__m256i src256 = _mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + begin3 + i * 8))); // 8*16->8*32 bits
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin3+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero128); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero128); // spread higher 4*uint16_t pixel value -> 4*32 bit
          __m256i src256 = _mm256_set_m128i(src_h, src_l);
          data_single = _mm256_cvtepi32_ps (src256); // Converts the 8x signed 32-bit integer values of a to single-precision, floating-point values.
        }
        else { // float
          data_single = _mm256_loadu_ps(reinterpret_cast<const float*>(src+begin3+i*8)); // float  8*32=256 8 pixels at a time
        }
        __m256 coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m256 dst = _mm256_mul_ps(data_single, coeff); // Multiply by coefficient
        result3 = _mm256_add_ps(result3, dst);

        current_coeff += 8;
      }

      // begin4, result4
      for (int i = 0; i < filter_size; i++) {
        __m256 data_single;
        if(sizeof(pixel_t)==2) // word
        {
          // AVX2 _mm256_cvtepu16_epi32
          //__m256i src256 = _mm256_cvtepu16_epi32(_mm_loadu_si128(reinterpret_cast<const __m128i*>(src + begin4 + i * 8))); // 8*16->8*32 bits
          __m128i src_p = _mm_loadu_si128(reinterpret_cast<const __m128i*>(src+begin4+i*8)); // uint16_t  8*16=128 8 pixels at a time
          __m128i src_l = _mm_unpacklo_epi16(src_p, zero128); // spread lower  4*uint16_t pixel value -> 4*32 bit
          __m128i src_h = _mm_unpackhi_epi16(src_p, zero128); // spread higher 4*uint16_t pixel value -> 4*32 bit
          __m256i src256 = _mm256_set_m128i(src_h, src_l);
          data_single = _mm256_cvtepi32_ps (src256); // Converts the 8x signed 32-bit integer values of a to single-precision, floating-point values.
        }
        else { // float
          data_single = _mm256_loadu_ps(reinterpret_cast<const float*>(src+begin4+i*8)); // float  8*32=256 8 pixels at a time
        }
        __m256 coeff = _mm256_load_ps(reinterpret_cast<const float*>(current_coeff));    // always aligned
        __m256 dst = _mm256_mul_ps(data_single, coeff); // Multiply by coefficient
        result4 = _mm256_add_ps(result4, dst);

        current_coeff += 8;
      }

      __m128 result;

      const __m128 sumQuad1 = _mm_add_ps(_mm256_castps256_ps128(result1), _mm256_extractf128_ps(result1, 1));
      const __m128 sumQuad2 = _mm_add_ps(_mm256_castps256_ps128(result2), _mm256_extractf128_ps(result2, 1));
      __m128 result12 = _mm_hadd_ps(sumQuad1, sumQuad2);
      const __m128 sumQuad3 = _mm_add_ps(_mm256_castps256_ps128(result3), _mm256_extractf128_ps(result3, 1));
      const __m128 sumQuad4 = _mm_add_ps(_mm256_castps256_ps128(result4), _mm256_extractf128_ps(result4, 1));
      __m128 result34 = _mm_hadd_ps(sumQuad3, sumQuad4);
      result = _mm_hadd_ps(result12, result34);

      if (sizeof(pixel_t) == 2)
      {
        result = _mm_min_ps(result, clamp_limit); // mainly for 10-14 bit
        // result = _mm_max_ps(result, zero); low limit through pack_us
      }
      if(sizeof(pixel_t)==2) // word
      {
        // Converts the four single-precision, floating-point values of a to signed 32-bit integer values.
        __m128i result_4x_int32  = _mm_cvtps_epi32(result);  // 4 * 32 bit integers
        __m128i result_4x_uint16 = _mm_packus_epi32(result_4x_int32, zero128); // 4*32+zeros = lower 4*16 OK
        _mm_storel_epi64(reinterpret_cast<__m128i *>(dst + x), result_4x_uint16);
      }
      else { // float
        // aligned
        _mm_store_ps(reinterpret_cast<float*>(dst+x), result); // 4 results at a time
      }

    }
#endif

    dst += dst_pitch;
    src += src_pitch;
  }
  _mm256_zeroupper();
}

// instantiate here
template void resizer_h_avx_generic_int16_float<uint16_t>(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);
template void resizer_h_avx_generic_int16_float<float   >(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);

template void resizer_h_ssse3_as_avx_generic_int16_float<uint16_t, false>(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);
template void resizer_h_ssse3_as_avx_generic_int16_float<uint16_t, true>(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);
template void resizer_h_ssse3_as_avx_generic_int16_float<float, false>(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);
template void resizer_h_ssse3_as_avx_generic_int16_float<float, true>(BYTE* dst8, const BYTE* src8, int dst_pitch, int src_pitch, ResamplingProgram* program, int width, int height, int bits_per_pixel);
