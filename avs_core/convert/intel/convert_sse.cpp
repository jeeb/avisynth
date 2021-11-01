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


#include "../convert.h"
#include "../convert_matrix.h"
#include "../convert_bits.h"
#include "../convert_planar.h"
#include "../convert_rgb.h"
#include "convert_yv12_sse.h"
#include "../convert_yuy2.h"
#include <avs/alignment.h>
#include <avs/minmax.h>
#include <avs/config.h>
#include <emmintrin.h>
#include <smmintrin.h> // SSE4.1
#include <tuple>
#include <map>
#include <algorithm>

#ifdef AVS_WINDOWS
#include <avs/win.h>
#else
#include <avs/posix.h>
#endif

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Convert_filters[] = {       // matrix can be "rec601", "rec709", "PC.601" or "PC.709" or "rec2020" or "PC.2020"
  { "ConvertToRGB",   BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)0 },
  { "ConvertToRGB24", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)24 },
  { "ConvertToRGB32", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)32 },
  { "ConvertToRGB48", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)48 },
  { "ConvertToRGB64", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)64 },
  { "ConvertToPlanarRGB",  BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-1 },
  { "ConvertToPlanarRGBA", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-2 },
  { "ConvertToY8",    BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY::Create, (void*)0 }, // user_data == 0 -> only 8 bit sources
  { "ConvertToYV12",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToYV12::Create, (void*)0 },
  { "ConvertToYV24",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444, (void*)0},
  { "ConvertToYV16",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToPlanarGeneric::CreateYUV422, (void*)0},
  { "ConvertToYV411", BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411, (void*)0},
  { "ConvertToYUY2",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToYUY2::Create },
  { "ConvertBackToYUY2", BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertBackToYUY2::Create },
  { "ConvertToY",       BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY::Create, (void*)1 }, // user_data == 1 -> any bit depth sources
  { "ConvertToYUV411", BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411, (void*)1}, // alias for ConvertToYV411, 8 bit check later
  { "ConvertToYUV420",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToPlanarGeneric::CreateYUV420, (void*)1},
  { "ConvertToYUV422",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToPlanarGeneric::CreateYUV422, (void*)1},
  { "ConvertToYUV444",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444, (void*)1},
  { "ConvertTo8bit",  BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)8 },
  { "ConvertTo16bit", BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)16 },
  { "ConvertToFloat", BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)32 },
  { "ConvertBits",    BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)0 },
  { "AddAlphaPlane",  BUILTIN_FUNC_PREFIX, "c[mask].", AddAlphaPlane::Create},
  { "RemoveAlphaPlane",  BUILTIN_FUNC_PREFIX, "c", RemoveAlphaPlane::Create},
  { 0 }
};


/****************************************
*******   Convert to RGB / RGBA   ******
***************************************/

// YUY2 only
ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix_name,
                           IScriptEnvironment* env )
                           : GenericVideoFilter(_child)
{
  theMatrix = Rec601;

  if (matrix_name) {
    theMatrix = getMatrix(matrix_name, env);
  }
  const int shift = 16; // for integer arithmetic; YUY2 is using 16 bits, later is divided back by 4 or 8
  const int bits_per_pixel = 8; // YUY2
  if (!do_BuildMatrix_Yuv2Rgb(theMatrix, shift, bits_per_pixel, /*ref*/matrix))
    env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter");

  // these constants are used with intentional minus operator in core calculations
  matrix.v_g = -matrix.v_g;
  matrix.u_g = -matrix.u_g;
  matrix.offset_y = -matrix.offset_y;

  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
}

#if defined(__SSE2__)
template<int rgb_size>
static AVS_FORCEINLINE __m128i convert_yuy2_to_rgb_sse2_core(const __m128i& src_luma_scaled, const __m128i& src_chroma, const __m128i &alpha,
                                                           const __m128i& v128, const __m128i& zero, const __m128i& rounder, const __m128i &ff,
                                                           const __m128i& ymul, const __m128i& bmul, const __m128i& gmul, const __m128i& rmul) {
  __m128i chroma_scaled = _mm_sub_epi16(src_chroma, v128);  //V2-128 | U2-128 | V1-128 | U1-128 | V1-128 | U1-128 | V0-128 | U0-128

  __m128i luma_scaled = _mm_madd_epi16(src_luma_scaled, ymul); // (y1-16)*cy | (y0-16)*cy
  luma_scaled = _mm_add_epi32(luma_scaled, rounder); // (y1-16)*cy + 8192 | (y0-16)*cy + 8192

  __m128i chroma_scaled2 = _mm_shuffle_epi32(chroma_scaled, _MM_SHUFFLE(2, 2, 0, 0)); //V1-128 | U1-128 | V1-128 | U1-128 | V0-128 | U0-128 | V0-128 | U0-128

  chroma_scaled = _mm_add_epi16(chroma_scaled, chroma_scaled2); // V0+V1-256 | U0+U1-256 | (V0-128)*2 | (U0-128)*2

  __m128i b = _mm_madd_epi16(chroma_scaled, bmul); //               0 + (U0+U1-256)*cbu |              0 + (U0-128)*2*cbu
  __m128i g = _mm_madd_epi16(chroma_scaled, gmul); // (V0+V1-256)*cgv + (U0+U1-256)*cgu | (V0-128)*2*cgv + (U0-128)*2*cgu
  __m128i r = _mm_madd_epi16(chroma_scaled, rmul); // (V0+V1-256)*crv + 0               | (V0-128)*2*crv + 0

  b = _mm_add_epi32(luma_scaled, b);
  g = _mm_add_epi32(luma_scaled, g);
  r = _mm_add_epi32(luma_scaled, r);

  b = _mm_srai_epi32(b, 14); //b3 b3 b3 b3 | b2 b2 b2 b2  | b1 b1 b1 b1 | b0 b0 b0 b0
  g = _mm_srai_epi32(g, 14); //g3 g3 g3 g3 | g2 g2 g2 g2  | g1 g1 g1 g1 | g0 g0 g0 g0
  r = _mm_srai_epi32(r, 14); //g3 g3 g3 g3 | g2 g2 g2 g2  | g1 g1 g1 g1 | g0 g0 g0 g0


  if constexpr(rgb_size == 4)
  {
    b = _mm_max_epi16(b, zero);
    g = _mm_max_epi16(g, zero);
    r = _mm_max_epi16(r, zero);

    b = _mm_min_epi16(b, ff); //00 00 00 b3 | 00 00 00 b2 | 00 00 00 b1 | 00 00 00 b0
    g = _mm_min_epi16(g, ff); //00 00 00 g3 | 00 00 00 g2 | 00 00 00 g1 | 00 00 00 g0
    r = _mm_min_epi16(r, ff); //00 00 00 r3 | 00 00 00 r2 | 00 00 00 r1 | 00 00 00 r0

    r = _mm_slli_epi32(r, 16);
    g = _mm_slli_epi32(g, 8);

    __m128i rb = _mm_or_si128(r, b);
    __m128i rgb = _mm_or_si128(rb, g);
    return _mm_or_si128(rgb, alpha);
  }
  else
  {
    __m128i bg = _mm_packs_epi32(b, g); //g3g3 | g2g2 | g1g1 | g0g0 | b3b3 | b2b2 | b1b1 | b0b0
    r = _mm_packs_epi32(r, zero); //0000 | 0000 | 0000 | 0000 | r3r3 | r2r2 | r1r1 | r0r0

    __m128i br = _mm_unpacklo_epi16(bg, r); //r3r3 | b3b3 | r2r2 | b2b2 | r1r1 | b1b1 | r0r0 | b0b0
    g = _mm_unpackhi_epi16(bg, zero); //0000 | g3g3 | 0000 | g2g2 | 0000 | g1g1 | 0000 | g0g0

    __m128i rgb_lo = _mm_unpacklo_epi16(br, g); //0000 | r1r1 | g1g1 | b1b1 | 0000 | r0r0 | g0g0 | b0b0
    rgb_lo = _mm_shufflelo_epi16(rgb_lo, _MM_SHUFFLE(2, 1, 0, 3)); //0000 | r1r1 | g1g1 | b1b1 | r0r0 | g0g0 | b0b0 | 0000
    __m128i rgb_hi = _mm_unpackhi_epi16(br, g); //0000 | r3r3 | g3g3 | b3b3 | 0000 | r2r2 | g2g2 | b2b2
    rgb_hi = _mm_shufflelo_epi16(rgb_hi, _MM_SHUFFLE(2, 1, 0, 3)); //0000 | r3r3 | g3g3 | b3b3 | r2r2 | g2g2 | b2b2 | 0000

    return _mm_packus_epi16(rgb_lo, rgb_hi); //00 | r3 | g3 | b3 | r2 | g2 | b2 | 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0 | 00
  }
}

