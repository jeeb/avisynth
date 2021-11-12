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

#include "convert_bits.h"
#ifdef INTEL_INTRINSICS
#include "intel/convert_bits_sse.h"
#include "intel/convert_bits_avx.h"
#include "intel/convert_bits_avx2.h"
#endif
#include "convert_helper.h"

#include <avs/alignment.h>
#include <avs/minmax.h>
#include <avs/config.h>
#include <tuple>
#include <map>
#include <algorithm>

#ifdef AVS_WINDOWS
#include <avs/win.h>
#else
#include <avs/posix.h>
#endif


template<uint8_t sourcebits, int dither_mode, int TARGET_DITHER_BITDEPTH, int rgb_step>
static void convert_rgb_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = 8; // here is constant (uint8_t target)

  // for test, make it 2,4,6,8. sourcebits-TARGET_DITHER_BITDEPTH cannot exceed 8 bit
  // const int TARGET_DITHER_BITDEPTH = 2;

  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits - TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for(int y=0; y<src_height; y++)
  {
    if constexpr(dither_mode == 0)
      _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      if constexpr(dither_mode < 0) // -1: no dither
      {
        const float mulfactor = sourcebits == 16 ? (1.0f / 257.0f) :
          sourcebits == 14 ? (255.0f / 16383.0f) :
          sourcebits == 12 ? (255.0f / 4095.0f) :
          (255.0f / 1023.0f); // 10 bits

        dstp[x] = (uint8_t)(srcp0[x] * mulfactor + 0.5f);
        // C cast truncates, use +0.5f rounder, which uses cvttss2si

        // old method: no rounding but fast
        // no integer division (fast tricky algorithm by compiler), rounding problems, pic gets darker
        // dstp[x] = srcp0[x] / 257; // RGB: full range 0..255 <-> 0..65535 (*255 / 65535)
        // dstp[x] = srcp0[x] * 255 / 16383; // RGB: full range 0..255 <-> 0..16384-1
        // dstp[x] = srcp0[x] * 255 / 4095; // RGB: full range 0..255 <-> 0..4096-1
        // dstp[x] = srcp0[x] * 255 / 1023; // RGB: full range 0..255 <-> 0..1024-1
      }
      else { // dither_mode == 0 -> ordered dither
        const int corr = matrix[_y | ((x / rgb_step) & MASK)];
        // vvv for the non-fullscale version: int new_pixel = ((srcp0[x] + corr) >> DITHER_BIT_DIFF);
        int new_pixel;

        const float mulfactor =
          DITHER_BIT_DIFF == 8 ? (1.0f / 257.0f) :
          DITHER_BIT_DIFF == 6 ? (255.0f / 16383.0f) :
          DITHER_BIT_DIFF == 4 ? (255.0f / 4095.0f) :
          DITHER_BIT_DIFF == 2 ? (255.0f / 1023.0f) : // 10 bits
          1.0f;

        if constexpr(TARGET_DITHER_BITDEPTH <= 4)
          new_pixel = (uint16_t)((srcp0[x] + corr) * mulfactor); // rounding here makes brightness shift
        else if constexpr(DITHER_BIT_DIFF > 0)
          new_pixel = (uint16_t)((srcp0[x] + corr) * mulfactor + 0.5f);
        else
          new_pixel = (uint16_t)(srcp0[x] + corr);

        new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper

        // scale back to the required bit depth
        // for generality. Now target == 8 bit, and dither_target is also 8 bit
        // for test: source:10 bit, target=8 bit, dither_target=4 bit
        const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
        if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
          new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // if implemented non-8bit dither target, this should be fullscale
        dstp[x] = (BYTE)new_pixel;
      }
    } // x
    dstp += dst_pitch;
    srcp0 += src_pitch;
  }
}



// idea borrowed from fmtConv
#define FS_OPTIMIZED_SERPENTINE_COEF

template<int direction>
static AVS_FORCEINLINE void diffuse_floyd(int err, int &nextError, int *error_ptr)
{
#if defined (FS_OPTIMIZED_SERPENTINE_COEF)
  const int      e1 = 0;
  const int      e3 = (err * 4 + 8) >> 4;
#else
  const int      e1 = (err + 8) >> 4;
  const int      e3 = (err * 3 + 8) >> 4;
#endif
  const int      e5 = (err * 5 + 8) >> 4;
  const int      e7 = err - e1 - e3 - e5;

  nextError = error_ptr[direction];
  error_ptr[-direction] += e3;
  error_ptr[0] += e5;
  error_ptr[direction] = e1;
  nextError += e7;
}

template<int direction>
static void diffuse_floyd_f(float err, float &nextError, float *error_ptr)
{
#if defined (FS_OPTIMIZED_SERPENTINE_COEF)
  const float    e1 = 0;
  const float    e3 = err * (4.0f / 16);
#else
  const float    e1 = err * (1.0f / 16);
  const float    e3 = err * (3.0f / 16);
#endif
  const float    e5 = err * (5.0f / 16);
  const float    e7 = err * (7.0f / 16);

  nextError = error_ptr[direction];
  error_ptr[-direction] += e3;
  error_ptr[0] += e5;
  error_ptr[direction] = e1;
  nextError += e7;
}

template<typename source_pixel_t, typename target_pixel_t, uint8_t sourcebits, uint8_t TARGET_BITDEPTH, int TARGET_DITHER_BITDEPTH>
static void convert_uint_floyd_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const source_pixel_t *srcp = reinterpret_cast<const source_pixel_t *>(srcp8);
  src_pitch = src_pitch / sizeof(source_pixel_t);
  int src_width = src_rowsize / sizeof(source_pixel_t);

  target_pixel_t *dstp = reinterpret_cast<target_pixel_t *>(dstp8);
  dst_pitch = dst_pitch / sizeof(target_pixel_t);

  const int max_pixel_value = (1 << TARGET_BITDEPTH) - 1;
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);

  int *error_ptr_safe = new int[1 + src_width + 1]; // accumulated errors
  std::fill_n(error_ptr_safe, src_width + 2, 0);

  int *error_ptr = error_ptr_safe + 1;

  const int INTERNAL_BITS = DITHER_BIT_DIFF < 6 ? sourcebits+8 : sourcebits; // keep accuracy
  const int SHIFTBITS_TO_INTERNAL = INTERNAL_BITS - sourcebits;
  const int SHIFTBITS_FROM_INTERNAL = INTERNAL_BITS - TARGET_DITHER_BITDEPTH;
  const int ROUNDER = 1 << (SHIFTBITS_FROM_INTERNAL - 1); // rounding

  for (int y = 0; y < src_height; y++)
  {
    int nextError = error_ptr[0];
    // serpentine forward
    if ((y & 1) == 0)
    {
      for (int x = 0; x < src_width; x++)
      {
        int err = nextError;
        int new_pixel = srcp[x] << SHIFTBITS_TO_INTERNAL; // if necessary
        int sum = new_pixel + err;
        int quantized = (sum + ROUNDER) >> (SHIFTBITS_FROM_INTERNAL);
        err = sum - (quantized << SHIFTBITS_FROM_INTERNAL);
        quantized <<= BITDIFF_BETWEEN_DITHER_AND_TARGET;
        int pix = max(min(max_pixel_value, quantized), 0); // clamp to target bit
        dstp[x] = (target_pixel_t)pix;
        diffuse_floyd<1>(err, nextError, error_ptr+x);
      }
    }
    else {
      // serpentine backward
      for (int x = src_width - 1; x >= 0; --x)
      {
        int err = nextError;
        int new_pixel = srcp[x] << SHIFTBITS_TO_INTERNAL; // if necessary
        int sum = new_pixel + err;
        int quantized = (sum + ROUNDER) >> (SHIFTBITS_FROM_INTERNAL);
        err = sum - (quantized << SHIFTBITS_FROM_INTERNAL);
        quantized <<= BITDIFF_BETWEEN_DITHER_AND_TARGET;
        int pix = max(min(max_pixel_value, quantized), 0); // clamp to target bit
        dstp[x] = (target_pixel_t)pix;
        diffuse_floyd<-1>(err, nextError, error_ptr + x);
      }
    }
    error_ptr[0] = nextError;
    dstp += dst_pitch;
    srcp += src_pitch;
  }

  delete[] error_ptr_safe;
}

