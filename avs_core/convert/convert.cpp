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


#include "convert.h"
#include "convert_planar.h"
#include "convert_rgb.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include <avs/alignment.h>
#include <avs/win.h>
#include <avs/minmax.h>
#include <emmintrin.h>



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Convert_filters[] = {       // matrix can be "rec601", rec709", "PC.601" or "PC.709"
  { "ConvertToRGB",   BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)0 },
  { "ConvertToRGB24", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)24 },
  { "ConvertToRGB32", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)32 },
  { "ConvertToRGB48", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)48 },
  { "ConvertToRGB64", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)64 },
  { "ConvertToPlanarRGB",  BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-1 },
  { "ConvertToPlanarRGBA", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-2 },
  { "ConvertToY8",    BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY8::Create },
  { "ConvertToYV12",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToYV12::Create },
  { "ConvertToYV24",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444},
  { "ConvertToYV16",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV422},
  { "ConvertToYV411", BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411},
  { "ConvertToYUY2",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToYUY2::Create },
  { "ConvertBackToYUY2", BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertBackToYUY2::Create },
  { "ConvertToY",       BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY8::Create },
  { "ConvertToYUV420",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToPlanarGeneric::CreateYUV420},
  { "ConvertToYUV422",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV422},
  { "ConvertToYUV444",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444},
  { "ConvertTo8bit",  BUILTIN_FUNC_PREFIX, "c[truerange]b[dither]i[scale]f", ConvertTo8bit::Create},
  { "ConvertTo16bit", BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]i[scale]f", ConvertTo16bit::Create},
  { "ConvertToFloat", BUILTIN_FUNC_PREFIX, "c[truerange]b[scale]f", ConvertToFloat::Create},
  { 0 }
};

static const int crv_rec601 = int(1.596*65536+0.5);
static const int cgv_rec601 = int(0.813*65536+0.5);
static const int cgu_rec601 = int(0.391*65536+0.5);
static const int cbu_rec601 = int(2.018*65536+0.5);

static const int crv_rec709 = int(1.793*65536+0.5);
static const int cgv_rec709 = int(0.533*65536+0.5);
static const int cgu_rec709 = int(0.213*65536+0.5);
static const int cbu_rec709 = int(2.112*65536+0.5);

static const int crv_pc601 = int(1.407*65536+0.5);
static const int cgv_pc601 = int(0.717*65536+0.5);
static const int cgu_pc601 = int(0.345*65536+0.5);
static const int cbu_pc601 = int(1.779*65536+0.5);

static const int crv_pc709 = int(1.581*65536+0.5);
static const int cgv_pc709 = int(0.470*65536+0.5);
static const int cgu_pc709 = int(0.188*65536+0.5);
static const int cbu_pc709 = int(1.863*65536+0.5);

static const int cy_rec = int((255.0/219.0)*65536+0.5);
static const int cy_pc = 65536;

static const int crv_values[4] = { crv_rec601, crv_rec709, crv_pc601, crv_pc709 };
static const int cgv_values[4] = { cgv_rec601, cgv_rec709, cgv_pc601, cgv_pc709 };
static const int cgu_values[4] = { cgu_rec601, cgu_rec709, cgu_pc601, cgu_pc709 };
static const int cbu_values[4] = { cbu_rec601, cbu_rec709, cbu_pc601, cbu_pc709 };
static const int cy_values[4]  = { cy_rec,     cy_rec,     cy_pc,     cy_pc};


int getMatrix( const char* matrix, IScriptEnvironment* env) {
  if (matrix) {
    if (!lstrcmpi(matrix, "rec601"))
      return Rec601;
    if (!lstrcmpi(matrix, "rec709"))
      return Rec709;
    if (!lstrcmpi(matrix, "PC.601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC.709"))
      return PC_709;
    if (!lstrcmpi(matrix, "PC601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC709"))
      return PC_709;
    if (!lstrcmpi(matrix, "AVERAGE"))
      return AVERAGE;
    env->ThrowError("Convert: Unknown colormatrix");
  }
  return Rec601; // Default colorspace conversion for AviSynth
}


/****************************************
*******   Convert to RGB / RGBA   ******
***************************************/

// YUY2 only
ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix,
                           IScriptEnvironment* env )
                           : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "PC709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"PC.601\" or \"PC.709\")");
  }
  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
}