template<int rgb_size>
static void convert_yuy2_to_rgb_sse2(const BYTE *srcp, BYTE* dstp, int src_pitch, int dst_pitch, int height, int width, int crv, int cgv, int cgu, int cbu, int cy, int tv_scale) {
  srcp += height * src_pitch;
  int mod4_width = width / 4 * 4;

  __m128i tv_scale_vector = _mm_set1_epi16(tv_scale);
  __m128i zero = _mm_setzero_si128();
  __m128i ymul = _mm_set1_epi16(cy / 4);

  __m128i bmul = _mm_set_epi16(0, cbu/8, 0, cbu/8, 0, cbu/8, 0, cbu/8);
  __m128i gmul = _mm_set_epi16(-cgv/8, -cgu/8, -cgv/8, -cgu/8, -cgv/8, -cgu/8, -cgv/8, -cgu/8);
  __m128i rmul = _mm_set_epi16(crv/8, 0, crv/8, 0, crv/8, 0, crv/8, 0);
  __m128i alpha = _mm_set1_epi32(0xFF000000);
  __m128i ff = _mm_set1_epi16(0x00FF);
  __m128i v128 =  _mm_set1_epi16(128);
  __m128i rounder = _mm_set1_epi32(1 << 13);

  for (int y = 0; y < height; ++y) {
    srcp -= src_pitch;
    for (int x = 0; x < mod4_width-2; x+= 4) {
      __m128i src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+x*2)); //xx xx xx xx V2 xx U2 xx V1 Y3 U1 Y2 V0 Y1 U0 Y0

      __m128i src_luma = _mm_and_si128(src, ff);//0 xx 0 xx 0 xx 0 xx 0 Y3 0 Y2 0 Y1 0 Y0
      __m128i src_chroma = _mm_srli_epi16(src, 8); //00 xx 00 xx 00 V2 00 U2 00 V1 00 U1 00 V0 00 U0

      src_chroma = _mm_shuffle_epi32(src_chroma, _MM_SHUFFLE(2, 1, 1, 0)); //00 V2 00 U2 00 V1 00 U1 00 V1 00 U1 00 V0 00 U0

      __m128i luma_scaled   = _mm_sub_epi16(src_luma, tv_scale_vector); //Y3-16 | Y2-16 | Y1-16 | Y0-16
      luma_scaled = _mm_unpacklo_epi16(luma_scaled, zero); // 0000 | Y3-16 | 0000 | Y2-16 | 0000 | Y1-16 | 0000 | Y0-16

      __m128i rgb = convert_yuy2_to_rgb_sse2_core<rgb_size>(luma_scaled, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if constexpr(rgb_size == 4) {
        _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*4), rgb);
      } else {
        //input: 00 | r3 | g3 | b3 | r2 | g2 | b2 | 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0 | 00

        rgb = _mm_srli_si128(rgb, 1); //00 | 00 | r3 | g3 | b3 | r2 | g2 | b2 || 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0
        *reinterpret_cast<int*>(dstp+x*3) = _mm_cvtsi128_si32(rgb);
        rgb = _mm_shufflelo_epi16(rgb, _MM_SHUFFLE(2, 3, 3, 3)); //00 | 00 | r3 | g3 | b3 | r2 | g2 | b2 || r1 | g1 | 00 | 00 | 00 | 00 | 00 | 00
        rgb = _mm_srli_si128(rgb, 6); //00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 || r3 | g3 | b3 | r2 | g2 | b2 | r1 | g1
        _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*3+4), rgb);
      }
    }

    if (mod4_width == width) {
      //two pixels left to process
      __m128i src = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(srcp+width*2 - 4)); //00 00 00 00 V0 Y1 U0 Y0
      __m128i src_luma   = _mm_and_si128(src, ff);//0 0 0 0 0 Y1 0 Y0
      __m128i src_chroma = _mm_srli_epi16(src, 8); //00 00 00 00 00 V0 00 U0
      src_chroma = _mm_shufflelo_epi16(src_chroma, _MM_SHUFFLE(1, 0, 1, 0)); //00 V0 00 U0 00 V0 00 U0

      __m128i luma_scaled   = _mm_sub_epi16(src_luma, tv_scale_vector); //0 | 0 | Y1-16 | Y0-16
      luma_scaled = _mm_unpacklo_epi16(luma_scaled, zero); // 0000 | Y1-16 | 0000 | Y0-16
      __m128i rgb = convert_yuy2_to_rgb_sse2_core<rgb_size>(luma_scaled, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if constexpr(rgb_size == 4){
        _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width*4-8), rgb);
      } else {
        //input: 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0 | 00

        rgb = _mm_srli_si128(rgb, 1); //00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 || 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0
        *reinterpret_cast<int*>(dstp+width*3-6) = _mm_cvtsi128_si32(rgb);
        rgb = _mm_srli_si128(rgb, 4); //00 ... r1 | g1
        *reinterpret_cast<short*>(dstp+width*3-2) = (short)_mm_cvtsi128_si32(rgb);
      }
    } else {
      //four pixels
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+width*2-8)); //0 0 0 0 0 0 0 0 V1 Y3 U1 Y2 V0 Y1 U0 Y0

      __m128i src_luma = _mm_and_si128(src, ff);//0 0 0 0 0 0 0 0 0 Y3 0 Y2 0 Y1 0 Y0
      __m128i src_chroma = _mm_srli_epi16(src, 8); //0 0 0 0 0 0 0 0 0 V1 0 U1 0 V0 0 U0

      src_chroma = _mm_shuffle_epi32(src_chroma, _MM_SHUFFLE(1, 1, 1, 0)); //00 V1 00 U1 00 V1 00 U1 00 V1 00 U1 00 V0 00 U0

      __m128i luma_scaled  = _mm_sub_epi16(src_luma, tv_scale_vector); //Y3-16 | Y2-16 | Y1-16 | Y0-16
      luma_scaled = _mm_unpacklo_epi16(luma_scaled, zero); // 0000 | Y3-16 | 0000 | Y2-16 | 0000 | Y1-16 | 0000 | Y0-16

      __m128i rgb = convert_yuy2_to_rgb_sse2_core<rgb_size>(luma_scaled, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if constexpr(rgb_size == 4) {
        _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+width*4-16), rgb);
      } else {
        //input: 00 | r3 | g3 | b3 | r2 | g2 | b2 | 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0 | 00

        rgb = _mm_srli_si128(rgb, 1); //00 | 00 | r3 | g3 | b3 | r2 | g2 | b2 || 00 | 00 | r1 | g1 | b1 | r0 | g0 | b0
        *reinterpret_cast<int*>(dstp+width*3-12) = _mm_cvtsi128_si32(rgb);
        rgb = _mm_shufflelo_epi16(rgb, _MM_SHUFFLE(2, 3, 3, 3)); //00 | 00 | r3 | g3 | b3 | r2 | g2 | b2 || r1 | g1 | 00 | 00 | 00 | 00 | 00 | 00
        rgb = _mm_srli_si128(rgb, 6); //00 | 00 | 00 | 00 | 00 | 00 | 00 | 00 || r3 | g3 | b3 | r2 | g2 | b2 | r1 | g1
        _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width*3-8), rgb);
      }
    }

    dstp += dst_pitch;
  }
}
#endif