// FIXME: make it common:
// convert_uint16_to_8_c and convert_uint16_to_uint16_dither_c
// make template only source and pixel_types, and probably TARGET_DITHER_BITDEPTH?

// YUV conversions (bit shifts)
// BitDepthConvFuncPtr
// Conversion from 16-14-12-10 to 8 bits (bitshift: 8-6-4-2)
// both dither and non-dither
template<uint8_t sourcebits, int dither_mode, int TARGET_DITHER_BITDEPTH>
static void convert_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  assert(dither_mode == 0);
  // ordered dither only

  const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);

  const int TARGET_BITDEPTH = 8; // here is constant (uint8_t target)
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits-TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for(int y=0; y<src_height; y++)
  {
    int _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      int corr = matrix[_y | (x & MASK)];
      //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
      int new_pixel = ((srcp0[x] + corr) >> DITHER_BIT_DIFF);
      new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper
      // scale back to the required bit depth
      // for generality. Now target == 8 bit, and dither_target is also 8 bit
      // for test: source:10 bit, target=8 bit, dither_target=4 bit
      const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
      if constexpr (BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
        new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // closest in palette: simple shift with
      dstp[x] = (BYTE)new_pixel;
    }
    dstp += dst_pitch;
    srcp0 += src_pitch;
  }
}

// float to 8 bit, float to 10/12/14/16 bit

template<typename pixel_t, bool chroma, bool fulls, bool fulld>
static void convert_32_to_uintN_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const float *srcp0 = reinterpret_cast<const float *>(srcp);
  pixel_t *dstp0 = reinterpret_cast<pixel_t *>(dstp);

  src_pitch = src_pitch / sizeof(float);
  dst_pitch = dst_pitch / sizeof(pixel_t);

  int src_width = src_rowsize / sizeof(float);

  const float max_dst_pixelvalue = (float)((1<< target_bitdepth) - 1); // 255, 1023, 4095, 16383, 65535.0
  const float half = (float)(1 << (target_bitdepth - 1));

  const int limit_lo_d = fulld ? 0 : (16 << (target_bitdepth - 8));
  const int limit_hi_d = fulld ? ((1 << target_bitdepth) - 1) : ((chroma ? 240 : 235) << (target_bitdepth - 8));
  const float range_diff_d = (float)limit_hi_d - limit_lo_d;

  constexpr int limit_lo_s = fulls ? 0 : 16;
  constexpr float limit_lo_s_ps = limit_lo_s / 255.0f;
  constexpr int limit_hi_s = fulls ? 255 : (chroma ? 240 : 235);
  constexpr float range_diff_s = (limit_hi_s - limit_lo_s) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240       Y = Y * ((235-16) << (bpp-8)) + 16, Chroma= Chroma * ((240-16) << (bpp-8)) + 16
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255
  const float factor = range_diff_d / range_diff_s;

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel;
      if constexpr(chroma) {
        pixel = srcp0[x] * factor + half + 0.5f;
      }
      else {
        if constexpr(!fulls)
          pixel = (srcp0[x] - limit_lo_s_ps) * factor + 0.5f + limit_lo_d;
        else
          pixel = srcp0[x] * factor + 0.5f + limit_lo_d;
      }
      dstp0[x] = pixel_t(clamp(pixel, 0.0f, max_dst_pixelvalue)); // we clamp here!
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

