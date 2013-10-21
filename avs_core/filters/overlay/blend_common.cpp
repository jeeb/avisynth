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

// Masked blend
// for blend mode
static BYTE OV_FORCEINLINE overlay_blend_c_core(const BYTE p1, const BYTE p2, const int mask) {
  return (BYTE)((((p1<<8) | 128) + (p2-p1)*mask) >> 8);
}

static __m64 OV_FORCEINLINE overlay_blend_mmx_core(const __m64& p1, const __m64& p2, const __m64& mask, const __m64& v128) {
  __m64 tmp1 = _mm_mullo_pi16(_mm_sub_pi16(p2, p1), mask); // (p2-p1)*mask
  __m64 tmp2 = _mm_or_si64(_mm_slli_pi16(p1, 8), v128);    // p1<<8 + 128 == p1<<8 | 128
  return _mm_srli_pi16(_mm_add_pi16(tmp1, tmp2), 8);
}

static __m128i OV_FORCEINLINE overlay_blend_sse2_core(const __m128i& p1, const __m128i& p2, const __m128i& mask, const __m128i& v128) {
  __m128i tmp1 = _mm_mullo_epi16(_mm_sub_epi16(p2, p1), mask); // (p2-p1)*mask
  __m128i tmp2 = _mm_or_si128(_mm_slli_epi16(p1, 8), v128);    // p1<<8 + 128 == p1<<8 | 128
  return _mm_srli_epi16(_mm_add_epi16(tmp1, tmp2), 8);
}

// Merge mask
// Use to combine opacity mask and clip mask
static BYTE OV_FORCEINLINE overley_merge_mask_c(const BYTE p1, const int p2) {
  return (p1*p2) >> 8;
}

static __m64 OV_FORCEINLINE overlay_merge_mask_mmx(const __m64& p1, const __m64& p2) {
  __m64 t1 = _mm_mullo_pi16(p1, p2);
  __m64 t2 = _mm_srli_pi16(t1, 8);
  return t2;
}

static __m128i OV_FORCEINLINE overlay_merge_mask_sse2(const __m128i& p1, const __m128i& p2) {
  __m128i t1 = _mm_mullo_epi16(p1, p2);
  __m128i t2 = _mm_srli_epi16(t1, 8);
  return t2;
}

// Blend Opaque
// Used in lighten and darken mode
static BYTE OV_FORCEINLINE overlay_blend_opaque_c_core(const BYTE p1, const BYTE p2, const BYTE mask) {
  return (mask) ? p2 : p1;
}

__m64 OV_FORCEINLINE overlay_blend_opaque_mmx_core(const __m64& p1, const __m64& p2, const __m64& mask) {
  __m64 r1 = _mm_andnot_si64(mask, p1);
  __m64 r2 = _mm_and_si64   (mask, p2);
  return _mm_or_si64(r1, r2);
}

static __m128i OV_FORCEINLINE overlay_blend_opaque_sse2_core(const __m128i& p1, const __m128i& p2, const __m128i& mask) {
  __m128i r1 = _mm_andnot_si128(mask, p1);
  __m128i r2 = _mm_and_si128   (mask, p2);
  return _mm_or_si128(r1, r2);
}

static __m128i OV_FORCEINLINE overlay_blend_opaque_sse41_core(const __m128i& p1, const __m128i& p2, const __m128i& mask) {
  return _mm_blendv_epi8(p1, p2, mask);
}

/////////////////////////////////////////////
// Mode: Overlay
/////////////////////////////////////////////

void overlay_blend_c_plane_masked(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                  const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                  const int width, const int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      BYTE result = overlay_blend_c_core(p1[x], p2[x], static_cast<int>(mask[x]));
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

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

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_masked(original_p1+wMod8, original_p2+wMod8, original_mask+wMod8, p1_pitch, p2_pitch, mask_pitch, width-wMod8, height);
}