#ifdef X86_32

template<int rgb_size>
static AVS_FORCEINLINE __m64 convert_yuy2_to_rgb_isse_core(const __m64& src_luma_scaled, const __m64& src_chroma, const __m64 &alpha,
                                                         const __m64& v128, const __m64& zero, const __m64& rounder, const __m64 &ff,
                                                         const __m64& ymul, const __m64& bmul, const __m64& gmul, const __m64& rmul) {
  __m64 chroma_scaled = _mm_sub_pi16(src_chroma, v128); //V1-128 | U1-128 | V0-128 | U0-128

  __m64 luma_scaled = _mm_madd_pi16(src_luma_scaled, ymul); // (y1-16)*cy | (y0-16)*cy
  luma_scaled = _mm_add_pi32(luma_scaled, rounder); // (y1-16)*cy + 8192 | (y0-16)*cy + 8192

  __m64 chroma_scaled2 = _mm_shuffle_pi16(chroma_scaled, _MM_SHUFFLE(1, 0, 1, 0)); // V0-128 | U0-128 | V0-128 | U0-128

  chroma_scaled = _mm_add_pi16(chroma_scaled, chroma_scaled2); // V0+V1-256 | U0+U1-256 | (V0-128)*2 | (U0-128)*2

  __m64 b = _mm_madd_pi16(chroma_scaled, bmul); //               0 + (U0+U1-256)*cbu |              0 + (U0-128)*2*cbu
  __m64 g = _mm_madd_pi16(chroma_scaled, gmul); // (V0+V1-256)*cgv + (U0+U1-256)*cgu | (V0-128)*2*cgv + (U0-128)*2*cgu
  __m64 r = _mm_madd_pi16(chroma_scaled, rmul); // (V0+V1-256)*crv + 0               | (V0-128)*2*crv + 0

  b = _mm_add_pi32(luma_scaled, b);  //b1 | b0
  g = _mm_add_pi32(luma_scaled, g);  //g1 | g0
  r = _mm_add_pi32(luma_scaled, r);  //r1 | r0

  b = _mm_srai_pi32(b, 14); //BBBB | bbbb
  g = _mm_srai_pi32(g, 14); //GGGG | gggg
  r = _mm_srai_pi32(r, 14); //RRRR | rrrr

  if (rgb_size == 4)
  {
    b = _mm_max_pi16(b, zero);
    g = _mm_max_pi16(g, zero);
    r = _mm_max_pi16(r, zero);

    b = _mm_min_pi16(b, ff); //00 00 00 b1 | 00 00 00 b0
    g = _mm_min_pi16(g, ff); //00 00 00 g1 | 00 00 00 g0
    r = _mm_min_pi16(r, ff); //00 00 00 r1 | 00 00 00 r0

    r = _mm_slli_pi32(r, 16);
    g = _mm_slli_pi32(g, 8);

    __m64 rb = _mm_or_si64(r, b);
    __m64 rgb = _mm_or_si64(rb, g);
    return _mm_or_si64(rgb, alpha);
  }
  else
  {
    __m64 bg = _mm_packs_pi32(b, g); //GGGG | gggg | BBBB | bbbb
    r = _mm_packs_pi32(r, zero); //0000 | 0000 | RRRR | rrrr

    __m64 br = _mm_unpacklo_pi16(bg, r); //RRRR | BBBB | rrrr | bbbb
    g = _mm_unpackhi_pi16(bg, zero); //0000 | GGGG | 0000 | gggg

    __m64 rgb_lo = _mm_unpacklo_pi16(br, g); //0000 | rrrr | gggg | bbbb
    rgb_lo = _mm_slli_si64(rgb_lo, 16); //rrrr | gggg | bbbb | 0000
    __m64 rgb_hi = _mm_unpackhi_pi16(br, g); //0000 | RRRR | GGGG | BBBB

    return _mm_packs_pu16(rgb_lo, rgb_hi); //00 | RR | GG | BB | rr | gg | bb | 00
  }
}

