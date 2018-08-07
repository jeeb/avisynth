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



// Avisynth filter: Layer
// by "poptones" (poptones@myrealbox.com)

#include "layer.h"
#include <avs/win.h>
#include <avs/minmax.h>
#include <avs/alignment.h>
#include "../core/internal.h"
#include <emmintrin.h>
#include "../convert/convert_planar.h"
#include <algorithm>


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Layer_filters[] = {
  { "Mask",         BUILTIN_FUNC_PREFIX, "cc", Mask::Create },     // clip, mask
  { "ColorKeyMask", BUILTIN_FUNC_PREFIX, "ci[]i[]i[]i", ColorKeyMask::Create },    // clip, color, tolerance[B, toleranceG, toleranceR]
  { "ResetMask",    BUILTIN_FUNC_PREFIX, "c[mask]f", ResetMask::Create },
  { "Invert",       BUILTIN_FUNC_PREFIX, "c[channels]s", Invert::Create },
  { "ShowAlpha",    BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)3 }, // AVS+ also for YUVA, PRGBA
  { "ShowRed",      BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)2 },
  { "ShowGreen",    BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)1 },
  { "ShowBlue",     BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)0 },
  { "ShowY",        BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)4 }, // AVS+
  { "ShowU",        BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)5 }, // AVS+
  { "ShowV",        BUILTIN_FUNC_PREFIX, "c[pixel_type]s", ShowChannel::Create, (void*)6 }, // AVS+
  { "MergeRGB",     BUILTIN_FUNC_PREFIX, "ccc[pixel_type]s", MergeRGB::Create, (void*)0 },
  { "MergeARGB",    BUILTIN_FUNC_PREFIX, "cccc",             MergeRGB::Create, (void*)1 },
  { "Layer",        BUILTIN_FUNC_PREFIX, "cc[op]s[level]i[x]i[y]i[threshold]i[use_chroma]b", Layer::Create },
  /**
    * Layer(clip, overlayclip, operation, amount, xpos, ypos, [threshold=0], [use_chroma=true])
   **/
  { "Subtract", BUILTIN_FUNC_PREFIX, "cc", Subtract::Create },
  { NULL }
};


/******************************
 *******   Mask Filter   ******
 ******************************/

Mask::Mask(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : child1(_child1), child2(_child2)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Mask error: image dimensions don't match");
  if (! ((vi1.IsRGB32() && vi2.IsRGB32()) ||
        (vi1.IsRGB64() && vi2.IsRGB64()) ||
        (vi1.IsPlanarRGBA() && vi2.IsPlanarRGBA()))
    )
    env->ThrowError("Mask error: sources must be RGB32, RGB64 or Planar RGBA");

  if(vi1.BitsPerComponent() != vi2.BitsPerComponent())
    env->ThrowError("Mask error: Components are not of the same bit depths");

  vi = vi1;

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  mask_frames = vi2.num_frames;
}

static __forceinline __m128i mask_core_sse2(__m128i &src, __m128i &alpha, __m128i &not_alpha_mask, __m128i &zero, __m128i &matrix, __m128i &round_mask) {
  __m128i not_alpha = _mm_and_si128(src, not_alpha_mask);

  __m128i pixel0 = _mm_unpacklo_epi8(alpha, zero); 
  __m128i pixel1 = _mm_unpackhi_epi8(alpha, zero);

  pixel0 = _mm_madd_epi16(pixel0, matrix); 
  pixel1 = _mm_madd_epi16(pixel1, matrix); 

  __m128i tmp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel0), _mm_castsi128_ps(pixel1), _MM_SHUFFLE(3, 1, 3, 1)));
  __m128i tmp2 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel0), _mm_castsi128_ps(pixel1), _MM_SHUFFLE(2, 0, 2, 0)));

  tmp = _mm_add_epi32(tmp, tmp2);
  tmp = _mm_add_epi32(tmp, round_mask); 
  tmp = _mm_srli_epi32(tmp, 15); 
  __m128i result_alpha = _mm_slli_epi32(tmp, 24);

  return _mm_or_si128(result_alpha, not_alpha);
}

static void mask_sse2(BYTE *srcp, const BYTE *alphap, int src_pitch, int alpha_pitch, size_t width, size_t height, int cyb, int cyg, int cyr) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i not_alpha_mask = _mm_set1_epi32(0x00FFFFFF);

  size_t width_bytes = width * 4;
  size_t width_mod16 = width_bytes / 16 * 16;

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width_mod16; x+=16) {
      __m128i src    = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x)); 
      __m128i alpha  = _mm_load_si128(reinterpret_cast<const __m128i*>(alphap+x)); 
      __m128i result = mask_core_sse2(src, alpha, not_alpha_mask, zero, matrix, round_mask);

      _mm_store_si128(reinterpret_cast<__m128i*>(srcp+x), result);
    }

    if (width_mod16 < width_bytes) {
      __m128i src    = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+width_bytes-16)); 
      __m128i alpha  = _mm_loadu_si128(reinterpret_cast<const __m128i*>(alphap+width_bytes-16));
      __m128i result = mask_core_sse2(src, alpha, not_alpha_mask, zero, matrix, round_mask);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(srcp+width_bytes-16), result);
    }

    srcp += src_pitch;
    alphap += alpha_pitch;
  }
}

#ifdef X86_32

static __forceinline __m64 mask_core_mmx(__m64 &src, __m64 &alpha, __m64 &not_alpha_mask, __m64 &zero, __m64 &matrix, __m64 &round_mask) {
  __m64 not_alpha = _mm_and_si64(src, not_alpha_mask);

  __m64 pixel0 = _mm_unpacklo_pi8(alpha, zero); 
  __m64 pixel1 = _mm_unpackhi_pi8(alpha, zero);

  pixel0 = _mm_madd_pi16(pixel0, matrix); //a0*0 + r0*cyr | g0*cyg + b0*cyb
  pixel1 = _mm_madd_pi16(pixel1, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb

  __m64 tmp = _mm_unpackhi_pi32(pixel0, pixel1); // r1*cyr | r0*cyr
  __m64 tmp2 = _mm_unpacklo_pi32(pixel0, pixel1); // g1*cyg + b1*cyb | g0*cyg + b0*cyb

  tmp = _mm_add_pi32(tmp, tmp2); // r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  tmp = _mm_add_pi32(tmp, round_mask); // r1*cyr + g1*cyg + b1*cyb + 16384 | r0*cyr + g0*cyg + b0*cyb + 16384
  tmp = _mm_srli_pi32(tmp, 15); // 0 0 0 p2 | 0 0 0 p1
  __m64 result_alpha = _mm_slli_pi32(tmp, 24);

  return _mm_or_si64(result_alpha, not_alpha);
}

static void mask_mmx(BYTE *srcp, const BYTE *alphap, int src_pitch, int alpha_pitch, size_t width, size_t height, int cyb, int cyg, int cyr) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 not_alpha_mask = _mm_set1_pi32(0x00FFFFFF);

  size_t width_bytes = width * 4;
  size_t width_mod8 = width_bytes / 8 * 8;

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width_mod8; x+=8) {
      __m64 src    = *reinterpret_cast<const __m64*>(srcp+x); //pixels 0 and 1
      __m64 alpha  = *reinterpret_cast<const __m64*>(alphap+x); 
      __m64 result = mask_core_mmx(src, alpha, not_alpha_mask, zero, matrix, round_mask);

      *reinterpret_cast<__m64*>(srcp+x) = result;
    }

    if (width_mod8 < width_bytes) {
      __m64 src    = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+width_bytes-4)); 
      __m64 alpha  = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(alphap+width_bytes-4)); 

      __m64 result = mask_core_mmx(src, alpha, not_alpha_mask, zero, matrix, round_mask);

      *reinterpret_cast<int*>(srcp+width_bytes-4) = _mm_cvtsi64_si32(result);
    }

    srcp += src_pitch;
    alphap += alpha_pitch;
  }
  _mm_empty();
}

#endif


template<typename pixel_t>
static void mask_c(BYTE *srcp8, const BYTE *alphap8, int src_pitch, int alpha_pitch, size_t width, size_t height, int cyb, int cyg, int cyr) {
  pixel_t *srcp = reinterpret_cast<pixel_t *>(srcp8);
  const pixel_t *alphap = reinterpret_cast<const pixel_t *>(alphap8);

  src_pitch /= sizeof(pixel_t);
  alpha_pitch /= sizeof(pixel_t);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      srcp[x*4+3] = (cyb*alphap[x*4+0] + cyg*alphap[x*4+1] + cyr*alphap[x*4+2] + 16384) >> 15;
    }
    srcp += src_pitch;
    alphap += alpha_pitch;
  }
}

template<typename pixel_t>
static void mask_planar_rgb_c(BYTE *dstp8, const BYTE *srcp_r8, const BYTE *srcp_g8, const BYTE *srcp_b8, int dst_pitch, int src_pitch, size_t width, size_t height, int cyb, int cyg, int cyr, int bits_per_pixel) {
  // worst case uint16: 65535 * 19235 + 65535 * 9798 + 65535 * 3736 + 16384 = 65535 * 32769 + 16384 = 2147532799 = 8000BFFF -> int32 fail
  //                    2147532799 >> 15 = 65537 = 0x10001, needs clamping :( !!!!!
  // worst case uint16: 65535 * (!!!19234) + 65535 * 9798 + 65535 * 3736 + 16384 = 65535 * 32768 + 16384 = 2147467264 = 7FFFC000 -> int32 OK
  //                    2147467264 >> 15 = 65535 no need clamping
  // worst case uint14: 16383*(19235+9798+3736) + 16384 = 16383*32769 + 16384 = 536870911 >> 15 = 16383 -> int is enough, and no clamping needed
  // worst case uint12: 4095*(19235+9798+3736) + 16384 = 4095*32769 + 16384 = 134205439 >> 15 = 4095 -> int is enough, and no clamping needed
  // worst case uint10: 1023*(19235+9798+3736) + 16384 = 1023*32769 + 16384 = 33539071 >> 15 = 1023 -> int is enough, and no clamping needed
  // worst case uint8 : 255*(19235+9798+3736) + 16384 = 255*32769 + 16384 = 8372479 >> 15 = 255 -> int is enough, and no clamping needed

  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *srcp_r = reinterpret_cast<const pixel_t *>(srcp_r8);
  const pixel_t *srcp_g = reinterpret_cast<const pixel_t *>(srcp_g8);
  const pixel_t *srcp_b = reinterpret_cast<const pixel_t *>(srcp_b8);
  src_pitch /= sizeof(pixel_t);
  dst_pitch /= sizeof(pixel_t);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      dstp[x] = ((cyb*srcp_b[x] + cyg*srcp_g[x] + cyr*srcp_r[x] + 16384) >> 15);
    }
    dstp += dst_pitch;
    srcp_r += src_pitch;
    srcp_g += src_pitch;
    srcp_b += src_pitch;
  }
}

static void mask_planar_rgb_float_c(BYTE *dstp8, const BYTE *srcp_r8, const BYTE *srcp_g8, const BYTE *srcp_b8, int dst_pitch, int src_pitch, size_t width, size_t height, float cyb_f, float cyg_f, float cyr_f) {

  float *dstp = reinterpret_cast<float *>(dstp8);
  const float *srcp_r = reinterpret_cast<const float *>(srcp_r8);
  const float *srcp_g = reinterpret_cast<const float *>(srcp_g8);
  const float *srcp_b = reinterpret_cast<const float *>(srcp_b8);
  src_pitch /= sizeof(float);
  dst_pitch /= sizeof(float);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      dstp[x] = cyb_f*srcp_b[x] + cyg_f*srcp_g[x] + cyr_f*srcp_r[x];
    }
    dstp += dst_pitch;
    srcp_r += src_pitch;
    srcp_g += src_pitch;
    srcp_b += src_pitch;
  }
}

PVideoFrame __stdcall Mask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(min(n,mask_frames-1), env);

  env->MakeWritable(&src1);

  // 15 bit scaled
  // PF check: int32 overflow in 16 bits
  // 32769 * 65535 + 16384 = 8000BFFF int32 overflow
  // 32768 * 65535 + 16384 = 7FFFC000 OK
  // Let's make correction
  const int cyb = 3736;  // int(0.114 * 32768 + 0.5); // 3736
  const int cyg = 19235-1; // int(0.587 * 32768 + 0.5); // 19235
  const int cyr = 9798;  // int(0.299 * 32768 + 0.5); // 9798
  // w/o correction: 32769

  if (vi.IsPlanar()) {
    // planar RGB
    const float cyb_f = 0.114f;
    const float cyg_f = 0.587f;
    const float cyr_f = 0.299f;

    BYTE* dstp = src1->GetWritePtr(PLANAR_A); // destination Alpha plane

    const BYTE* srcp_g = src2->GetReadPtr(PLANAR_G);
    const BYTE* srcp_b = src2->GetReadPtr(PLANAR_B);
    const BYTE* srcp_r = src2->GetReadPtr(PLANAR_R);

    const int dst_pitch = src1->GetPitch();
    const int src_pitch = src2->GetPitch();

    // clip1_alpha = greyscale(clip2)
    if (pixelsize == 1)
      mask_planar_rgb_c<uint8_t>(dstp, srcp_r, srcp_g, srcp_b, dst_pitch, src_pitch, vi.width, vi.height, cyb, cyg, cyr, bits_per_pixel);
    else if (pixelsize == 2)
      mask_planar_rgb_c<uint16_t>(dstp, srcp_r, srcp_g, srcp_b, dst_pitch, src_pitch, vi.width, vi.height, cyb, cyg, cyr, bits_per_pixel);
    else
      mask_planar_rgb_float_c(dstp, srcp_r, srcp_g, srcp_b, dst_pitch, src_pitch, vi.width, vi.height, cyb_f, cyg_f, cyr_f);
  } else {
    // Packed RGB32/64
    BYTE* src1p = src1->GetWritePtr();
    const BYTE* src2p = src2->GetReadPtr();

    const int src1_pitch = src1->GetPitch();
    const int src2_pitch = src2->GetPitch();

    // clip1_alpha = greyscale(clip2)
    if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
    {
      mask_sse2(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
    }
    else
  #ifdef X86_32
    if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_MMX))
    {
      mask_mmx(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
    }
    else
  #endif
    {
      if (pixelsize == 1) {
        mask_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
      } else { // if (pixelsize == 2)
        mask_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
      }
    }
  }

    return src1;
}

AVSValue __cdecl Mask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Mask(args[0].AsClip(), args[1].AsClip(), env);
}


/**************************************
 *******   ColorKeyMask Filter   ******
 **************************************/


