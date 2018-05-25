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

// Overlay (c) 2003, 2004 by Klaus Post

#include <avs/config.h>

#include "blend_common.h"
#include "overlayfunctions.h"

// Intrinsics for SSE4.1, SSSE3, SSE3, SSE2, ISSE and MMX
#include <smmintrin.h>
#include <stdint.h>
#include <type_traits>


/*******************************
 ********* Masked Blend ********
 *******************************/

__forceinline static BYTE overlay_blend_c_core_8(const BYTE p1, const BYTE p2, const int mask) {
  if (mask == 0)
    return p1;
  if (mask == 0xFF)
    return p2;
  //  p1*(1-mask_f) + p2*mask_f -> p1 + (p2-p1)*mask_f
  return (BYTE)(((p1 << 8) + (p2 - p1)*mask + 128) >> 8);
}

template<int bits_per_pixel>
__forceinline static uint16_t overlay_blend_c_core_16(const uint16_t p1, const uint16_t p2, const int mask) {
  if (mask == 0)
    return p1;
  if (mask >= (1 << bits_per_pixel) -1)
    return p2;
  //  p1*(1-mask_f) + p2*mask_f -> p1 + (p2-p1)*mask_f
  const int half_rounder = 1 << (bits_per_pixel - 1);
  if constexpr(bits_per_pixel == 16) // int32 intermediate overflows
    return (uint16_t)((((int64_t)(p1) << bits_per_pixel) + (p2 - p1)*(int64_t)mask + half_rounder) >> bits_per_pixel);
  else // signed int arithmetic is enough
    return (uint16_t)(((p1 << bits_per_pixel) + (p2 - p1)*mask + half_rounder) >> bits_per_pixel);
}

__forceinline static float overlay_blend_c_core_f(const float p1, const float p2, const float mask) {
  return p1 + (p2-p1)*mask; // p1*(1-mask) + p2*mask
}


#ifdef X86_32
__forceinline static __m64 overlay_blend_mmx_core(const __m64& p1, const __m64& p2, const __m64& mask, const __m64& v128) {
  __m64 tmp1 = _mm_mullo_pi16(_mm_sub_pi16(p2, p1), mask); // (p2-p1)*mask
  __m64 tmp2 = _mm_or_si64(_mm_slli_pi16(p1, 8), v128);    // p1<<8 + 128 == p1<<8 | 128
  return _mm_srli_pi16(_mm_add_pi16(tmp1, tmp2), 8);
}
#endif

template<typename pixel_t, int bits_per_pixel>
__forceinline static __m128i overlay_blend_sse2_core(const __m128i& p1, const __m128i& p2, const __m128i& mask, const __m128i& v128) {
  // v128 is rounding half, no use for float
  if constexpr(sizeof(pixel_t) == 1) {
    // done outside: 0 ot 255 overlay values becoming 1 and 254 for full mask transparency
    // p1*(1-mask) + p2*mask = p1+(p2-p1)*mask
    // p1   p2    mask    (p2-p1)*mask   p1<<8 + 128     sum      result  good result
    // 255   0    255     -65025          65408          384         1         0
    //                     511            -128                     254
    // 0    255   255
    __m128i tmp1 = _mm_mullo_epi16(_mm_sub_epi16(p2, p1), mask); // (p2-p1)*mask
    __m128i tmp2 = _mm_or_si128(_mm_slli_epi16(p1, 8), v128);    // p1<<8 + 128 == p1<<8 | 128
    return _mm_srli_epi16(_mm_add_epi16(tmp1, tmp2), 8);
  }
  else if constexpr(sizeof(pixel_t) == 2) {
    __m128i tmp1 = _mm_mullo_epi32(_mm_sub_epi32(p2, p1), mask); // (p2-p1)*mask
    __m128i tmp2 = _mm_or_si128(_mm_slli_epi32(p1, bits_per_pixel), v128);    // p1<<bits_per_pixel + half == p1<<bits_per_pixel | half
    return _mm_srli_epi32(_mm_add_epi32(tmp1, tmp2), bits_per_pixel);
  }
  else if constexpr(sizeof(pixel_t) == 4) {
    __m128 mulres = _mm_mul_ps(_mm_sub_ps(_mm_castsi128_ps(p2), _mm_castsi128_ps(p1)), _mm_castsi128_ps(mask));
    return _mm_castps_si128(_mm_add_ps(_mm_castsi128_ps(p1),mulres)); // p1*(1-mask) + p2*mask = p1+(p2-p1)*mask
  }
}

/*******************************************
 ********* Merge Two Masks Function ********
 *******************************************/
template<typename pixel_t, typename intermediate_result_t, int bits_per_pixel>
__forceinline static pixel_t overlay_merge_mask_c(const pixel_t p1, const pixel_t p2) {
  return ((intermediate_result_t)p1*p2) >> bits_per_pixel;
}

__forceinline static BYTE overlay_merge_mask_c_8(const BYTE p1, const int p2) {
  return (p1*p2) >> 8;
}

#ifdef X86_32
__forceinline static __m64 overlay_merge_mask_mmx(const __m64& p1, const __m64& p2) {
  __m64 t1 = _mm_mullo_pi16(p1, p2);
  __m64 t2 = _mm_srli_pi16(t1, 8);
  return t2;
}
#endif