void overlay_blend_sse2_plane_masked(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                     const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                     const int width, const int height) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;
  const BYTE* original_mask = mask;

  __m128i v128 = _mm_set1_epi16(0x0080);
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i p1_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x));
      __m128i p1_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x+8));

      __m128i p2_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x));
      __m128i p2_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x+8));

      __m128i mask_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(mask+x));
      __m128i mask_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(mask+x+8));

      __m128i unpacked_p1_l = _mm_unpacklo_epi8(p1_l, zero);
      __m128i unpacked_p1_h = _mm_unpacklo_epi8(p1_h, zero);

      __m128i unpacked_p2_l = _mm_unpacklo_epi8(p2_l, zero);
      __m128i unpacked_p2_h = _mm_unpacklo_epi8(p2_h, zero);

      __m128i unpacked_mask_l = _mm_unpacklo_epi8(mask_l, zero);
      __m128i unpacked_mask_h = _mm_unpacklo_epi8(mask_h, zero);

      __m128i result_l = overlay_blend_sse2_core(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
      __m128i result_h = overlay_blend_sse2_core(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

      __m128i result = _mm_packus_epi16(result_l, result_h);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_masked(original_p1+wMod16, original_p2+wMod16, original_mask+wMod16, p1_pitch, p2_pitch, mask_pitch, width-wMod16, height);
}

void overlay_blend_c_plane_opacity(BYTE *p1, const BYTE *p2,
                                   const int p1_pitch, const int p2_pitch,
                                   const int width, const int height, const int opacity) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      BYTE result = overlay_blend_c_core(p1[x], p2[x], opacity);
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }
}

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

    p1   += p1_pitch;
    p2   += p2_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_opacity(original_p1+wMod8, original_p2+wMod8, p1_pitch, p2_pitch, width-wMod8, height, opacity);
}

void overlay_blend_sse2_plane_opacity(BYTE *p1, const BYTE *p2,
                                      const int p1_pitch, const int p2_pitch,
                                      const int width, const int height, const int opacity) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;

  __m128i v128 = _mm_set1_epi16(0x0080);
  __m128i zero = _mm_setzero_si128();
  __m128i mask = _mm_set1_epi16(static_cast<short>(opacity));

  int wMod16 = (width/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i p1_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x));
      __m128i p1_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x+8));

      __m128i p2_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x));
      __m128i p2_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x+8));

      __m128i unpacked_p1_l = _mm_unpacklo_epi8(p1_l, zero);
      __m128i unpacked_p1_h = _mm_unpacklo_epi8(p1_h, zero);

      __m128i unpacked_p2_l = _mm_unpacklo_epi8(p2_l, zero);
      __m128i unpacked_p2_h = _mm_unpacklo_epi8(p2_h, zero);

      __m128i result_l = overlay_blend_sse2_core(unpacked_p1_l, unpacked_p2_l, mask, v128);
      __m128i result_h = overlay_blend_sse2_core(unpacked_p1_h, unpacked_p2_h, mask, v128);

      __m128i result = _mm_packus_epi16(result_l, result_h);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_opacity(original_p1+wMod16, original_p2+wMod16, p1_pitch, p2_pitch, width-wMod16, height, opacity);
}

void overlay_blend_c_plane_masked_opacity(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                  const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                  const int width, const int height, const int opacity) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int new_mask = overley_merge_mask_c(mask[x], opacity);
      BYTE result = overlay_blend_c_core(p1[x], p2[x], static_cast<int>(new_mask));
      p1[x] = result;
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }
}

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

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_masked_opacity(original_p1+wMod8, original_p2+wMod8, original_mask+wMod8, p1_pitch, p2_pitch, mask_pitch, width-wMod8, height, opacity);
}