ColorKeyMask::ColorKeyMask(PClip _child, int _color, int _tolB, int _tolG, int _tolR, IScriptEnvironment *env)
  : GenericVideoFilter(_child), color(_color & 0xffffff), tolB(_tolB & 0xff), tolG(_tolG & 0xff), tolR(_tolR & 0xff)
{
  if (!vi.IsRGB32() && !vi.IsRGB64() && !vi.IsPlanarRGBA())
    env->ThrowError("ColorKeyMask: requires RGB32, RGB64 or Planar RGBA input");
  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
  max_pixel_value = (1 << bits_per_pixel) - 1;

  auto rgbcolor8to16 = [](uint8_t color8, int max_pixel_value) { return (uint16_t)(color8 * max_pixel_value / 255); };

  uint64_t r = rgbcolor8to16((color >> 16) & 0xFF, max_pixel_value);
  uint64_t g = rgbcolor8to16((color >> 8 ) & 0xFF, max_pixel_value);
  uint64_t b = rgbcolor8to16((color      ) & 0xFF, max_pixel_value);
  uint64_t a = rgbcolor8to16((color >> 24) & 0xFF, max_pixel_value);
  color64 = (a << 48) + (r << 32) + (g << 16) + (b);
  tolR16 = rgbcolor8to16(tolR & 0xFF, max_pixel_value); // scale tolerance
  tolG16 = rgbcolor8to16(tolG & 0xFF, max_pixel_value);
  tolB16 = rgbcolor8to16(tolB & 0xFF, max_pixel_value);
}

static void colorkeymask_sse2(BYTE* pf, int pitch, int color, int height, int width, int tolB, int tolG, int tolR) {
  unsigned int t = 0xFF000000 | (tolR << 16) | (tolG << 8) | tolB;
  __m128i tolerance = _mm_set1_epi32(t);
  __m128i colorv = _mm_set1_epi32(color);
  __m128i zero = _mm_setzero_si128();

  BYTE* endp = pf + pitch * height;

  while (pf < endp)
  {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(pf));
    __m128i gt = _mm_subs_epu8(colorv, src); 
    __m128i lt = _mm_subs_epu8(src, colorv); 
    __m128i absdiff = _mm_or_si128(gt, lt); //abs(color - src)

    __m128i not_passed = _mm_subs_epu8(absdiff, tolerance);
    __m128i passed = _mm_cmpeq_epi32(not_passed, zero);
    passed = _mm_slli_epi32(passed, 24);
    __m128i result = _mm_andnot_si128(passed, src);

    _mm_store_si128(reinterpret_cast<__m128i*>(pf), result);

    pf += 16;
  }
}

#ifdef X86_32

static __forceinline __m64 colorkeymask_core_mmx(const __m64 &src, const __m64 &colorv, const __m64 &tolerance, const __m64 &zero) {
  __m64 gt = _mm_subs_pu8(colorv, src); 
  __m64 lt = _mm_subs_pu8(src, colorv); 
  __m64 absdiff = _mm_or_si64(gt, lt); //abs(color - src)

  __m64 not_passed = _mm_subs_pu8(absdiff, tolerance);
  __m64 passed = _mm_cmpeq_pi32(not_passed, zero);
  passed = _mm_slli_pi32(passed, 24);
  return _mm_andnot_si64(passed, src);
}

static void colorkeymask_mmx(BYTE* pf, int pitch, int color, int height, int width, int tolB, int tolG, int tolR) {
#pragma warning(push)
#pragma warning(disable: 4309)
  __m64 tolerance = _mm_set_pi8(0xFF, tolR, tolG, tolB, 0xFF, tolR, tolG, tolB);
#pragma warning(pop)
  __m64 colorv = _mm_set1_pi32(color);
  __m64 zero = _mm_setzero_si64();

  int mod8_width = width / 8 * 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod8_width; x += 8) {
      __m64 src = *reinterpret_cast<const __m64*>(pf + x);
      __m64 result = colorkeymask_core_mmx(src, colorv, tolerance, zero);
      *reinterpret_cast<__m64*>(pf + x) = result;
    }

    if (mod8_width != width) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(pf + width - 4));
      __m64 result = colorkeymask_core_mmx(src, colorv, tolerance, zero);
      *reinterpret_cast<int*>(pf + width - 4) = _mm_cvtsi64_si32(result);
    }

    pf += pitch;
  }

  _mm_empty();
}

#endif

template<typename pixel_t>
static void colorkeymask_c(BYTE* pf8, int pitch, int R, int G, int B, int height, int rowsize, int tolB, int tolG, int tolR) {
  pixel_t *pf = reinterpret_cast<pixel_t *>(pf8);
  rowsize /= sizeof(pixel_t);
  pitch /= sizeof(pixel_t);
  for (int y = 0; y< height; y++) {
    for (int x = 0; x < rowsize; x+=4) {
      if (IsClose(pf[x],B,tolB) && IsClose(pf[x+1],G,tolG) && IsClose(pf[x+2],R,tolR))
        pf[x+3]=0;
    }
    pf += pitch;
  }
}

template<typename pixel_t>
static void colorkeymask_planar_c(const BYTE* pfR8, const BYTE* pfG8, const BYTE* pfB8, BYTE* pfA8, int pitch, int R, int G, int B, int height, int width, int tolB, int tolG, int tolR) {
  const pixel_t *pfR = reinterpret_cast<const pixel_t *>(pfR8);
  const pixel_t *pfG = reinterpret_cast<const pixel_t *>(pfG8);
  const pixel_t *pfB = reinterpret_cast<const pixel_t *>(pfB8);
  pixel_t *pfA = reinterpret_cast<pixel_t *>(pfA8);
  pitch /= sizeof(pixel_t);
  for (int y = 0; y< height; y++) {
    for (int x = 0; x < width; x++) {
      if (IsClose(pfB[x],B,tolB) && IsClose(pfG[x],G,tolG) && IsClose(pfR[x],R,tolR))
        pfA[x]=0;
    }
    pfR += pitch;
    pfG += pitch;
    pfB += pitch;
    pfA += pitch;
  }
}

static void colorkeymask_planar_float_c(const BYTE* pfR8, const BYTE* pfG8, const BYTE* pfB8, BYTE* pfA8, int pitch, float R, float G, float B, int height, int width, float tolB, float tolG, float tolR) {
  typedef float pixel_t;
  const pixel_t *pfR = reinterpret_cast<const pixel_t *>(pfR8);
  const pixel_t *pfG = reinterpret_cast<const pixel_t *>(pfG8);
  const pixel_t *pfB = reinterpret_cast<const pixel_t *>(pfB8);
  pixel_t *pfA = reinterpret_cast<pixel_t *>(pfA8);
  pitch /= sizeof(pixel_t);
  for (int y = 0; y< height; y++) {
    for (int x = 0; x < width; x++) {
      if (IsCloseFloat(pfB[x],B,tolB) && IsCloseFloat(pfG[x],G,tolG) && IsCloseFloat(pfR[x],R,tolR))
        pfA[x]=0;
    }
    pfR += pitch;
    pfG += pitch;
    pfB += pitch;
    pfA += pitch;
  }
}


PVideoFrame __stdcall ColorKeyMask::GetFrame(int n, IScriptEnvironment *env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  BYTE* pf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int rowsize = frame->GetRowSize();

  if(vi.IsPlanarRGBA()) {
    const BYTE* pf_g = frame->GetReadPtr(PLANAR_G);
    const BYTE* pf_b = frame->GetReadPtr(PLANAR_B);
    const BYTE* pf_r = frame->GetReadPtr(PLANAR_R);
    BYTE* pf_a = frame->GetWritePtr(PLANAR_A);

    const int pitch = frame->GetPitch();
    const int width = vi.width;

    if(pixelsize == 1) {
      const int R = (color >> 16) & 0xff;
      const int G = (color >> 8) & 0xff;
      const int B = color & 0xff;
      colorkeymask_planar_c<uint8_t>(pf_r, pf_g, pf_b, pf_a, pitch, R, G, B, vi.height, width, tolB, tolG, tolR);
    } else if (pixelsize == 2) {
      const int R = (color64 >> 32) & 0xffff;
      const int G = (color64 >> 16) & 0xffff;
      const int B = color64 & 0xffff;
      colorkeymask_planar_c<uint16_t>(pf_r, pf_g, pf_b, pf_a, pitch, R, G, B, vi.height, width, tolB16, tolG16, tolR16);
    } else { // float
      const float R = ((color >> 16) & 0xff) / 255.0f;
      const float G = ((color >> 8) & 0xff) / 255.0f;
      const float B = (color & 0xff) / 255.0f;
      colorkeymask_planar_float_c(pf_r, pf_g, pf_b, pf_a, pitch, R, G, B, vi.height, width, tolB / 255.0f, tolG / 255.0f, tolR / 255.0f);
    }
  } else {
    // RGB32, RGB64
    if ((pixelsize==1) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(pf, 16))
    {
      colorkeymask_sse2(pf, pitch, color, vi.height, rowsize, tolB, tolG, tolR);
    }
    else
  #ifdef X86_32
    if ((pixelsize==1) && (env->GetCPUFlags() & CPUF_MMX))
    {
      colorkeymask_mmx(pf, pitch, color, vi.height, rowsize, tolB, tolG, tolR);
    }
    else
  #endif
    {
      if(pixelsize == 1) {
        const int R = (color >> 16) & 0xff;
        const int G = (color >> 8) & 0xff;
        const int B = color & 0xff;
        colorkeymask_c<uint8_t>(pf, pitch, R, G, B, vi.height, rowsize, tolB, tolG, tolR);
      } else {
        const int R = (color64 >> 32) & 0xffff;
        const int G = (color64 >> 16) & 0xffff;
        const int B = color64 & 0xffff;
        colorkeymask_c<uint16_t>(pf, pitch, R, G, B, vi.height, rowsize, tolB16, tolG16, tolR16);
      }
    }
  }

  return frame;
}

AVSValue __cdecl ColorKeyMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  enum { CHILD, COLOR, TOLERANCE_B, TOLERANCE_G, TOLERANCE_R};
  return new ColorKeyMask(args[CHILD].AsClip(),
    args[COLOR].AsInt(0),
    args[TOLERANCE_B].AsInt(10),
    args[TOLERANCE_G].AsInt(args[TOLERANCE_B].AsInt(10)),
    args[TOLERANCE_R].AsInt(args[TOLERANCE_B].AsInt(10)), env);
}


/********************************
 ******  ResetMask filter  ******
 ********************************/


ResetMask::ResetMask(PClip _child, float _mask_f, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if (!(vi.IsRGB32() || vi.IsRGB64() || vi.IsPlanarRGBA() || vi.IsYUVA()))
    env->ThrowError("ResetMask: format has no alpha channel");

  // new: resetmask has parameter. If none->max transparency

  int max_pixel_value = (1 << vi.BitsPerComponent()) - 1;
  if(_mask_f < 0) {
    mask_f = 1.0f;
    mask = max_pixel_value;
  }
  else {
    mask_f = _mask_f;
    if (mask_f < 0) mask_f = 0;
    mask = (int)mask_f;

    mask = clamp(mask, 0, max_pixel_value);
    mask_f = clamp(mask_f, 0.0f, 1.0f);
  }
}


PVideoFrame ResetMask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  env->MakeWritable(&f);

  if (vi.IsPlanarRGBA() || vi.IsYUVA()) {
    const int dst_pitchA = f->GetPitch(PLANAR_A);
    BYTE* dstp_a = f->GetWritePtr(PLANAR_A);
    const int heightA = f->GetHeight(PLANAR_A);

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
    return f;
  }
  // RGB32 and RGB64

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();
  int width = vi.width;

  if(vi.IsRGB32()) {
    for (int y = 0; y<height; y++) {
      for (int x = 3; x<rowsize; x += 4) {
        pf[x] = mask;
      }
      pf += pitch;
    }
  }
  else if (vi.IsRGB64()) {
    rowsize /= sizeof(uint16_t);
    for (int y = 0; y<height; y++) {
      for (int x = 3; x<rowsize; x += 4) {
        reinterpret_cast<uint16_t *>(pf)[x] = mask;
      }
      pf += pitch;
    }
  }

  return f;
}


AVSValue ResetMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ResetMask(args[0].AsClip(), (float)args[1].AsFloat(-1.0f), env);
}


/********************************
 ******  Invert filter  ******
 ********************************/


Invert::Invert(PClip _child, const char * _channels, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  doB = doG = doR = doA = doY = doU = doV = false;

  for (int k = 0; _channels[k] != '\0'; ++k) {
    switch (_channels[k]) {
    case 'B':
    case 'b':
      doB = true;
      break;
    case 'G':
    case 'g':
      doG = true;
      break;
    case 'R':
    case 'r':
      doR = true;
      break;
    case 'A':
    case 'a':
      doA = (vi.NumComponents() > 3);
      break;
    case 'Y':
    case 'y':
      doY = true;
      break;
    case 'U':
    case 'u':
      doU = (vi.NumComponents() > 1);
      break;
    case 'V':
    case 'v':
      doV = (vi.NumComponents() > 1);
      break;
    default:
      break;
    }
  }
  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
  if (vi.IsYUY2()) {
    mask = doY ? 0x00ff00ff : 0;
    mask |= doU ? 0x0000ff00 : 0;
    mask |= doV ? 0xff000000 : 0;
  }
  else if (vi.IsRGB32()) {
    mask = doB ? 0x000000ff : 0;
    mask |= doG ? 0x0000ff00 : 0;
    mask |= doR ? 0x00ff0000 : 0;
    mask |= doA ? 0xff000000 : 0;
  }
  else if (vi.IsRGB64()) {
    mask64 = doB ? 0x000000000000ffffull : 0;
    mask64 |= (doG ? 0x00000000ffff0000ull : 0);
    mask64 |= (doR ? 0x0000ffff00000000ull : 0);
    mask64 |= (doA ? 0xffff000000000000ull : 0);
  }
  else {
    mask = 0xffffffff;
    mask64 = (1 << bits_per_pixel) - 1;
    mask64 |= (mask64 << 48) | (mask64 << 32) | (mask64 << 16); // works for 10 bit, too
    // RGB24/48 is special case no use of this mask
  }
}

static void invert_frame_sse2(BYTE* frame, int pitch, int width, int height, int mask) {
  __m128i maskv = _mm_set1_epi32(mask);

  BYTE* endp = frame + pitch * height;

  while (frame < endp) {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(frame));
    __m128i inv = _mm_xor_si128(src, maskv);
    _mm_store_si128(reinterpret_cast<__m128i*>(frame), inv);
    frame += 16;
  }
}