#if defined(__SSE2__)
template<int rgb_size>
static __forceinline __m128i convert_yuy2_to_rgb_sse2_core(const __m128i& src_luma_scaled, const __m128i& src_chroma, const __m128i &alpha,
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


  if (rgb_size == 4)
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

      if (rgb_size == 4) {
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

      if (rgb_size == 4){
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

      if (rgb_size == 4) {
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
static __forceinline __m64 convert_yuy2_to_rgb_isse_core(const __m64& src_luma_scaled, const __m64& src_chroma, const __m64 &alpha,
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

      if (rgb_size == 4) {
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

    if (rgb_size == 4) {
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

  PVideoFrame dst = env->NewVideoFrame(vi);
  const int dst_pitch = dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  int tv_scale = theMatrix == Rec601 || theMatrix == Rec709 ? 16 : 0;


#ifdef __SSE2__
  if (env->GetCPUFlags() & CPUF_SSE2) {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_sse2<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    } else {
      convert_yuy2_to_rgb_sse2<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    }
  }
  else
#endif
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_isse<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    } else {
      convert_yuy2_to_rgb_isse<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    }
  }
  else
#endif
  {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_c<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    } else {
      convert_yuy2_to_rgb_c<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
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
  const VideoInfo& vi = clip->GetVideoInfo();

  // todo bitdepth conversion on-the-fly

  // common Create for all CreateRGB24/32/48/64/Planar(RGBP:-1, RGPAP:-2) using user_data
  int target_rgbtype = (int)reinterpret_cast<intptr_t>(user_data);
  // -1,-2: Planar RGB(A)
  //  0: not specified (leave if input is packed RGB, convert to rgb32/64 input colorspace dependent)
  // 24,32,48,64: RGB24/32/48/64

  // planar YUV-like
  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    // conversion to planar or packed RGB is always from 444
    clip = ConvertToPlanarGeneric::CreateYUV444(AVSValue(new_args, 5), NULL, env).AsClip();
    if((target_rgbtype==24 || target_rgbtype==32) && vi.ComponentSize()!=1)
        env->ThrowError("ConvertToRGB%d: conversion is allowed only from 8 bit colorspace",target_rgbtype);
    if((target_rgbtype==48 || target_rgbtype==64) && vi.ComponentSize()!=2)
        env->ThrowError("ConvertToRGB%d: conversion is allowed only from 16 bit colorspace",target_rgbtype);
    if(target_rgbtype==0 && vi.ComponentSize()==4)
        env->ThrowError("ConvertToRGB: conversion is allowed only from 8 or 16 bit colorspaces");
    int rgbtype_param;
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
    case 48:
        rgbtype_param = 6; break; // RGB48
    case 64:
        rgbtype_param = 8; break; // RGB64
    }
    return new ConvertYUV444ToRGB(clip, getMatrix(matrix, env), rgbtype_param , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB: ChromaPlacement and ChromaResample options are not supported.");

  // planar RGB-like source
  if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
  {
      if (target_rgbtype < 0) // planar to planar
      {
         if ((vi.IsPlanarRGB() && target_rgbtype==-1) || (vi.IsPlanarRGBA() && target_rgbtype==-2))
           return clip;
         env->ThrowError("ConvertToPlanarRGB: cannon convert between RGBP and RGBAP");
      }
      if(vi.ComponentSize() == 4)
          env->ThrowError("ConvertToRGB: conversion from float colorspace is not supported.");
      if((target_rgbtype==24 || target_rgbtype==32) && vi.ComponentSize()!=1)
          env->ThrowError("ConvertToRGB: conversion is allowed only from 8 bit colorspace");
      if((target_rgbtype==48 || target_rgbtype==64) && vi.ComponentSize()!=2)
          env->ThrowError("ConvertToRGB: conversion is allowed only from 16 bit colorspace");
      return new PlanarRGBtoPackedRGB(clip, (target_rgbtype==32 || target_rgbtype==64));
  }

  // YUY2
  if (vi.IsYUV()) // at this point IsYUV means YUY2 (non-planar)
  {
    if (target_rgbtype==48 || target_rgbtype==64 || target_rgbtype < 0)
        env->ThrowError("ConvertToRGB: conversion from YUY2 is allowed only to 8 bit packed RGB");
    return new ConvertToRGB(clip, target_rgbtype==24, matrix, env);
  }

  // conversions from packed RGB

  if((target_rgbtype==24 || target_rgbtype==32) && vi.ComponentSize()!=1)
      env->ThrowError("ConvertToRGB%d: conversion is allowed only from 8 bit colorspace",target_rgbtype);
  if((target_rgbtype==48 || target_rgbtype==64) && vi.ComponentSize()!=2)
      env->ThrowError("ConvertToRGB%d: conversion is allowed only from 16 bit colorspace",target_rgbtype);

  if(target_rgbtype==32 || target_rgbtype==64)
      if (vi.IsRGB24() || vi.IsRGB48())
          return new RGBtoRGBA(clip);

  if(target_rgbtype==24 || target_rgbtype==48)
      if (vi.IsRGB32() || vi.IsRGB64())
          return new RGBAtoRGB(clip);

  if (target_rgbtype < 0)
    return new PackedRGBtoPlanarRGB(clip, vi.IsRGB32() || vi.IsRGB64(), target_rgbtype==-2);

  return clip;
}


/**********************************
*******   Convert to YV12   ******
*********************************/


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
  PVideoFrame dst = env->NewVideoFrame(vi);

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

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsYUY2() && !args[3].Defined() && !args[4].Defined() && !args[5].Defined())  // User has not requested options, do it fast!
    return new ConvertToYV12(clip,args[1].AsBool(false),env);

  return ConvertToPlanarGeneric::CreateYUV420(args, NULL,env);
}

/**********************************
******  Bitdepth conversions  *****
**********************************/

template<uint8_t sourcebits>
static void convert_rgb_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    src_pitch = src_pitch / sizeof(uint16_t);
    int src_width = src_rowsize / sizeof(uint16_t);
    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            // test
            if(sourcebits==16)
              dstp[x] = srcp0[x] / 257; // RGB: full range 0..255 <-> 0..65535 (*255 / 65535)
              // hint for simd code writers:
              // compilers are smart. Some divisions near 2^n can be performed by tricky multiplication
              // such as x/257
              // 32 bit (x) * 0xFF00FF01 = edx_eax
              // Result of /257 is in: (edx>>8) and &FF !!
              //   movzx  edx, WORD PTR [esi+ecx*2]
              //   mov       eax, -16711935                                                ; ff00ff01H
              //   mul       edx
              //   shr       edx, 8
              //   mov       BYTE PTR [ecx+edi], dl
            else if (sourcebits==14)
                dstp[x] = srcp0[x] * 255 / 16383; // RGB: full range 0..255 <-> 0..16384-1
            /*
            movzx	eax, WORD PTR [edi+ecx*2]
            imul	esi, eax, 255
            mov	eax, -2147352567			; 80020009H
            imul	esi
            add	edx, esi
            sar	edx, 13					; 0000000dH
            mov	eax, edx
            shr	eax, 31					; 0000001fH
            add	eax, edx
            mov	BYTE PTR [ecx+ebx], al
            */
            /*
            and w/o mul 255: byte_y = uint16_t_x/16383:
            movzx	ebx, WORD PTR [esi+ecx*2]
            mov	eax, 262161				; 00040011H
            mul	ebx
            sub	ebx, edx
            shr	ebx, 1
            add	ebx, edx
            shr	ebx, 13					; 0000000dH
            mov	BYTE PTR [ecx+edi], bl
            */
            else if (sourcebits==12)
                dstp[x] = srcp0[x] * 255 / 4095; // RGB: full range 0..255 <-> 0..4096-1
            /*
            movzx	eax, WORD PTR [edi+ecx*2]
            imul	esi, eax, 255
            mov	eax, -2146959231			; 80080081H
            imul	esi
            add	edx, esi
            sar	edx, 11					; 0000000bH
            mov	eax, edx
            shr	eax, 31					; 0000001fH
            add	eax, edx
            mov	BYTE PTR [ecx+ebx], al
            */
            else if (sourcebits==10)
                dstp[x] = srcp0[x] * 255 / 1023; // RGB: full range 0..255 <-> 0..1024-1
            /*
            movzx	eax, WORD PTR [edi+ecx*2]
            imul	esi, eax, 255
            mov	eax, -2145384445			; 80200803H
            imul	esi
            add	edx, esi
            sar	edx, 9
            mov	eax, edx
            shr	eax, 31					; 0000001fH
            add	eax, edx
            mov	BYTE PTR [ecx+ebx], al
            */
        }
        dstp += dst_pitch;
        srcp0 += src_pitch;
    }
}