// YUV: bit shift 8-16 <=> 8-16 bits
// shift right or left, depending on expandrange
template<typename pixel_t_s, typename pixel_t_d>
static void convert_uint_limited_c(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const pixel_t_s* srcp0 = reinterpret_cast<const pixel_t_s*>(srcp);
  pixel_t_d* dstp0 = reinterpret_cast<pixel_t_d*>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t_s);
  dst_pitch = dst_pitch / sizeof(pixel_t_d);

  const int src_width = src_rowsize / sizeof(pixel_t_s);

  if (target_bitdepth > source_bitdepth) // expandrange
  {
    const int shift_bits = target_bitdepth - source_bitdepth;
    for (int y = 0; y < src_height; y++)
    {
      for (int x = 0; x < src_width; x++) {
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
    for (int y = 0; y < src_height; y++)
    {
      for (int x = 0; x < src_width; x++) {
        dstp0[x] = (srcp0[x] + round) >> shift_bits;  // reduce range
      }
      dstp0 += dst_pitch;
      srcp0 += src_pitch;
    }
  }
}

// rgb/alpha: full scale. No bit shift, scale full ranges
// chroma is special in full range
// mixed cases
template<typename pixel_t_s, typename pixel_t_d, bool chroma, bool fulls, bool fulld>
static void convert_uint_c(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  // limited to limited is bitshift, see in other function
  if constexpr (!fulls && !fulld) {
    convert_uint_limited_c< pixel_t_s, pixel_t_d>(srcp, dstp, src_rowsize, src_height, src_pitch, dst_pitch, source_bitdepth, target_bitdepth);
    return;
  }

  const pixel_t_s* srcp0 = reinterpret_cast<const pixel_t_s*>(srcp);
  pixel_t_d* dstp0 = reinterpret_cast<pixel_t_d*>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t_s);
  dst_pitch = dst_pitch / sizeof(pixel_t_d);

  int src_width = src_rowsize / sizeof(pixel_t_s);

  if constexpr (sizeof(pixel_t_s) == 1 && sizeof(pixel_t_d) == 2) {
    if (fulls && fulld && !chroma && source_bitdepth == 8 && target_bitdepth == 16) {
      // special case * 65535 / 255 = *257 exactly
      for (int y = 0; y < src_height; y++)
      {
        for (int x = 0; x < src_width; x++)
        {
          dstp0[x] = (pixel_t_d)(srcp0[x] * 257);
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
      }
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

  auto dst_offset_plus_round = dst_offset + 0.5f;
  constexpr auto target_min = chroma && fulld ? 1 : 0;

  for (int y = 0; y < src_height; y++) {
    for (int x = 0; x < src_width; x++)
    {
      const float val = (srcp0[x] - src_offset) * mul_factor + dst_offset_plus_round;
      dstp0[x] = clamp((int)val, target_min, target_max);
    }

    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}



template<uint8_t sourcebits, uint8_t targetbits, int TARGET_DITHER_BITDEPTH>
static void convert_rgb_uint16_to_uint16_dither_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const uint16_t *srcp = reinterpret_cast<const uint16_t *>(srcp8);
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  src_pitch = src_pitch / sizeof(uint16_t);
  dst_pitch = dst_pitch / sizeof(uint16_t);

  const int src_width = src_rowsize / sizeof(uint16_t);

  const int source_max = (1 << sourcebits) - 1;

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = targetbits;
  const int max_pixel_value = (1 << TARGET_BITDEPTH) - 1;
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits - TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for (int y = 0; y < src_height; y++)
  {
    _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      int corr = matrix[_y | (x & MASK)];
      //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
      //int new_pixel = ((srcp[x] + corr) >> DITHER_BIT_DIFF);
      int64_t new_pixel = (int64_t)(srcp[x] + corr) * max_pixel_value_dithered / source_max;

      // new_pixel = min(new_pixel, max_pixel_value_dithered_i); // clamp upper
      // scale back to the required bit depth
      const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
      if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0) {
        new_pixel = new_pixel * max_pixel_value / max_pixel_value_dithered;
      }
      dstp[x] = (uint16_t)(min((int)new_pixel, max_pixel_value));
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }

}


// YUV: bit shift 10-12-14-16 <=> 10-12-14-16 bits
// shift right or left, depending on expandrange template param
template<uint8_t sourcebits, uint8_t targetbits, int TARGET_DITHER_BITDEPTH>
static void convert_uint16_to_uint16_dither_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const uint16_t *srcp = reinterpret_cast<const uint16_t *>(srcp8);
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  src_pitch = src_pitch / sizeof(uint16_t);
  dst_pitch = dst_pitch / sizeof(uint16_t);

  const int src_width = src_rowsize / sizeof(uint16_t);

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = targetbits;
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits - TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for (int y = 0; y < src_height; y++)
  {
    _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      int corr = matrix[_y | (x & MASK)];
      //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
      int new_pixel = ((srcp[x] + corr) >> DITHER_BIT_DIFF);
      new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper
      // scale back to the required bit depth
      // for generality. Now target == 8 bit, and dither_target is also 8 bit
      // for test: source:10 bit, target=8 bit, dither_target=4 bit
      const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
      if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
        new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // closest in palette: simple shift with
      dstp[x] = (uint16_t)new_pixel;
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

// 8 bit to float, 16/14/12/10 bits to float
template<typename pixel_t, bool chroma, bool fulls, bool fulld>
static void convert_uintN_to_float_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  const pixel_t *srcp0 = reinterpret_cast<const pixel_t *>(srcp);
  float *dstp0 = reinterpret_cast<float *>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t);
  dst_pitch = dst_pitch / sizeof(float);

  int src_width = src_rowsize / sizeof(pixel_t);

  const int limit_lo_s = (fulls ? 0 : 16) << (source_bitdepth - 8);
  const int limit_hi_s = fulls ? ((1 << source_bitdepth) - 1) : ((chroma ? 240 : 235) << (source_bitdepth - 8));
  const float range_diff_s = (float)limit_hi_s - limit_lo_s;

  const int limit_lo_d = fulld ? 0 : 16;
  const int limit_hi_d = fulld ? 255 : (chroma ? 240 : 235);
  const float range_diff_d = (limit_hi_d - limit_lo_d) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240       Y = Y * ((235-16) << (bpp-8)) + 16, Chroma= Chroma * ((240-16) << (bpp-8)) + 16
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255
  const float factor = range_diff_d / range_diff_s;

  const int half = 1 << (source_bitdepth - 1);
  const int half_full = (int)(range_diff_s / 2.0f + 0.5f);
  // chroma center when full (but why is chroma 0..255, does not have proper center?)

  // 0..255,65535 -> 0..1.0 (or -0.5..+0.5) or less if !full

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel;
      if (chroma) {
        if (fulls)
          pixel = (srcp0[x] - half_full) * factor; // (255-0)/2.0f -> 0
        else
          pixel = (srcp0[x] - half) * factor; // -0.5..0.5 when fulld
      }
      else {
        pixel = (srcp0[x] - limit_lo_s) * factor + limit_lo_d / 255.0f;
      }
      dstp0[x] = pixel;
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

// float to float
template<bool chroma, bool fulls, bool fulld>
static void convert_float_to_float_c(const BYTE* srcp, BYTE* dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth)
{
  // float is good if always full range. For historical reasons Avisynth has "limited" range float,
  // which is simply a /255.0 reduction of original byte pixels
  const float* srcp0 = reinterpret_cast<const float*>(srcp);
  float* dstp0 = reinterpret_cast<float*>(dstp);

  src_pitch = src_pitch / sizeof(float);
  dst_pitch = dst_pitch / sizeof(float);

  int src_width = src_rowsize / sizeof(float);

  const int limit_lo_s = fulls ? 0 : 16;
  const int limit_hi_s = fulls ? 255 : (chroma ? 240 : 235);
  const float range_diff_s = (limit_hi_s - limit_lo_s) / 255.0f;

  const int limit_lo_d = fulld ? 0 : 16;
  const int limit_hi_d = fulld ? 255 : (chroma ? 240 : 235);
  const float range_diff_d = (limit_hi_d - limit_lo_d) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240     - 128 / 255.0
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255  - 128 / 255.0 
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240     - 128 / 255.0
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255  - 128 / 255.0 
  const float factor = range_diff_d / range_diff_s;
  const float limit_lo_s_redu = limit_lo_s / 255.0f;
  const float limit_lo_d_redu = limit_lo_d / 255.0f;
  for (int y = 0; y < src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel;
      if (chroma) {
        pixel = srcp0[x] * factor; // Zero center does not move
      }
      else {
        pixel = (srcp0[x] - limit_lo_s_redu) * factor + limit_lo_d_redu;
      }
      dstp0[x] = pixel;
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

BitDepthConvFuncPtr get_convert_to_8_function(bool full_scale, int source_bitdepth, int dither_mode, int dither_bitdepth, int rgb_step, int cpu)
{
  std::map<std::tuple<bool, int, int, int, int, int>, BitDepthConvFuncPtr> func_copy;
  using std::make_tuple;
 

  const int DITHER_TARGET_BITDEPTH_8 = 8;
  const int DITHER_TARGET_BITDEPTH_7 = 7;
  const int DITHER_TARGET_BITDEPTH_6 = 6;
  const int DITHER_TARGET_BITDEPTH_5 = 5;
  const int DITHER_TARGET_BITDEPTH_4 = 4;
  const int DITHER_TARGET_BITDEPTH_3 = 3;
  const int DITHER_TARGET_BITDEPTH_2 = 2;
  const int DITHER_TARGET_BITDEPTH_1 = 1;
  const int DITHER_TARGET_BITDEPTH_0 = 0;

  if (dither_mode < 0)
    dither_bitdepth = 8; // default entry in the tables below
  if (dither_mode == 1) // no special version for fullscale dithering down.
  {
    // floyd
    rgb_step = 1; // rgb_step n/a for packed rgb formats.
    // packed rgb floyd result is not the same as because it treats the rgb pixels
    // as a consecutive bgrbgrbgr pixel flow, packed rgb is converted to and from planar rgb instead.
    full_scale = false;
  }

  // full scale

  //-----------
  // full scale, dither, C
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_4, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_2, 1>;

  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_4, 1>;

  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_6, 1>;

  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  // for RGB48 and RGB64 source
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 3, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 3>; // dither rgb_step param is filled
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 4, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 4>; // dither rgb_step param is filled

#ifdef INTEL_INTRINSICS
  //-----------
  // full scale, dither, SSE2
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<10, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<10, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<10, 0, DITHER_TARGET_BITDEPTH_4, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<10, 0, DITHER_TARGET_BITDEPTH_2, 1>;

  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<12, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<12, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<12, 0, DITHER_TARGET_BITDEPTH_4, 1>;

  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<14, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<14, 0, DITHER_TARGET_BITDEPTH_6, 1>;

  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<16, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  // for RGB48 and RGB64 source
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 3, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<16, 0, DITHER_TARGET_BITDEPTH_8, 3>; // dither rgb_step param is filled
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 4, CPUF_SSE2)] = convert_rgb_uint16_to_8_sse2<16, 0, DITHER_TARGET_BITDEPTH_8, 4>; // dither rgb_step param is filled
#endif

  //-----------
  // Floyd dither, C, dither to 8 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_8>;
  // Floyd dither, C, dither to 7 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_7>;
  // Floyd dither, C, dither to 6 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_6>;
  // Floyd dither, C, dither to 5 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_5>;
  // Floyd dither, C, dither to 4 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_4>;
  // Floyd dither, C, dither to 3 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_3>;
  // Floyd dither, C, dither to 2 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_2>;
  // Floyd dither, C, dither to 1 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_1>;
  // Floyd dither, C, dither to 0 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_0>;

  // shifted scale (YUV)

  // dither, C, dither to 8 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8>;

#ifdef INTEL_INTRINSICS
  // dither, SSE2
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<10, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<12, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<14, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<16, DITHER_TARGET_BITDEPTH_8>;
#endif

  // dither, C, dither to 6 bits, max diff 8, allowed from 10-14 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_6>;

#ifdef INTEL_INTRINSICS
  // dither, SSE2
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<10, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<12, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<14, DITHER_TARGET_BITDEPTH_6>;
#endif

  // dither, C, dither to 4 bits, max diff 8, allowed from 10-12 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_4>;
#ifdef INTEL_INTRINSICS
  // dither, SSE2
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<10, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<12, DITHER_TARGET_BITDEPTH_4>;
#endif

  // dither, C, dither to 2 bits, max diff 8, allowed from 10 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_2>;
#ifdef INTEL_INTRINSICS
  // dither, SSE2
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, CPUF_SSE2)] = convert_uint16_to_8_dither_sse2<10, DITHER_TARGET_BITDEPTH_2>;
#endif

  BitDepthConvFuncPtr result = func_copy[make_tuple(full_scale, source_bitdepth, dither_mode, dither_bitdepth, rgb_step, cpu)];
  if (result == nullptr)
    result = func_copy[make_tuple(full_scale, source_bitdepth, dither_mode, dither_bitdepth, rgb_step, 0)]; // fallback to C
  return result;
}