//todo: omg this thing is overcomplicated
template<int rgb_size>
static void convert_yuy2_to_rgb_isse(const BYTE *srcp, BYTE* dstp, int src_pitch, int dst_pitch, int height, int width, int crv, int cgv, int cgu, int cbu, int cy, int tv_scale) {
  srcp += height * src_pitch;
  int mod4_width = width / 4 * 4;

  __m64 tv_scale_vector = _mm_set1_pi16(tv_scale);
  __m64 zero = _mm_setzero_si64();
  __m64 ymul = _mm_set1_pi16(cy / 4);

  __m64 bmul = _mm_set_pi16(0, cbu/8, 0, cbu/8);
  __m64 gmul = _mm_set_pi16(-cgv/8, -cgu/8, -cgv/8, -cgu/8);
  __m64 rmul = _mm_set_pi16(crv/8, 0, crv/8, 0);
  __m64 alpha = _mm_set1_pi32(0xFF000000);
  __m64 ff = _mm_set1_pi16(0x00FF);
  __m64 v128 =  _mm_set1_pi16(128);
  __m64 rounder = _mm_set1_pi32(1 << 13);

  for (int y = 0; y < height; ++y) {
    srcp -= src_pitch;
    for (int x = 0; x < mod4_width-2; x+= 4) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+x*2); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 src2 = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x*2+8)); //00 00 00 00 V2 xx U2 xx

      __m64 src_luma   = _mm_and_si64(src, ff);//0 Y3 0 Y2 0 Y1 0 Y0
      __m64 src_chroma = _mm_srli_pi16(src, 8); //00 V1 00 U1 00 V0 00 U0
      __m64 src_chroma2 = _mm_srli_pi16(src2, 8); //00 00 00 00 00 V2 00 U2

      src_chroma2 = _mm_or_si64(_mm_slli_si64(src_chroma2, 32), _mm_srli_si64(src_chroma, 32)); //00 V2 00 U2 00 V1 00 U1

      __m64 luma_scaled   = _mm_sub_pi16(src_luma, tv_scale_vector); //Y3-16 | Y2-16 | Y1-16 | Y0-16
      __m64 luma_scaled_lo = _mm_unpacklo_pi16(luma_scaled, zero); // 0000 | Y1-16 | 0000 | Y0-16
      __m64 luma_scaled_hi = _mm_unpackhi_pi16(luma_scaled, zero);

      __m64 rgb_lo = convert_yuy2_to_rgb_isse_core<rgb_size>(luma_scaled_lo, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);
      __m64 rgb_hi = convert_yuy2_to_rgb_isse_core<rgb_size>(luma_scaled_hi, src_chroma2, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if (rgb_size == 4) {
        *reinterpret_cast<__m64*>(dstp+x*4) = rgb_lo;
        *reinterpret_cast<__m64*>(dstp+x*4+8) = rgb_hi;
      } else {
        //input: 00 | RR | GG | BB | rr | gg | bb | 00
        rgb_lo = _mm_srli_si64(rgb_lo, 8); //00 | 00 | RR | GG | BB | rr | gg | bb
        rgb_hi = _mm_slli_si64(rgb_hi, 8); //RR | GG | BB | rr | gg | bb | 00 | 00
        *reinterpret_cast<int*>(dstp+x*3) = _mm_cvtsi64_si32(rgb_lo);
        rgb_lo = _mm_srli_si64(rgb_lo, 32); //00 | 00 | 00 | 00 | 00 | 00 | RR | GG
        rgb_hi = _mm_or_si64(rgb_lo, rgb_hi);
        *reinterpret_cast<__m64*>(dstp+x*3+4) = rgb_hi;
      }
    }

    if (mod4_width == width) {
      //two pixels left to process
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width*2 - 4)); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 src_luma   = _mm_and_si64(src, ff);//0 Y3 0 Y2 0 Y1 0 Y0
      __m64 src_chroma = _mm_srli_pi16(src, 8); //00 V1 00 U1 00 V0 00 U0
      src_chroma = _mm_shuffle_pi16(src_chroma, _MM_SHUFFLE(1, 0, 1, 0)); //00 V0 00 U0 00 V0 00 U0

      __m64 luma_scaled   = _mm_sub_pi16(src_luma, tv_scale_vector); //Y3-16 | Y2-16 | Y1-16 | Y0-16
      luma_scaled = _mm_unpacklo_pi16(luma_scaled, zero); // 0000 | Y1-16 | 0000 | Y0-16
      __m64 rgb = convert_yuy2_to_rgb_isse_core<rgb_size>(luma_scaled, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if (rgb_size == 4){
        *reinterpret_cast<__m64*>(dstp+width*4-8) = rgb;
      } else {
        //input: 00 | RR | GG | BB | rr | gg | bb | 00
        rgb = _mm_srli_si64(rgb, 8); //00 | 00 | RR | GG | BB | rr | gg | bb
        *reinterpret_cast<int*>(dstp+width*3-6) = _mm_cvtsi64_si32(rgb);
        rgb = _mm_srli_si64(rgb, 32); //00 | 00 | 00 | 00 | 00 | 00 | RR | GG
        *reinterpret_cast<short*>(dstp+width*3-2) = (short)_mm_cvtsi64_si32(rgb);
      }
    } else {
      //four pixels
      __m64 src = *reinterpret_cast<const __m64*>(srcp+width*2 - 8); //V1 Y3 U1 Y2 V0 Y1 U0 Y0
      __m64 src_luma   = _mm_and_si64(src, ff);//0 Y3 0 Y2 0 Y1 0 Y0
      __m64 src_chroma = _mm_srli_pi16(src, 8); //00 V1 00 U1 00 V0 00 U0
      __m64 src_chroma2 = _mm_shuffle_pi16(src_chroma, _MM_SHUFFLE(3, 2, 3, 2)); //00 V1 00 U1 00 V1 00 U1

      __m64 luma_scaled   = _mm_sub_pi16(src_luma, tv_scale_vector); //Y3-16 | Y2-16 | Y1-16 | Y0-16
      __m64 luma_scaled_lo = _mm_unpacklo_pi16(luma_scaled, zero); // 0000 | Y1-16 | 0000 | Y0-16
      __m64 luma_scaled_hi = _mm_unpackhi_pi16(luma_scaled, zero);

      __m64 rgb_lo = convert_yuy2_to_rgb_isse_core<rgb_size>(luma_scaled_lo, src_chroma, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);
      __m64 rgb_hi = convert_yuy2_to_rgb_isse_core<rgb_size>(luma_scaled_hi, src_chroma2, alpha, v128, zero, rounder, ff, ymul, bmul, gmul, rmul);

      if (rgb_size == 4) {
        *reinterpret_cast<__m64*>(dstp+width*4-16) = rgb_lo;
        *reinterpret_cast<__m64*>(dstp+width*4-8)  = rgb_hi;
      } else {
        //input: 00 | RR | GG | BB | rr | gg | bb | 00
        rgb_lo = _mm_srli_si64(rgb_lo, 8); //00 | 00 | RR | GG | BB | rr | gg | bb
        rgb_hi = _mm_slli_si64(rgb_hi, 8); //RR | GG | BB | rr | gg | bb | 00 | 00
        *reinterpret_cast<int*>(dstp+width*3-12) = _mm_cvtsi64_si32(rgb_lo);
        rgb_lo = _mm_srli_si64(rgb_lo, 32); //00 | 00 | 00 | 00 | 00 | 00 | RR | GG
        rgb_hi = _mm_or_si64(rgb_lo, rgb_hi);
        *reinterpret_cast<__m64*>(dstp+width*3-8) = rgb_hi;
      }
    }

    dstp += dst_pitch;
  }
  _mm_empty();
}