// YUV conversions (bit shifts)
// BitDepthConvFuncPtr
// Conversion from 16-14-12-10 to 8 bits (bitshift: 8-6-4-2)
template<uint8_t sourcebits>
static void convert_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);
  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      dstp[x] = srcp0[x] >> (sourcebits-8); // no dithering, no range conversion, simply shift
    }
    dstp += dst_pitch;
    srcp0 += src_pitch;
  }
}

template<uint8_t sourcebits>
static void convert_uint16_to_8_sse2(const BYTE *srcp8, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  const uint16_t *srcp = reinterpret_cast<const uint16_t *>(srcp8);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);
  int wmod16 = (src_width / 16) * 16;

  __m128i zero = _mm_setzero_si128();
  // no dithering, no range conversion, simply shift
  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x+=16)
    {
      __m128i src_lo = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x)); // 8* uint16
      __m128i src_hi = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x+8));
      src_lo = _mm_srli_epi16(src_lo, (sourcebits - 8));
      src_hi = _mm_srli_epi16(src_hi, (sourcebits - 8));
      __m128i dst = _mm_packus_epi16(src_lo, src_hi);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), dst);
    }
    // rest
    for (int x = wmod16; x < src_width; x++)
    {
      dstp[x] = srcp[x] >> (sourcebits-8);
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

// float to 8 bit, float to 10/12/14/16 bit
template<typename pixel_t, uint8_t targetbits>
static void convert_32_to_uintN_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  const float *srcp0 = reinterpret_cast<const float *>(srcp);
  pixel_t *dstp0 = reinterpret_cast<pixel_t *>(dstp);

  src_pitch = src_pitch / sizeof(float);
  dst_pitch = dst_pitch / sizeof(pixel_t);

  int src_width = src_rowsize / sizeof(float);

  float max_dst_pixelvalue = (float)((1<<targetbits) - 1); // 255, 1023, 4095, 16383, 65535.0

  float factor = 1.0f / float_range * max_dst_pixelvalue;

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel = srcp0[x] * factor + 0.5f; // 0.5f: keep the neutral grey level of float 0.5
      dstp0[x] = pixel_t(clamp(pixel, 0.0f, max_dst_pixelvalue)); // we clamp here!
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

// rgb: full scale. Difference to YUV: *257 instead of << 8 (full 16 bit sample)
template<uint8_t targetbits>
static void convert_rgb_8_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
    const uint8_t *srcp0 = reinterpret_cast<const uint8_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint8_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    int src_width = src_rowsize / sizeof(uint8_t);

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            // test
            if(targetbits==16)
                dstp0[x] = srcp0[x] * 257; // RGB: full range 0..255 <-> 0..65535 (257 = 65535 / 255)
            else if (targetbits==14)
                dstp0[x] = srcp0[x] * 16383 / 255; // RGB: full range 0..255 <-> 0..16384-1
            else if (targetbits==12)
                dstp0[x] = srcp0[x] * 4095 / 255; // RGB: full range 0..255 <-> 0..4096-1
            else if (targetbits==10)
                dstp0[x] = srcp0[x] * 1023 / 255; // RGB: full range 0..255 <-> 0..1024-1
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

// YUV: bit shift 8 to 10-12-14-16 bits
template<uint8_t targetbits>
static void convert_8_to_uint16_c(const BYTE *srcp, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  dst_pitch = dst_pitch / sizeof(uint16_t);

  int src_width = src_rowsize / sizeof(uint8_t); // intentional

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
        dstp[x] = srcp[x] << (targetbits-8);
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

template<uint8_t targetbits>
static void convert_8_to_uint16_sse2(const BYTE *srcp, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  dst_pitch = dst_pitch / sizeof(uint16_t);

  int src_width = src_rowsize / sizeof(uint8_t);
  int wmod16 = (src_width / 16) * 16;

  __m128i zero = _mm_setzero_si128();

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x+=16)
    {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x)); // 16 bytes
      __m128i dst_lo = _mm_unpacklo_epi8(src, zero);
      __m128i dst_hi = _mm_unpackhi_epi8(src, zero);
      dst_lo = _mm_slli_epi16(dst_lo, (targetbits - 8));
      dst_hi = _mm_slli_epi16(dst_hi, (targetbits - 8));
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), dst_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x + 8), dst_hi);
    }
    // rest
    for (int x = wmod16; x < src_width; x++)
    {
      dstp[x] = srcp[x] << (targetbits-8);
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}