BitDepthConvFuncPtr get_convert_to_16_16_down_dither_function(bool full_scale, int source_bitdepth, int target_bitdepth, int dither_mode, int dither_bitdepth, int rgb_step, int cpu)
{
  std::map<std::tuple<bool, int /*src*/, int /*target*/, int /*dithermode*/, int /*ditherbits*/, int /*rgbstep*/, int /*cpu*/>, BitDepthConvFuncPtr> func_copy;
  using std::make_tuple;

  const int DITHER_TARGET_BITDEPTH_14 = 14;
  const int DITHER_TARGET_BITDEPTH_12 = 12;
  const int DITHER_TARGET_BITDEPTH_10 = 10;
  const int DITHER_TARGET_BITDEPTH_8 = 8;
  const int DITHER_TARGET_BITDEPTH_7 = 7;
  const int DITHER_TARGET_BITDEPTH_6 = 6;
  const int DITHER_TARGET_BITDEPTH_5 = 5;
  const int DITHER_TARGET_BITDEPTH_4 = 4;
  const int DITHER_TARGET_BITDEPTH_3 = 3;
  const int DITHER_TARGET_BITDEPTH_2 = 2; // only for 10->10 bits, but dithering_bits==2
  const int DITHER_TARGET_BITDEPTH_1 = 1; // FloydSteinberg allows any difference in the implementation
  const int DITHER_TARGET_BITDEPTH_0 = 0; // FloydSteinberg allows any difference in the implementation

  if (dither_mode == 1) // no special version for fullscale dithering down.
    full_scale = false;

  if (full_scale) {
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 16, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_8>;

    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_6>;

    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_4>;

    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_4>;

    // 10->10
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_2>;
  }
  else {

    // floyd 16->
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_0>;
    // floyd 14->
    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_0>;
    // floyd 12->
    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_0>;
    // floyd 12->
    // 10->10
    // dither, C, dither to N bits
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_0>;

    // end of floyd

    // shifted scale
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 16, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_8>;

    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_6>;

    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_4>;

    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_4>;

    // 10->10 only dither down
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_2>;

  }
  BitDepthConvFuncPtr result = func_copy[make_tuple(full_scale, source_bitdepth, target_bitdepth, dither_mode, dither_bitdepth, rgb_step, cpu)];
  if (result == nullptr)
    result = func_copy[make_tuple(full_scale, source_bitdepth, target_bitdepth, dither_mode, dither_bitdepth, rgb_step, 0)]; // fallback to C
  return result;
}

static void get_convert_32_to_uintN_functions(int target_bitdepth, bool fulls, bool fulld, 
#ifdef INTEL_INTRINSICS
  bool sse2, bool sse4, bool avx2,
#endif
  BitDepthConvFuncPtr& conv_function, BitDepthConvFuncPtr& conv_function_chroma, BitDepthConvFuncPtr& conv_function_a)
{
  // 32bit->8-16bits support fulls fulld
      // pure C
#define convert_32_to_uintN_functions(uint_X_t) \
      conv_function_a = convert_32_to_uintN_c<uint_X_t, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, false, true, true>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, false, true, false>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, false, false, true>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, false, false, false>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, true, false, false>; \
      }

#ifdef INTEL_INTRINSICS
#undef convert_32_to_uintN_functions

#define convert_32_to_uintN_functions(uint_X_t) \
      conv_function_a = avx2 ? convert_32_to_uintN_avx2<uint_X_t, false, true, true> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, false, true, true> : convert_32_to_uintN_c<uint_X_t, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = avx2 ? convert_32_to_uintN_avx2<uint_X_t, false, true, true> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, false, true, true> : convert_32_to_uintN_c<uint_X_t, false, true, true>; \
        conv_function_chroma = avx2 ? convert_32_to_uintN_avx2<uint_X_t, true, true, true> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, true, true, true> : convert_32_to_uintN_c<uint_X_t, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = avx2 ? convert_32_to_uintN_avx2<uint_X_t, false, true, false> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, false, true, false> : convert_32_to_uintN_c<uint_X_t, false, true, false>; \
        conv_function_chroma = avx2 ? convert_32_to_uintN_avx2<uint_X_t, true, true, false> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, true, true, false> : convert_32_to_uintN_c<uint_X_t, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = avx2 ? convert_32_to_uintN_avx2<uint_X_t, false, false, true> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, false, false, true> : convert_32_to_uintN_c<uint_X_t, false, false, true>; \
        conv_function_chroma = avx2 ? convert_32_to_uintN_avx2<uint_X_t, true, false, true> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, true, false, true> : convert_32_to_uintN_c<uint_X_t, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = avx2 ? convert_32_to_uintN_avx2<uint_X_t, false, false, false> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, false, false, false> : convert_32_to_uintN_c<uint_X_t, false, false, false>; \
        conv_function_chroma = avx2 ? convert_32_to_uintN_avx2<uint_X_t, true, false, false> : sse4 ? convert_32_to_uintN_sse41<uint_X_t, true, false, false> : convert_32_to_uintN_c<uint_X_t, true, false, false>; \
      }