template<typename pixel_t, int bits_per_pixel>
__forceinline static __m128i overlay_merge_mask_sse2(const __m128i& p1, const __m128i& p2) {
  if constexpr(sizeof(pixel_t) == 1) {
    __m128i t1 = _mm_mullo_epi16(p1, p2);
    __m128i t2 = _mm_srli_epi16(t1, 8);
    return t2;
  }
  else if constexpr(sizeof(pixel_t) == 2) {
    __m128i t1 = _mm_mullo_epi32(p1, p2);
    __m128i t2 = _mm_srli_epi32(t1, bits_per_pixel);
    return t2;
  }
  else if constexpr(sizeof(pixel_t) == 4) {
    __m128 mulres = _mm_mul_ps(_mm_castsi128_ps(p1), _mm_castsi128_ps(p2));
    return _mm_castps_si128(mulres);
  }
}

/********************************
 ********* Blend Opaque *********
 ** Use for Lighten and Darken **
 ********************************/
template<typename pixel_t>
__forceinline pixel_t overlay_blend_opaque_c_core(const pixel_t p1, const pixel_t p2, const pixel_t mask) {
  return (mask) ? p2 : p1;
}

#ifdef X86_32
__forceinline __m64 overlay_blend_opaque_mmx_core(const __m64& p1, const __m64& p2, const __m64& mask) {
  __m64 r1 = _mm_andnot_si64(mask, p1);
  __m64 r2 = _mm_and_si64   (mask, p2);
  return _mm_or_si64(r1, r2);
}
#endif

__forceinline __m128i overlay_blend_opaque_sse2_core(const __m128i& p1, const __m128i& p2, const __m128i& mask) {
  __m128i r1 = _mm_andnot_si128(mask, p1);
  __m128i r2 = _mm_and_si128   (mask, p2);
  return _mm_or_si128(r1, r2);
}

__forceinline __m128i overlay_blend_opaque_sse41_core(const __m128i& p1, const __m128i& p2, const __m128i& mask) {
  return _mm_blendv_epi8(p1, p2, mask);
}

/******************************
 ********* Mode: Blend ********
 ******************************/

template<typename pixel_t, int bits_per_pixel>
void overlay_blend_c_plane_masked(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                  const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                  const int width, const int height) {
  const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
  const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));
  const int max_pixel_value = (1 << bits_per_pixel) - 1;

  // avoid "uint16*uint16 can't get into int32" overflows
  typedef typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int new_mask = reinterpret_cast<const pixel_t *>(mask)[x];
      pixel_t p1x = reinterpret_cast<pixel_t *>(p1)[x];
      pixel_t p2x = reinterpret_cast<const pixel_t *>(p2)[x];
      pixel_t result;
      if constexpr(bits_per_pixel == 8)
        result = (pixel_t)overlay_blend_c_core_8((BYTE)p1x, (BYTE)p2x, new_mask);
      else
        result = (pixel_t)overlay_blend_c_core_16<bits_per_pixel>((uint16_t)p1x, (uint16_t)p2x, new_mask);
      reinterpret_cast<pixel_t *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

void overlay_blend_c_plane_masked_f(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height) {

  typedef float pixel_t;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixel_t new_mask = reinterpret_cast<const pixel_t *>(mask)[x];
      pixel_t p1x = reinterpret_cast<pixel_t *>(p1)[x];
      pixel_t p2x = reinterpret_cast<const pixel_t *>(p2)[x];
      pixel_t result = p1x + (p2x-p1x)*new_mask; // p1x*(1-new_mask) + p2x*mask

      //pixel_t result = overlay_blend_c_core(reinterpret_cast<pixel_t *>(p1)[x], reinterpret_cast<pixel_t *>(p2)[x], static_cast<int>(reinterpret_cast<pixel_t *>(mask)[x]));
      reinterpret_cast<pixel_t *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

// instantiate
template void overlay_blend_c_plane_masked<uint8_t, 8>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height);
template void overlay_blend_c_plane_masked<uint16_t,10>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height);
template void overlay_blend_c_plane_masked<uint16_t,12>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height);
template void overlay_blend_c_plane_masked<uint16_t,14>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height);
template void overlay_blend_c_plane_masked<uint16_t,16>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height);