// RGB full range: 10-12-14 <=> 16 bits
template<uint8_t sourcebits, uint8_t targetbits>
static void convert_rgb_uint16_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint16_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    const int src_width = src_rowsize / sizeof(uint16_t);

    const uint16_t source_max = (1 << sourcebits) - 1;
    const uint16_t target_max = (1 << targetbits) - 1;

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            // int64: avoid unsigned * unsigned = signed arithmetic overflow
            if(sourcebits<targetbits) {
                dstp0[x] = (uint16_t)((int64_t)srcp0[x] * target_max / source_max); // expand range
            } else {
                dstp0[x] = (uint16_t)((int64_t)srcp0[x] * source_max / target_max); // reduce range
            }
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

// YUV: bit shift 10-12-14-16 <=> 16 bits
template<bool expandrange, uint8_t shiftbits>
static void convert_uint16_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint16_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    const int src_width = src_rowsize / sizeof(uint16_t);

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            if(expandrange)
                dstp0[x] = srcp0[x] << shiftbits;  // expand range. No clamp before, source is assumed to have valid range
            else
                dstp0[x] = srcp0[x] >> shiftbits;  // reduce range
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

// 8 bit to float, 16/14/12/10 bits to float
template<typename pixel_t, uint8_t sourcebits>
static void convert_uintN_to_float_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range)
{
  const pixel_t *srcp0 = reinterpret_cast<const pixel_t *>(srcp);
  float *dstp0 = reinterpret_cast<float *>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t);
  dst_pitch = dst_pitch / sizeof(float);

  int src_width = src_rowsize / sizeof(pixel_t);

  float max_src_pixelvalue = (float)((1<<sourcebits) - 1); // 255, 1023, 4095, 16383, 65535.0

  // 0..255,65535 -> 0..float_range

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      dstp0[x] = srcp0[x] / max_src_pixelvalue * float_range; //  or lookup
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