#endif

  switch (target_bitdepth)
  {
  case 8:
    convert_32_to_uintN_functions(uint8_t); // all variations of fulls fulld
    break;
  default:
    // 10-16 bits
    convert_32_to_uintN_functions(uint16_t); // all variations of fulls fulld
    break;
  }

#undef convert_32_to_uintN_functions
}

static void get_convert_float_to_float_functions(bool fulls, bool fulld,
  BitDepthConvFuncPtr& conv_function, BitDepthConvFuncPtr& conv_function_chroma, BitDepthConvFuncPtr& conv_function_a)
{
  // 32bit->32bits support fulls fulld, alpha is always full-full
  conv_function_a = convert_float_to_float_c<false, true, true>; /* full-full */
  if (fulls && fulld) {
    conv_function = convert_float_to_float_c<false, true, true>;
    conv_function_chroma = convert_float_to_float_c<true, true, true>;
  }
  else if (fulls && !fulld) {
    conv_function = convert_float_to_float_c<false, true, false>;
    conv_function_chroma = convert_float_to_float_c<true, true, false>;
  }
  else if (!fulls && fulld) { \
    conv_function = convert_float_to_float_c<false, false, true>;
    conv_function_chroma = convert_float_to_float_c<true, false, true>;
  }
  else if (!fulls && !fulld) {
    conv_function = convert_float_to_float_c<false, false, false>;
    conv_function_chroma = convert_float_to_float_c<true, false, false>;
  }
}

static void get_convert_uintN_to_float_functions(int bits_per_pixel, bool fulls, bool fulld,
  BitDepthConvFuncPtr& conv_function, BitDepthConvFuncPtr& conv_function_chroma, BitDepthConvFuncPtr& conv_function_a)
{
  // 8-16bit->32bits support fulls fulld, alpha is always full-full
#define convert_uintN_to_float_functions(uint_X_t) \
      conv_function_a = convert_uintN_to_float_c<uint_X_t, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, false, true, true>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, false, true, false>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, false, false, true>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, false, false, false>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, true, false, false>; \
      }
  switch (bits_per_pixel) {
  case 8: 
    convert_uintN_to_float_functions(uint8_t);
    break;
  default: // 10-16 bits
    convert_uintN_to_float_functions(uint16_t);
    break;
  }
#undef convert_uintN_to_float_functions
}

static void get_convert_uintN_to_uintN_functions(int source_bitdepth, int target_bitdepth, bool fulls, bool fulld,
#ifdef INTEL_INTRINSICS
  bool sse2, bool sse4, bool avx2,
#endif
  BitDepthConvFuncPtr& conv_function, BitDepthConvFuncPtr& conv_function_chroma, BitDepthConvFuncPtr& conv_function_a)
{
  // 8-16->8-16 bits support any fulls fulld combination
  // pure C
#define convert_uintN_to_uintN_functions(uint_X_t, uint_X_dest_t) \
      conv_function_a = convert_uint_c<uint_X_t, uint_X_dest_t, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = convert_uint_c<uint_X_t, uint_X_dest_t, false, true, true>; \
        conv_function_chroma = convert_uint_c<uint_X_t, uint_X_dest_t, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = convert_uint_c<uint_X_t, uint_X_dest_t, false, true, false>; \
        conv_function_chroma = convert_uint_c<uint_X_t, uint_X_dest_t, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = convert_uint_c<uint_X_t, uint_X_dest_t, false, false, true>; \
        conv_function_chroma = convert_uint_c<uint_X_t, uint_X_dest_t, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = convert_uint_c<uint_X_t, uint_X_dest_t, false, false, false>; \
        conv_function_chroma = convert_uint_c<uint_X_t, uint_X_dest_t, true, false, false>; \
      }

#ifdef INTEL_INTRINSICS
#undef convert_uintN_to_uintN_functions

#define convert_uintN_to_uintN_functions(uint_X_t, uint_X_dest_t) \
      conv_function_a = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, false, true, true> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, false, true, true> : convert_uint_c<uint_X_t, uint_X_dest_t, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, false, true, true> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, false, true, true> : convert_uint_c<uint_X_t, uint_X_dest_t, false, true, true>; \
        conv_function_chroma = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, true, true, true> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, true, true, true> : convert_uint_c<uint_X_t, uint_X_dest_t, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, false, true, false> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, false, true, false> : convert_uint_c<uint_X_t, uint_X_dest_t, false, true, false>; \
        conv_function_chroma = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, true, true, false> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, true, true, false> : convert_uint_c<uint_X_t, uint_X_dest_t, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, false, false, true> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, false, false, true> : convert_uint_c<uint_X_t, uint_X_dest_t, false, false, true>; \
        conv_function_chroma = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, true, false, true> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, true, false, true> : convert_uint_c<uint_X_t, uint_X_dest_t, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, false, false, false> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, false, false, false> : convert_uint_c<uint_X_t, uint_X_dest_t, false, false, false>; \
        conv_function_chroma = avx2 ? convert_uint_avx2<uint_X_t, uint_X_dest_t, true, false, false> : sse4 ? convert_uint_sse41<uint_X_t, uint_X_dest_t, true, false, false> : convert_uint_c<uint_X_t, uint_X_dest_t, true, false, false>; \
      }
#endif

   // all variations of fulls fulld byte/word source/target
  switch (target_bitdepth)
  {
  case 8:
    if (source_bitdepth == 8) {
      convert_uintN_to_uintN_functions(uint8_t, uint8_t);
    }
    else {
      convert_uintN_to_uintN_functions(uint16_t, uint8_t);
    }
    break;
  default:
    // 10-16 bits
    if (source_bitdepth == 8)
    {
      convert_uintN_to_uintN_functions(uint8_t, uint16_t);
    }
    else {
      convert_uintN_to_uintN_functions(uint16_t, uint16_t);
    }
    break;
  }

#undef convert_uintN_to_uintN_functions
}