static void invert_frame_uint16_sse2(BYTE* frame, int pitch, int width, int height, uint64_t mask64) {
  __m128i maskv = _mm_set_epi32((uint32_t)(mask64 >> 32),(uint32_t)mask64,(uint32_t)(mask64 >> 32),(uint32_t)mask64);

  BYTE* endp = frame + pitch * height;

  while (frame < endp) {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(frame));
    __m128i inv = _mm_xor_si128(src, maskv);
    _mm_store_si128(reinterpret_cast<__m128i*>(frame), inv);
    frame += 16;
  }
}

#ifdef X86_32

//mod4 width (in bytes) is required
static void invert_frame_mmx(BYTE* frame, int pitch, int width, int height, int mask) 
{
  __m64 maskv = _mm_set1_pi32(mask);
  int mod8_width = width / 8 * 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(frame+x);
      __m64 inv = _mm_xor_si64(src, maskv);
      *reinterpret_cast<__m64*>(frame+x) = inv;
    }
    
    if (mod8_width != width) {
      //last four pixels
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(frame+width-4));
      __m64 inv = _mm_xor_si64(src, maskv);
      *reinterpret_cast<int*>(frame+width-4) = _mm_cvtsi64_si32(inv);
    }
    frame += pitch;
  }
  _mm_empty();
}

static void invert_plane_mmx(BYTE* frame, int pitch, int width, int height) 
{
#pragma warning(push)
#pragma warning(disable: 4309)
  __m64 maskv = _mm_set1_pi8(0xFF);
#pragma warning(pop)
  int mod8_width = width / 8 * 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(frame+x);
      __m64 inv = _mm_xor_si64(src, maskv);
      *reinterpret_cast<__m64*>(frame+x) = inv;
    }

    for (int x = mod8_width; x < width; ++x) {
      frame[x] = frame[x] ^ 255;
    }
    frame += pitch;
  }
  _mm_empty();
}

#endif

//mod4 width is required
static void invert_frame_c(BYTE* frame, int pitch, int width, int height, int mask) {
  for (int y = 0; y < height; ++y) {
    int* intptr = reinterpret_cast<int*>(frame);

    for (int x = 0; x < width / 4; ++x) {
      intptr[x] = intptr[x] ^ mask;
    }
    frame += pitch;
  }
}

static void invert_frame_uint16_c(BYTE* frame, int pitch, int width, int height, uint64_t mask64) {
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width / 8; ++x) {
      reinterpret_cast<uint64_t *>(frame)[x] = reinterpret_cast<uint64_t *>(frame)[x] ^ mask64;
    }
    frame += pitch;
  }
}

static void invert_plane_c(BYTE* frame, int pitch, int row_size, int height) {
  int mod4_width = row_size / 4 * 4;
  for (int y = 0; y < height; ++y) {
    int* intptr = reinterpret_cast<int*>(frame);

    for (int x = 0; x < mod4_width / 4; ++x) {
      intptr[x] = intptr[x] ^ 0xFFFFFFFF;
    }

    for (int x = mod4_width; x < row_size; ++x) {
      frame[x] = frame[x] ^ 255;
    }
    frame += pitch;
  }
}

static void invert_plane_uint16_c(BYTE* frame, int pitch, int row_size, int height, uint64_t mask64) {
  int mod8_width = row_size / 8 * 8;
  uint16_t mask16 = mask64 & 0xFFFF; // for planes, all 16 bit parts of 64 bit mask is the same
  for (int y = 0; y < height; ++y) {

    for (int x = 0; x < mod8_width / 8; ++x) {
      reinterpret_cast<uint64_t *>(frame)[x] ^= mask64;
    }

    for (int x = mod8_width; x < row_size; ++x) {
      reinterpret_cast<uint16_t *>(frame)[x] ^= mask16;
    }
    frame += pitch;
  }
}

static void invert_plane_float_c(BYTE* frame, int pitch, int row_size, int height, bool chroma) {
  const int width = row_size / sizeof(float);
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float max = 1.0f;
#else
  const float max = chroma ? 0.0f : 1.0f;
#endif
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      reinterpret_cast<float *>(frame)[x] = max - reinterpret_cast<float *>(frame)[x];
    }
    frame += pitch;
  }
}

static void invert_frame(BYTE* frame, int pitch, int rowsize, int height, int mask, uint64_t mask64, int pixelsize, IScriptEnvironment *env) {
  if ((pixelsize == 1 || pixelsize == 2) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(frame, 16))
  {
    if(pixelsize == 1)
      invert_frame_sse2(frame, pitch, rowsize, height, mask);
    else
      invert_frame_uint16_sse2(frame, pitch, rowsize, height, mask64);
  }
#ifdef X86_32
  else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_MMX))
  {
    invert_frame_mmx(frame, pitch, rowsize, height, mask);
  }
#endif
  else 
  {
    if(pixelsize == 1)
      invert_frame_c(frame, pitch, rowsize, height, mask);
    else
      invert_frame_uint16_c(frame, pitch, rowsize, height, mask64);
  }
}

static void invert_frame_uint16(BYTE* frame, int pitch, int rowsize, int height, uint64_t mask64, IScriptEnvironment *env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(frame, 16)) 
  {
    invert_frame_uint16_sse2(frame, pitch, rowsize, height, mask64);
  }
  else
  {
    invert_frame_uint16_c(frame, pitch, rowsize, height, mask64);
  }
}


static void invert_plane(BYTE* frame, int pitch, int rowsize, int height, int pixelsize, uint64_t mask64, bool chroma, IScriptEnvironment *env) {
  if ((pixelsize == 1 || pixelsize == 2) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(frame, 16))
  {
    if(pixelsize == 1)
      invert_frame_sse2(frame, pitch, rowsize, height, 0xffffffff);
    else if(pixelsize == 2)
      invert_frame_uint16_sse2(frame, pitch, rowsize, height, mask64);
  }
#ifdef X86_32
  else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_MMX))
  {
    invert_plane_mmx(frame, pitch, rowsize, height);
  }
#endif
  else 
  {
    if(pixelsize == 1)
      invert_plane_c(frame, pitch, rowsize, height);
    else if (pixelsize == 2)
      invert_plane_uint16_c(frame, pitch, rowsize, height, mask64);
    else {
      invert_plane_float_c(frame, pitch, rowsize, height, chroma);
    }
  }
}

PVideoFrame Invert::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);

  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();

  if (vi.IsPlanar()) {
    // planar YUV
    if (vi.IsYUV() || vi.IsYUVA()) {
      if (doY)
        invert_plane(pf, pitch, f->GetRowSize(PLANAR_Y_ALIGNED), height, pixelsize, mask64, false, env);
      if (doU)
        invert_plane(f->GetWritePtr(PLANAR_U), f->GetPitch(PLANAR_U), f->GetRowSize(PLANAR_U_ALIGNED), f->GetHeight(PLANAR_U), pixelsize, mask64, true, env);
      if (doV)
        invert_plane(f->GetWritePtr(PLANAR_V), f->GetPitch(PLANAR_V), f->GetRowSize(PLANAR_V_ALIGNED), f->GetHeight(PLANAR_V), pixelsize, mask64, true, env);
    }
    // planar RGB
    if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
      if (doG) // first plane, GetWritePtr w/o parameters
        invert_plane(pf, pitch, f->GetRowSize(PLANAR_G_ALIGNED), height, pixelsize, mask64, false, env);
      if (doB)
        invert_plane(f->GetWritePtr(PLANAR_B), f->GetPitch(PLANAR_B), f->GetRowSize(PLANAR_B_ALIGNED), f->GetHeight(PLANAR_B), pixelsize, mask64, false, env);
      if (doR)
        invert_plane(f->GetWritePtr(PLANAR_R), f->GetPitch(PLANAR_R), f->GetRowSize(PLANAR_R_ALIGNED), f->GetHeight(PLANAR_R), pixelsize, mask64, false, env);
    }
    // alpha
    if (doA && (vi.IsPlanarRGBA() || vi.IsYUVA()))
      invert_plane(f->GetWritePtr(PLANAR_A), f->GetPitch(PLANAR_A), f->GetRowSize(PLANAR_A_ALIGNED), f->GetHeight(PLANAR_A), pixelsize, mask64, false, env);
  }
  else if (vi.IsYUY2() || vi.IsRGB32() || vi.IsRGB64()) {
    invert_frame(pf, pitch, rowsize, height, mask, mask64, pixelsize, env);
  }
  else if (vi.IsRGB24()) {
    int rMask= doR ? 0xff : 0;
    int gMask= doG ? 0xff : 0;
    int bMask= doB ? 0xff : 0;
    for (int i=0; i<height; i++) {

      for (int j=0; j<rowsize; j+=3) {
        pf[j+0] = pf[j+0] ^ bMask;
        pf[j+1] = pf[j+1] ^ gMask;
        pf[j+2] = pf[j+2] ^ rMask;
      }
      pf += pitch;
    }
  }
  else if (vi.IsRGB48()) {
    int rMask= doR ? 0xffff : 0;
    int gMask= doG ? 0xffff : 0;
    int bMask= doB ? 0xffff : 0;
    for (int i=0; i<height; i++) {
      for (int j=0; j<rowsize/pixelsize; j+=3) {
        reinterpret_cast<uint16_t *>(pf)[j+0] ^= bMask;
        reinterpret_cast<uint16_t *>(pf)[j+1] ^= gMask;
        reinterpret_cast<uint16_t *>(pf)[j+2] ^= rMask;
      }
      pf += pitch;
    }
  }

  return f;
}


AVSValue Invert::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Invert(args[0].AsClip(), args[0].AsClip()->GetVideoInfo().IsRGB() ? args[1].AsString("RGBA") : args[1].AsString("YUVA"), env);
}


/**********************************
 ******  ShowChannel filter  ******
 **********************************/


ShowChannel::ShowChannel(PClip _child, const char * pixel_type, int _channel, IScriptEnvironment* env)
  : GenericVideoFilter(_child), channel(_channel), input_type(_child->GetVideoInfo().pixel_type),
    pixelsize(_child->GetVideoInfo().ComponentSize()), bits_per_pixel(_child->GetVideoInfo().BitsPerComponent())
{
  static const char * const ShowText[7] = {"Blue", "Green", "Red", "Alpha", "Y", "U", "V"};

  input_type_is_planar_rgb = vi.IsPlanarRGB();
  input_type_is_planar_rgba = vi.IsPlanarRGBA();
  input_type_is_yuva = vi.IsYUVA();
  input_type_is_yuv = vi.IsYUV() && vi.IsPlanar();
  input_type_is_planar = vi.IsPlanar();

  if(vi.IsYUY2())
    env->ThrowError("Show%s: YUY2 source not supported", ShowText[channel]);

  int orig_channel = channel;

  // A channel
  if ((channel == 3) && !vi.IsRGB32() && !vi.IsRGB64() && !vi.IsPlanarRGBA() && !vi.IsYUVA())
    env->ThrowError("ShowAlpha: RGB32, RGB64, Planar RGBA or YUVA data only");

  // R, G, B channel
  if ((channel >=0) && (channel <= 2) && !vi.IsRGB())
    env->ThrowError("Show%s: plane is valid only with RGB or planar RGB(A) source", ShowText[channel]);

  // Y, U, V channel (4,5,6)
  if ((channel >=4) && (channel <= 6)) {
    if (!vi.IsYUV() && !vi.IsYUVA())
      env->ThrowError("Show%s: plane is valid only with YUV(A) source", ShowText[channel]);
    if(channel != 4 && vi.IsY())
      env->ThrowError("Show%s: invalid plane for greyscale source", ShowText[channel]);
    channel -= 4; // map to 0,1,2
  }

  /*if(vi.IsPlanarRGB() || vi.IsPlanarRGBA())
    env->ThrowError("Show%s: Planar RGB source is not supported", ShowText[channel]);
    */

  int target_pixelsize;
  int target_bits_per_pixel;

  if(input_type_is_yuv || input_type_is_yuva)
  {
    if(channel == 1 || channel == 2) // U or V: target can be smaller than Y
    {
      vi.width >>= vi.GetPlaneWidthSubsampling(PLANAR_U);
      vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
    }
  }

  if (!lstrcmpi(pixel_type, "rgb")) { // target is packed RGB, rgb const is adaptively 32 or 64 bits
    switch (bits_per_pixel) {
    case 8: vi.pixel_type = VideoInfo::CS_BGR32; break; // bit-depth adaptive
    case 16: vi.pixel_type = VideoInfo::CS_BGR64; break;
    default: env->ThrowError("Show%s: source must be 8 or 16 bits", ShowText[orig_channel]);
    }
    target_pixelsize = pixelsize;
    target_bits_per_pixel = bits_per_pixel;
  } else {
    int new_pixel_type = GetPixelTypeFromName(pixel_type);
    if(new_pixel_type == VideoInfo::CS_UNKNOWN)
      env->ThrowError("Show%s: invalid pixel_type!", ShowText[orig_channel]);
    // new output format
    vi.pixel_type = new_pixel_type;

    if (new_pixel_type == VideoInfo::CS_YUY2) {
      if (vi.width & 1) {
        env->ThrowError("Show%s: width must be mod 2 for yuy2", ShowText[orig_channel]);
      }
    }
    if (vi.Is420()) {
      if (vi.width & 1) {
        env->ThrowError("Show%s: width must be mod 2 for 4:2:0 target", ShowText[orig_channel]);
      }
      if (vi.height & 1) {
        env->ThrowError("Show%s: height must be mod 2 for 4:2:0 target", ShowText[orig_channel]);
      }
    }
    if(vi.Is422()) {
      if (vi.width & 1) {
        env->ThrowError("Show%s: width must be mod 2 for 4:2:2 target", ShowText[orig_channel]);
      }
    }

    target_pixelsize = vi.ComponentSize();
    target_bits_per_pixel = vi.BitsPerComponent();
  }

  if(target_bits_per_pixel != bits_per_pixel)
    env->ThrowError("Show%s: source bit depth must be %d for %s", ShowText[orig_channel], target_bits_per_pixel, pixel_type);
}


PVideoFrame ShowChannel::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);

  // for planar these will be reread for proper plane
  const BYTE* pf = f->GetReadPtr();
  const int height = f->GetHeight();
  const int pitch = f->GetPitch();
  const int rowsize = f->GetRowSize();

  const int width = rowsize / pixelsize;

#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
  const float chroma_center_f = 0.5f;
#else
  const float chroma_center_f = 0.0f;