#endif // X86_32

template<int rgb_size>
static void convert_yuy2_to_rgb_c(const BYTE *srcp, BYTE* dstp, int src_pitch, int dst_pitch, int height, int width, int crv, int cgv, int cgu, int cbu, int cy, int tv_scale) {
  srcp += height * src_pitch;
  for (int y = height; y > 0; --y) {
    srcp -= src_pitch;
    int x;
    for (x = 0; x < width-2; x+=2) {
      int scaled_y0 = (srcp[x*2+0] - tv_scale) * cy;
      int u0 = srcp[x*2+1]-128;
      int v0 = srcp[x*2+3]-128;
      int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
      int u1 = srcp[x*2+5]-128;
      int v1 = srcp[x*2+7]-128;

      dstp[x*rgb_size + 0] = ScaledPixelClip(scaled_y0 + u0 * cbu);                 // blue
      dstp[x*rgb_size + 1] = ScaledPixelClip(scaled_y0 - u0 * cgu - v0 * cgv); // green
      dstp[x*rgb_size + 2] = ScaledPixelClip(scaled_y0            + v0 * crv); // red

      dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + (u0+u1) * (cbu / 2));                     // blue
      dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - (u0+u1) * (cgu / 2) - (v0+v1) * (cgv/2)); // green
      dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1                       + (v0+v1) * (crv/2)); // red

      if constexpr(rgb_size == 4) {
        dstp[x*4+3] = 255;
        dstp[x*4+7] = 255;
      }
    }

    int scaled_y0 = (srcp[x*2+0] - tv_scale) * cy;
    int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
    int u = srcp[x*2+1]-128;
    int v = srcp[x*2+3]-128;

    dstp[x*rgb_size + 0]     = ScaledPixelClip(scaled_y0 + u * cbu);                 // blue
    dstp[x*rgb_size + 1]     = ScaledPixelClip(scaled_y0 - u * cgu - v * cgv); // green
    dstp[x*rgb_size + 2]     = ScaledPixelClip(scaled_y0           + v * crv); // red

    dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + u * cbu);                 // blue
    dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - u * cgu - v * cgv); // green
    dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1           + v * crv); // red

    if constexpr(rgb_size == 4) {
      dstp[x*4+3] = 255;
      dstp[x*4+7] = 255;
    }
    dstp += dst_pitch;
  }
}

// YUY2 only
PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  const int dst_pitch = dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  int tv_scale = matrix.offset_y;


#ifdef __SSE2__
  if (env->GetCPUFlags() & CPUF_SSE2) {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_sse2<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
      matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    } else {
      convert_yuy2_to_rgb_sse2<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    }
  }
  else
#endif
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_isse<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    } else {
      convert_yuy2_to_rgb_isse<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    }
  }
  else
#endif
  {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_c<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    } else {
      convert_yuy2_to_rgb_c<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        matrix.v_r, matrix.v_g, matrix.u_g, matrix.u_b, matrix.y_r, tv_scale);
    }
  }
  return dst;
}