ConvertTo8bit::ConvertTo8bit(PClip _child, const float _float_range, const int _dither_mode, const int _bitdepth, const int _truerange, IScriptEnvironment* env) :
    GenericVideoFilter(_child), float_range(_float_range), dither_mode(_dither_mode), source_bitdepth(_bitdepth), truerange(_truerange)
{
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2); // frames are always 16 bit aligned

  if (vi.ComponentSize() == 2) // 16(,14,12,10)->8 bit
  {
      // for RGB scaling is not shift by 8 as in YUV but 0..65535->0..255
      if (vi.IsRGB48() || vi.IsRGB64())
          conv_function = convert_rgb_uint16_to_8_c<16>;
      else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
          if(truerange) {
              switch(source_bitdepth)
              {
              case 10: conv_function = convert_rgb_uint16_to_8_c<10>; break;
              case 12: conv_function = convert_rgb_uint16_to_8_c<12>; break;
              case 14: conv_function = convert_rgb_uint16_to_8_c<14>; break;
              case 16: conv_function = convert_rgb_uint16_to_8_c<16>; break;
              default: env->ThrowError("ConvertTo8bit: invalid source bitdepth");
              }
          } else {
              conv_function = convert_rgb_uint16_to_8_c<16>;
          }
      } else if (vi.IsYUV() || vi.IsYUVA())
      {
          if(truerange) {
              switch(source_bitdepth)
              {
              case 10: conv_function = sse2 ? convert_uint16_to_8_sse2<10> : convert_uint16_to_8_c<10>; break;
              case 12: conv_function = sse2 ? convert_uint16_to_8_sse2<12> : convert_uint16_to_8_c<12>; break;
              case 14: conv_function = sse2 ? convert_uint16_to_8_sse2<14> : convert_uint16_to_8_c<14>; break;
              case 16: conv_function = sse2 ? convert_uint16_to_8_sse2<16> : convert_uint16_to_8_c<16>; break;
              default: env->ThrowError("ConvertTo8bit: invalid source bitdepth");
              }
          } else {
              conv_function = sse2 ? convert_uint16_to_8_sse2<16> : convert_uint16_to_8_c<16>; // always convert from 16 bit scale
          }
      } else
          env->ThrowError("ConvertTo8bit: unsupported color space");
  } else if (vi.ComponentSize() == 4) // 32->8 bit
  {
    conv_function = convert_32_to_uintN_c<uint8_t, 8>;
  } else
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
}


AVSValue __cdecl ConvertTo8bit::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  const VideoInfo &vi = clip->GetVideoInfo();

  // c[truerange]b[dither]i[scale]f,

  if (!vi.IsPlanar() && !vi.IsRGB())
    env->ThrowError("ConvertTo8bit: Can only convert from Planar YUV/RGB or packed RGB.");

  if (vi.ComponentSize() == 1)
    return clip; // 8 bit -> 8 bit: no conversion

  if (vi.ComponentSize() != 4 && args[1].Defined())
    env->ThrowError("ConvertTo8bit: Float range parameter for non float source");

  // float range parameter
  float float_range = (float)args[1].AsFloat(1.0f);

  if (vi.ComponentSize() == 4) {
    if(float_range<=0.0)
      env->ThrowError("ConvertTo8bit: Float range parameter cannot be <= 0");
    // other checkings
  }

  // dither parameter rfu
  int dither_type = args[2].AsInt(-1);

  if ((!vi.IsPlanar() || vi.ComponentSize() != 2) && args[3].Defined())
      env->ThrowError("ConvertTo8bit: truerange specified for non-16bit or non-planar source");

  int source_bitdepth = 16; // n/a
  if (vi.IsPlanar() && vi.ComponentSize() == 2)
      source_bitdepth = vi.BitsPerComponent();

  // when converting from 10-16 bit formats, truerange=false indicates bitdepth of 16 bits regardless of the 10-12-14 bit format
  int assume_truerange = args[3].AsBool(true); // n/a for non planar formats

  return new ConvertTo8bit(clip, float_range, dither_type, source_bitdepth, assume_truerange, env);
}

PVideoFrame __stdcall ConvertTo8bit::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  if(vi.IsPlanar())
  {
    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    for (int p = 0; p < vi.NumComponents(); ++p) {
        const int plane = planes[p];
        conv_function(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane), float_range /*, dither_mode */);
    }
  }
  else {
      // packed RGBs
      conv_function(src->GetReadPtr(), dst->GetWritePtr(),
          src->GetRowSize(), src->GetHeight(),
          src->GetPitch(), dst->GetPitch(), float_range /*, dither_mode */);
  }

  return dst;
}