void overlay_blend_sse2_plane_masked_opacity(BYTE *p1, const BYTE *p2, const BYTE *mask,
                                     const int p1_pitch, const int p2_pitch, const int mask_pitch,
                                     const int width, const int height, const int opacity) {
        BYTE* original_p1 = p1;
  const BYTE* original_p2 = p2;
  const BYTE* original_mask = mask;

  __m128i v128 = _mm_set1_epi16(0x0080);
  __m128i zero = _mm_setzero_si128();
  __m128i opacity_mask = _mm_set1_epi16(static_cast<short>(opacity));

  int wMod16 = (width/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x += 16) {
      __m128i p1_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x));
      __m128i p1_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p1+x+8));

      __m128i p2_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x));
      __m128i p2_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(p2+x+8));

      __m128i mask_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(mask+x));
      __m128i mask_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(mask+x+8));

      __m128i unpacked_p1_l = _mm_unpacklo_epi8(p1_l, zero);
      __m128i unpacked_p1_h = _mm_unpacklo_epi8(p1_h, zero);

      __m128i unpacked_p2_l = _mm_unpacklo_epi8(p2_l, zero);
      __m128i unpacked_p2_h = _mm_unpacklo_epi8(p2_h, zero);

      __m128i unpacked_mask_l = _mm_unpacklo_epi8(mask_l, zero);
      __m128i unpacked_mask_h = _mm_unpacklo_epi8(mask_h, zero);

      unpacked_mask_l = overlay_merge_mask_sse2(unpacked_mask_l, opacity_mask);
      unpacked_mask_h = overlay_merge_mask_sse2(unpacked_mask_h, opacity_mask);

      __m128i result_l = overlay_blend_sse2_core(unpacked_p1_l, unpacked_p2_l, unpacked_mask_l, v128);
      __m128i result_h = overlay_blend_sse2_core(unpacked_p1_h, unpacked_p2_h, unpacked_mask_h, v128);

      __m128i result = _mm_packus_epi16(result_l, result_h);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result);
    }

    p1   += p1_pitch;
    p2   += p2_pitch;
    mask += mask_pitch;
  }

  // Leftover value
  overlay_blend_c_plane_masked_opacity(original_p1+wMod16, original_p2+wMod16, original_mask+wMod16, p1_pitch, p2_pitch, mask_pitch, width-wMod16, height, opacity);
}

/////////////////////////////////////////////
// Mode: Darken
/////////////////////////////////////////////

void overlay_darken_c(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int mask = p2Y[x] <= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}