// general for all colorspaces
// however class is constructed only for YUY2 input
AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  VideoInfo vi = clip->GetVideoInfo();

  // common Create for all CreateRGB24/32/48/64/Planar(RGBP:-1, RGPAP:-2) using user_data
  int target_rgbtype = (int)reinterpret_cast<intptr_t>(user_data);
  // -1,-2: Planar RGB(A)
  //  0: not specified (leave if input is packed RGB, convert to rgb32/64 input colorspace dependent)
  // 24,32,48,64: RGB24/32/48/64

  // planar YUV-like
  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) {
    bool needConvertFinalBitdepth = false;
    int finalBitdepth = -1;

    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    // conversion to planar or packed RGB is always from 444
    clip = ConvertToPlanarGeneric::CreateYUV444(AVSValue(new_args, 5), (void *)1, env).AsClip(); // (void *)1: not restricted to 8 bits
    if ((target_rgbtype == 24 || target_rgbtype == 32)) {
      if (vi.BitsPerComponent() != 8) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 8;
        target_rgbtype = (target_rgbtype == 24) ? -1 : -2; // planar rgb intermediate
      }
    }
    else if ((target_rgbtype == 48 || target_rgbtype == 64)) {
      if (vi.BitsPerComponent() != 16) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 16;
        target_rgbtype = (target_rgbtype == 48) ? -1 : -2; // planar rgb intermediate
      }
    }
    else if(target_rgbtype==0 && vi.ComponentSize()==4)
        env->ThrowError("ConvertToRGB: conversion is allowed only from 8 or 16 bit colorspaces");
    int rgbtype_param;
    bool reallyConvert = true;
    switch (target_rgbtype)
    {
    case -1: case -2:
        rgbtype_param = target_rgbtype; break; // planar RGB(A)
    case 0:
        rgbtype_param = vi.ComponentSize() == 1 ? 4 : 8; break; // input bitdepth adaptive
    case 24:
        rgbtype_param = 3; break; // RGB24
    case 32:
        rgbtype_param = 4; break; // RGB32
    case 48: {
            // instead of C code of YUV444P16->RGB48
            // we convert to PlanarRGB then to RGB48 (both is fast)
          AVSValue new_args2[5] = { clip, args[1], args[2], args[3], args[4] };
          clip = ConvertToRGB::Create(AVSValue(new_args2, 5), (void *)-1, env).AsClip();
          vi = clip->GetVideoInfo();
          reallyConvert = false;
          rgbtype_param = 6; // old option RGB48 target, slow C
        }
        break; // RGB48
    case 64: {
        // instead of C code of YUV(A)444P16->RGB64
        // we convert to PlanarRGB(A) then to RGB64 (both is fast)
        AVSValue new_args2[5] = { clip, args[1], args[2], args[3], args[4] };
        clip = ConvertToRGB::Create(AVSValue(new_args2, 5), vi.IsYUVA() ? (void *)-2 : (void *)-1, env).AsClip();
        vi = clip->GetVideoInfo();
        reallyConvert = false;
        rgbtype_param = 8; // old option RGB64 target, slow C
      }
      break; // RGB64
    }
    if (reallyConvert) {
      clip = new ConvertYUV444ToRGB(clip, getMatrix(matrix, env), rgbtype_param, env);

      if (needConvertFinalBitdepth) {
        // from any planar rgb(a) -> rgb24/32/48/64
        clip = new ConvertBits(clip, -1 /*dither_type*/, finalBitdepth /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
        vi = clip->GetVideoInfo();

        // source here is always a 8/16bit planar RGB(A), finally it has to be converted to RGB24/32/48/64
        const bool isRGBA = target_rgbtype == -2;
        clip = new PlanarRGBtoPackedRGB(clip, isRGBA);
        vi = clip->GetVideoInfo();
      }
      return clip;
    }
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB: ChromaPlacement and ChromaResample options are not supported.");

  // planar RGB-like source
  if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
  {
    if (target_rgbtype < 0) // planar to planar
    {
      if (vi.IsPlanarRGB()) {
        if (target_rgbtype == -1)
          return clip;
        // prgb->prgba create with default alpha
        return new AddAlphaPlane(clip, nullptr, 0.0f, false, env);
      }
      // planar rgba source
      if (target_rgbtype == -2)
        return clip;
      return new RemoveAlphaPlane(clip, env);
    }

    // planar to packed 24/32/48/64
    bool needConvertFinalBitdepth = false;
    int finalBitdepth = -1;

    if (target_rgbtype == 24 || target_rgbtype == 32) {
      if (vi.BitsPerComponent() != 8) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 8;
      }
    }
    else if (target_rgbtype == 48 || target_rgbtype == 64) {
      if (vi.BitsPerComponent() != 16) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 16;
      }
    }

    if (needConvertFinalBitdepth) {
      // from any bitdepth planar rgb(a) -> 8/16 bits
      clip = new ConvertBits(clip, -1 /*dither_type*/, finalBitdepth /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo();
    }

    return new PlanarRGBtoPackedRGB(clip, (target_rgbtype == 32 || target_rgbtype == 64));
  }

  // YUY2
  if (vi.IsYUV()) // at this point IsYUV means YUY2 (non-planar)
  {
    if (target_rgbtype == 48 || target_rgbtype == 64)
      env->ThrowError("ConvertToRGB: conversion from YUY2 is allowed only to 8 bits");
    if (target_rgbtype < 0) {
      // rgb32 intermediate is faster
      clip = new ConvertToRGB(clip, false, matrix, env); // YUY2->RGB32
      return new PackedRGBtoPlanarRGB(clip, true, target_rgbtype == -2);
    }
    else
      return new ConvertToRGB(clip, target_rgbtype == 24, matrix, env);
  }

  // conversions from packed RGB

  if (target_rgbtype == 24 || target_rgbtype == 32) {
    if (vi.ComponentSize() != 1) {
      // 64->32, 48->24
      clip = new ConvertBits(clip, -1 /*dither_type*/, 8 /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo(); // new format
    }
  }
  else if (target_rgbtype == 48 || target_rgbtype == 64) {
    if (vi.ComponentSize() != 2) {
      // 32->64, 24->48
      clip = new ConvertBits(clip, -1 /*dither_type*/, 16 /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo(); // new format
    }
  }

  if(target_rgbtype==32 || target_rgbtype==64)
      if (vi.IsRGB24() || vi.IsRGB48())
          return new RGBtoRGBA(clip); // 24->32 or 48->64

  if(target_rgbtype==24 || target_rgbtype==48)
      if (vi.IsRGB32() || vi.IsRGB64())
          return new RGBAtoRGB(clip); // 32->24 or 64->48

  // <0: target is planar RGB(A)
  if (target_rgbtype < 0) {
    // RGB24/32/48/64 ->
    const bool isSrcRGBA = vi.IsRGB32() || vi.IsRGB64();
    const bool isTargetRGBA = target_rgbtype == -2;
    return new PackedRGBtoPlanarRGB(clip, isSrcRGBA, isTargetRGBA);
  }

  return clip;
}


/**********************************
*******   Convert to YV12   ******
*********************************/

// for YUY2->YV12 only
// all other sources use ConvertToPlanarGeneric
ConvertToYV12::ConvertToYV12(PClip _child, bool _interlaced, IScriptEnvironment* env)
  : GenericVideoFilter(_child),
  interlaced(_interlaced)
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYV12: Image width must be multiple of 2");

  if (interlaced && (vi.height & 3))
    env->ThrowError("ConvertToYV12: Interlaced image height must be multiple of 4");

  if ((!interlaced) && (vi.height & 1))
    env->ThrowError("ConvertToYV12: Image height must be multiple of 2");

  if (!vi.IsYUY2())
    env->ThrowError("ConvertToYV12: Source must be YUY2.");

  vi.pixel_type = VideoInfo::CS_YV12;

  if ((env->GetCPUFlags() & CPUF_MMX) == 0)
    env->ThrowError("ConvertToYV12: YV12 support require a MMX capable processor.");
}