// Conversion to uint16_t targets
// planar YUV(A) and RGB(A):
// from 8 bit -> 10/12/14/16 with strict range expansion or expansion to 16
// from 10/12/14 -> 16 bit with strict source range (expansion from 10/12/14 to 16 bit) or just casting pixel_type
// from 16 bit -> 10/12/14 bit with strict target range (reducing range from 16 bit to 10/12/14 bits) or just casting pixel_type
// from float -> 10/12/14/16 with strict range expansion or expansion to 16
// packed RGB:
//   RGB24->RGB48, RGB32->RGB64
ConvertTo16bit::ConvertTo16bit(PClip _child, const float _float_range, const int _dither_mode, const int _source_bitdepth, const int _target_bitdepth, bool _truerange, IScriptEnvironment* env) :
  GenericVideoFilter(_child), float_range(_float_range), dither_mode(_dither_mode), source_bitdepth(_source_bitdepth), target_bitdepth(_target_bitdepth), truerange(_truerange)
{
    change_only_format = false;

    bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2); // frames are always 16 bit aligned

    if (vi.ComponentSize() == 1) // 8->10-12-14-16 bit
    {
        // RGB scaling is not shift by 8 as in YUV but like 0..255->0..65535
        if (vi.IsRGB24() || vi.IsRGB32())
            conv_function = convert_rgb_8_to_uint16_c<16>;
        else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
            if (truerange)
            {
                switch (target_bitdepth)
                {
                case 10: conv_function = convert_rgb_8_to_uint16_c<10>; break;
                case 12: conv_function = convert_rgb_8_to_uint16_c<12>; break;
                case 14: conv_function = convert_rgb_8_to_uint16_c<14>; break;
                case 16: conv_function = convert_rgb_8_to_uint16_c<16>; break;
                default: env->ThrowError("ConvertTo16bit: unsupported bit depth");
                }
            }
            else {
                conv_function = convert_rgb_8_to_uint16_c<16>;
            }
        }
        else if (vi.IsYUV() || vi.IsYUVA()) {
            if (truerange)
            {
                switch (target_bitdepth)
                {
                case 10: conv_function = sse2 ? convert_8_to_uint16_sse2<10> : convert_8_to_uint16_c<10>; break;
                case 12: conv_function = sse2 ? convert_8_to_uint16_sse2<12> : convert_8_to_uint16_c<12>; break;
                case 14: conv_function = sse2 ? convert_8_to_uint16_sse2<14> : convert_8_to_uint16_c<14>; break;
                case 16: conv_function = sse2 ? convert_8_to_uint16_sse2<16> : convert_8_to_uint16_c<16>; break;
                default: env->ThrowError("ConvertTo16bit: unsupported bit depth");
                }
            }
            else {
                conv_function = sse2 ? convert_8_to_uint16_sse2<16> : convert_8_to_uint16_c<16>; // always 16 bit scale
            }
        }
        else
            env->ThrowError("ConvertTo16bit: unsupported color space");
    }
    else if (vi.ComponentSize() == 2)
    {
        // 10/12/14 -> 16 bit or 16 bit -> 10/12/14 bit
        // range reducing or expansion (truerange=true), or just overriding the pixel_type, keeping scale at 16 bits
        if (truerange) {
            // invalid combinations were already checked
            if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
                if (source_bitdepth > target_bitdepth) // reduce range
                {
                    if (source_bitdepth == 16) // 16->10/12/14 keep full range
                        switch (target_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<16, 10>; break;
                        case 12: conv_function = convert_rgb_uint16_to_uint16_c<16, 12>; break;
                        case 14: conv_function = convert_rgb_uint16_to_uint16_c<16, 14>; break;
                        }
                    else if (source_bitdepth == 14) // 14->10/12 keep full range
                        switch (target_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<14, 10>; break;
                        case 12: conv_function = convert_rgb_uint16_to_uint16_c<14, 12>; break;
                        }
                    else if (source_bitdepth == 12) // 14->10/12 keep full range
                        switch (target_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<12, 10>; break;
                        }
                } else {// expand
                    if (target_bitdepth == 16) // 10/12/14->16 keep full range
                        switch (source_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<10, 16>; break;
                        case 12: conv_function = convert_rgb_uint16_to_uint16_c<12, 16>; break;
                        case 14: conv_function = convert_rgb_uint16_to_uint16_c<14, 16>; break;
                        }
                    else if (target_bitdepth == 14) // 10/12->14 keep full range
                        switch (source_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<10, 14>; break;
                        case 12: conv_function = convert_rgb_uint16_to_uint16_c<12, 14>; break;
                        }
                    else if (target_bitdepth == 12) // 10->12 keep full range
                        switch (source_bitdepth)
                        {
                        case 10: conv_function = convert_rgb_uint16_to_uint16_c<10, 12>; break;
                        }
                }
            }
            else if (vi.IsYUV() || vi.IsYUVA()) {
                if (source_bitdepth > target_bitdepth) // reduce range 16->14/12/10 14->12/10 12->10. template: bitshift
                    switch (source_bitdepth - target_bitdepth)
                    {
                    case 2: conv_function = convert_uint16_to_uint16_c<false, 2>; break;
                    case 4: conv_function = convert_uint16_to_uint16_c<false, 4>; break;
                    case 6: conv_function = convert_uint16_to_uint16_c<false, 6>; break;
                    }
                else // expand range
                    switch (target_bitdepth - source_bitdepth)
                    {
                    case 2: conv_function = convert_uint16_to_uint16_c<true, 2>; break;
                    case 4: conv_function = convert_uint16_to_uint16_c<true, 4>; break;
                    case 6: conv_function = convert_uint16_to_uint16_c<true, 6>; break;
                    }
            }
        }
        else { // truerange==false
            change_only_format = true;
        }
    }
    else if (vi.ComponentSize() == 4) // 32->16 bit
    {
        if (truerange) {
            switch(target_bitdepth)
            {
            case 10: conv_function = convert_32_to_uintN_c<uint16_t, 10>; break;
            case 12: conv_function = convert_32_to_uintN_c<uint16_t, 12>; break;
            case 14: conv_function = convert_32_to_uintN_c<uint16_t, 14>; break;
            case 16: conv_function = convert_32_to_uintN_c<uint16_t, 16>; break;
            }
        } else {
            conv_function = convert_32_to_uintN_c<uint16_t, 16>;
        }
    } else
        env->ThrowError("ConvertTo16bit: unsupported bit depth");

  if (vi.NumComponents() == 1) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = VideoInfo::CS_Y10; break;
      case 12: vi.pixel_type = VideoInfo::CS_Y12; break;
      case 14: vi.pixel_type = VideoInfo::CS_Y14; break;
      case 16: vi.pixel_type = VideoInfo::CS_Y16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if (vi.Is420()) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420P10 : VideoInfo::CS_YUV420P10; break;
      case 12: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420P12 : VideoInfo::CS_YUV420P12; break;
      case 14: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420P14 : VideoInfo::CS_YUV420P14; break;
      case 16: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420P16 : VideoInfo::CS_YUV420P16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if (vi.Is422()) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422P10 : VideoInfo::CS_YUV422P10; break;
      case 12: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422P12 : VideoInfo::CS_YUV422P12; break;
      case 14: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422P14 : VideoInfo::CS_YUV422P14; break;
      case 16: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422P16 : VideoInfo::CS_YUV422P16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if (vi.Is444()) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444P10 : VideoInfo::CS_YUV444P10; break;
      case 12: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444P12 : VideoInfo::CS_YUV444P12; break;
      case 14: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444P14 : VideoInfo::CS_YUV444P14; break;
      case 16: vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444P16 : VideoInfo::CS_YUV444P16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if (vi.IsPlanarRGB()) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = VideoInfo::CS_RGBP10; break;
      case 12: vi.pixel_type = VideoInfo::CS_RGBP12; break;
      case 14: vi.pixel_type = VideoInfo::CS_RGBP14; break;
      case 16: vi.pixel_type = VideoInfo::CS_RGBP16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if (vi.IsPlanarRGBA()) {
      switch(target_bitdepth)
      {
      case 10: vi.pixel_type = VideoInfo::CS_RGBAP10; break;
      case 12: vi.pixel_type = VideoInfo::CS_RGBAP12; break;
      case 14: vi.pixel_type = VideoInfo::CS_RGBAP14; break;
      case 16: vi.pixel_type = VideoInfo::CS_RGBAP16; break;
      default:
          env->ThrowError("ConvertTo16bit: unsupported effective bit depth");
      }
  } else if(vi.IsRGB24()) {
      if(target_bitdepth == 16)
          vi.pixel_type = VideoInfo::CS_BGR48;
      else
          env->ThrowError("ConvertTo16bit: unsupported bit depth");
  } else if(vi.IsRGB32()) {
      if(target_bitdepth == 16)
          vi.pixel_type = VideoInfo::CS_BGR64;
      else
          env->ThrowError("ConvertTo16bit: unsupported bit depth");
  } else
    env->ThrowError("ConvertTo16bit: unsupported color space");
}