void overlay_darken_mmx(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m64 zero = _mm_setzero_si64();

  int wMod8 = (width/8) * 8;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x+=8) {
      // Load Y Plane
      __m64 p1_y = *(reinterpret_cast<const __m64*>(p1Y+x));
      __m64 p2_y = *(reinterpret_cast<const __m64*>(p2Y+x));

      // Compare
      // (p2 <= p1) (because old mmx version does it like this)
      __m64 diff = _mm_subs_pu8(p2_y, p1_y);
      __m64 cmp_result = _mm_cmpeq_pi8(diff, zero);

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
      int mask = p2Y[x] <= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
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
#ifdef X86_32
void overlay_darken_sse2(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Load Y Plane
      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1Y+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2Y+x));

      // Compare
      // (p2 <= p1) (because old mmx version does it like this)
      __m128i diff = _mm_subs_epu8(p2_y, p1_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);

      // Process U Plane
      __m128i result_y = overlay_blend_opaque_sse2_core(p1_y, p2_y, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1Y+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i result_u = overlay_blend_opaque_sse2_core(p1_u, p2_u, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i result_v = overlay_blend_opaque_sse2_core(p1_v, p2_v, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }

    // Leftover value
    for (int x = wMod16; x < width; x++) {
      int mask = p2Y[x] <= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}
#endif
void overlay_darken_sse41(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Load Y Plane
      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1Y+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2Y+x));

      // Compare
      // (p2 <= p1) (because old mmx version does it like this)
      __m128i diff = _mm_subs_epu8(p2_y, p1_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);

      // Process U Plane
      __m128i result_y = overlay_blend_opaque_sse41_core(p1_y, p2_y, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1Y+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i result_u = overlay_blend_opaque_sse41_core(p1_u, p2_u, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i result_v = overlay_blend_opaque_sse41_core(p1_v, p2_v, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }

    // Leftover value
    for (int x = wMod16; x < width; x++) {
      int mask = p2Y[x] <= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}

/////////////////////////////////////////////
// Mode: Lighten (mostly copy-pasta from darken)
/////////////////////////////////////////////

void overlay_lighten_c(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      int mask = p2Y[x] >= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
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
void overlay_lighten_mmx(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m64 zero = _mm_setzero_si64();

  int wMod8 = (width/8) * 8;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod8; x+=8) {
      // Load Y Plane
      __m64 p1_y = *(reinterpret_cast<const __m64*>(p1Y+x));
      __m64 p2_y = *(reinterpret_cast<const __m64*>(p2Y+x));

      // Compare
      // (p2 >= p1) (because old mmx version does it like this)
      __m64 diff = _mm_subs_pu8(p1_y, p2_y);
      __m64 cmp_result = _mm_cmpeq_pi8(diff, zero);

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
      int mask = p2Y[x] >= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
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
void overlay_lighten_sse2(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Load Y Plane
      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1Y+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2Y+x));

      // Compare
      // (p2 >= p1) (because old mmx version does it like this)
      __m128i diff = _mm_subs_epu8(p1_y, p2_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);

      // Process U Plane
      __m128i result_y = overlay_blend_opaque_sse2_core(p1_y, p2_y, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1Y+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i result_u = overlay_blend_opaque_sse2_core(p1_u, p2_u, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i result_v = overlay_blend_opaque_sse2_core(p1_v, p2_v, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }

    // Leftover value
    for (int x = wMod16; x < width; x++) {
      int mask = p2Y[x] >= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}

void overlay_lighten_sse41(BYTE *p1Y, BYTE *p1U, BYTE *p1V, const BYTE *p2Y, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch, int width, int height) {
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (width/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Load Y Plane
      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1Y+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2Y+x));

      // Compare
      // (p2 >= p1) (because old mmx version does it like this)
      __m128i diff = _mm_subs_epu8(p1_y, p2_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);

      // Process U Plane
      __m128i result_y = overlay_blend_opaque_sse41_core(p1_y, p2_y, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1Y+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i result_u = overlay_blend_opaque_sse41_core(p1_u, p2_u, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i result_v = overlay_blend_opaque_sse41_core(p1_v, p2_v, cmp_result);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }

    // Leftover value
    for (int x = wMod16; x >= width; x++) {
      int mask = p2Y[x] >= p1Y[x];
      p1Y[x] = overlay_blend_opaque_c_core(p1Y[x], p2Y[x], mask);
      p1U[x] = overlay_blend_opaque_c_core(p1U[x], p2U[x], mask);
      p1V[x] = overlay_blend_opaque_c_core(p1V[x], p2V[x], mask);
    }

    p1Y += p1_pitch;
    p1U += p1_pitch;
    p1V += p1_pitch;

    p2Y += p2_pitch;
    p2U += p2_pitch;
    p2V += p2_pitch;
  }
}


/////////////////////////////////////////////////////////////////////////////////////////////
void MMerge_SSE(BYTE *dstp, const BYTE *srcp,
        const BYTE *maskp, const int dst_pitch, const int src_pitch,
        const int mask_pitch, const int row_size, const int height) {

  __m128i v128 = _mm_set1_epi16(0x0080);
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (row_size/16) * 16;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      //dstp[x] = (BYTE)(((dstp[x]*(256-maskp[x])) + (srcp[x]*maskp[x]+128))>>8);
      // simplify to (dst<<8 + (src-dst)*mask + 128) >> 8 (as per original MMX code)

      __m128i dst_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x));
      __m128i dst_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x+8));

      __m128i src_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x));
      __m128i src_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x+8));

      __m128i mask_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(maskp+x));
      __m128i mask_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(maskp+x+8));

      __m128i unpacked_dst_l = _mm_unpacklo_epi8(dst_l, zero);
      __m128i unpacked_dst_h = _mm_unpacklo_epi8(dst_h, zero);

      __m128i unpacked_src_l = _mm_unpacklo_epi8(src_l, zero);
      __m128i unpacked_src_h = _mm_unpacklo_epi8(src_h, zero);

      __m128i unpacked_mask_l = _mm_unpacklo_epi8(mask_l, zero);
      __m128i unpacked_mask_h = _mm_unpacklo_epi8(mask_h, zero);

      __m128i tmp1_l =  _mm_mullo_epi16(_mm_sub_epi16(unpacked_src_l, unpacked_dst_l), unpacked_mask_l); // (src-dst)*mask
      __m128i tmp1_h =  _mm_mullo_epi16(_mm_sub_epi16(unpacked_src_h, unpacked_dst_h), unpacked_mask_h);

      __m128i tmp2_l = _mm_or_si128(_mm_slli_epi16(unpacked_dst_l, 8), v128); // dst<<8 + 128 == dst<<8 | 128
      __m128i tmp2_h = _mm_or_si128(_mm_slli_epi16(unpacked_dst_h, 8), v128);

      __m128i result_l = _mm_srli_epi16(_mm_add_epi16(tmp1_l, tmp2_l), 8); 
      __m128i result_h = _mm_srli_epi16(_mm_add_epi16(tmp1_h, tmp2_h), 8); 

      __m128i result = _mm_packus_epi16(result_l, result_h);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x), result);
    }
    
    for (int x = wMod16; x < row_size; x++) {
      //dstp[x] = (BYTE)(((dstp[x]*(256-maskp[x])) + (srcp[x]*maskp[x]+128))>>8);
      dstp[x] = (BYTE)(((dstp[x]<<8) + (srcp[x]-dstp[x])*maskp[x] + 128) >> 8);
    }

    dstp  += dst_pitch;
    srcp  += src_pitch;
    maskp += mask_pitch;
  }
}