#endif

  if (input_type == VideoInfo::CS_BGR32 || input_type == VideoInfo::CS_BGR64) {
    if (vi.pixel_type == VideoInfo::CS_BGR32 || vi.pixel_type == VideoInfo::CS_BGR64) // RGB32->RGB32, RGB64->RGB64
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=4) {
              dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
            }
            dstp += pitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=4) {
              uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
              dstp16[j + 0] = dstp16[j + 1] = dstp16[j + 2] = dstp16[j + channel];
            }
            dstp += pitch;
          }
        }
        return f;
      }
      else { // RGB32->RGB32 not in-place
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=4) {
              dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
              dstp[j + 3] = pf[j + 3];
            }
            pf   += pitch;
            dstp += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=4) {
              uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
              dstp16[j + 0] = dstp16[j + 1] = dstp16[j + 2] = reinterpret_cast<const uint16_t *>(pf)[j + channel];
              dstp16[j + 3] = reinterpret_cast<const uint16_t *>(pf)[j + 3];
            }
            pf   += pitch;
            dstp += dstpitch;
          }
        }
        return dst;
      }
    }
    else if (vi.pixel_type == VideoInfo::CS_BGR24 || vi.pixel_type == VideoInfo::CS_BGR48) // RGB32->RGB24, RGB64->RGB48
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      if(pixelsize==1) {
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width/4; j++) {
            dstp[j*3 + 0] = dstp[j*3 + 1] = dstp[j*3 + 2] = pf[j*4 + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
      }
      else { // pixelsize==2
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width/4; j++) {
            uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
            dstp16[j*3 + 0] = dstp16[j*3 + 1] = dstp16[j*3 + 2] = reinterpret_cast<const uint16_t *>(pf)[j*4 + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }

      }
      return dst;
    }
    else if (vi.pixel_type == VideoInfo::CS_YUY2) // RGB32->YUY2
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      const int dstrowsize = dst->GetRowSize();

      // RGB is upside-down
      pf += (height-1) * pitch;

      for (int i=0; i<height; ++i) {
        for (int j=0; j<dstrowsize; j+=2) {
          dstp[j + 0] = pf[j*2 + channel];
          dstp[j + 1] = 128;
        }
        pf -= pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else
    { // RGB32->YV12/16/24/Y8 + 16bit
      // 444, 422 support + 16 bits
      if (vi.Is444() || vi.Is422() || vi.Is420() || vi.IsY()) // Y8, YV12, Y16, YUV420P16, etc.
      {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;

        // packed RGB is upside-down
        pf += (height-1) * pitch;

        // copy to luma
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp[j] = pf[j*4 + channel];
            }
            pf -= pitch;
            dstp += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp)[j] = reinterpret_cast<const uint16_t *>(pf)[j*4 + channel];
            }
            pf -= pitch;
            dstp += dstpitch;
          }
        }
        if (!vi.IsY())
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstp_u = dst->GetWritePtr(PLANAR_U);
          BYTE * dstp_v = dst->GetWritePtr(PLANAR_V);
          switch (pixelsize) {
          case 1: fill_chroma<BYTE>(dstp_u, dstp_v, dstheight, dstpitch, (BYTE)0x80); break;
          case 2: fill_chroma<uint16_t>(dstp_u, dstp_v, dstheight, dstpitch, 1 << (vi.BitsPerComponent() - 1)); break;
          case 4: 
            fill_chroma<float>(dstp_u, dstp_v, dstheight, dstpitch, chroma_center_f); 
            break;
          }
        }
        return dst;
      }
      else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
      {  // RGB32/64 -> Planar RGB 8/16 bit
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp_g = dst->GetWritePtr(PLANAR_G);
        BYTE * dstp_b = dst->GetWritePtr(PLANAR_B);
        BYTE * dstp_r = dst->GetWritePtr(PLANAR_R);
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;

        // packed RGB is upside-down
        pf += (height-1) * pitch;

        // copy to luma
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp_g[j] = dstp_b[j] = dstp_r[j] = pf[j*4 + channel];
            }
            pf -= pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp_g)[j] =
                reinterpret_cast<uint16_t *>(dstp_b)[j] =
                  reinterpret_cast<uint16_t *>(dstp_r)[j] = reinterpret_cast<const uint16_t *>(pf)[j*4 + channel];
            }
            pf -= pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
      }
    }
  } // end of RGB32/64 source
  else if (input_type == VideoInfo::CS_BGR24 || input_type == VideoInfo::CS_BGR48)
  {
    if (vi.pixel_type == VideoInfo::CS_BGR24 || vi.pixel_type == VideoInfo::CS_BGR48) // RGB24->RGB24, RGB48->RGB48
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();

        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=3) {
              dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
            }
            dstp += pitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=3) {
              uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
              dstp16[j + 0] = dstp16[j + 1] = dstp16[j + 2] = dstp16[j + channel];
            }
            dstp += pitch;
          }
        }
        return f;
      }
      else { // RGB24->RGB24 not in-place
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=3) {
              dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
            }
            pf   += pitch;
            dstp += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<width; j+=3) {
              uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
              dstp16[j + 0] = dstp16[j + 1] = dstp16[j + 2] = reinterpret_cast<const uint16_t *>(pf)[j + channel];
            }
            pf   += pitch;
            dstp += dstpitch;
          }
        }


        return dst;
      }
    }
    else if (vi.pixel_type == VideoInfo::CS_BGR32 || vi.pixel_type == VideoInfo::CS_BGR64) // RGB24->RGB32
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();

      if(pixelsize==1) {
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width/3; j++) {
            dstp[j*4 + 0] = dstp[j*4 + 1] = dstp[j*4 + 2] = dstp[j*4 + 3] = pf[j*3 + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
      }
      else {
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width/3; j++) {
            uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
            dstp16[j*4 + 0] = dstp16[j*4 + 1] = dstp16[j*4 + 2] = dstp16[j*4 + 3] = reinterpret_cast<const uint16_t *>(pf)[j*3 + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
      }
      return dst;
    }
    else if (vi.pixel_type == VideoInfo::CS_YUY2) // RGB24->YUY2
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      const int dstrowsize = dst->GetRowSize()/2;

      // RGB is upside-down
      pf += (height-1) * pitch;

      for (int i=0; i<height; ++i) {
        for (int j=0; j<dstrowsize; j++) {
          dstp[j*2 + 0] = pf[j*3 + channel];
          dstp[j*2 + 1] = 128;
        }
        pf -= pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else
    { // // RGB24->YV12/16/24/Y8 + 16bit
      if (vi.Is444() || vi.Is422() || vi.Is420() || vi.IsY()) // Y8, YV12, Y16, YUV420P16, etc.
      {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;

        // RGB is upside-down
        pf += (height-1) * pitch;

        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp[j] = pf[j*3 + channel];
            }
            pf -= pitch;
            dstp += dstpitch;
          }
        }
        else {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp)[j] = reinterpret_cast<const uint16_t *>(pf)[j*3 + channel];
            }
            pf -= pitch;
            dstp += dstpitch;
          }
        }
        if (!vi.IsY())
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstp_u = dst->GetWritePtr(PLANAR_U);
          BYTE * dstp_v = dst->GetWritePtr(PLANAR_V);
          switch (pixelsize) {
          case 1: fill_chroma<uint8_t>(dstp_u, dstp_v, dstheight, dstpitch, (BYTE)0x80); break;
          case 2: fill_chroma<uint16_t>(dstp_u, dstp_v, dstheight, dstpitch, 1 << (vi.BitsPerComponent() - 1)); break;
          case 4: 
            fill_chroma<float>(dstp_u, dstp_v, dstheight, dstpitch, chroma_center_f);
            break;
          }
        }
        return dst;
      }
      else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
      {  // RGB24/48 -> Planar RGB 8/16 bit
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp_g = dst->GetWritePtr(PLANAR_G);
        BYTE * dstp_b = dst->GetWritePtr(PLANAR_B);
        BYTE * dstp_r = dst->GetWritePtr(PLANAR_R);
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;
        // packed RGB is upside-down
        pf += (height-1) * pitch;

        // copy to luma
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp_g[j] = dstp_b[j] = dstp_r[j] = pf[j*3 + channel];
            }
            pf -= pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp_g)[j] =
                reinterpret_cast<uint16_t *>(dstp_b)[j] =
                reinterpret_cast<uint16_t *>(dstp_r)[j] = reinterpret_cast<const uint16_t *>(pf)[j*3 + channel];
            }
            pf -= pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
        return dst;
      }
    }
  } // end of RGB24/48 source
  else if (input_type_is_planar_rgb || input_type_is_planar_rgba || input_type_is_yuv || input_type_is_yuva) {
    // planar source
    const int planesYUV[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A};
    const int planesRGB[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A};
    const int *planes = (input_type_is_planar_rgb || input_type_is_planar_rgba) ? planesRGB : planesYUV;
    const int plane = planes[channel];

    bool hasAlpha = input_type_is_planar_rgba || input_type_is_yuva;
    const BYTE* srcp = f->GetReadPtr(plane); // source plane
    const BYTE* srcp_a = hasAlpha ? f->GetReadPtr(PLANAR_A) : nullptr;

    const int width = f->GetRowSize(plane) / pixelsize;
    const int height = f->GetHeight(plane);
    const int pitch = f->GetPitch(plane);

    if (vi.pixel_type == VideoInfo::CS_BGR32 || vi.pixel_type == VideoInfo::CS_BGR64) // PRGB/YUVA->RGB32/RGB64
    {
      { // Planar RGBA/YUVA  ->RGB32/64
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();
        // RGB is upside-down
        dstp += (height-1) * dstpitch;

        if(pixelsize==1) {
          if (hasAlpha) {
            for (int i = 0; i < height; ++i) {
              for (int j = 0; j < width; j++) {
                dstp[j * 4 + 0] = dstp[j * 4 + 1] = dstp[j * 4 + 2] = srcp[j];
                dstp[j * 4 + 3] = srcp_a[j];
              }
              srcp += pitch;
              srcp_a += pitch;
              dstp -= dstpitch;
            }
          }
          else {
            const int alpha = 255;
            for (int i = 0; i < height; ++i) {
              for (int j = 0; j < width; j++) {
                dstp[j * 4 + 0] = dstp[j * 4 + 1] = dstp[j * 4 + 2] = srcp[j];
                dstp[j * 4 + 3] = alpha;
              }
              srcp += pitch;
              dstp -= dstpitch;
            }
          }
        }
        else { // pixelsize==2
          if (hasAlpha) {
            for (int i = 0; i < height; ++i) {
              for (int j = 0; j < width; j++) {
                uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
                dstp16[j * 4 + 0] = dstp16[j * 4 + 1] = dstp16[j * 4 + 2] = reinterpret_cast<const uint16_t *>(srcp)[j];
                dstp16[j * 4 + 3] = reinterpret_cast<const uint16_t *>(srcp_a)[j];
              }
              srcp += pitch;
              srcp_a += pitch;
              dstp -= dstpitch;
            }
          }
          else {
            const int alpha = 0xFFFF;
            for (int i = 0; i < height; ++i) {
              for (int j = 0; j < width; j++) {
                uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
                dstp16[j * 4 + 0] = dstp16[j * 4 + 1] = dstp16[j * 4 + 2] = reinterpret_cast<const uint16_t *>(srcp)[j];
                dstp16[j * 4 + 3] = alpha;
              }
              srcp += pitch;
              dstp -= dstpitch;
            }
          }
        }
        return dst;
      }
    }
    else if (vi.pixel_type == VideoInfo::CS_BGR24 || vi.pixel_type == VideoInfo::CS_BGR48) // PRGB(A)/YUVA->RGB24, PRGB(A)16/YUVA16->RGB48
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      // RGB is upside-down
      dstp += (height-1) * dstpitch;

      if(pixelsize==1) {
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width; j++) {
            dstp[j*3 + 0] = dstp[j*3 + 1] = dstp[j*3 + 2] = srcp[j];
          }
          srcp   += pitch;
          dstp -= dstpitch;
        }
      }
      else { // pixelsize==2
        for (int i=0; i<height; ++i) {
          for (int j=0; j<width; j++) {
            uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
            dstp16[j*3 + 0] = dstp16[j*3 + 1] = dstp16[j*3 + 2] = reinterpret_cast<const uint16_t *>(srcp)[j];
          }
          srcp += pitch;
          dstp -= dstpitch;
        }

      }
      return dst;
    }
    else if (vi.pixel_type == VideoInfo::CS_YUY2) // // PRGB(A)/YUVA->YUY2
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();
      const int dstrowsize = dst->GetRowSize();

      for (int i=0; i<height; ++i) {
        for (int j=0; j<width; j++) {
          dstp[j*2 + 0] = srcp[j];
          dstp[j*2 + 1] = 128;
        }
        srcp += pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else
    { // RGB(A)P/YUVA->YV12/16/24/Y8 + 16bit
      // 444, 422 support + 16 bits
      const bool targetHasAlpha = vi.IsPlanarRGBA() || vi.IsYUVA();
      PVideoFrame dst = env->NewVideoFrame(vi);

      if (vi.Is444() || vi.Is422() || vi.Is420() || vi.IsY()) // Y8, YV12, Y16, YUV420P16, etc.
      {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;

        // copy to luma
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp[j] = srcp[j];
            }
            srcp += pitch;
            dstp += dstpitch;
          }
        }
        else if (pixelsize == 2 ) { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp)[j] = reinterpret_cast<const uint16_t *>(srcp)[j];
            }
            srcp += pitch;
            dstp += dstpitch;
          }
        }
        else { // pixelsize == 4
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<float *>(dstp)[j] = reinterpret_cast<const float *>(srcp)[j];
            }
            srcp += pitch;
            dstp += dstpitch;
          }
        }
        if (!vi.IsY())
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstp_u = dst->GetWritePtr(PLANAR_U);
          BYTE * dstp_v = dst->GetWritePtr(PLANAR_V);
          switch (pixelsize) {
          case 1: fill_chroma<BYTE>(dstp_u, dstp_v, dstheight, dstpitch, (BYTE)0x80); break;
          case 2: fill_chroma<uint16_t>(dstp_u, dstp_v, dstheight, dstpitch, 1 << (vi.BitsPerComponent() - 1)); break;
          case 4: fill_chroma<float>(dstp_u, dstp_v, dstheight, dstpitch, chroma_center_f); break;
          }
        }
      }
      else if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
      {  // PRGB(A)/YUVA -> Planar RGB
        BYTE * dstp_g = dst->GetWritePtr(PLANAR_G);
        BYTE * dstp_b = dst->GetWritePtr(PLANAR_B);
        BYTE * dstp_r = dst->GetWritePtr(PLANAR_R);

        BYTE * dstp_a = targetHasAlpha ? dst->GetWritePtr(PLANAR_A) : nullptr;
        int dstpitch = dst->GetPitch();
        int dstwidth = dst->GetRowSize() / pixelsize;

        // copy to luma
        if(pixelsize==1) {
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              dstp_g[j] = dstp_b[j] = dstp_r[j] = srcp[j];
            }
            srcp += pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
        else { // pixelsize==2
          for (int i=0; i<height; ++i) {
            for (int j=0; j<dstwidth; ++j) {
              reinterpret_cast<uint16_t *>(dstp_g)[j] =
                reinterpret_cast<uint16_t *>(dstp_b)[j] =
                reinterpret_cast<uint16_t *>(dstp_r)[j] = reinterpret_cast<const uint16_t *>(srcp)[j];
            }
            srcp += pitch;
            dstp_g += dstpitch;
            dstp_b += dstpitch;
            dstp_r += dstpitch;
          }
        }
      }
      if (targetHasAlpha) {
        // fill with transparent
        const int dst_pitchA = dst->GetPitch(PLANAR_A);
        BYTE* dstp_a = dst->GetWritePtr(PLANAR_A);
        const int heightA = dst->GetHeight(PLANAR_A);

        switch (vi.ComponentSize())
        {
        case 1:
          fill_plane<BYTE>(dstp_a, heightA, dst_pitchA, 0xFF);
          break;
        case 2:
          fill_plane<uint16_t>(dstp_a, heightA, dst_pitchA, (1 << vi.BitsPerComponent()) - 1);
          break;
        case 4:
          fill_plane<float>(dstp_a, heightA, dst_pitchA, 1.0f);
          break;
        }
      }
      return dst;
    }
  } // planar RGB(A) or YUVA source

  env->ThrowError("ShowChannel: unexpected end of function");
  return f;
}