ConvertBits::ConvertBits(PClip _child, const int _dither_mode, const int _target_bitdepth, bool _truerange, 
  int _ColorRange_src, int _ColorRange_dest,
  int _dither_bitdepth, IScriptEnvironment* env) :
  GenericVideoFilter(_child), dither_mode(_dither_mode), target_bitdepth(_target_bitdepth), truerange(_truerange),
  dither_bitdepth(_dither_bitdepth),
  conv_function(nullptr), conv_function_chroma(nullptr), conv_function_a(nullptr),
  fulld(false), fulls(false)
{

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
  format_change_only = false;

#ifdef INTEL_INTRINSICS
  const bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  const bool sse4 = !!(env->GetCPUFlags() & CPUF_SSE4_1);
  const bool avx =  !!(env->GetCPUFlags() & CPUF_AVX);
  const bool avx2 =  !!(env->GetCPUFlags() & CPUF_AVX2);
#endif

  BitDepthConvFuncPtr conv_function_full_scale;
  BitDepthConvFuncPtr conv_function_shifted_scale;

  // full or limited decision
  // dest: if undefined, use src
  if (_ColorRange_dest != ColorRange_e::AVS_RANGE_LIMITED && _ColorRange_dest != ColorRange_e::AVS_RANGE_FULL) {
    _ColorRange_dest = _ColorRange_src;
  }
  //
  fulls = _ColorRange_src == ColorRange_e::AVS_RANGE_FULL;
  fulld = _ColorRange_dest == ColorRange_e::AVS_RANGE_FULL;

  // ConvertToFloat
  if (target_bitdepth == 32) {
    if (pixelsize == 1) // 8->32 bit
    {
      get_convert_uintN_to_float_functions(8, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
    }
    else if (pixelsize == 2) // 16->32 bit
    {
      if (vi.IsPlanar() && truerange)
        get_convert_uintN_to_float_functions(bits_per_pixel, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
      else
        get_convert_uintN_to_float_functions(16, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
    }
    else
      get_convert_float_to_float_functions(fulls, fulld, conv_function, conv_function_chroma, conv_function_a);

    if (vi.NumComponents() == 1)
      vi.pixel_type = VideoInfo::CS_Y32;
    else if (vi.Is420())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420PS : VideoInfo::CS_YUV420PS;
    else if (vi.Is422())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422PS : VideoInfo::CS_YUV422PS;
    else if (vi.Is444())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444PS : VideoInfo::CS_YUV444PS;
    else if (vi.IsPlanarRGB())
      vi.pixel_type = VideoInfo::CS_RGBPS;
    else if (vi.IsPlanarRGBA())
      vi.pixel_type = VideoInfo::CS_RGBAPS;
    else
      env->ThrowError("ConvertToFloat: unsupported color space");

    return;
  }
  // ConvertToFloat end

  // ConvertTo16bit() (10, 12, 14, 16)
  // Conversion to uint16_t targets
  // planar YUV(A) and RGB(A):
  //   from 8 bit -> 10/12/14/16 with strict range expansion or expansion to 16
  //   from 10/12/14 -> 16 bit with strict source range (expansion from 10/12/14 to 16 bit) or just casting pixel_type
  //   from 16 bit -> 10/12/14 bit with strict target range (reducing range from 16 bit to 10/12/14 bits) or just casting pixel_type
  //   from float -> 10/12/14/16 with strict range expansion or expansion to 16
  // packed RGB:
  //   RGB24->RGB48, RGB32->RGB64
  if (target_bitdepth > 8 && target_bitdepth <= 16) {
    // 8,10-16,32 -> 16 bit
    if (pixelsize == 1) // 8->10-12-14-16 bit
    {
      if (!truerange)
        target_bitdepth = 16;

      // 8->10+ bits: do dither for sure
#ifdef INTEL_INTRINSICS
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a);
#else
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
#endif
    }
    else if (pixelsize == 2)
    {
      // 10-16->10-16
      if (truerange)
      {
        // get basic non-dithered versions
        if (bits_per_pixel != target_bitdepth || fulls != fulld || dither_mode >= 0)
        {
#ifdef INTEL_INTRINSICS
          get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a);
#else
          get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
#endif
          if (bits_per_pixel >= target_bitdepth && dither_mode >= 0) { // reduce range or dither down keeping bit-depth format
            conv_function = get_convert_to_16_16_down_dither_function(fulls, bits_per_pixel, target_bitdepth, dither_mode, dither_bitdepth, 1/*rgb_step n/a*/, 0 /*cpu none*/);
            conv_function_chroma = nullptr; // fixme: no mixed fulls/fulld scale exists, and no special chroma handling :(
          }
        }
      }
      else {
        // no conversion for truerange == false
        format_change_only = true;
      }
    }
    else if (pixelsize == 4) // 32->10-16 bit
    {
      if (!truerange)
        target_bitdepth = 16;

#ifdef INTEL_INTRINSICS
      get_convert_32_to_uintN_functions(target_bitdepth, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a);
#else
      get_convert_32_to_uintN_functions(target_bitdepth, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
#endif
    }
    else {
      env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }

    // set output vi format
    if (vi.IsRGB24()) {
      if (target_bitdepth == 16)
        vi.pixel_type = VideoInfo::CS_BGR48;
      else
        env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }
    else if (vi.IsRGB32()) {
      if (target_bitdepth == 16)
        vi.pixel_type = VideoInfo::CS_BGR64;
      else
        env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }
    else {
      // Y or YUV(A) or PlanarRGB(A)
      if (vi.IsYV12()) // YV12 can have an exotic compatibility constant
        vi.pixel_type = VideoInfo::CS_YV12;
      int new_bitdepth_bits;
      switch (target_bitdepth) {
      case 8: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_8; break;
      case 10: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_10; break;
      case 12: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_12; break;
      case 14: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_14; break;
      case 16: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_16; break;
      case 32: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_32; break;
      }
      vi.pixel_type = (vi.pixel_type & ~VideoInfo::CS_Sample_Bits_Mask) | new_bitdepth_bits;
    }

    return;
  }

  // ConvertTo8bit()
  if (target_bitdepth == 8) {
    if (pixelsize == 1) {
      // 8->8 bits: do dither for sure
      // fixme: really?? lower dither_bits?
#ifdef INTEL_INTRINSICS
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a);
#else
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
#endif
    }
    else if (pixelsize == 2) // 16(,14,12,10)->8 bit
    {
      // it gets complicated, so we better using tuples for function lookup
      // parameters for full scale: source bitdepth, dither_type (0:ordered 1:floyc), target_dither_bitdepth(default 8, 2,4,6), rgb_step(3 for RGB48, 4 for RGB64, 1 for all planars)
      // rgb_step can differ from 1 only when source bits_per_pixel==16 and packed RGB type
      // target_dither_bitdepth==8 (RFU for dithering down from e.g. 10->2 bit)

      if(dither_mode==0 && (dither_bitdepth !=2 && dither_bitdepth != 4 && dither_bitdepth != 6 && dither_bitdepth != 8))
        env->ThrowError("ConvertBits: invalid dither target bitdepth %d", dither_bitdepth);

      int cpuType = 0;
#ifdef INTEL_INTRINSICS
      if (sse2)
        cpuType = CPUF_SSE2;
#endif

      if (!truerange)
        bits_per_pixel = 16;

      // get basic non-dithered versions
#ifdef INTEL_INTRINSICS
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a);
#else
      get_convert_uintN_to_uintN_functions(bits_per_pixel, target_bitdepth, fulls, fulld, conv_function, conv_function_chroma, conv_function_a);
#endif

      if (dither_mode >= 0) {
        conv_function_chroma = nullptr; // fixme: no mixed fulls/fulld scale exists, and no special chroma handling :( make it 'full'-friendly like at regular conversions

        // fill conv_function_full_scale and conv_function_shifted_scale
        // conv_function_full_scale_no_dither: for alpha plane
        conv_function_full_scale = get_convert_to_8_function(true, bits_per_pixel, dither_mode, dither_bitdepth, 1, cpuType);
        conv_function_shifted_scale = get_convert_to_8_function(false, bits_per_pixel, dither_mode, dither_bitdepth, 1, cpuType);

        // FIXME: Check it below, really do we have a special case because of these old formats?
        // For Floyd we convert them to PlanarRGB, maybe we should act for ordered dither as well.
        // // Until then, we support them.
        // override for RGB48 and 64 (internal rgb_step may differ when dithering is used
        if (vi.IsRGB48()) { // packed RGB: specify rgb_step 3 or 4 for dither table access
          conv_function_full_scale = get_convert_to_8_function(true, 16, dither_mode, dither_bitdepth, 3, cpuType);
        }
        else if (vi.IsRGB64()) {
          conv_function_full_scale = get_convert_to_8_function(true, 16, dither_mode, dither_bitdepth, 4, cpuType);
        }

        // packed RGB scaling is full_scale 0..65535->0..255
        if (fulls)
          conv_function = conv_function_full_scale; // rgb default, RGB scaling is not shift by 2/4/6/8 as in YUV but like 0..255->0..65535
        else
          conv_function = conv_function_shifted_scale; // yuv default
      }
    }
    else if (vi.ComponentSize() == 4) // 32->8 bit, no dithering option atm
    {
#ifdef INTEL_INTRINSICS
      get_convert_32_to_uintN_functions(8, fulls, fulld, sse2, sse4, avx2, conv_function, conv_function_chroma, conv_function_a); // all combinations of fulls, fulld
#else
      get_convert_32_to_uintN_functions(8, fulls, fulld, conv_function, conv_function_chroma, conv_function_a); // all combinations of fulls, fulld
#endif
    }
    else
      env->ThrowError("ConvertTo8bit: unsupported bit depth");

    if (vi.NumComponents() == 1)
      vi.pixel_type = VideoInfo::CS_Y8;
    else if (vi.Is420())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420 : VideoInfo::CS_YV12;
    else if (vi.Is422())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422 : VideoInfo::CS_YV16;
    else if (vi.Is444())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444 : VideoInfo::CS_YV24;
    else if (vi.IsRGB48())
      vi.pixel_type = VideoInfo::CS_BGR24;
    else if (vi.IsRGB64())
      vi.pixel_type = VideoInfo::CS_BGR32;
    else if (vi.IsPlanarRGB())
      vi.pixel_type = VideoInfo::CS_RGBP;
    else if (vi.IsPlanarRGBA())
      vi.pixel_type = VideoInfo::CS_RGBAP;
    else
      env->ThrowError("ConvertTo8bit: unsupported color space");

    return;
  }

  env->ThrowError("ConvertBits: unsupported target bit-depth (%d)", target_bitdepth);

}