void sse_weigh_planar(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch,int row_size, int height, int weight) {
  __m128i v128 = _mm_set1_epi16(0x0080);
  __m128i v_mask = _mm_set1_epi16(static_cast<short>(weight));
  __m128i zero = _mm_setzero_si128();

  int wMod16 = (row_size/16) * 16;
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      //dstp[x] = (BYTE)(((dstp[x]*(256-maskp[x])) + (srcp[x]*maskp[x]+128))>>8);
      // (dst*(256-mask) + src*mask + 128) / 256
      // simplify to (dst<<8 + (src-dst)*mask + 128) >> 8 (as per original MMX code)

      __m128i dst_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x));
      __m128i dst_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x+8));

      __m128i src_l = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x));
      __m128i src_h = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x+8));

      __m128i unpacked_dst_l = _mm_unpacklo_epi8(dst_l, zero);
      __m128i unpacked_dst_h = _mm_unpacklo_epi8(dst_h, zero);

      __m128i unpacked_src_l = _mm_unpacklo_epi8(src_l, zero);
      __m128i unpacked_src_h = _mm_unpacklo_epi8(src_h, zero);

      __m128i tmp1_l =  _mm_mullo_epi16(_mm_sub_epi16(unpacked_src_l, unpacked_dst_l), v_mask); // (src-dst)*mask
      __m128i tmp1_h =  _mm_mullo_epi16(_mm_sub_epi16(unpacked_src_h, unpacked_dst_h), v_mask);

      __m128i tmp2_l = _mm_or_si128(_mm_slli_epi16(unpacked_dst_l, 8), v128); // dst<<8 + 128 == dst<<8 | 128
      __m128i tmp2_h = _mm_or_si128(_mm_slli_epi16(unpacked_dst_h, 8), v128);

      __m128i result_l = _mm_srli_epi16(_mm_add_epi16(tmp1_l, tmp2_l), 8); 
      __m128i result_h = _mm_srli_epi16(_mm_add_epi16(tmp1_h, tmp2_h), 8); 

      __m128i result = _mm_packus_epi16(result_l, result_h);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x), result);
    }
    
    for (int x = wMod16; x < row_size; x++) {
      //dstp[x] = (BYTE)(((dstp[x]*(256-maskp[x])) + (srcp[x]*maskp[x]+128))>>8);
      dstp[x] = (BYTE)(((dstp[x]<<8) + (srcp[x]-dstp[x])*weight + 128) >> 8);
    }

    dstp  += dst_pitch;
    srcp  += src_pitch;
  }
}