AVSValue ShowChannel::Create(AVSValue args, void* channel, IScriptEnvironment* env)
{
  return new ShowChannel(args[0].AsClip(), args[1].AsString("RGB"), (int)(size_t)channel, env);
}


/**********************************
 ******  MergeRGB filter  ******
 **********************************/


MergeRGB::MergeRGB(PClip _child, PClip _blue, PClip _green, PClip _red, PClip _alpha,
                   const char * pixel_type, IScriptEnvironment* env)
  : GenericVideoFilter(_child), blue(_blue), green(_green), red(_red), alpha(_alpha),
    viB(blue->GetVideoInfo()), viG(green->GetVideoInfo()), viR(red->GetVideoInfo()),
    viA(((alpha) ? alpha : child)->GetVideoInfo()), myname((alpha) ? "MergeARGB" : "MergeRGB")
{

  if (!lstrcmpi(pixel_type, "rgb32") || (!lstrcmpi(pixel_type, "") && vi.ComponentSize()==1)) {
      // default for 1 byte pixels
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (alpha && (viA.pixel_type == VideoInfo::CS_BGR24))
      env->ThrowError("MergeARGB: Alpha source channel may not be RGB24");
  }
  else if (!lstrcmpi(pixel_type, "rgb64") || (!lstrcmpi(pixel_type, "") && vi.ComponentSize()==2)) {
      // default for 2 byte pixels
      vi.pixel_type = VideoInfo::CS_BGR64;
      if (alpha && (viA.pixel_type == VideoInfo::CS_BGR48))
          env->ThrowError("MergeARGB: Alpha source channel may not be RGB48");
  }
  else if (!lstrcmpi(pixel_type, "rgb24")) {
    vi.pixel_type = VideoInfo::CS_BGR24;
  }
  else if (!lstrcmpi(pixel_type, "rgb48")) {
      vi.pixel_type = VideoInfo::CS_BGR48;
  }
  else {
    env->ThrowError("MergeRGB: supports the following output pixel types: RGB24, RGB32, RGB48, RGB64");
  }

  if ((vi.ComponentSize() != viB.ComponentSize()) || (vi.ComponentSize() != viG.ComponentSize()) || 
      (vi.ComponentSize() != viR.ComponentSize()) || (vi.ComponentSize() != viA.ComponentSize()))
      env->ThrowError("%s: All clips must have the same bit depth.", myname);

  if ((vi.width  != viB.width)  || (vi.width  != viG.width)  || (vi.width  != viR.width)  || (vi.width != viA.width))
    env->ThrowError("%s: All clips must have the same width.", myname);

  if ((vi.height != viB.height) || (vi.height != viG.height) || (vi.height != viR.height) || (vi.height != viA.height))
    env->ThrowError("%s: All clips must have the same height.", myname);
}


PVideoFrame MergeRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame B = blue->GetFrame(n, env);
  PVideoFrame G = green->GetFrame(n, env);
  PVideoFrame R = red->GetFrame(n, env);
  PVideoFrame A = (alpha) ? alpha->GetFrame(n, env) : 0;

  PVideoFrame dst = env->NewVideoFrame(vi);

  const int height = dst->GetHeight();
  const int pitch = dst->GetPitch();
  const int rowsize = dst->GetRowSize();
  const int modulo = pitch - rowsize;

  BYTE* dstp = dst->GetWritePtr();

  int pixelsize = viA.ComponentSize();

  // RGB is upside-down, backscan any YUV to match
  const int Bpitch = (viB.IsYUV()) ? -(B->GetPitch()) : B->GetPitch();
  const int Gpitch = (viG.IsYUV()) ? -(G->GetPitch()) : G->GetPitch();
  const int Rpitch = (viR.IsYUV()) ? -(R->GetPitch()) : R->GetPitch();

  // Bump any RGB channels, move any YUV channels to last line
  const BYTE* Bp = B->GetReadPtr() + ((Bpitch < 0) ? Bpitch * (1-height) : 0);
  const BYTE* Gp = G->GetReadPtr() + ((Gpitch < 0) ? Gpitch * (1-height) : (1 * pixelsize));
  const BYTE* Rp = R->GetReadPtr() + ((Rpitch < 0) ? Rpitch * (1-height) : (2 * pixelsize));

  // Adjustment from the end of 1 line to the start of the next
  const int Bmodulo = Bpitch - B->GetRowSize();
  const int Gmodulo = Gpitch - G->GetRowSize();
  const int Rmodulo = Rpitch - R->GetRowSize();

  // Number of bytes per pixel (1, 2, 3 or 4 .. 8)
  const int Bstride = viB.IsPlanar() ? pixelsize : (viB.BitsPerPixel()>>3);
  const int Gstride = viG.IsPlanar() ? pixelsize : (viG.BitsPerPixel()>>3);
  const int Rstride = viR.IsPlanar() ? pixelsize : (viR.BitsPerPixel()>>3);

  // End of VFB
  BYTE const * yend = dstp + pitch*height;

  if (alpha) { // ARGB mode
    const int Apitch = (viA.IsYUV()) ? -(A->GetPitch()) : A->GetPitch();
    const BYTE* Ap = A->GetReadPtr() + ((Apitch < 0) ? Apitch * (1-height) : (3 * pixelsize));
    const int Amodulo = Apitch - A->GetRowSize();
    const int Astride = viA.IsPlanar() ? pixelsize : (viA.BitsPerPixel()>>3);

    switch(pixelsize) {
    case 1:
        while (dstp < yend) {
          BYTE const * xend = dstp + rowsize;
          while (dstp < xend) {
            *dstp++ = *Bp; Bp += Bstride;
            *dstp++ = *Gp; Gp += Gstride;
            *dstp++ = *Rp; Rp += Rstride;
            *dstp++ = *Ap; Ap += Astride;
          }
          dstp += modulo;
          Bp += Bmodulo;
          Gp += Gmodulo;
          Rp += Rmodulo;
          Ap += Amodulo;
        }
        break;
    case 2:
        {
            uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
            uint16_t const * yend16 = dstp16 + pitch*height/sizeof(uint16_t);
            while (dstp16 < yend16) {
                uint16_t const * xend16 = dstp16 + rowsize / sizeof(uint16_t);
                while (dstp16 < xend16) {
                    *dstp16++ = *reinterpret_cast<const uint16_t *>(Bp); Bp += Bstride;
                    *dstp16++ = *reinterpret_cast<const uint16_t *>(Gp); Gp += Gstride;
                    *dstp16++ = *reinterpret_cast<const uint16_t *>(Rp); Rp += Rstride;
                    *dstp16++ = *reinterpret_cast<const uint16_t *>(Ap); Ap += Astride;
                }
                dstp16 += modulo / sizeof(uint16_t);
                Bp += Bmodulo;
                Gp += Gmodulo;
                Rp += Rmodulo;
                Ap += Amodulo;
            }
        }
        break;
    default:
        env->ThrowError("%s: float pixel type not supported", myname);
        break;
    }
  }
  else if (vi.pixel_type == VideoInfo::CS_BGR32 || vi.pixel_type == VideoInfo::CS_BGR64) { // RGB32 mode
      switch(pixelsize) {
      case 1:
        while (dstp < yend) {
            BYTE const * xend = dstp + rowsize;
            while (dstp < xend) {
            *dstp++ = *Bp; Bp += Bstride;
            *dstp++ = *Gp; Gp += Gstride;
            *dstp++ = *Rp; Rp += Rstride;
            *dstp++ = 0;
            }
            dstp += modulo;
            Bp += Bmodulo;
            Gp += Gmodulo;
            Rp += Rmodulo;
        }
        break;
      case 2:
      {
          uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
          uint16_t const * yend16 = dstp16 + pitch*height/sizeof(uint16_t);
          while (dstp16 < yend16) {
              uint16_t const * xend16 = dstp16 + rowsize / sizeof(uint16_t);
              while (dstp16 < xend16) {
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Bp); Bp += Bstride;
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Gp); Gp += Gstride;
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Rp); Rp += Rstride;
                  *dstp16++ = 0;
              }
              dstp16 += modulo / sizeof(uint16_t);
              Bp += Bmodulo;
              Gp += Gmodulo;
              Rp += Rmodulo;
          }
      }
      break;
      default:
          env->ThrowError("%s: float pixel type not supported", myname);
          break;
      }
  }
  else if (vi.pixel_type == VideoInfo::CS_BGR24 || vi.pixel_type == VideoInfo::CS_BGR48) { // RGB24 mode
      switch(pixelsize) {
      case 1:
          while (dstp < yend) {
              BYTE const * xend = dstp + rowsize;
              while (dstp < xend) {
                *dstp++ = *Bp; Bp += Bstride;
                *dstp++ = *Gp; Gp += Gstride;
                *dstp++ = *Rp; Rp += Rstride;
              }
              dstp += modulo;
              Bp += Bmodulo;
              Gp += Gmodulo;
              Rp += Rmodulo;
         }
          break;
      case 2:
      {
          uint16_t *dstp16 = reinterpret_cast<uint16_t *>(dstp);
          uint16_t const * yend16 = dstp16 + pitch*height/sizeof(uint16_t);
          while (dstp16 < yend16) {
              uint16_t const * xend16 = dstp16 + rowsize / sizeof(uint16_t);
              while (dstp16 < xend16) {
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Bp); Bp += Bstride;
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Gp); Gp += Gstride;
                  *dstp16++ = *reinterpret_cast<const uint16_t *>(Rp); Rp += Rstride;
              }
              dstp16 += modulo / sizeof(uint16_t);
              Bp += Bmodulo;
              Gp += Gmodulo;
              Rp += Rmodulo;
          }
      }
      break;
      default:
          env->ThrowError("%s: float pixel type not supported", myname);
          break;
      }
  }
  else
    env->ThrowError("%s: unexpected end of function", myname);

  return dst;
}


AVSValue MergeRGB::Create(AVSValue args, void* mode, IScriptEnvironment* env)
{
  if (mode) // ARGB
    return new MergeRGB(args[0].AsClip(), args[3].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), "", env); 
  else      // RGB[type]
    return new MergeRGB(args[0].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), 0, args[3].AsString(""), env);
  // default pixel_type now dynamic by bit_depth: RGB32 or RGB64
}


/*******************************
 *******   Layer Filter   ******
 *******************************/

Layer::Layer( PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y,
              int _t, bool _chroma, IScriptEnvironment* env )
  : child1(_child1), child2(_child2), levelB(_lev), ofsX(_x), ofsY(_y), Op(_op),
    ThresholdParam(_t), chroma(_chroma)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();

  if (vi1.pixel_type != vi2.pixel_type)
    env->ThrowError("Layer: image formats don't match");

  if (! (vi1.IsRGB32() || vi1.IsYUY2() || vi1.IsRGB64()) )
    env->ThrowError("Layer only support RGB32, RGB64 and YUY2 formats");

  vi = vi1;

  if (levelB == -1) { // default
    if (vi.IsRGB64())
      levelB = 65537; // (65535* 65537 +1 ) / 65536 = 65536
    else
      levelB = 257;   // (65535* 257   +1 ) / 256   = 256
  }


  if (vi.IsRGB32() || vi.IsRGB64()) ofsY = vi.height-vi2.height-ofsY; //RGB is upside down
  else ofsX = ofsX & 0xFFFFFFFE; //YUV must be aligned on even pixels

  xdest=(ofsX < 0)? 0: ofsX;
  ydest=(ofsY < 0)? 0: ofsY;

  xsrc=(ofsX < 0)? (0-ofsX): 0;
  ysrc=(ofsY < 0)? (0-ofsY): 0;

  xcount = (vi.width < (ofsX + vi2.width))? (vi.width-xdest) : (vi2.width - xsrc);
  ycount = (vi.height <  (ofsY + vi2.height))? (vi.height-ydest) : (vi2.height - ysrc);

  if (!( !lstrcmpi(Op, "Mul") || !lstrcmpi(Op, "Add") || !lstrcmpi(Op, "Fast") ||
         !lstrcmpi(Op, "Subtract") || !lstrcmpi(Op, "Lighten") || !lstrcmpi(Op, "Darken") ))
    env->ThrowError("Layer supports the following ops: Fast, Lighten, Darken, Add, Subtract, Mul");

  if (!chroma)
  {
    if (!lstrcmpi(Op, "Darken") ) env->ThrowError("Layer: monochrome darken illegal op");
    if (!lstrcmpi(Op, "Lighten")) env->ThrowError("Layer: monochrome lighten illegal op");
    if (!lstrcmpi(Op, "Fast")   ) env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
  }

  overlay_frames = vi2.num_frames;
}

// 15 bit scaled
// PF check: int32 overflow in 16 bits
// 32769 * 65535 + 16384 = 8000BFFF int32 overflow
// 32768 * 65535 + 16384 = 7FFFC000 OK
// Let's make correction
const int cyb = 3736;    // int(0.114 * 32768 + 0.5); // 3736
const int cyg = 19235-1; // int(0.587 * 32768 + 0.5); // 19235
const int cyr = 9798;    // int(0.299 * 32768 + 0.5); // 9798
// w/o correction: 32769

enum
{
  LIGHTEN = 0,
  DARKEN = 1
};

/* YUY2 */