AVSValue __cdecl ConvertBits::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  //0   1        2        3         4         5           6
  //c[bits]i[truerange]b[dither]i[dither_bits]i[fulls]b[fulld]b

  const VideoInfo &vi = clip->GetVideoInfo();

  int create_param = (int)reinterpret_cast<intptr_t>(user_data);

  // bits parameter is compulsory
  if (!args[1].Defined() && create_param == 0) {
    env->ThrowError("ConvertBits: missing bits parameter");
  }

  // when converting from/true 10-16 bit formats, truerange=false indicates bitdepth of 16 bits regardless of the 10-12-14 bit format
  // FIXME: stop supporting this parameter (a workaround in the dawn of hbd?)
  bool assume_truerange = args[2].AsBool(true); // n/a for non planar formats
                                                // bits parameter

  int target_bitdepth = args[1].AsInt(create_param); // default comes by calling from old To8,To16,ToFloat functions
  int source_bitdepth = vi.BitsPerComponent();
  int dither_bitdepth = args[4].AsInt(target_bitdepth);

  if(target_bitdepth!=8 && target_bitdepth!=10 && target_bitdepth!=12 && target_bitdepth!=14 && target_bitdepth!=16 && target_bitdepth!=32)
    env->ThrowError("ConvertBits: invalid bit depth: %d", target_bitdepth);

  if(create_param == 8 && target_bitdepth !=8)
    env->ThrowError("ConvertTo8Bit: invalid bit depth: %d", target_bitdepth);
  if(create_param == 32 && target_bitdepth !=32)
    env->ThrowError("ConvertToFloat: invalid bit depth: %d", target_bitdepth);
  if(create_param == 16 && (target_bitdepth == 8 || target_bitdepth ==32))
    env->ThrowError("ConvertTo16bit: invalid bit depth: %d", target_bitdepth);

  if (args[2].Defined()) {
    if (!vi.IsPlanar())
      env->ThrowError("ConvertBits: truerange specified for non-planar source");
  }

  // retrieve full/limited
  int ColorRange_src;
  int ColorRange_dest;
  if (args[5].Defined())
    ColorRange_src = args[5].AsBool() ? ColorRange_e::AVS_RANGE_FULL : ColorRange_e::AVS_RANGE_LIMITED;
  else
    ColorRange_src = -1; // undefined. A frame property may override
  if (args[6].Defined())
    ColorRange_dest = args[6].AsBool() ? ColorRange_e::AVS_RANGE_FULL : ColorRange_e::AVS_RANGE_LIMITED;
  else
    ColorRange_dest = -1; // undefined. A frame property or ColorRange_src may override
  // try getting frame props if parameter is not specified
  auto frame0 = clip->GetFrame(0, env);
  const AVSMap* props = env->getFramePropsRO(frame0);
  if (ColorRange_src != ColorRange_e::AVS_RANGE_LIMITED && ColorRange_src != ColorRange_e::AVS_RANGE_FULL) {
    // undefined: frame property check
    if (env->propNumElements(props, "_ColorRange") > 0) {
      ColorRange_src = env->propGetInt(props, "_ColorRange", 0, nullptr); // fixme: range check
    }
    else {
      // no param, no frame property -> rgb is full others are limited
      ColorRange_src = vi.IsRGB() ? ColorRange_e::AVS_RANGE_FULL : ColorRange_e::AVS_RANGE_LIMITED;
    }
  }
  // cr_dest = cr_source if not specified
  if (ColorRange_dest != ColorRange_e::AVS_RANGE_LIMITED && ColorRange_dest != ColorRange_e::AVS_RANGE_FULL) {
    ColorRange_dest = ColorRange_src;
  }
  bool fulls = ColorRange_src == ColorRange_e::AVS_RANGE_FULL;
  bool fulld = ColorRange_dest == ColorRange_e::AVS_RANGE_FULL;


  int dither_type = args[3].AsInt(-1);
  bool dither_defined = args[3].Defined();
  if(dither_defined && dither_type != 1 && dither_type != 0 && dither_type != -1)
    env->ThrowError("ConvertBits: invalid dither type parameter. Only -1 (disabled), 0 (ordered dither) or 1 (Floyd-S) is allowed");

  if (dither_type >= 0) {
    if (source_bitdepth < target_bitdepth)
      env->ThrowError("ConvertBits: dithering is allowed only for scale down");
    if (dither_bitdepth > target_bitdepth)
      env->ThrowError("ConvertBits: dither_bits must be <= target bitdepth");
    if (target_bitdepth == 32)
      env->ThrowError("ConvertBits: dithering is not allowed into 32 bit float target");
  }

  // 3.7.1 t25
  // Unfortunately 32 bit float dithering is not implemented, thus we convert to 16 bit 
  // intermediate clip
  if (source_bitdepth == 32 && (dither_type == 0 || dither_type == 1)) {
    // c[bits]i[truerange]b[dither]i[dither_bits]i[fulls]b[fulld]b
    AVSValue new_args[7] = { clip, 16, true, -1 /* no dither */, AVSValue() /*dither_bits*/, fulls, fulld };
    clip = env->Invoke("ConvertBits", AVSValue(new_args, 7)).AsClip();

    clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
    source_bitdepth = 16;
    // and now the source range becomes the previous target
    fulls = fulld;
    ColorRange_src = ColorRange_dest;
  }

  if (source_bitdepth == 8 && dither_bitdepth == 8)
    dither_type = -1; // ignore dithering from 8 to 8 bit

  if(dither_type == 0) {

    /* note: two-phase, using 16 bit intermediate
    if (source_bitdepth == 32)
      env->ThrowError("ConvertBits: dithering is not allowed for 32 bit sources");
    */

    if (dither_bitdepth < 2 || dither_bitdepth > 16)
      env->ThrowError("ConvertBits: invalid dither_bits specified");

    if (dither_bitdepth % 2)
      env->ThrowError("ConvertBits: dither_bits must be even");

    if(source_bitdepth - dither_bitdepth > 8)
      env->ThrowError("ConvertBits: dither_bits cannot differ with more than 8 bits from source");

    if (source_bitdepth == 8)
      env->ThrowError("ConvertBits: dithering down to less than 8 bits is not supported for 8 bit sources");
  }

  // floyd
  if (dither_type == 1) {

    if (source_bitdepth == 8 || source_bitdepth == 32)
      env->ThrowError("ConvertBits: Floyd-S: dithering is allowed only for 10-16 bit sources");

    if (dither_bitdepth < 0 || dither_bitdepth > 16)
      env->ThrowError("ConvertBits: Floyd-S: invalid dither_bits specified");

    if ((dither_bitdepth > 8 && (dither_bitdepth%2) != 0)) // must be even above 8 bits. 0 is ok, means real b/w
      env->ThrowError("ConvertBits: Floyd-S: dither_bits must be 0..8, 10, 12, 14, 16");
  }

  // no change -> return unmodified if no transform required
  if (source_bitdepth == target_bitdepth) { // 10->10 .. 16->16
    if((dither_type < 0 || dither_bitdepth == target_bitdepth) && fulls == fulld)
      return clip;
    if(vi.IsRGB() && !vi.IsPlanar())
      env->ThrowError("ConvertBits: dithering_bits should be the same as target bitdepth for packed RGB formats");
    // here: we allow e.g. a 16->16 bit conversion with dithering bitdepth of 8
  }

  // YUY2 conversion is limited
  if (vi.IsYUY2()) {
    env->ThrowError("ConvertBits: YUY2 source is 8-bit only");
  }

  if (vi.IsYV411()) {
    env->ThrowError("ConvertBits: YV411 source cannot be converted");
  }

  // packed RGB conversion is limited
  if (vi.IsRGB24() || vi.IsRGB32()) {
    if (target_bitdepth != 16)
      env->ThrowError("ConvertBits: invalid bit-depth specified for packed RGB");
  }

  if (vi.IsRGB48() || vi.IsRGB64()) {
    if (target_bitdepth != 8)
      env->ThrowError("ConvertBits: invalid bit-depth specified for packed RGB");
  }

    // remark
    // source_10_bit.ConvertTo16bit(truerange=true)  : upscale range
    // source_10_bit.ConvertTo16bit(truerange=false) : leaves data, only format conversion
    // source_10_bit.ConvertTo16bit(bits=12,truerange=true)  : upscale range from 10 to 12
    // source_10_bit.ConvertTo16bit(bits=12,truerange=false) : leaves data, only format conversion
    // source_16_bit.ConvertTo16bit(bits=10, truerange=true)  : downscale range
    // source_16_bit.ConvertTo16bit(bits=10, truerange=false) : leaves data, only format conversion

  // for floyd, planar rgb conversion happens
  bool need_convert_48 = vi.IsRGB48() && dither_type == 1 && target_bitdepth == 8;
  bool need_convert_64 = vi.IsRGB64() && dither_type == 1 && target_bitdepth == 8;

  // convert to planar on the fly
  if (need_convert_48) {
    AVSValue new_args[1] = { clip };
    clip = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 1)).AsClip();
  } else if (need_convert_64) {
    AVSValue new_args[1] = { clip };
    clip = env->Invoke("ConvertToPlanarRGBA", AVSValue(new_args, 1)).AsClip();
  }

  // 3.7.1 t25: this case is covered with float32 bit intermediate
  // 3.7.1 t26: only dither option + different fulls-fulld was left behind to do FIXME
  if (fulls != fulld && target_bitdepth < 32 && source_bitdepth < 32 && dither_type>=0) {
    // Convert to float

    // c[bits]i[truerange]b[dither]i[dither_bits]i[fulls]b[fulld]b
    AVSValue new_args[7] = { clip, 32, true, -1 /* no dither */, AVSValue() /*dither_bits*/, fulls, fulld };
    auto clip = env->Invoke("ConvertBits", AVSValue(new_args, 7)).AsClip();

    clip = env->Invoke("Cache", AVSValue(clip)).AsClip();
    source_bitdepth = 16;
    // and now the source range becomes the previous target
    fulls = fulld;

    // Convert back to integer
    AVSValue new_args2[7] = { clip, target_bitdepth, true, dither_type, dither_bitdepth /*dither_bits*/, fulls, fulld };
    return env->Invoke("ConvertBits", AVSValue(new_args2, 7)).AsClip();
  }

  AVSValue result = new ConvertBits(clip, dither_type, target_bitdepth, assume_truerange, ColorRange_src, ColorRange_dest, dither_bitdepth, env);

  // convert back to packed rgb on the fly
  if (need_convert_48) {
    AVSValue new_args[1] = { result };
    result = env->Invoke("ConvertToRGB24", AVSValue(new_args, 1)).AsClip();
  } else if (need_convert_64) {
    AVSValue new_args[1] = { result };
    result = env->Invoke("ConvertToRGB32", AVSValue(new_args, 1)).AsClip();
  }

  return result;
}