PVideoFrame __stdcall ConvertToYV12::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);

  if (interlaced) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src->GetReadPtr(), 16))
    {
      convert_yuy2_to_yv12_interlaced_sse2(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE))
      {
        convert_yuy2_to_yv12_interlaced_isse(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
      else
#endif
      {
        convert_yuy2_to_yv12_interlaced_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
  }
  else
  {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src->GetReadPtr(), 16))
    {
      convert_yuy2_to_yv12_progressive_sse2(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE))
      {
        convert_yuy2_to_yv12_progressive_isse(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
      else
#endif
      {
        convert_yuy2_to_yv12_progressive_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
  }

  return dst;

}

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();
  bool only_8bit = reinterpret_cast<intptr_t>(user_data) == 0;
  if (only_8bit && vi.BitsPerComponent() != 8)
    env->ThrowError("ConvertToYV12: only 8 bit sources allowed");

  if (vi.IsYUY2() && !args[3].Defined() && !args[4].Defined() && !args[5].Defined())  // User has not requested options, do it fast!
    return new ConvertToYV12(clip,args[1].AsBool(false),env);

  return ConvertToPlanarGeneric::CreateYUV420(args, NULL,env);
}

AVSValue AddAlphaPlane::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  bool isMaskDefined = args[1].Defined();
  bool maskIsClip = false;
  // if mask is not defined and videoformat has Alpha then we return
  if(isMaskDefined && !args[1].IsClip() && !args[1].IsFloat())
    env->ThrowError("AddAlphaPlane: mask parameter should be clip or number");
  const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
  if (!isMaskDefined && (vi.IsPlanarRGBA() || vi.IsYUVA() || vi.IsRGB32() || vi.IsRGB64()))
    return args[0].AsClip();
  PClip alphaClip = nullptr;
  if (isMaskDefined && args[1].IsClip()) {
    const VideoInfo& viAlphaClip = args[1].AsClip()->GetVideoInfo();
    maskIsClip = true;
    if(viAlphaClip.BitsPerComponent() != vi.BitsPerComponent())
      env->ThrowError("AddAlphaPlane: alpha clip is of different bit depth");
    if(viAlphaClip.width != vi.width || viAlphaClip.height != vi.height )
      env->ThrowError("AddAlphaPlane: alpha clip is of different size");
    if (viAlphaClip.IsY())
      alphaClip = args[1].AsClip();
    else if (viAlphaClip.NumComponents() == 4) {
      AVSValue new_args[1] = { args[1].AsClip() };
      alphaClip = env->Invoke("ExtractA", AVSValue(new_args, 1)).AsClip();
    } else {
      env->ThrowError("AddAlphaPlane: alpha clip should be greyscale or should have alpha plane");
    }
    // alphaClip is always greyscale here
  }
  float maskAsFloat = -1.0f;
  if (!maskIsClip)
    maskAsFloat = (float)args[1].AsFloat(-1.0f);
  if (vi.IsRGB24()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    PClip child = env->Invoke("ConvertToRGB32", AVSValue(new_args, 1)).AsClip();
    return new AddAlphaPlane(child, alphaClip, maskAsFloat, isMaskDefined, env);
  } else if(vi.IsRGB48()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    PClip child = env->Invoke("ConvertToRGB64", AVSValue(new_args, 1)).AsClip();
    return new AddAlphaPlane(child, alphaClip, maskAsFloat, isMaskDefined, env);
  }
  return new AddAlphaPlane(args[0].AsClip(), alphaClip, maskAsFloat, isMaskDefined, env);
}

AddAlphaPlane::AddAlphaPlane(PClip _child, PClip _alphaClip, float _mask_f, bool isMaskDefined, IScriptEnvironment* env)
  : GenericVideoFilter(_child), alphaClip(_alphaClip)
{
  if(vi.IsYUY2())
    env->ThrowError("AddAlphaPlane: YUY2 is not allowed");
  if(vi.IsY())
    env->ThrowError("AddAlphaPlane: greyscale source is not allowed");
  if(vi.IsYUV() && !vi.Is420() && !vi.Is422() && !vi.Is444()) // e.g. 410
    env->ThrowError("AddAlphaPlane: YUV format not supported, must be 420, 422 or 444");
  if(!vi.IsYUV() && !vi.IsYUVA() && !vi.IsRGB())
    env->ThrowError("AddAlphaPlane: format not supported");

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (vi.IsYUV()) {
    int pixel_type = vi.pixel_type;
    if (vi.IsYV12())
      pixel_type = VideoInfo::CS_YV12;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_YUV) | VideoInfo::CS_YUVA;
    vi.pixel_type = new_pixel_type;
  } else if(vi.IsPlanarRGB()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_RGB_TYPE) | VideoInfo::CS_RGBA_TYPE;
    vi.pixel_type = new_pixel_type;
  }
  // RGB24 and RGB48 already converted to 32/64
  // RGB32, RGB64, YUVA and RGBA: no change

  // mask parameter. If none->max transparency

  if (!alphaClip) {
    int max_pixel_value = (1 << bits_per_pixel) - 1; // n/a for float
    if (!isMaskDefined) {
      mask_f = 1.0f;
      mask = max_pixel_value;
    }
    else {
      mask_f = _mask_f;
      mask = (mask_f < 0) ? 0 : (mask_f > max_pixel_value) ? max_pixel_value : (int)mask_f;
      mask = clamp(mask, 0, max_pixel_value);
      // no clamp for float
    }
  }
}