#ifdef X86_32
// The following disable EMMS warning, since it's caller-called.
#pragma warning (push)
#pragma warning (disable: 4799)
void overlay_blend_mmx_plane_masked(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                    const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                    const int width, const int height) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;
  const BYTE* original_mask = mask;

  __m64 v128 = _mm_set1_pi16(0x0080);
  __m64 zero = _mm_setzero_si64();

  int wMod8 = (width/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 p1_l = *(reinterpret_cast<const __m64*>(p1+x));
      __m64 p2_l = *(reinterpret_cast<const __m64*>(p2+x));
      __m64 mask_l = *(reinterpret_cast<const __m64*>(mask+x));

      __m64 unpacked_p1_l = _mm_unpacklo_pi8(p1_l, zero);
      __m64 unpacked_p1_h = _mm_unpackhi_pi8(p1_l, zero);

      __m64 unpacked_p2_l = _mm_unpacklo_pi8(p2_l, zero);
      __m64 unpacked_p2_h = _mm_unpackhi_pi8(p2_l, zero);

      __m64 unpacked_mask_l = _mm_unpacklo_pi8(mask_l, zero);
      __m64 unpacked_mask_h = _mm_unpackhi_pi8(mask_l, zero);

      __m64 result_l = overlay_blend_mmx_core(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
      __m64 result_h = overlay_blend_mmx_core(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

      __m64 result = _m_packuswb(result_l, result_h);

      *reinterpret_cast<__m64*>(p1+x) = result;
    }
    
    // Leftover value
    for (int x = wMod8; x < width; x++) {
      BYTE result = overlay_blend_c_core_8(p1[x], p2[x], static_cast<int>(mask[x]));
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}
#pragma warning (pop)
#endif

template<bool hasSSE4>
static __forceinline __m128i simd_blend_epi8(__m128i const &selector, __m128i const &a, __m128i const &b) {
  if (hasSSE4) {
    return _mm_blendv_epi8(b, a, selector);
  }
  else {
    return _mm_or_si128(_mm_and_si128(selector, a), _mm_andnot_si128(selector, b));
  }
}

// non-existant in simd
static __forceinline __m128i _MM_CMPLE_EPU16(__m128i x, __m128i y)
{
  // Returns 0xFFFF where x <= y:
  return _mm_cmpeq_epi16(_mm_subs_epu16(x, y), _mm_setzero_si128());
}


template<typename pixel_t, int bits_per_pixel, bool hasSSE4>
void overlay_blend_sse2_plane_masked(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                     const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                     const int width, const int height) {
  __m128i v128;
  if constexpr(sizeof(pixel_t) == 1)
    v128 = _mm_set1_epi16(0x0080);
  else if constexpr(sizeof(pixel_t) == 2) {
    const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
    const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));
    v128 = _mm_set1_epi32(half_pixel_value_rounding);
  }
  else { // float
    v128 = _mm_setzero_si128(); // n/a
  }
  const int realwidth = width * sizeof(pixel_t);

  __m128i zero = _mm_setzero_si128();

  int wMod16 = (realwidth/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i dst;
      __m128i src;
      __m128i msk;
      __m128i p1_f, p2_f, mask_f;

      if constexpr(sizeof(pixel_t) == 1 || sizeof(pixel_t) == 2) {
        dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1 + x));
        src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2 + x));
        msk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask + x));
      }
      else {
        p1_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1 + x));
        p2_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2 + x));
        mask_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask + x));
      }

      __m128i result;
      if constexpr(sizeof(pixel_t) == 1) {
        
        __m128i unpacked_mask_l = _mm_unpacklo_epi8(msk, zero);
        __m128i unpacked_mask_h = _mm_unpackhi_epi8(msk, zero);
        
        __m128i unpacked_p1_l = _mm_unpacklo_epi8(dst, zero);
        __m128i unpacked_p1_h = _mm_unpackhi_epi8(dst, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi8(src, zero);
        __m128i unpacked_p2_h = _mm_unpackhi_epi8(src, zero);

        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

        result = _mm_packus_epi16(result_l, result_h);
        
        // when mask is FF, keep src
        // when mask is 00, keep dst
        //auto msk = _mm_packus_epi16(unpacked_mask_l, unpacked_mask_h); // we have mask here
        auto mask_00 = _mm_cmpeq_epi8(msk, zero);
        result = simd_blend_epi8<hasSSE4>(mask_00, dst, result); // ensure that zero mask value returns dst
        auto max_pixel_value = _mm_set1_epi8(static_cast<unsigned char>(0xFF)); // for comparison
        auto mask_FF = _mm_cmpeq_epi8(msk, max_pixel_value); // mask == max ? FF : 00
        result = simd_blend_epi8<hasSSE4>(mask_FF, src, result); // ensure that max mask value returns src
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        __m128i unpacked_p1_l = _mm_unpacklo_epi16(dst, zero);
        __m128i unpacked_p1_h = _mm_unpackhi_epi16(dst, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi16(src, zero);
        __m128i unpacked_p2_h = _mm_unpackhi_epi16(src, zero);

        __m128i unpacked_mask_l = _mm_unpacklo_epi16(msk, zero);
        __m128i unpacked_mask_h = _mm_unpackhi_epi16(msk, zero);

        // for uint16, this is SSE4
        // maybe _MM_MULLO_EPI32 and _MM_PACKUS_EPI32 could be used, but sometimes C is faster
        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

        result = _mm_packus_epi32(result_l, result_h);

        __m128i max_pixel_value = _mm_set1_epi16(static_cast<uint16_t>((1 << bits_per_pixel) - 1));

        if constexpr(bits_per_pixel < 16) // otherwise no clamp needed
          result = _mm_min_epi16(result, max_pixel_value); // SSE2 epi16 is enough

        __m128i mask_FFFF;
        if constexpr(bits_per_pixel < 16) // paranoia 
          mask_FFFF = _MM_CMPLE_EPU16(max_pixel_value, msk); // mask >= max_value ? FFFF : 0000 -> max_value <= mask 
        else
          mask_FFFF = _mm_cmpeq_epi16(msk, max_pixel_value); // mask == max ? FFFF : 0000
        auto mask_zero = _mm_cmpeq_epi16(msk, zero);

        result = simd_blend_epi8<hasSSE4>(mask_FFFF, src, result); // ensure that max mask value returns src
        result = simd_blend_epi8<hasSSE4>(mask_zero, dst, result); // ensure that zero mask value returns dst
      }
      else {
        // sizeof(pixel_t) == 4, float
        result = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(p1_f, p2_f, mask_f, v128);
      }

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }
    
    // Leftover value

    for (int x = wMod16/sizeof(pixel_t); x < width; x++) {
      if constexpr(sizeof(pixel_t) == 1) {
        BYTE result = overlay_blend_c_core_8(p1[x], p2[x], static_cast<int>(mask[x]));
        p1[x] = result;
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        int new_mask = static_cast<int>(reinterpret_cast<const uint16_t *>(mask)[x]);
        uint16_t result = overlay_blend_c_core_16<bits_per_pixel>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], new_mask);
        reinterpret_cast<pixel_t *>(p1)[x] = (pixel_t)result;
      }
      else { // float
        float result = overlay_blend_c_core_f(reinterpret_cast<float *>(p1)[x], reinterpret_cast<const float *>(p2)[x], reinterpret_cast<const float *>(mask)[x]);
        reinterpret_cast<pixel_t *>(p1)[x] = (pixel_t)result;
      }
    }


    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