PVideoFrame __stdcall ConvertBits::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (format_change_only)
  {
    // for 10-16 bit: simple format override in constructor
    return src;
  }

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);

  auto props = env->getFramePropsRW(dst);
  update_ColorRange(props, fulld ? ColorRange_e::AVS_RANGE_FULL : ColorRange_e::AVS_RANGE_LIMITED, env);

  if(vi.IsPlanar())
  {
    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    for (int p = 0; p < vi.NumComponents(); ++p) {
      const int plane = planes[p];
      if (plane == PLANAR_A) {
        if (conv_function_a == nullptr)
          env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
        else
          conv_function_a(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane),
            bits_per_pixel, target_bitdepth
          );
      }
      else if (conv_function == nullptr)
        env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
      else {
        const bool chroma = (plane == PLANAR_U || plane == PLANAR_V);
        if (chroma && conv_function_chroma != nullptr)
          // 32bit float and 8-16 when full-range involved needs separate signed-aware conversion
          conv_function_chroma(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane),
            bits_per_pixel, target_bitdepth);
        else
          conv_function(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane),
            bits_per_pixel, target_bitdepth);
      }
    }
  }
  else {
    // packed RGBs
    conv_function(src->GetReadPtr(), dst->GetWritePtr(),
      src->GetRowSize(), src->GetHeight(),
      src->GetPitch(), dst->GetPitch(),
      bits_per_pixel, target_bitdepth);
  }
  return dst;
}
