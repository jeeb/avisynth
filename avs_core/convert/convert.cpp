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
#include "convert_rgb.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include "convert_planar.h"
#include "avs/alignment.h"
#include <cstdlib>
#include <emmintrin.h>



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Convert_filters[] = {       // matrix can be "rec601", rec709", "PC.601" or "PC.709"
  { "ConvertToRGB",   "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create },
  { "ConvertToRGB24", "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create32 },
  { "ConvertToY8",    "c[matrix]s", ConvertToY8::Create },
  { "ConvertToYV12",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToYV12::Create },
  { "ConvertToYV24",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV24},
  { "ConvertToYV16",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV16},
  { "ConvertToYV411", "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411},
  { "ConvertToYUY2",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToYUY2::Create },
  { "ConvertBackToYUY2", "c[matrix]s", ConvertBackToYUY2::Create },
  { 0 }
};

const int crv_rec601 = int(1.596*65536+0.5);
const int cgv_rec601 = int(0.813*65536+0.5);
const int cgu_rec601 = int(0.391*65536+0.5);
const int cbu_rec601 = int(2.018*65536+0.5);

const int crv_rec709 = int(1.793*65536+0.5); 
const int cgv_rec709 = int(0.533*65536+0.5);
const int cgu_rec709 = int(0.213*65536+0.5);
const int cbu_rec709 = int(2.112*65536+0.5);

const int crv_pc601 = int(1.407*65536+0.5);
const int cgv_pc601 = int(0.717*65536+0.5);
const int cgu_pc601 = int(0.345*65536+0.5); 
const int cbu_pc601 = int(1.779*65536+0.5);

const int crv_pc709 = int(1.581*65536+0.5);
const int cgv_pc709 = int(0.470*65536+0.5);
const int cgu_pc709 = int(0.188*65536+0.5);
const int cbu_pc709 = int(1.863*65536+0.5);

const int cy_rec = int((255.0/219.0)*65536+0.5);
const int cy_pc = 65536;

const int crv_values[4] = { crv_rec601, crv_rec709, crv_pc601, crv_pc709 };
const int cgv_values[4] = { cgv_rec601, cgv_rec709, cgv_pc601, cgv_pc709 };
const int cgu_values[4] = { cgu_rec601, cgu_rec709, cgu_pc601, cgu_pc709 };
const int cbu_values[4] = { cbu_rec601, cbu_rec709, cbu_pc601, cbu_pc709 };
const int cy_values[4]  = { cy_rec,     cy_rec,     cy_pc,     cy_pc};


/****************************************
*******   Convert to RGB / RGBA   ******
***************************************/

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
  __m64 low_pixel_mask = _mm_set_pi32(0, 0x00FFFFFF);
  __m64 high_pixel_mask = _mm_set_pi32(0x00FFFFFF, 0);

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
      __m64 src_chroma2 = _mm_shuffle_pi16(src_chroma, _MM_SHUFFLE(3, 2, 3, 2)); //00 V0 00 U0 00 V0 00 U0

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


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();

  PVideoFrame dst = env->NewVideoFrame(vi);
  const int dst_pitch = dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  int tv_scale = theMatrix == Rec601 || theMatrix == Rec709 ? 16 : 0;


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


AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 4 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);

  return clip;
}


AVSValue __cdecl ConvertToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 4 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB32: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);

  if (vi.IsRGB24())
    return new RGB24to32(clip);

  return clip;
}


AVSValue __cdecl ConvertToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 3 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB24: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, true, matrix, env);

  if (vi.IsRGB32())
    return new RGB32to24(clip);

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

  return ConvertToPlanarGeneric::CreateYV12(args,0,env);
}