// instantiate
// last false//true: hasSSE4
template void overlay_blend_sse2_plane_masked<uint8_t, 8, false>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
template void overlay_blend_sse2_plane_masked<uint8_t, 8, true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
// 16 bit: SSE4 only
template void overlay_blend_sse2_plane_masked<uint16_t, 10, true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
template void overlay_blend_sse2_plane_masked<uint16_t, 12, true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
template void overlay_blend_sse2_plane_masked<uint16_t, 14, true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
template void overlay_blend_sse2_plane_masked<uint16_t, 16, true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);
// float: SSE2 only
template void overlay_blend_sse2_plane_masked<float, 8, false>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height);

template<typename pixel_t, int bits_per_pixel>
void overlay_blend_c_plane_opacity(BYTE *p1, const BYTE *p2,
                                   const int p1_pitch, const int p2_pitch,
                                   const int width, const int height, const int opacity) {

  const int OPACITY_SHIFT  = 8; // opacity always max 0..256
  const int MASK_CORR_SHIFT = OPACITY_SHIFT; // no mask, mask = opacity, 8 bits always
  const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));

  // avoid "uint16*uint16 can't get into int32" overflows
  // no need here, opacity as mask is always 8 bit
  // typedef std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixel_t p1x = reinterpret_cast<pixel_t *>(p1)[x];
      pixel_t p2x = reinterpret_cast<const pixel_t *>(p2)[x];
      pixel_t result = (pixel_t)((((p1x << MASK_CORR_SHIFT) | half_pixel_value_rounding) + (p2x-p1x)*opacity) >> MASK_CORR_SHIFT);
      //BYTE result = overlay_blend_c_core_8(p1[x], p2[x], opacity);
      reinterpret_cast<pixel_t *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }
}

void overlay_blend_c_plane_opacity_f(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const float opacity_f) {

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float p1x = reinterpret_cast<float *>(p1)[x];
      float p2x = reinterpret_cast<const float *>(p2)[x];
      float result = p1x + (p2x-p1x)*opacity_f;
      reinterpret_cast<float *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }
}

// instantiate
template void overlay_blend_c_plane_opacity<uint8_t, 8>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_opacity<uint16_t,10>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_opacity<uint16_t,12>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_opacity<uint16_t,14>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_opacity<uint16_t,16>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity);


#ifdef X86_32
// The following disable EMMS warning, since it's caller-called.
#pragma warning (push)
#pragma warning (disable: 4799)
void overlay_blend_mmx_plane_opacity(BYTE *p1, const BYTE *p2,
                                     const int p1_pitch, const int p2_pitch,
                                     const int width, const int height, const int opacity) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;

  __m64 v128 = _mm_set1_pi16(0x0080);
  __m64 zero = _mm_setzero_si64();
  __m64 mask = _mm_set1_pi16(static_cast<short>(opacity));

  int wMod8 = (width/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 p1_l = *(reinterpret_cast<const __m64*>(p1+x));
      __m64 p2_l = *(reinterpret_cast<const __m64*>(p2+x));

      __m64 unpacked_p1_l = _mm_unpacklo_pi8(p1_l, zero);
      __m64 unpacked_p1_h = _mm_unpackhi_pi8(p1_l, zero);

      __m64 unpacked_p2_l = _mm_unpacklo_pi8(p2_l, zero);
      __m64 unpacked_p2_h = _mm_unpackhi_pi8(p2_l, zero);

      __m64 result_l = overlay_blend_mmx_core(unpacked_p1_l, unpacked_p2_l, mask, v128);
      __m64 result_h = overlay_blend_mmx_core(unpacked_p1_h, unpacked_p2_h, mask, v128);

      __m64 result = _m_packuswb(result_l, result_h);

      *reinterpret_cast<__m64*>(p1+x) = result;
    }

    // Leftover value
    for (int x = wMod8; x < width; x++) {
      BYTE result = overlay_blend_c_core_8(p1[x], p2[x], opacity);
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }
}
#pragma warning (pop)
#endif