template<bool use_chroma>
static void layer_yuy2_mul_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  int width_mod8 = width / 8 * 8;

  __m128i alpha = _mm_set1_epi16(level);
  __m128i half_alpha = _mm_srli_epi16(alpha, 1);
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
  __m128i v128 = _mm_set1_epi16(128);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width_mod8; x+=8) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(dstp+x*2));
      __m128i ovr = _mm_load_si128(reinterpret_cast<const __m128i*>(ovrp+x*2));

      __m128i src_luma = _mm_and_si128(src, luma_mask);
      __m128i ovr_luma = _mm_and_si128(ovr, luma_mask);

      __m128i src_chroma = _mm_srli_epi16(src, 8);
      __m128i ovr_chroma = _mm_srli_epi16(ovr, 8);

      __m128i luma = _mm_mullo_epi16(src_luma, ovr_luma);
      luma = _mm_srli_epi16(luma, 8);
      luma = _mm_subs_epi16(luma, src_luma);
      luma = _mm_mullo_epi16(luma, alpha);
      luma = _mm_srli_epi16(luma, 8);

      __m128i chroma;
      if (use_chroma) {
        chroma = _mm_subs_epi16(ovr_chroma, src_chroma);
        chroma = _mm_mullo_epi16(chroma, alpha);
      } else {
        chroma = _mm_subs_epi16(v128, src_chroma);
        chroma = _mm_mullo_epi16(chroma, half_alpha);
      }

      //it's fine, don't optimize
      chroma = _mm_srli_epi16(chroma, 8);
      chroma = _mm_slli_epi16(chroma, 8); 

      __m128i dst = _mm_or_si128(luma, chroma);
      dst = _mm_add_epi8(src, dst);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*2), dst);
    }

    for (int x = width_mod8; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((((ovrp[x*2] * dstp[x*2]) >> 8) - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2+1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * (level/2)) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static __forceinline __m64 layer_yuy2_mul_mmx_core(const __m64 &src, const __m64 &ovr, const __m64 &luma_mask, 
                                       const __m64 &alpha, const __m64 &half_alpha, const __m64 &v128) {
  __m64 src_luma = _mm_and_si64(src, luma_mask);
  __m64 ovr_luma = _mm_and_si64(ovr, luma_mask);

  __m64 src_chroma = _mm_srli_pi16(src, 8);
  __m64 ovr_chroma = _mm_srli_pi16(ovr, 8);

  __m64 luma = _mm_mullo_pi16(src_luma, ovr_luma);
  luma = _mm_srli_pi16(luma, 8);
  luma = _mm_subs_pi16(luma, src_luma);
  luma = _mm_mullo_pi16(luma, alpha);
  luma = _mm_srli_pi16(luma, 8);

  __m64 chroma;
  if (use_chroma) {
    chroma = _mm_subs_pi16(ovr_chroma, src_chroma);
    chroma = _mm_mullo_pi16(chroma, alpha);
  } else {
    chroma = _mm_subs_pi16(v128, src_chroma);
    chroma = _mm_mullo_pi16(chroma, half_alpha);
  }

  //it's fine, don't optimize
  chroma = _mm_srli_pi16(chroma, 8);
  chroma = _mm_slli_pi16(chroma, 8); 

  __m64 dst = _mm_or_si64(luma, chroma);
  return _mm_add_pi8(src, dst);
}

template<bool use_chroma>
static void layer_yuy2_mul_mmx(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  int width_mod4 = width / 4 * 4;

  __m64 alpha = _mm_set1_pi16(level);
  __m64 half_alpha = _mm_srli_pi16(alpha, 1);
  __m64 luma_mask = _mm_set1_pi16(0x00FF);
  __m64 v128 = _mm_set1_pi16(128);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width_mod4; x+=4) {
      __m64 src = *reinterpret_cast<const __m64*>(dstp+x*2);
      __m64 ovr = *reinterpret_cast<const __m64*>(ovrp+x*2);

      *reinterpret_cast<__m64*>(dstp+x*2) = layer_yuy2_mul_mmx_core<use_chroma>(src, ovr, luma_mask, alpha, half_alpha, v128);
    }

    if (width_mod4 != width) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+width_mod4*2));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+width_mod4*2));

      *reinterpret_cast<int*>(dstp+width_mod4*2) = _mm_cvtsi64_si32(layer_yuy2_mul_mmx_core<use_chroma>(src, ovr, luma_mask, alpha, half_alpha, v128));
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<bool use_chroma>
static void layer_yuy2_mul_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((((ovrp[x*2] * dstp[x*2]) >> 8) - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2+1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * (level/2)) >> 8);
      }
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


template<bool use_chroma>
static void layer_yuy2_add_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m128i alpha = _mm_set1_epi16(level);
  __m128i zero = _mm_setzero_si128();
  __m128i v128 = _mm_set1_epi32(0x00800000);
  __m128i luma_mask = _mm_set1_epi32(0x000000FF);
  int mod4_width = width / 4 * 4;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod4_width; x+=4) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*2));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*2));

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      if (!use_chroma) {
        ovr = _mm_and_si128(ovr, luma_mask); 
        ovr = _mm_or_si128(ovr, v128); 
      }

      __m128i diff = _mm_subs_epi16(ovr, src);
      diff = _mm_mullo_epi16(diff, alpha);
      diff = _mm_srli_epi16(diff, 8);

      __m128i dst = _mm_add_epi8(diff, src);
      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), dst);
    }

    for (int x = mod4_width; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((ovrp[x*2] - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2-1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * level) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static void layer_yuy2_add_mmx(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m64 alpha = _mm_set1_pi16(level);
  __m64 zero = _mm_setzero_si64();
  __m64 v128 = _mm_set1_pi32(0x00800000);
  __m64 luma_mask = _mm_set1_pi32(0x000000FF);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*2));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*2));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      if (!use_chroma) {
        ovr = _mm_and_si64(ovr, luma_mask); //00 00 00 YY 00 00 00 YY
        ovr = _mm_or_si64(ovr, v128); //00 128 00 YY 00 128 00 YY
      }

      __m64 diff = _mm_subs_pi16(ovr, src);
      diff = _mm_mullo_pi16(diff, alpha);
      diff = _mm_srli_pi16(diff, 8);

      __m64 dst = _mm_add_pi8(diff, src);
      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*2) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<bool use_chroma>
static void layer_yuy2_add_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((ovrp[x*2] - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2-1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * level) >> 8);
      }
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


static void layer_yuy2_fast_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  AVS_UNUSED(level);
  int width_bytes = width * 2;
  int width_mod16 = width_bytes / 16 * 16;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width_mod16; x+=16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(dstp+x));
      __m128i ovr = _mm_load_si128(reinterpret_cast<const __m128i*>(ovrp+x));

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), _mm_avg_epu8(src, ovr));
    }

    for (int x = width_mod16; x < width_bytes; ++x) {
      dstp[x] = (dstp[x] + ovrp[x] + 1) / 2;
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
static void layer_yuy2_fast_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  int width_bytes = width * 2;
  int width_mod8 = width_bytes / 8 * 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width_mod8; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(dstp+x);
      __m64 ovr = *reinterpret_cast<const __m64*>(ovrp+x);

      *reinterpret_cast<__m64*>(dstp+x) = _mm_avg_pu8(src, ovr);
    }

    if (width_mod8 != width_bytes) {
      //two last pixels
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+width_mod8-4));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+width_mod8-4));

      *reinterpret_cast<int*>(dstp+width_bytes-4) = _mm_cvtsi64_si32(_mm_avg_pu8(src, ovr));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<typename pixel_t>
static void layer_yuy2_fast_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  AVS_UNUSED(level);
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width*2; ++x) {
      dstp[x] = (dstp[x] + ovrp[x] + 1) / 2;
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


template<bool use_chroma>
static void layer_yuy2_subtract_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m128i alpha = _mm_set1_epi16(level);
  __m128i zero = _mm_setzero_si128();
  __m128i ff = _mm_set1_epi16(0x00FF);
  __m128i v127 = _mm_set1_epi32(0x007F0000);
  __m128i luma_mask = _mm_set1_epi32(0x000000FF);
  int mod4_width = width / 4 * 4;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod4_width; x+=4) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*2));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*2));

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      if (!use_chroma) {
        ovr = _mm_and_si128(ovr, luma_mask);
        ovr = _mm_or_si128(ovr, v127); //255-127 on the next step will be 128
      }

      __m128i diff = _mm_subs_epi16(ff, ovr);
      diff = _mm_subs_epi16(diff, src);
      diff = _mm_mullo_epi16(diff, alpha);
      diff = _mm_srli_epi16(diff, 8);

      __m128i dst = _mm_add_epi8(diff, src);
      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), dst);
    }

    for (int x = mod4_width; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((255 - ovrp[x*2] - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((255 - ovrp[x*2-1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * level) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static void layer_yuy2_subtract_mmx(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m64 alpha = _mm_set1_pi16(level);
  __m64 zero = _mm_setzero_si64();
  __m64 ff = _mm_set1_pi16(0x00FF);
  __m64 v127 = _mm_set1_pi32(0x007F0000);
  __m64 luma_mask = _mm_set1_pi32(0x000000FF);
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*2));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*2));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      if (!use_chroma) {
        ovr = _mm_and_si64(ovr, luma_mask);
        ovr = _mm_or_si64(ovr, v127); //255-127 on the next step will be 128
      }

      __m64 diff = _mm_subs_pi16(ff, ovr);
      diff = _mm_subs_pi16(diff, src);
      diff = _mm_mullo_pi16(diff, alpha);
      diff = _mm_srli_pi16(diff, 8);

      __m64 dst = _mm_add_pi8(diff, src);
      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*2) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<bool use_chroma>
static void layer_yuy2_subtract_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      dstp[x*2]   = dstp[x*2]  + (((255 - ovrp[x*2] - dstp[x*2]) * level) >> 8);
      if (use_chroma) {
        dstp[x*2+1] = dstp[x*2+1]  + (((255 - ovrp[x*2-1] - dstp[x*2+1]) * level) >> 8);
      } else {
        dstp[x*2+1] = dstp[x*2+1]  + (((128 - dstp[x*2+1]) * level) >> 8);
      }
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


template<int mode>
static void layer_yuy2_lighten_darken_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) {
  int mod4_width = width / 4 * 4;

  __m128i alpha = _mm_set1_epi16(level);
  __m128i zero = _mm_setzero_si128();
  __m128i threshold = _mm_set1_epi32(thresh);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod4_width; x+=4) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*2));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*2));

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i mask;
      if constexpr(mode == LIGHTEN) {
        __m128i temp = _mm_add_epi16(ovr, threshold);
        mask = _mm_cmpgt_epi16(temp, src);
      } else {
        __m128i temp = _mm_add_epi16(src, threshold);
        mask = _mm_cmpgt_epi16(temp, ovr);
      }

      mask = _mm_shufflelo_epi16(mask, _MM_SHUFFLE(2, 2, 0, 0));
      mask = _mm_shufflehi_epi16(mask, _MM_SHUFFLE(2, 2, 0, 0));

      __m128i alpha_mask = _mm_and_si128(mask, alpha);

      __m128i diff = _mm_subs_epi16(ovr, src);
      diff = _mm_mullo_epi16(diff, alpha_mask);
      diff = _mm_srli_epi16(diff, 8);

      __m128i dst = _mm_add_epi8(diff, src);
      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), dst);
    }

    for (int x = mod4_width; x < width; ++x) {
      int alpha_mask;
      if constexpr(mode == LIGHTEN) {
        alpha_mask = (thresh + ovrp[x*2]) > dstp[x*2] ? level : 0;
      } else {
        alpha_mask = (thresh + dstp[x*2]) > ovrp[x*2] ? level : 0;
      }

      dstp[x*2]   = dstp[x*2]  + (((ovrp[x*2] - dstp[x*2]) * alpha_mask) >> 8);
      dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2+1] - dstp[x*2+1]) * alpha_mask) >> 8);
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<int mode>
static void layer_yuy2_lighten_darken_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) {
  __m64 alpha = _mm_set1_pi16(level);
  __m64 zero = _mm_setzero_si64();
  __m64 threshold = _mm_set1_pi32(thresh);
  
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+=2) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*2));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*2));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);
      
      __m64 mask;
      if (mode == LIGHTEN) {
        __m64 temp = _mm_add_pi16(ovr, threshold);
        mask = _mm_cmpgt_pi16(temp, src);
      } else {
        __m64 temp = _mm_add_pi16(src, threshold);
        mask = _mm_cmpgt_pi16(temp, ovr);
      }

      mask = _mm_shuffle_pi16(mask, _MM_SHUFFLE(2, 2, 0, 0));
      __m64 alpha_mask = _mm_and_si64(mask, alpha);

      __m64 diff = _mm_subs_pi16(ovr, src);
      diff = _mm_mullo_pi16(diff, alpha_mask);
      diff = _mm_srli_pi16(diff, 8);

      __m64 dst = _mm_add_pi8(diff, src);
      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*2) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<int mode>
static void layer_yuy2_lighten_darken_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int alpha_mask;
      if constexpr(mode == LIGHTEN) {
        alpha_mask = (thresh + ovrp[x*2]) > dstp[x*2] ? level : 0;
      } else {
        alpha_mask = (thresh + dstp[x*2]) > ovrp[x*2] ? level : 0;
      }

      dstp[x*2]   = dstp[x*2]  + (((ovrp[x*2] - dstp[x*2]) * alpha_mask) >> 8);
      dstp[x*2+1] = dstp[x*2+1]  + (((ovrp[x*2+1] - dstp[x*2+1]) * alpha_mask) >> 8);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

/* RGB32 */

//src format: xx xx xx xx | xx xx xx xx | a1 xx xx xx | a0 xx xx xx
//level_vector and one should be vectors of 32bit packed integers
static __forceinline __m128i calculate_monochrome_alpha_sse2(const __m128i &src, const __m128i &level_vector, const __m128i &one) {
  __m128i alpha = _mm_srli_epi32(src, 24);
  alpha = _mm_mullo_epi16(alpha, level_vector);
  alpha = _mm_add_epi32(alpha, one);
  alpha = _mm_srli_epi32(alpha, 8);
  alpha = _mm_shufflelo_epi16(alpha, _MM_SHUFFLE(2, 2, 0, 0));
  return _mm_shuffle_epi32(alpha, _MM_SHUFFLE(1, 1, 0, 0));
}

static __forceinline __m128i calculate_luma_sse2(const __m128i &src, const __m128i &rgb_coeffs, const __m128i &zero) {
  AVS_UNUSED(zero);
  __m128i temp = _mm_madd_epi16(src, rgb_coeffs); 
  __m128i low = _mm_shuffle_epi32(temp, _MM_SHUFFLE(3, 3, 1, 1));
  temp = _mm_add_epi32(low, temp);
  temp = _mm_srli_epi32(temp, 15);
  __m128i result = _mm_shufflelo_epi16(temp, _MM_SHUFFLE(0, 0, 0, 0));
  return _mm_shufflehi_epi16(result, _MM_SHUFFLE(0, 0, 0, 0));
}