void sse_darken_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int row_size, int height) {
  int wMod16 = (row_size/16) * 16;
  __m128i zero = _mm_setzero_si128();
  __m128i all1 = _mm_set1_epi8(0xFF);
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Replace pixel that has overlay (p2) darker than clip (p1)
      // darker == p2.y < p1.y

      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2+x));

      // (p2 <= p1) (because mmx version do it like this)
      __m128i diff = _mm_subs_epu8(p2_y, p1_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);
      
      __m128i p1y_base = _mm_andnot_si128(cmp_result, p1_y);
      __m128i p2y_base = _mm_and_si128   (cmp_result, p2_y);

      __m128i result_y = _mm_or_si128(p1y_base, p2y_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i p1u_base = _mm_andnot_si128(cmp_result, p1_u);
      __m128i p2u_base = _mm_and_si128   (cmp_result, p2_u);

      __m128i result_u = _mm_or_si128(p1u_base, p2u_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i p1v_base = _mm_andnot_si128(cmp_result, p1_v);
      __m128i p2v_base = _mm_and_si128   (cmp_result, p2_v);

      __m128i result_v = _mm_or_si128(p1v_base, p2v_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }
    
    for (int x = wMod16; x < row_size; x++) {
      if (p2[x] <= p1[x])  { 
          p1[x]  = p2[x];
          p1U[x] = p2U[x];
          p1V[x] = p2V[x];
        }
    }

    p1 += p1_pitch;
    p2 += p2_pitch;

    p1U += p1_pitch;
    p2U += p2_pitch;

    p1V += p1_pitch;
    p2V += p2_pitch;
  }
}

void sse_lighten_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int row_size, int height) {
  int wMod16 = (row_size/16) * 16;
  __m128i zero = _mm_setzero_si128();
  __m128i all1 = _mm_set1_epi8(0xFF);
  
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < wMod16; x+=16) {
      // Replace pixel that has overlay (p2) lighter than clip (p1)
      // lighter == p2.y > p1.y

      __m128i p1_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1+x));
      __m128i p2_y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2+x));

      // (p2 >= p1) (because mmx version do it like this)
      __m128i diff = _mm_subs_epu8(p1_y, p2_y);
      __m128i cmp_result = _mm_cmpeq_epi8(diff, zero);
      
      __m128i p1y_base = _mm_andnot_si128(cmp_result, p1_y);
      __m128i p2y_base = _mm_and_si128   (cmp_result, p2_y);

      __m128i result_y = _mm_or_si128(p1y_base, p2y_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1+x), result_y);

      // Process U plane
      __m128i p1_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1U+x));
      __m128i p2_u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2U+x));
      
      __m128i p1u_base = _mm_andnot_si128(cmp_result, p1_u);
      __m128i p2u_base = _mm_and_si128   (cmp_result, p2_u);

      __m128i result_u = _mm_or_si128(p1u_base, p2u_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1U+x), result_u);

      // Process V plane
      __m128i p1_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p1V+x));
      __m128i p2_v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(p2V+x));
      
      __m128i p1v_base = _mm_andnot_si128(cmp_result, p1_v);
      __m128i p2v_base = _mm_and_si128   (cmp_result, p2_v);

      __m128i result_v = _mm_or_si128(p1v_base, p2v_base);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(p1V+x), result_v);
    }
    
    for (int x = wMod16; x < row_size; x++) {
      if (p2[x] >= p1[x])  { 
          p1[x]  = p2[x];
          p1U[x] = p2U[x];
          p1V[x] = p2V[x];
        }
    }

    p1 += p1_pitch;
    p2 += p2_pitch;

    p1U += p1_pitch;
    p2U += p2_pitch;

    p1V += p1_pitch;
    p2V += p2_pitch;
  }
}

#ifdef X86_32