AVSValue __cdecl ConvertTo16bit::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  //0   1        2        3         4
  //c[bits]i[truerange]b[dither]i[scale]f

  const VideoInfo &vi = clip->GetVideoInfo();

  if (!vi.IsPlanar() && !vi.IsRGB24() && !vi.IsRGB32())
      env->ThrowError("ConvertTo16bit: Can only convert from Planar YUV/RGB or packed RGB.");
  if (vi.ComponentSize() != 4 && args[4].Defined())
      env->ThrowError("ConvertTo16bit: Float range parameter not allowed for non float source");
  // float range parameter
  float float_range = (float)args[4].AsFloat(1.0f);

  // when converting from/true 10-16 bit formats, truerange=false indicates bitdepth of 16 bits regardless of the 10-12-14 bit format
  bool assume_truerange = args[2].AsBool(true); // n/a for non planar formats
  int target_bitdepth = args[1].AsInt(16); // default: 16 bit. can override with 10/12/14 bits
  int source_bitdepth = vi.BitsPerComponent();

  if(target_bitdepth!=10 && target_bitdepth!=12 && target_bitdepth!=14 && target_bitdepth!=16)
      env->ThrowError("ConvertTo16bit: invalid bit depth");

  if (!vi.IsPlanar() && args[2].Defined())
      env->ThrowError("ConvertTo16bit: truerange specified for non-planar source");

  if (vi.IsRGB24() || vi.IsRGB32()) {
      if (target_bitdepth != 16)
          env->ThrowError("ConvertTo16bit: only 16 bit allowed for packed RGB");
  }

  // 10/12/14/16 -> 10/12/14/16
  if (vi.ComponentSize() == 2)
  {
      if((source_bitdepth == target_bitdepth) && assume_truerange) // 10->10 .. 16->16
        return clip;
      // source_10_bit.ConvertTo16bit(truerange=true)  : upscale range
      // source_10_bit.ConvertTo16bit(truerange=false) : leaves data, only format conversion
      // source_10_bit.ConvertTo16bit(bits=12,truerange=true)  : upscale range from 10 to 12
      // source_10_bit.ConvertTo16bit(bits=12,truerange=false) : leaves data, only format conversion
      // source_16_bit.ConvertTo16bit(bits=10, truerange=true)  : downscale range
      // source_16_bit.ConvertTo16bit(bits=10, truerange=false) : leaves data, only format conversion
  }

  if (vi.ComponentSize() == 4) {
    if(float_range<=0.0)
      env->ThrowError("ConvertTo16bit: Float range parameter cannot be <= 0");
    // other checkings
  }

  // dither parameter, rfu
  int dither_type = args[3].AsInt(-1);

  return new ConvertTo16bit(clip, float_range, dither_type, source_bitdepth, target_bitdepth, assume_truerange, env);
}