PVideoFrame AddAlphaPlane::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  if(vi.IsPlanar())
  {
    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    // copy existing 3 planes
    for (int p = 0; p < 3; ++p) {
      const int plane = planes[p];
      env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane),
           src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
    }
  } else {
    // Packed RGB, already converted to RGB32 or RGB64
    env->BitBlt(dst->GetWritePtr(), dst->GetPitch(), src->GetReadPtr(),
      src->GetPitch(), src->GetRowSize(), src->GetHeight());
  }

  if (vi.IsPlanarRGBA() || vi.IsYUVA()) {
    if (alphaClip) {
      PVideoFrame srcAlpha = alphaClip->GetFrame(n, env);
      env->BitBlt(dst->GetWritePtr(PLANAR_A), dst->GetPitch(PLANAR_A), srcAlpha->GetReadPtr(PLANAR_Y),
        srcAlpha->GetPitch(PLANAR_Y), srcAlpha->GetRowSize(PLANAR_Y), srcAlpha->GetHeight(PLANAR_Y));
    }
    else {
      // default constant
      const int dst_pitchA = dst->GetPitch(PLANAR_A);
      BYTE* dstp_a = dst->GetWritePtr(PLANAR_A);
      const int heightA = dst->GetHeight(PLANAR_A);

      switch (vi.ComponentSize())
      {
      case 1:
        fill_plane<BYTE>(dstp_a, heightA, dst_pitchA, mask);
        break;
      case 2:
        fill_plane<uint16_t>(dstp_a, heightA, dst_pitchA, mask);
        break;
      case 4:
        fill_plane<float>(dstp_a, heightA, dst_pitchA, mask_f);
        break;
      }
    }
    return dst;
  }
  // RGB32 and RGB64

  BYTE* pf = dst->GetWritePtr();
  int pitch = dst->GetPitch();
  int rowsize = dst->GetRowSize();
  int height = dst->GetHeight();
  int width = vi.width;

  if (alphaClip) {
    // fill by alpha clip already converted to grey-only
    PVideoFrame srcAlpha = alphaClip->GetFrame(n, env);
    const BYTE* srcp_a = srcAlpha->GetReadPtr(PLANAR_Y);
    size_t pitch_a = srcAlpha->GetPitch(PLANAR_Y);

    pf += pitch * (vi.height - 1); // start from bottom: packed RGB is upside down

    if (vi.IsRGB32()) {
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x ++) {
          pf[x*4+3] = srcp_a[x];
        }
        pf -= pitch; // packed RGB is upside down
        srcp_a += pitch_a;
      }
    }
    else if (vi.IsRGB64()) {
      rowsize /= sizeof(uint16_t);
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x ++) {
          reinterpret_cast<uint16_t *>(pf)[x*4+3] = reinterpret_cast<const uint16_t *>(srcp_a)[x];
        }
        pf -= pitch; // packed RGB is upside down
        srcp_a += pitch_a;
      }
    }
  }
  else {
    // fill with constant
    if (vi.IsRGB32()) {
      for (int y = 0; y < height; y++) {
        for (int x = 3; x < rowsize; x += 4) {
          pf[x] = mask;
        }
        pf += pitch;
      }
    }
    else if (vi.IsRGB64()) {
      rowsize /= sizeof(uint16_t);
      for (int y = 0; y < height; y++) {
        for (int x = 3; x < rowsize; x += 4) {
          reinterpret_cast<uint16_t *>(pf)[x] = mask;
        }
        pf += pitch;
      }
    }
  }

  return dst;
}

AVSValue RemoveAlphaPlane::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  // if videoformat has no Alpha then we return
  const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
  if(vi.IsPlanar() && (vi.IsYUV() || vi.IsPlanarRGB())) // planar and no alpha
    return args[0].AsClip();
  if (vi.IsYUY2()) // YUY2: no alpha
    return args[0].AsClip();
  if(vi.IsRGB24() || vi.IsRGB48()) // packed RGB and no alpha
    return args[0].AsClip();
  if (vi.IsRGB32()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    return env->Invoke("ConvertToRGB24", AVSValue(new_args, 1)).AsClip();
  }
  if (vi.IsRGB64()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    return env->Invoke("ConvertToRGB48", AVSValue(new_args, 1)).AsClip();
  }
  return new RemoveAlphaPlane(args[0].AsClip(), env);
}

RemoveAlphaPlane::RemoveAlphaPlane(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if(vi.IsYUY2())
    env->ThrowError("RemoveAlphaPlane: YUY2 is not allowed");
  if(vi.IsY())
    env->ThrowError("RemoveAlphaPlane: greyscale source is not allowed");

  if (vi.IsYUVA()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_YUVA) | VideoInfo::CS_YUV;
    vi.pixel_type = new_pixel_type;
  } else if(vi.IsPlanarRGBA()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_RGBA_TYPE) | VideoInfo::CS_RGB_TYPE;
    vi.pixel_type = new_pixel_type;
  }
}

PVideoFrame RemoveAlphaPlane::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  // Packed RGB: already handled in ::Create through Invoke 32->24 or 64->48 conversion
  // only planar here
  int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
  int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
  int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
  // Abuse Subframe to snatch the YUV/GBR planes
  return env->SubframePlanar(src, 0, src->GetPitch(planes[0]), src->GetRowSize(planes[0]), src->GetHeight(planes[0]),0,0,src->GetPitch(planes[1]));

#if 0
  // BitBlt version. Kept for reference
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  // copy 3 planes w/o alpha
  for (int p = 0; p < 3; ++p) {
    const int plane = planes[p];
    env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane),
      src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
  }
return dst;
#endif
}