template<typename pixel_t, int bits_per_pixel>
void overlay_blend_sse2_plane_opacity(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f) {
/*
  const int OPACITY_SHIFT  = 8; // opacity always max 0..256
  const int MASK_CORR_SHIFT = OPACITY_SHIFT; // no mask, mask = opacity, 8 bits always
  const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));

  // avoid "uint16*uint16 can't get into int32" overflows
  // no need here, opacity as mask is always 8 bit
  // typedef std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      pixel_t p1x = reinterpret_cast<pixel_t *>(p1)[x];
      pixel_t p2x = reinterpret_cast<const pixel_t *>(p2)[x];
      pixel_t result = (pixel_t)((((p1x << MASK_CORR_SHIFT) | half_pixel_value_rounding) + (p2x-p1x)*opacity) >> MASK_CORR_SHIFT);
      //BYTE result = overlay_blend_c_core_8(p1[x], p2[x], opacity);
      reinterpret_cast<pixel_t *>(p1)[x] = result;
    }
*/
  const int OPACITY_SHIFT = 8; // opacity always max 0..256

  __m128i v128, mask;
  __m128i zero = _mm_setzero_si128();
  int opacity_scaled = 0;
  if constexpr(sizeof(pixel_t) == 1) {
    v128 = _mm_set1_epi16(0x0080);
    mask = _mm_set1_epi16(static_cast<short>(opacity));
  }
  else if constexpr(sizeof(pixel_t) == 2) {
    const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
    const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));
    v128 = _mm_set1_epi32(half_pixel_value_rounding);
    opacity_scaled = opacity << (MASK_CORR_SHIFT - 8);
    mask = _mm_set1_epi32(opacity_scaled); // opacity always max 0..256, have to scale
  }
  else { // float
    v128 = _mm_setzero_si128(); // n/a
    mask = _mm_castps_si128(_mm_set1_ps(opacity_f));
  }
  const int realwidth = width * sizeof(pixel_t);

  int wMod16 = (realwidth/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i p1_l, p1_h;
      __m128i p2_l, p2_h;
      __m128i p1_f, p2_f;

      if constexpr(sizeof(pixel_t) == 1 || sizeof(pixel_t) == 2) {
        p1_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1 + x));
        p1_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1 + x + 8));

        p2_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2 + x));
        p2_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2 + x + 8));
      }
      else {
        p1_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1 + x));
        p2_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2 + x));
      }

      __m128i result;
      if constexpr(sizeof(pixel_t) == 1) {
        __m128i unpacked_p1_l = _mm_unpacklo_epi8(p1_l, zero);
        __m128i unpacked_p1_h = _mm_unpacklo_epi8(p1_h, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi8(p2_l, zero);
        __m128i unpacked_p2_h = _mm_unpacklo_epi8(p2_h, zero);

        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, mask, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, mask, v128);

        result = _mm_packus_epi16(result_l, result_h);
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        __m128i unpacked_p1_l = _mm_unpacklo_epi16(p1_l, zero);
        __m128i unpacked_p1_h = _mm_unpacklo_epi16(p1_h, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi16(p2_l, zero);
        __m128i unpacked_p2_h = _mm_unpacklo_epi16(p2_h, zero);

        // for uint16, this is SSE4
        // maybe _MM_MULLO_EPI32 and _MM_PACKUS_EPI32 could be used, but sometimes C is faster
        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, mask, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, mask, v128);

        result = _mm_packus_epi32(result_l, result_h);
      }
      else {
        // sizeof(pixel_t) == 4, float
        result = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(p1_f, p2_f, mask, v128);
      }

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }

    // Leftover value
    for (int x = wMod16/sizeof(pixel_t); x < width; x++) {
      if constexpr(sizeof(pixel_t) == 1) {
        BYTE result = overlay_blend_c_core_8(p1[x], p2[x], opacity);
        p1[x] = result;
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        uint16_t result;
        switch (bits_per_pixel) {
        case 10: result = overlay_blend_c_core_16<10>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], opacity_scaled);
          break;
        case 12: result = overlay_blend_c_core_16<12>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], opacity_scaled);
          break;
        case 14: result = overlay_blend_c_core_16<14>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], opacity_scaled);
          break;
        case 16: result = overlay_blend_c_core_16<16>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], opacity_scaled);
          break;
        }
        reinterpret_cast<pixel_t *>(p1)[x] = (pixel_t)result;
      }
      else { // float
        float result = overlay_blend_c_core_f(reinterpret_cast<float *>(p1)[x], reinterpret_cast<const float *>(p2)[x], opacity_f);
        reinterpret_cast<pixel_t *>(p1)[x] = (pixel_t)result;
      }
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }
}

// instantiate
template void overlay_blend_sse2_plane_opacity<uint8_t,8>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_opacity<uint16_t,10>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_opacity<uint16_t,12>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_opacity<uint16_t,14>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_opacity<uint16_t,16>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_opacity<float,8>(BYTE *p1, const BYTE *p2,
  const int p1_pitch, const int p2_pitch,
  const int width, const int height, const int opacity, const float opacity_f);


template<typename pixel_t, int bits_per_pixel>
void overlay_blend_c_plane_masked_opacity(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                  const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                  const int width, const int height, const int opacity) {
  const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
  const int OPACITY_SHIFT  = 8; // opacity always max 0..256
  const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));

  // avoid "uint16*uint16 can't get into int32" overflows
  typedef typename std::conditional < sizeof(pixel_t) == 1, int, typename std::conditional < sizeof(pixel_t) == 2, __int64, float>::type >::type result_t;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int new_mask = (reinterpret_cast<const pixel_t *>(mask)[x] * opacity) >> OPACITY_SHIFT; // int is enough, opacity is 8 bits
      result_t p1x = reinterpret_cast<pixel_t *>(p1)[x];
      pixel_t p2x = reinterpret_cast<const pixel_t *>(p2)[x];

      pixel_t result = (pixel_t)((((p1x << MASK_CORR_SHIFT) | half_pixel_value_rounding) + (p2x-p1x)*new_mask) >> MASK_CORR_SHIFT);
      //int new_mask = overlay_merge_mask_c<pixel_t, intermediate_result_t, bits_per_pixel>(mask[x], opacity);
      //BYTE result = overlay_blend_c_core_8(p1[x], p2[x], static_cast<int>(new_mask));
      reinterpret_cast<pixel_t *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

// instantiate
template void overlay_blend_c_plane_masked_opacity<uint8_t, 8>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_masked_opacity<uint16_t,10>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_masked_opacity<uint16_t,12>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_masked_opacity<uint16_t,14>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const int opacity);
template void overlay_blend_c_plane_masked_opacity<uint16_t,16>(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const int opacity);