#ifdef X86_32
static __forceinline __m64 calculate_luma_isse(const __m64 &src, const __m64 &rgb_coeffs, const __m64 &zero) {
  __m64 temp = _mm_madd_pi16(src, rgb_coeffs);
  __m64 low = _mm_unpackhi_pi32(temp, zero);
  temp = _mm_add_pi32(low, temp);
  temp = _mm_srli_pi32(temp, 15);
  return _mm_shuffle_pi16(temp, _MM_SHUFFLE(0, 0, 0, 0));
}
#endif

template<bool use_chroma>
static void layer_rgb32_mul_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  int mod2_width = width / 2 * 2;

  __m128i zero = _mm_setzero_si128();
  __m128i level_vector = _mm_set1_epi32(level);
  __m128i one = _mm_set1_epi32(1);
  __m128i rgb_coeffs = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod2_width; x+=2) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*4));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*4));

      __m128i alpha = calculate_monochrome_alpha_sse2(ovr, level_vector, one);

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i luma;
      if (use_chroma) {
        luma = ovr;
      } else {
        luma = calculate_luma_sse2(ovr, rgb_coeffs, zero);
      }

      __m128i dst = _mm_mullo_epi16(luma, src);
      dst = _mm_srli_epi16(dst, 8);
      dst = _mm_subs_epi16(dst, src);
      dst = _mm_mullo_epi16(dst, alpha);
      dst = _mm_srli_epi16(dst, 8);
      dst = _mm_add_epi8(src, dst);

      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*4), dst);
    }

    if (width != mod2_width) {
      int x = mod2_width;
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      if (use_chroma) {
        dstp[x*4]   = dstp[x*4]   + (((((ovrp[x*4]   * dstp[x*4]) >> 8)   - dstp[x*4]  ) * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((((ovrp[x*4+1] * dstp[x*4+1]) >> 8) - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((((ovrp[x*4+2] * dstp[x*4+2]) >> 8) - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((((ovrp[x*4+3] * dstp[x*4+3]) >> 8) - dstp[x*4+3]) * alpha) >> 8);
      } else {
        int luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

        dstp[x*4]   = dstp[x*4]   + (((((luma * dstp[x*4]) >> 8)   - dstp[x*4]  ) * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((((luma * dstp[x*4+1]) >> 8) - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((((luma * dstp[x*4+2]) >> 8) - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((((luma * dstp[x*4+3]) >> 8) - dstp[x*4+3]) * alpha) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static void layer_rgb32_mul_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  __m64 zero = _mm_setzero_si64();
  __m64 rgb_coeffs = _mm_set_pi16(0, cyr, cyg, cyb);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*4));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*4));

      __m64 alpha = _mm_cvtsi32_si64((ovrp[x*4+3] * level + 1) >> 8);
      alpha = _mm_shuffle_pi16(alpha, _MM_SHUFFLE(0,0,0,0));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      __m64 luma;
      if (use_chroma) {
        luma = ovr;
      } else {
        luma = calculate_luma_isse(ovr, rgb_coeffs, zero);
      }

      __m64 dst = _mm_mullo_pi16(luma, src);
      dst = _mm_srli_pi16(dst, 8);
      dst = _mm_subs_pi16(dst, src);
      dst = _mm_mullo_pi16(dst, alpha);
      dst = _mm_srli_pi16(dst, 8);
      dst = _mm_add_pi8(src, dst);

      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*4) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

// For Full Strenth: 8 bit Level must be 257, 16 bit must be 65537!
// in 8 bit:   (255*257+1)/256 = (65535+1)/256 = 256 -> alpha_max = 256
// in 16 bit:  (65535*65537+1)/65536 = 65536, x=? 7FFFFFFF, x=65537 -> alpha_max = 65536

template<typename pixel_t>
static void layer_rgb32_mul_chroma_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;

      dstp[x*4+0] = (pixel_t)(dstp[x*4+0] + ((((((calc_t)ovrp[x*4+0] * dstp[x*4+0]) >> SHIFT) - dstp[x*4+0]) * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + ((((((calc_t)ovrp[x*4+1] * dstp[x*4+1]) >> SHIFT) - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + ((((((calc_t)ovrp[x*4+2] * dstp[x*4+2]) >> SHIFT) - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + ((((((calc_t)ovrp[x*4+3] * dstp[x*4+3]) >> SHIFT) - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

template<typename pixel_t>
static void layer_rgb32_mul_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;
      calc_t luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

      dstp[x*4+0] = (pixel_t)(dstp[x*4+0] + (((((luma * dstp[x*4+0]) >> SHIFT) - dstp[x*4+0]) * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + (((((luma * dstp[x*4+1]) >> SHIFT) - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + (((((luma * dstp[x*4+2]) >> SHIFT) - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + (((((luma * dstp[x*4+3]) >> SHIFT) - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


template<bool use_chroma>
static void layer_rgb32_add_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  int mod2_width = width / 2 * 2;

  __m128i zero = _mm_setzero_si128();
  __m128i level_vector = _mm_set1_epi32(level);
  __m128i one = _mm_set1_epi32(1);
  __m128i rgb_coeffs = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod2_width; x+=2) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*4));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*4));

      __m128i alpha = calculate_monochrome_alpha_sse2(ovr, level_vector, one);

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i luma;
      if (use_chroma) {
        luma = ovr;
      } else {
        luma = calculate_luma_sse2(ovr, rgb_coeffs, zero);
      }

      __m128i dst = _mm_subs_epi16(luma, src);
      dst = _mm_mullo_epi16(dst, alpha);
      dst = _mm_srli_epi16(dst, 8);
      dst = _mm_add_epi8(src, dst);

      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*4), dst);
    }

    if (width != mod2_width) {
      int x = mod2_width;
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      if (use_chroma) {
        dstp[x*4]   = dstp[x*4]   + (((ovrp[x*4]   - dstp[x*4])   * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> 8);
      } else {
        int luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

        dstp[x*4]   = dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static void layer_rgb32_add_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m64 zero = _mm_setzero_si64();
  __m64 rgb_coeffs = _mm_set_pi16(0, cyr, cyg, cyb);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*4));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*4));

      __m64 alpha = _mm_cvtsi32_si64((ovrp[x*4+3] * level + 1) >> 8);
      alpha = _mm_shuffle_pi16(alpha, _MM_SHUFFLE(0,0,0,0));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      __m64 luma;
      if (use_chroma) {
        luma = ovr;
      } else {
        luma = calculate_luma_isse(ovr, rgb_coeffs, zero);
      }

      __m64 dst = _mm_subs_pi16(luma, src);
      dst = _mm_mullo_pi16(dst, alpha);
      dst = _mm_srli_pi16(dst, 8);
      dst = _mm_add_pi8(src, dst);
      
      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*4) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<typename pixel_t>
static void layer_rgb32_add_chroma_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;

      dstp[x*4]   = (pixel_t)(dstp[x*4]   + ((((calc_t)ovrp[x*4]   - dstp[x*4])   * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + ((((calc_t)ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + ((((calc_t)ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + ((((calc_t)ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

template<typename pixel_t>
static void layer_rgb32_add_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> 8;
      calc_t luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

      dstp[x*4]   = (pixel_t)(dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


static void layer_rgb32_fast_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  layer_yuy2_fast_sse2(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
}

#ifdef X86_32
static void layer_rgb32_fast_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  layer_yuy2_fast_isse(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
}
#endif

template<typename pixel_t>
static void layer_rgb32_fast_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  layer_yuy2_fast_c<pixel_t>(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
}


template<bool use_chroma>
static void layer_rgb32_subtract_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  int mod2_width = width / 2 * 2;

  __m128i zero = _mm_setzero_si128();
  __m128i level_vector = _mm_set1_epi32(level);
  __m128i one = _mm_set1_epi32(1);
  __m128i rgb_coeffs = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i ff = _mm_set1_epi16(0x00FF);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod2_width; x+=2) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*4));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*4));

      __m128i alpha = calculate_monochrome_alpha_sse2(ovr, level_vector, one);

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i luma;
      if (use_chroma) {
        luma = _mm_subs_epi16(ff, ovr);
      } else {
        luma = calculate_luma_sse2(_mm_andnot_si128(ovr, ff), rgb_coeffs, zero);
      }

      __m128i dst = _mm_subs_epi16(luma, src);
      dst = _mm_mullo_epi16(dst, alpha);
      dst = _mm_srli_epi16(dst, 8);
      dst = _mm_add_epi8(src, dst);

      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*4), dst);
    }

    if (width != mod2_width) {
      int x = mod2_width;
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      if (use_chroma) {
        dstp[x*4]   = dstp[x*4]   + (((255 - ovrp[x*4]   - dstp[x*4])   * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((255 - ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((255 - ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((255 - ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> 8);
      } else {
        int luma = (cyb * (255 - ovrp[x*4]) + cyg * (255 - ovrp[x*4+1]) + cyr * (255 - ovrp[x*4+2])) >> 15;

        dstp[x*4]   = dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> 8);
        dstp[x*4+1] = dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> 8);
        dstp[x*4+2] = dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> 8);
        dstp[x*4+3] = dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> 8);
      }
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<bool use_chroma>
static void layer_rgb32_subtract_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  __m64 zero = _mm_setzero_si64();
  __m64 rgb_coeffs = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 ff = _mm_set1_pi16(0x00FF);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*4));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*4));

      __m64 alpha = _mm_cvtsi32_si64((ovrp[x*4+3] * level + 1) >> 8);
      alpha = _mm_shuffle_pi16(alpha, _MM_SHUFFLE(0,0,0,0));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      __m64 luma;
      if (use_chroma) {
        luma = _mm_subs_pi16(ff, ovr);
      } else {
        luma = calculate_luma_isse(_mm_andnot_si64(ovr, ff), rgb_coeffs, zero);
      }

      __m64 dst = _mm_subs_pi16(luma, src);
      dst = _mm_mullo_pi16(dst, alpha);
      dst = _mm_srli_pi16(dst, 8);
      dst = _mm_add_pi8(src, dst);

      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*4) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<typename pixel_t>
static void layer_rgb32_subtract_chroma_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  const calc_t MAX_PIXEL_VALUE = sizeof(pixel_t) == 1 ? 255 : 65535;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;

      dstp[x*4]   = (pixel_t)(dstp[x*4]   + (((MAX_PIXEL_VALUE - ovrp[x*4]   - dstp[x*4])   * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + (((MAX_PIXEL_VALUE - ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + (((MAX_PIXEL_VALUE - ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + (((MAX_PIXEL_VALUE - ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

template<typename pixel_t>
static void layer_rgb32_subtract_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  const calc_t MAX_PIXEL_VALUE = sizeof(pixel_t) == 1 ? 255 : 65535;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;
      calc_t luma = (cyb * (MAX_PIXEL_VALUE - ovrp[x*4]) + cyg * (MAX_PIXEL_VALUE - ovrp[x*4+1]) + cyr * (MAX_PIXEL_VALUE - ovrp[x*4+2])) >> 15;

      dstp[x*4]   = (pixel_t)(dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


template<int mode>
static void layer_rgb32_lighten_darken_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) { 
  int mod2_width = width / 2 * 2;

  __m128i zero = _mm_setzero_si128();
  __m128i level_vector = _mm_set1_epi32(level);
  __m128i one = _mm_set1_epi32(1);
  __m128i rgb_coeffs = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i threshold = _mm_set1_epi16(thresh);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < mod2_width; x+=2) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(dstp+x*4));
      __m128i ovr = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(ovrp+x*4));

      __m128i alpha = calculate_monochrome_alpha_sse2(ovr, level_vector, one);

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i luma_ovr = calculate_luma_sse2(ovr, rgb_coeffs, zero);
      __m128i luma_src = calculate_luma_sse2(src, rgb_coeffs, zero);

      __m128i tmp = _mm_add_epi16(threshold, luma_src);
      __m128i mask;
      if constexpr(mode == LIGHTEN) {
        mask = _mm_cmpgt_epi16(luma_ovr, tmp);
      } else {
        mask = _mm_cmpgt_epi16(tmp, luma_ovr);
      }

      alpha = _mm_and_si128(alpha, mask);

      __m128i dst = _mm_subs_epi16(ovr, src);
      dst = _mm_mullo_epi16(dst, alpha);
      dst = _mm_srli_epi16(dst, 8);
      dst = _mm_add_epi8(src, dst);

      dst = _mm_packus_epi16(dst, zero);

      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*4), dst);
    }

    if (width != mod2_width) {
      int x = mod2_width;
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;
      int luma_ovr = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;
      int luma_src = (cyb * dstp[x*4] + cyg * dstp[x*4+1] + cyr * dstp[x*4+2]) >> 15;

      if constexpr(mode == LIGHTEN) {
        alpha = luma_ovr > thresh + luma_src ? alpha : 0;
      } else {
        alpha = luma_ovr < thresh + luma_src ? alpha : 0;
      }

      dstp[x*4]   = dstp[x*4]   + (((ovrp[x*4]   - dstp[x*4])   * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> 8);
    }

    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

#ifdef X86_32
template<int mode>
static void layer_rgb32_lighten_darken_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) { 
  __m64 zero = _mm_setzero_si64();
  __m64 rgb_coeffs = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 threshold = _mm_set1_pi16(thresh);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(dstp+x*4));
      __m64 ovr = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(ovrp+x*4));

      __m64 alpha = _mm_cvtsi32_si64((ovrp[x*4+3] * level + 1) >> 8);
      alpha = _mm_shuffle_pi16(alpha, _MM_SHUFFLE(0,0,0,0));

      src = _mm_unpacklo_pi8(src, zero);
      ovr = _mm_unpacklo_pi8(ovr, zero);

      __m64 luma_ovr = calculate_luma_isse(ovr, rgb_coeffs, zero);
      __m64 luma_src = calculate_luma_isse(src, rgb_coeffs, zero);

      __m64 tmp = _mm_add_pi16(threshold, luma_src);
      __m64 mask;
      if (mode == LIGHTEN) {
        mask = _mm_cmpgt_pi16(luma_ovr, tmp);
      } else {
        mask = _mm_cmpgt_pi16(tmp, luma_ovr);
      }

      alpha = _mm_and_si64(alpha, mask);

      __m64 dst = _mm_subs_pi16(ovr, src);
      dst = _mm_mullo_pi16(dst, alpha);
      dst = _mm_srli_pi16(dst, 8);
      dst = _mm_add_pi8(src, dst);

      dst = _mm_packs_pu16(dst, zero);

      *reinterpret_cast<int*>(dstp+x*4) = _mm_cvtsi64_si32(dst);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
  _mm_empty();
}
#endif

template<int mode, typename pixel_t>
static void layer_rgb32_lighten_darken_c(BYTE* dstp8, const BYTE* ovrp8, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) {
  pixel_t *dstp = reinterpret_cast<pixel_t *>(dstp8);
  const pixel_t *ovrp = reinterpret_cast<const pixel_t *>(ovrp8);
  dst_pitch /= sizeof(pixel_t);
  overlay_pitch /= sizeof(pixel_t);
  const int SHIFT = sizeof(pixel_t) == 1 ? 8 : 16;

  typedef typename std::conditional < sizeof(pixel_t) == 1, int, __int64>::type calc_t;

  const calc_t MAX_PIXEL_VALUE = sizeof(pixel_t) == 1 ? 255 : 65535;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      calc_t alpha = ((calc_t)ovrp[x*4+3] * level + 1) >> SHIFT;
      int luma_ovr = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;
      int luma_src = (cyb * dstp[x*4] + cyg * dstp[x*4+1] + cyr * dstp[x*4+2]) >> 15;

      if constexpr(mode == LIGHTEN) {
        alpha = luma_ovr > thresh + luma_src ? alpha : 0;
      } else {
        alpha = luma_ovr < thresh + luma_src ? alpha : 0;
      }

      dstp[x*4]   = (pixel_t)(dstp[x*4]   + ((((calc_t)ovrp[x*4]   - dstp[x*4])   * alpha) >> SHIFT));
      dstp[x*4+1] = (pixel_t)(dstp[x*4+1] + ((((calc_t)ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> SHIFT));
      dstp[x*4+2] = (pixel_t)(dstp[x*4+2] + ((((calc_t)ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> SHIFT));
      dstp[x*4+3] = (pixel_t)(dstp[x*4+3] + ((((calc_t)ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> SHIFT));
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


PVideoFrame __stdcall Layer::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);

  if (xcount<=0 || ycount<=0) return src1;

  PVideoFrame src2 = child2->GetFrame(min(n,overlay_frames-1), env);

  env->MakeWritable(&src1);

  const int src1_pitch = src1->GetPitch();
  const int src2_pitch = src2->GetPitch();
  const int src2_row_size = src2->GetRowSize();
  const int row_size = src1->GetRowSize();
  const int mylevel = levelB;
  const int height = ycount;
  const int width = xcount;
  BYTE* src1p = src1->GetWritePtr();
  const BYTE* src2p = src2->GetReadPtr();

  if(vi.IsYUY2()) {
    src1p += (src1_pitch * ydest) + (xdest * 2);
    src2p += (src2_pitch * ysrc) + (xsrc * 2);

    int thresh= ((ThresholdParam & 0xFF) <<16)| (ThresholdParam & 0xFF);

    if (!lstrcmpi(Op, "Mul"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16)) 
        {
          layer_yuy2_mul_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_mul_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else
        {
          layer_yuy2_mul_c<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16)) 
        {
          layer_yuy2_mul_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_mul_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          layer_yuy2_mul_c<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Add"))
    {
      if (chroma)
      {
        if (env->GetCPUFlags() & CPUF_SSE2)
        {
          layer_yuy2_add_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_add_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else
        {
          layer_yuy2_add_c<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if (env->GetCPUFlags() & CPUF_SSE2) 
        {
          layer_yuy2_add_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_add_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          layer_yuy2_add_c<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Fast"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16)) 
        {
          layer_yuy2_fast_sse2(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_fast_isse(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else
        {
          layer_yuy2_fast_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
      }
    }
    if (!lstrcmpi(Op, "Subtract"))
    {
      if (chroma) 
      {
        if (env->GetCPUFlags() & CPUF_SSE2) 
        {
          layer_yuy2_subtract_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_subtract_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else
        {
          layer_yuy2_subtract_c<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if (env->GetCPUFlags() & CPUF_SSE2) 
        {
          layer_yuy2_subtract_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_subtract_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          layer_yuy2_subtract_c<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Lighten"))
    {
      if (chroma) 
      {
        if (env->GetCPUFlags() & CPUF_SSE2) 
        {
          layer_yuy2_lighten_darken_sse2<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_lighten_darken_isse<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#endif
        else
        {
          layer_yuy2_lighten_darken_c<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
      } else 
      {
        env->ThrowError("Layer: monochrome lighten illegal op");
      }
    }
    if (!lstrcmpi(Op, "Darken"))
    {
      if (chroma) 
      {
        if (env->GetCPUFlags() & CPUF_SSE2)
        {
          layer_yuy2_lighten_darken_sse2<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
#ifdef X86_32
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_lighten_darken_isse<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#endif
        else
        {
          layer_yuy2_lighten_darken_c<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
      } else 
      {
        env->ThrowError("Layer: monochrome darken illegal op");
      }
    }
  }
  else if (vi.IsRGB32() || vi.IsRGB64())
  {
    int rgb_step = vi.BytesFromPixels(1); // 4 or 8
    int pixelsize = vi.ComponentSize();

    src1p += (src1_pitch * ydest) + (xdest * rgb_step);
    src2p += (src2_pitch * ysrc) + (xsrc * rgb_step);

    int thresh = ThresholdParam & (pixelsize == 1 ? 0xFF : 0xFFFF);

    if (!lstrcmpi(Op, "Mul"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_mul_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_mul_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          if(pixelsize == 1)
            layer_rgb32_mul_chroma_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_mul_chroma_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else // Mul, chroma==false
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_mul_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_mul_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#endif
        else 
        {
          if(pixelsize == 1)
            layer_rgb32_mul_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_mul_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Add"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_add_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_add_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          if(pixelsize == 1)
            layer_rgb32_add_chroma_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_add_chroma_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else // Add, chroma == false
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_add_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_add_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#endif
        else 
        {
          if(pixelsize == 1)
            layer_rgb32_add_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_add_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Lighten"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_lighten_darken_sse2<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_lighten_darken_isse<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#endif
        else 
        {
          if(pixelsize==1)
            layer_rgb32_lighten_darken_c<LIGHTEN, uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
          else
            layer_rgb32_lighten_darken_c<LIGHTEN, uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
      } 
      else 
      {
        env->ThrowError("Layer: monochrome lighten illegal op");
      }
    }
    if (!lstrcmpi(Op, "Darken"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_lighten_darken_sse2<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_lighten_darken_isse<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
#endif
        else 
        {
          if (pixelsize==1)
            layer_rgb32_lighten_darken_c<DARKEN, uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
          else
            layer_rgb32_lighten_darken_c<DARKEN, uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        }
      } 
      else 
      {
        env->ThrowError("Layer: monochrome darken illegal op");
      }
    }
    if (!lstrcmpi(Op, "Fast"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_fast_sse2(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_fast_isse(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          if(pixelsize==1)
            layer_rgb32_fast_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_fast_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
      }
    }
    if (!lstrcmpi(Op, "Subtract"))
    {
      if (chroma) 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_subtract_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_subtract_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#endif
        else 
        {
          if(pixelsize==1)
            layer_rgb32_subtract_chroma_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_subtract_chroma_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_SSE2))
        {
          layer_rgb32_subtract_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
#ifdef X86_32 
        else if ((pixelsize == 1) && (env->GetCPUFlags() & CPUF_INTEGER_SSE))
        {
          layer_rgb32_subtract_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
#endif
        else 
        {
          if(pixelsize==1)
            layer_rgb32_subtract_c<uint8_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
          else
            layer_rgb32_subtract_c<uint16_t>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
  }
  return src1;
}


AVSValue __cdecl Layer::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Layer( args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(-1),
                    args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0), args[7].AsBool(true), env );
}



/**********************************
 *******   Subtract Filter   ******
 *********************************/
bool Subtract::DiffFlag = false;
BYTE Subtract::LUT_Diff8[513];

Subtract::Subtract(PClip _child1, PClip _child2, IScriptEnvironment* env)
  : child1(_child1), child2(_child2)
{
  VideoInfo vi1 = child1->GetVideoInfo();
  VideoInfo vi2 = child2->GetVideoInfo();

  if (vi1.width != vi2.width || vi1.height != vi2.height)
    env->ThrowError("Subtract: image dimensions don't match");

  if (!(vi1.IsSameColorspace(vi2)))
    env->ThrowError("Subtract: image formats don't match");

  vi = vi1;
  vi.num_frames = max(vi1.num_frames, vi2.num_frames);
  vi.num_audio_samples = max(vi1.num_audio_samples, vi2.num_audio_samples);

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (!DiffFlag) { // Init the global Diff table
    DiffFlag = true;
    for (int i=0; i<=512; i++) LUT_Diff8[i] = max(0,min(255,i-129));
    // 0 ..  129  130 131   ... 255 256 257 258     384 ... 512
    // 0 ..   0    1   2  3 ... 126 127 128 129 ... 255 ... 255
  }
}

template<typename pixel_t, int midpixel, bool chroma>
static void subtract_plane(BYTE *src1p, const BYTE *src2p, int src1_pitch, int src2_pitch, int width, int height, int bits_per_pixel)
{
  typedef typename std::conditional < sizeof(pixel_t) == 4, float, int>::type limits_t;

  const limits_t limit_lo = sizeof(pixel_t) <= 2 ? 0 : (limits_t)(chroma ? uv8tof(0) : c8tof(0));
  const limits_t limit_hi = sizeof(pixel_t) == 1 ? 255 : sizeof(pixel_t) == 2 ? ((1 << bits_per_pixel) - 1) : (limits_t)(chroma ? uv8tof(255) : c8tof(255));
  const limits_t equal_luma = sizeof(pixel_t) == 1 ? midpixel : sizeof(pixel_t) == 2 ? (midpixel << (bits_per_pixel - 8)) : (limits_t)( chroma ? uv8tof(midpixel) : c8tof(midpixel));
  for (int y=0; y<height; y++) {
    for (int x=0; x<width; x++) {
      reinterpret_cast<pixel_t *>(src1p)[x] =
        (pixel_t)clamp(
        (limits_t)(reinterpret_cast<pixel_t *>(src1p)[x] - reinterpret_cast<const pixel_t *>(src2p)[x] + equal_luma), // 126: luma of equality
          limit_lo,
          limit_hi);
    }
    src1p += src1_pitch;
    src2p += src2_pitch;
  }
}

PVideoFrame __stdcall Subtract::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(n, env);

  env->MakeWritable(&src1);

  BYTE* src1p = src1->GetWritePtr();
  const BYTE* src2p = src2->GetReadPtr();
  int row_size = src1->GetRowSize();
  int src1_pitch = src1->GetPitch();
  int src2_pitch = src2->GetPitch();

  int width = row_size / pixelsize;
  int height = vi.height;

  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) {
    // alpha
    if (pixelsize == 1) {
      // LUT is a bit faster than clamp version
      for (int y=0; y<vi.height; y++) {
        for (int x=0; x<row_size; x++) {
          src1p[x] = LUT_Diff8[src1p[x] - src2p[x] + 126 + 129];
        }
        src1p += src1->GetPitch();
        src2p += src2->GetPitch();
      }
    } else if (pixelsize==2)
      subtract_plane<uint16_t, 126, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
    else //if (pixelsize==4)
      subtract_plane<float, 126, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);

    // chroma
    row_size=src1->GetRowSize(PLANAR_U);
    if (row_size) {
      width = row_size / pixelsize;
      height = src1->GetHeight(PLANAR_U);
      src1_pitch = src1->GetPitch(PLANAR_U);
      src2_pitch = src2->GetPitch(PLANAR_U);
      // U_plane exists
      BYTE* src1p = src1->GetWritePtr(PLANAR_U);
      const BYTE* src2p = src2->GetReadPtr(PLANAR_U);
      BYTE* src1pV = src1->GetWritePtr(PLANAR_V);
      const BYTE* src2pV = src2->GetReadPtr(PLANAR_V);

      if (pixelsize == 1) {
        // LUT is a bit faster than clamp version
        for (int y=0; y<height; y++) {
          for (int x=0; x<width; x++) {
            src1p[x] = LUT_Diff8[src1p[x] - src2p[x] + 128 + 129];
            src1pV[x] = LUT_Diff8[src1pV[x] - src2pV[x] + 128 + 129];
          }
          src1p += src1_pitch;
          src2p += src2_pitch;
          src1pV += src1_pitch;
          src2pV += src2_pitch;
        }
      } else if (pixelsize==2) {
        subtract_plane<uint16_t, 128, true>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
        subtract_plane<uint16_t, 128, true>(src1pV, src2pV, src1_pitch, src2_pitch, width, height, bits_per_pixel);
      } else { //if (pixelsize==4)
        subtract_plane<float, 128, true>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
        subtract_plane<float, 128, true>(src1pV, src2pV, src1_pitch, src2_pitch, width, height, bits_per_pixel);
      }
    }
    return src1;
  } // End planar YUV

  // For YUY2, 50% gray is about (126,128,128) instead of (128,128,128).  Grr...
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; x+=2) {
        src1p[x] = LUT_Diff8[src1p[x] - src2p[x] + 126 + 129];
        src1p[x+1] = LUT_Diff8[src1p[x+1] - src2p[x+1] + 128 + 129];
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
  }
  else { // RGB
    if (vi.IsPlanarRGB() || vi.IsPlanarRGBA()) {
      const int planesRGB[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A};

      // do not diff Alpha
      for (int p = 0; p < 3; p++) {
        const int plane = planesRGB[p];
        src1p = src1->GetWritePtr(plane);
        src2p = src2->GetReadPtr(plane);
        src1_pitch = src1->GetPitch(plane);
        src2_pitch = src2->GetPitch(plane);
        if(pixelsize==1)
          subtract_plane<uint8_t, 128, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
        else if(pixelsize==2)
          subtract_plane<uint16_t, 128, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
        else
          subtract_plane<float, 128, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
      }
    } else { // packed RGB
      if(pixelsize == 1) {
        for (int y=0; y<vi.height; ++y) {
          for (int x=0; x<row_size; ++x)
            src1p[x] = LUT_Diff8[src1p[x] - src2p[x] + 128 + 129];

          src1p += src1->GetPitch();
          src2p += src2->GetPitch();
        }
      }
      else { // pixelsize == 2: RGB48, RGB64
        // width is getrowsize based here: ok.
        subtract_plane<uint16_t, 128, false>(src1p, src2p, src1_pitch, src2_pitch, width, height, bits_per_pixel);
      }
    }
  }
  return src1;
}



AVSValue __cdecl Subtract::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Subtract(args[0].AsClip(), args[1].AsClip(), env);
}