/*******************
 * Blends two planes.
 * A weight between the two planes are given.
 * Has rather ok pairing, 
 * and has very little memory usage.
 * Processes four pixels per loop, so rowsize must be mod 4.
 * Thanks to ARDA for squeezing out a bit more performance.
 * 
 * Weights must be multipled by 32767
 * Returns the blended plane in p1;
 * (c) 2002 by sh0dan.
 ********/

void mmx_weigh_planar(BYTE *p1, const BYTE *p2, int p1_pitch, int p2_pitch,int rowsize, int height, int weight, int invweight) {
  __int64 weight64  = (__int64)weight | (((__int64)invweight)<<16) | (((__int64)weight)<<32) |(((__int64)invweight)<<48);
  __int64 rounder = 0x0000400000004000;   // (0.5)<<15 in each dword

  __asm {
	  push ebx
      movq mm5,[rounder]
      pxor mm6,mm6
      movq mm7,[weight64]
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      mov edx,[height]
      align 16
yloopback:
      cmp ecx, edx
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      punpcklbw mm0,[esi+eax]  // 4 pixels
       pxor mm3,mm3
      punpcklbw mm1,[edi+eax]  // y300 y200 y100 y000
       psrlw mm0,8              // 00y3 00y2 00y1 00y0
      psrlw mm1,8              // 00y3 00y2 00y1 00y0  
       pxor mm2,mm2
      movq mm4,mm1
       punpcklwd mm2,mm0
      punpckhwd mm3,mm0  
       punpcklwd mm4,mm6
      punpckhwd mm1,mm6
       por mm2,mm4
      por mm3,mm1
       pmaddwd mm2,mm7     // Multiply pixels by weights.  pixel = img1*weight + img2*invweight (twice)
      pmaddwd mm3,mm7      // Stalls 1 cycle (multiply unit stall)
       paddd mm2,mm5       // Add rounder
      paddd mm3,mm5
       psrld mm2,15        // Shift down, so there is no fraction.
      psrld mm3,15        
      packssdw mm2,mm3
      packuswb mm2,mm6 
      movd [esi+eax],mm2
      add eax,4
      jmp testloop
      align 16
outloop:
      inc ecx
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}

// From MaskedMerge - Weighed merge of video planes depending on masks
// MaskedMerge (C) 2003 Kurosu (kurosu@inforezo.org)

void MMerge_MMX(BYTE *dstp, const BYTE *srcp,
        const BYTE *maskp, const int dst_pitch, const int src_pitch,
        const int mask_pitch, const int row_size, const int height)
{
  static const __int64 Addi = 0x0080008000800080i64;

  __asm {
	push    ebx
    mov     eax,[dstp]
    mov     ebx,[srcp]
    mov     ecx,[maskp]
    mov     edx,[row_size]
    mov     edi,0
    mov     esi,[height]
    movq    mm7,[Addi]   // sh0dan: Stored in register to avoid having to read from cache.
    pxor    mm6, mm6
    align 16
loopx:
    movq    mm0, [eax+edi]
     movq    mm1, [ebx+edi]
    movq    mm2, [ecx+edi]
     movq    mm3, mm0
    movq    mm4, mm1
     movq    mm5, mm2  // sh0dan: No longer loading & unpacking the same data.

    punpcklbw mm0, mm6
     punpcklbw   mm1, mm6
    punpcklbw mm2, mm6
     punpckhbw   mm3, mm6
    punpckhbw mm4, mm6
     punpckhbw   mm5, mm6

    psubw   mm1, mm0
     psubw     mm4, mm3

    pmullw    mm1, mm2
     pmullw    mm4, mm5

    psllw   mm0, 8
     psllw     mm3, 8

    pxor    mm0, mm7
     pxor    mm3, mm7

    paddw   mm0, mm1
     paddw     mm3, mm4

    psrlw   mm0, 8
     psrlw     mm3, 8

    packuswb  mm0, mm3

    movq    [eax+edi],mm0

    add     edi,8
    cmp     edi,edx
    jl      loopx

    mov     edi,0
    add     eax,[dst_pitch]
    add     ebx,[src_pitch]
    add     ecx,[mask_pitch]
    dec     esi

    jnz     loopx

    emms
	pop     ebx
  }
}


void mmx_darken_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int rowsize, int height) {
  static const __int64 AllOnes = 0xffffffffffffffff;
  const BYTE* bpp[6];
  bpp[0] = p1; bpp[1] = p2; bpp[2] = p1U; bpp[3] = p2U; bpp[4] = p1V; bpp[5] = p2V;

  __asm {
	  push ebx
      movq mm7,[AllOnes]
      pxor mm6,mm6
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      lea edx, bpp
      align 16
yloopback:
      cmp ecx, [height]
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      mov esi,[edx+0]   // Y
      mov edi,[edx+4]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      movq mm2, mm1
      psubusb mm2,mm0
      pcmpeqb mm2,mm6
      
      movq mm3,mm2   
      pxor mm2,mm7   // Inverted
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      mov esi,[edx+8]  // U
      mov edi,[edx+12]
      movq mm4,[eax+esi]
      movq mm5,[eax+edi]
      pand mm4,mm2
      pand mm5,mm3
      por mm4,mm5
      movq [eax+esi], mm4

      mov esi,[edx+16]  // V
      mov edi,[edx+20]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      add eax,8
      jmp testloop
      align 16
outloop:
      inc ecx
      mov esi,[edx+0]
      mov edi,[edx+4]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+0], esi
      mov [edx+4], edi

      mov esi,[edx+8]
      mov edi,[edx+12]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+8], esi
      mov [edx+12], edi

      mov esi,[edx+16]
      mov edi,[edx+20]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+16], esi
      mov [edx+20], edi

      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}