void overlay_blend_c_plane_masked_opacity_f(BYTE *p1, const BYTE *p2, const BYTE *mask,
  const int p1_pitch, const int p2_pitch, const int mask_pitch,
  const int width, const int height, const float opacity_f) {

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      float new_mask = (reinterpret_cast<const float *>(mask)[x] * opacity_f);
      float p1x = reinterpret_cast<float *>(p1)[x];
      float p2x = reinterpret_cast<const float *>(p2)[x];

      float result = p1x + (p2x-p1x)*new_mask;
      reinterpret_cast<float *>(p1)[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}


#ifdef X86_32
// The following disable EMMS warning, since it's caller-called.
#pragma warning (push)
#pragma warning (disable: 4799)
void overlay_blend_mmx_plane_masked_opacity(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                    const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                    const int width, const int height, const int opacity) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;
  const BYTE* original_mask = mask;

  __m64 v128 = _mm_set1_pi16(0x0080);
  __m64 zero = _mm_setzero_si64();
  __m64 opacity_mask = _mm_set1_pi16(static_cast<short>(opacity));

  int wMod8 = (width/8) * 8;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x += 8) {
      __m64 p1_l = *(reinterpret_cast<const __m64*>(p1+x));
      __m64 p2_l = *(reinterpret_cast<const __m64*>(p2+x));
      __m64 mask_l = *(reinterpret_cast<const __m64*>(mask+x));

      __m64 unpacked_p1_l = _mm_unpacklo_pi8(p1_l, zero);
      __m64 unpacked_p1_h = _mm_unpackhi_pi8(p1_l, zero);

      __m64 unpacked_p2_l = _mm_unpacklo_pi8(p2_l, zero);
      __m64 unpacked_p2_h = _mm_unpackhi_pi8(p2_l, zero);

      __m64 unpacked_mask_l = _mm_unpacklo_pi8(mask_l, zero);
      __m64 unpacked_mask_h = _mm_unpackhi_pi8(mask_l, zero);

      unpacked_mask_l = overlay_merge_mask_mmx(unpacked_mask_l, opacity_mask);
      unpacked_mask_h = overlay_merge_mask_mmx(unpacked_mask_h, opacity_mask);

      __m64 result_l = overlay_blend_mmx_core(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
      __m64 result_h = overlay_blend_mmx_core(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

      __m64 result = _m_packuswb(result_l, result_h);

      *reinterpret_cast<__m64*>(p1+x) = result;
    }

    // Leftover value
    for (int x = wMod8; x < width; x++) {
      int new_mask = overlay_merge_mask_c_8(mask[x], opacity);
      BYTE result = overlay_blend_c_core_8(p1[x], p2[x], static_cast<int>(new_mask));
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}
#pragma warning(pop)
#endif

template<typename pixel_t, int bits_per_pixel, bool hasSSE4>
void overlay_blend_sse2_plane_masked_opacity(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                     const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                     const int width, const int height, const int opacity, const float opacity_f) {

  AVS_UNUSED(opacity_f);

  const int OPACITY_SHIFT = 8; // opacity always max 0..256

  __m128i v128, opacity_mask;
  __m128i zero = _mm_setzero_si128();
  if constexpr(sizeof(pixel_t) == 1) {
    v128 = _mm_set1_epi16(0x0080);
    opacity_mask = _mm_set1_epi16(static_cast<short>(opacity));
  }
  else if constexpr(sizeof(pixel_t) == 2) {
    const int MASK_CORR_SHIFT = (sizeof(pixel_t) == 1) ? 8 : bits_per_pixel;
    const int half_pixel_value_rounding = (1 << (MASK_CORR_SHIFT - 1));
    v128 = _mm_set1_epi32(half_pixel_value_rounding);
    const int opacity_scaled = opacity << (MASK_CORR_SHIFT - 8);
    opacity_mask = _mm_set1_epi32(opacity_scaled); // opacity always max 0..256, have to scale
  }
  else { // float
    v128 = _mm_setzero_si128(); // n/a
    opacity_mask = _mm_castps_si128(_mm_set1_ps(opacity_f));
  }
  const int realwidth = width * sizeof(pixel_t);

  int wMod16 = (realwidth/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i dst;
      __m128i src;
      __m128i msk;
      __m128i p1_f, p2_f;
      __m128i mask_f;

      if constexpr(sizeof(pixel_t) == 1 || sizeof(pixel_t) == 2) {
        dst = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1 + x));
        src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2 + x));
        msk = _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask + x));
      }
      else {
        p1_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1 + x));
        p2_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2 + x));
        mask_f = _mm_loadu_si128(reinterpret_cast<const __m128i*>(mask + x));
      }

      __m128i result;
      if constexpr(sizeof(pixel_t) == 1) {
        __m128i unpacked_mask_l = _mm_unpacklo_epi8(msk, zero);
        __m128i unpacked_mask_h = _mm_unpackhi_epi8(msk, zero);

        __m128i unpacked_p1_l = _mm_unpacklo_epi8(dst, zero);
        __m128i unpacked_p1_h = _mm_unpackhi_epi8(dst, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi8(src, zero);
        __m128i unpacked_p2_h = _mm_unpackhi_epi8(src, zero);

        unpacked_mask_l = overlay_merge_mask_sse2<pixel_t, bits_per_pixel>(unpacked_mask_l, opacity_mask);
        unpacked_mask_h = overlay_merge_mask_sse2<pixel_t, bits_per_pixel>(unpacked_mask_h, opacity_mask);

        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

        result = _mm_packus_epi16(result_l, result_h);

        // when mask is FF, keep src
        // when mask is 00, keep dst
        // unlike full opacity==1.0 blend, we have to watch zero mask only, opacity*mask is never max (0xFF)
        auto msk = _mm_packus_epi16(unpacked_mask_l, unpacked_mask_h); // we have mask here
        auto mask_00 = _mm_cmpeq_epi8(msk, zero);
        result = simd_blend_epi8<hasSSE4>(mask_00, dst, result); // ensure that zero mask value returns dst
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        __m128i unpacked_p1_l = _mm_unpacklo_epi16(dst, zero);
        __m128i unpacked_p1_h = _mm_unpackhi_epi16(dst, zero);

        __m128i unpacked_p2_l = _mm_unpacklo_epi16(src, zero);
        __m128i unpacked_p2_h = _mm_unpackhi_epi16(src, zero);

        __m128i unpacked_mask_l = _mm_unpacklo_epi16(msk, zero);
        __m128i unpacked_mask_h = _mm_unpackhi_epi16(msk, zero);

        // for uint16, this is SSE4
        // maybe _MM_MULLO_EPI32 and _MM_PACKUS_EPI32 could be used, but sometimes C is faster
        unpacked_mask_l = overlay_merge_mask_sse2<pixel_t, bits_per_pixel>(unpacked_mask_l, opacity_mask);
        unpacked_mask_h = overlay_merge_mask_sse2<pixel_t, bits_per_pixel>(unpacked_mask_h, opacity_mask);

        __m128i result_l = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
        __m128i result_h = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

        result = _mm_packus_epi32(result_l, result_h);

        __m128i max_pixel_value = _mm_set1_epi16(static_cast<uint16_t>((1 << bits_per_pixel) - 1));

        if constexpr(bits_per_pixel < 16) // otherwise no clamp needed
          result = _mm_min_epi16(result, max_pixel_value); // SSE2 epi16 is enough

        // unlike full opacity==1.0 blend, we have to watch zero mask only, opacity*mask is never max
        auto msk = _mm_packus_epi16(unpacked_mask_l, unpacked_mask_h); // we have mask here
        auto mask_zero = _mm_cmpeq_epi16(msk, zero);
        result = simd_blend_epi8<hasSSE4>(mask_zero, dst, result); // ensure that zero mask value returns dst
      }
      else {
        // sizeof(pixel_t) == 4, float
        mask_f = overlay_merge_mask_sse2<pixel_t, bits_per_pixel>(mask_f, opacity_mask);
        result = overlay_blend_sse2_core<pixel_t, bits_per_pixel>(p1_f, p2_f, mask_f, v128);
      }

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }

    // Leftover value
    for (int x = wMod16; x < width; x++) {
      if constexpr(sizeof(pixel_t) == 1) {
        int new_mask = overlay_merge_mask_c_8(mask[x], opacity);
        BYTE result = overlay_blend_c_core_8(p1[x], p2[x], static_cast<int>(new_mask));
        p1[x] = result;
      }
      else if constexpr(sizeof(pixel_t) == 2) {
        int new_mask = (reinterpret_cast<const uint16_t *>(mask)[x] * opacity) >> OPACITY_SHIFT; // int is enough, opacity is 8 bits
        uint16_t result = overlay_blend_c_core_16<bits_per_pixel>(reinterpret_cast<uint16_t *>(p1)[x], reinterpret_cast<const uint16_t *>(p2)[x], new_mask);
        reinterpret_cast<uint16_t *>(p1)[x] = result;
      }
      else { // float
        float new_mask = (reinterpret_cast<const float *>(mask)[x] * opacity_f);
        float p1x = reinterpret_cast<float *>(p1)[x];
        float p2x = reinterpret_cast<const float *>(p2)[x];

        float result = p1x + (p2x-p1x)*new_mask;
        reinterpret_cast<float *>(p1)[x] = result;
      }
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

// instantiate
// last false//true: hasSSE4
template void overlay_blend_sse2_plane_masked_opacity<uint8_t,8,false>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_masked_opacity<uint8_t,8,true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
// SSE4 only
template void overlay_blend_sse2_plane_masked_opacity<uint16_t,10,true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_masked_opacity<uint16_t,12,true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_masked_opacity<uint16_t,14,true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
template void overlay_blend_sse2_plane_masked_opacity<uint16_t,16,true>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);
// SSE2 only
template void overlay_blend_sse2_plane_masked_opacity<float,8,false>(BYTE *p1, const BYTE *p2, const BYTE *mask, const int p1_pitch, const int p2_pitch, const int mask_pitch, const int width, const int height, const int opacity, const float opacity_f);