PVideoFrame __stdcall ConvertTo16bit::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  if (change_only_format) {
      // only vi changed, all planes are copied unmodified
      int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
      int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
      int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
      for (int p = 0; p < vi.NumComponents(); ++p) {
          const int plane = planes[p];
          env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
      }
      return dst;
  }
  if(vi.IsPlanar())
  {
      int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
      int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
      int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
      for (int p = 0; p < vi.NumComponents(); ++p) {
          const int plane = planes[p];
          conv_function(src->GetReadPtr(plane), dst->GetWritePtr(plane),
              src->GetRowSize(plane), src->GetHeight(plane),
              src->GetPitch(plane), dst->GetPitch(plane), float_range /*, dither_mode */);
      }
  }
  else {
      // packed RGBs
      conv_function(src->GetReadPtr(), dst->GetWritePtr(),
          src->GetRowSize(), src->GetHeight(),
          src->GetPitch(), dst->GetPitch(), float_range /*, dither_mode */);
  }

  return dst;
}


// float 32 bit
ConvertToFloat::ConvertToFloat(PClip _child, const float _float_range, const int _source_bitdepth, bool _truerange, IScriptEnvironment* env) :
  GenericVideoFilter(_child), float_range(_float_range), source_bitdepth(_source_bitdepth), truerange(_truerange)
{

  if (vi.ComponentSize() == 1) // 8->32 bit
  {
    conv_function = convert_uintN_to_float_c<uint8_t, 8>;
  } else if (vi.ComponentSize() == 2) // 16->32 bit
  {
      if (vi.IsPlanar() && truerange)
      {
          switch (source_bitdepth)
          {
          case 10: conv_function = convert_uintN_to_float_c<uint16_t, 10>; break;
          case 12: conv_function = convert_uintN_to_float_c<uint16_t, 12>; break;
          case 14: conv_function = convert_uintN_to_float_c<uint16_t, 14>; break;
          case 16: conv_function = convert_uintN_to_float_c<uint16_t, 16>; break;
          default: env->ThrowError("ConvertToFloat: unsupported bit depth");
          }
      } else {
        conv_function = convert_uintN_to_float_c<uint16_t, 16>;
      }
  } else
    env->ThrowError("ConvertToFloat: unsupported bit depth");

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
}


AVSValue __cdecl ConvertToFloat::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  const VideoInfo &vi = clip->GetVideoInfo();
  //0    1         2
  //c[truerange]b[scale]f

  if (!vi.IsPlanar())
    env->ThrowError("ConvertToFloat: Can only convert from Planar YUV(A) or RGB(A).");

  if (vi.ComponentSize() == 4)
    return clip; // 32 bit -> 32 bit: no conversion

  // float range parameter
  float float_range = (float)args[2].AsFloat(1.0f);

  if(float_range<=0.0)
      env->ThrowError("ConvertToFloat: Float range parameter cannot be <= 0");

  bool assume_truerange = args[1].AsBool(true); // n/a for non planar formats
  int source_bitdepth = vi.BitsPerComponent();

  if (vi.ComponentSize() != 2 && args[1].Defined())
      env->ThrowError("ConvertToFloat: truerange specified for 8 bit source");

  return new ConvertToFloat(clip, float_range, source_bitdepth, assume_truerange, env);
}

PVideoFrame __stdcall ConvertToFloat::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
  int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
  int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
  for (int p = 0; p < vi.NumComponents(); ++p) {
    const int plane = planes[p];
    conv_function(src->GetReadPtr(plane), dst->GetWritePtr(plane),
      src->GetRowSize(plane), src->GetHeight(plane),
      src->GetPitch(plane), dst->GetPitch(plane), float_range /*, dither_mode */);
  }

  return dst;
}