void mmx_lighten_planar(BYTE *p1, BYTE *p1U, BYTE *p1V, const BYTE *p2, const BYTE *p2U, const BYTE *p2V, int p1_pitch, int p2_pitch,int rowsize, int height) {
  static const __int64 AllOnes = 0xffffffffffffffff;
  const BYTE* bpp[6];
  bpp[0] = p1; bpp[1] = p2; bpp[2] = p1U; bpp[3] = p2U; bpp[4] = p1V; bpp[5] = p2V;

  __asm {
	  push ebx
      movq mm7,[AllOnes]
      pxor mm6,mm6
      mov ebx,[rowsize]
      mov esi,[p1]
      mov edi,[p2]
      xor ecx, ecx  // Height
      lea edx, bpp
      align 16
yloopback:
      cmp ecx, [height]
      jge outy
      xor eax, eax
      align 16 
testloop:
      cmp ebx, eax
      jle outloop
      mov esi,[edx+0]   // Y
      mov edi,[edx+4]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      movq mm2, mm0
      psubusb mm2,mm1
      pcmpeqb mm2,mm6
      
      movq mm3,mm2   
      pxor mm2,mm7   // Inverted
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      mov esi,[edx+8]  // U
      mov edi,[edx+12]
      movq mm4,[eax+esi]
      movq mm5,[eax+edi]
      pand mm4,mm2
      pand mm5,mm3
      por mm4,mm5
      movq [eax+esi], mm4

      mov esi,[edx+16]  // V
      mov edi,[edx+20]
      movq mm0,[eax+esi]
      movq mm1,[eax+edi]
      pand mm0,mm2
      pand mm1,mm3
      por mm0,mm1
      movq [eax+esi], mm0

      add eax,8
      jmp testloop
      align 16
outloop:
      inc ecx
      mov esi,[edx+0]
      mov edi,[edx+4]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+0], esi
      mov [edx+4], edi

      mov esi,[edx+8]
      mov edi,[edx+12]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+8], esi
      mov [edx+12], edi

      mov esi,[edx+16]
      mov edi,[edx+20]
      add esi, [p1_pitch];
      add edi, [p2_pitch];
      mov [edx+16], esi
      mov [edx+20], edi

      jmp yloopback
outy:
      emms
	  pop ebx
  } // end asm
}


#endif //X86_32