/***************************************
 ********* Mode: Lighten/Darken ********
 ***************************************/

typedef __m128i (OverlaySseBlendOpaque)(const __m128i&, const __m128i&, const __m128i&);
typedef __m128i (OverlaySseCompare)(const __m128i&, const __m128i&, const __m128i&);
#ifdef X86_32
typedef   __m64 (OverlayMmxCompare)(const __m64&, const __m64&, const __m64&);
#endif

typedef int (OverlayCCompare)(BYTE, BYTE);

template<typename pixel_t, bool darken /* OverlayCCompare<pixel_t> compare*/>
__forceinline void overlay_darklighten_c(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height) {
  pixel_t* p1Y = reinterpret_cast<pixel_t *>(p1Y_8);
  pixel_t* p1U = reinterpret_cast<pixel_t *>(p1U_8);
  pixel_t* p1V = reinterpret_cast<pixel_t *>(p1V_8);

  const pixel_t* p2Y = reinterpret_cast<const pixel_t *>(p2Y_8);
  const pixel_t* p2U = reinterpret_cast<const pixel_t *>(p2U_8);
  const pixel_t* p2V = reinterpret_cast<const pixel_t *>(p2V_8);

  // pitches are already scaled
  //p1_pitch /= sizeof(pixel_t);
  //p2_pitch /= sizeof(pixel_t);

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int mask = darken ? (p2Y[x] <= p1Y[x]) : (p2Y[x] >= p1Y[x]); // compare(p1Y[x], p2Y[x]);
      p1Y[x] = overlay_blend_opaque_c_core<pixel_t>(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core<pixel_t>(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core<pixel_t>(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}

#ifdef X86_32
template<OverlayMmxCompare compare, OverlayCCompare compare_c>
__forceinline void overlay_darklighten_mmx(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m64 zero = _mm_setzero_si64();

  int wMod8 = (width/8) * 8;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x+=8) {
      // Load Y Plane
      __m64 p1_y = *(reinterpret_cast<const __m64*>(p1Y+x));
      __m64 p2_y = *(reinterpret_cast<const __m64*>(p2Y+x));

      // Compare
      __m64 cmp_result = compare(p1_y, p2_y, zero);

      // Process U Plane
      __m64 result_y = overlay_blend_opaque_mmx_core(p1_y, p2_y, cmp_result);
      *reinterpret_cast<__m64*>(p1Y+x) = result_y;

      // Process U plane
      __m64 p1_u = *(reinterpret_cast<const __m64*>(p1U+x));
      __m64 p2_u = *(reinterpret_cast<const __m64*>(p2U+x));
      
      __m64 result_u = overlay_blend_opaque_mmx_core(p1_u, p2_u, cmp_result);
      *reinterpret_cast<__m64*>(p1U+x) = result_u;

      // Process V plane
      __m64 p1_v = *(reinterpret_cast<const __m64*>(p1V+x));
      __m64 p2_v = *(reinterpret_cast<const __m64*>(p2V+x));
      
      __m64 result_v = overlay_blend_opaque_mmx_core(p1_v, p2_v, cmp_result);
      *reinterpret_cast<__m64*>(p1V+x) = result_v;
    }

    // Leftover value
    for (int x = wMod8; x < width; x++) {
      int mask = compare_c(p1Y[x], p2Y[x]);
      p1Y[x] = overlay_blend_opaque_c_core<uint8_t>(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core<uint8_t>(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core<uint8_t>(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }

  _mm_empty();
}
#endif

template <OverlaySseBlendOpaque blend, OverlaySseCompare compare, OverlayCCompare compare_c>
__forceinline void overlay_darklighten_sse(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Load Y Plane
      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1Y+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2Y+x));

      // Compare
      __m128i cmp_result = compare(p1_y, p2_y, zero);

      // Process U Plane
      __m128i result_y = blend(p1_y, p2_y, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1Y+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i result_u = blend(p1_u, p2_u, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i result_v = blend(p1_v, p2_v, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }

    // Leftover value
    for (int x = wMod16; x < width; x++) {
      int mask = compare_c(p1Y[x], p2Y[x]);
      p1Y[x] = overlay_blend_opaque_c_core<uint8_t>(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core<uint8_t>(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core<uint8_t>(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}

// Compare functions for lighten and darken mode
__forceinline int overlay_darken_c_cmp(BYTE p1, BYTE p2) {
  return p2 <= p1;
}

#ifdef X86_32
__forceinline __m64 overlay_darken_mmx_cmp(const __m64& p1, const __m64& p2, const __m64& zero) {
  __m64 diff = _mm_subs_pu8(p2, p1);
  return _mm_cmpeq_pi8(diff, zero);
}
#endif

__forceinline __m128i overlay_darken_sse_cmp(const __m128i& p1, const __m128i& p2, const __m128i& zero) {
  __m128i diff = _mm_subs_epu8(p2, p1);
  return _mm_cmpeq_epi8(diff, zero);
}

template<typename pixel_t>
__forceinline int overlay_lighten_c_cmp(pixel_t p1, pixel_t p2) {
  return p2 >= p1;
}

#ifdef X86_32
__forceinline __m64 overlay_lighten_mmx_cmp(const __m64& p1, const __m64& p2, const __m64& zero) {
  __m64 diff = _mm_subs_pu8(p1, p2);
  return _mm_cmpeq_pi8(diff, zero);
}
#endif

__forceinline __m128i overlay_lighten_sse_cmp(const __m128i& p1, const __m128i& p2, const __m128i& zero) {
  __m128i diff = _mm_subs_epu8(p1, p2);
  return _mm_cmpeq_epi8(diff, zero);
}

// Exported function
template<typename pixel_t>
void overlay_darken_c(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_c<pixel_t, true /*overlay_darken_c_cmp */>(p1Y_8, p1U_8, p1V_8, p2Y_8, p2U_8, p2V_8, p1_pitch, p2_pitch, width, height);
}
// instantiate
template void overlay_darken_c<uint8_t>(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height);
template void overlay_darken_c<uint16_t>(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height);

template<typename pixel_t>
void overlay_lighten_c(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_c<pixel_t, false /*overlay_lighten_c_cmp*/>(p1Y_8, p1U_8, p1V_8, p2Y_8, p2U_8, p2V_8, p1_pitch, p2_pitch, width, height);
}

// instantiate
template void overlay_lighten_c<uint8_t>(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height);
template void overlay_lighten_c<uint16_t>(BYTE *p1Y_8, BYTE *p1U_8, BYTE *p1V_8, const BYTE *p2Y_8, const BYTE *p2U_8, const BYTE *p2V_8, int p1_pitch, int p2_pitch, int width, int height);


#ifdef X86_32
void overlay_darken_mmx(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_mmx<overlay_darken_mmx_cmp, overlay_darken_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}
void overlay_lighten_mmx(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_mmx<overlay_lighten_mmx_cmp, overlay_lighten_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}
#endif

void overlay_darken_sse2(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_sse<overlay_blend_opaque_sse2_core, overlay_darken_sse_cmp, overlay_darken_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}
void overlay_lighten_sse2(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_sse<overlay_blend_opaque_sse2_core, overlay_lighten_sse_cmp, overlay_lighten_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}

void overlay_darken_sse41(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_sse<overlay_blend_opaque_sse41_core, overlay_darken_sse_cmp, overlay_darken_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}
void overlay_lighten_sse41(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  overlay_darklighten_sse<overlay_blend_opaque_sse41_core, overlay_lighten_sse_cmp, overlay_lighten_c_cmp>(p1Y, p1U, p1V, p2Y, p2U, p2V, p1_pitch, p2_pitch, width, height);
}

