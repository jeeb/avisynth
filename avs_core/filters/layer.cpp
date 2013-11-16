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
#include "../core/internal.h"
#include <emmintrin.h>
#include "avs/alignment.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Layer_filters[] = {
  { "Mask", "cc", Mask::Create },     // clip, mask
  { "ColorKeyMask", "ci[]i[]i[]i", ColorKeyMask::Create },    // clip, color, tolerance[B, toleranceG, toleranceR]
  { "ResetMask", "c", ResetMask::Create },
  { "Invert", "c[channels]s", Invert::Create },
  { "ShowAlpha", "c[pixel_type]s", ShowChannel::Create, (void*)3 },
  { "ShowRed", "c[pixel_type]s", ShowChannel::Create, (void*)2 },
  { "ShowGreen", "c[pixel_type]s", ShowChannel::Create, (void*)1 },
  { "ShowBlue", "c[pixel_type]s", ShowChannel::Create, (void*)0 },
  { "MergeRGB",  "ccc[pixel_type]s", MergeRGB::Create, (void*)0 },
  { "MergeARGB", "cccc",             MergeRGB::Create, (void*)1 },
  { "Layer", "cc[op]s[level]i[x]i[y]i[threshold]i[use_chroma]b", Layer::Create },
  /**
    * Layer(clip, overlayclip, operation, amount, xpos, ypos, [threshold=0], [use_chroma=true])
   **/
  { "Subtract", "cc", Subtract::Create },
  { 0,0,0 }
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
  if (!vi1.IsRGB32() | !vi2.IsRGB32())
    env->ThrowError("Mask error: sources must be RGB32");

  vi = vi1;
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

static void mask_c(BYTE *srcp, const BYTE *alphap, int src_pitch, int alpha_pitch, size_t width, size_t height, int cyb, int cyg, int cyr) {
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; ++x) {
      srcp[x*4+3] = (cyb*alphap[x*4+0] + cyg*alphap[x*4+1] + cyr*alphap[x*4+2] + 16384) >> 15;
    }
    srcp += src_pitch;
    alphap += alpha_pitch;
  }
}

PVideoFrame __stdcall Mask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);
  PVideoFrame src2 = child2->GetFrame(min(n,mask_frames-1), env);


  env->MakeWritable(&src1);

  BYTE* src1p = src1->GetWritePtr();
  const BYTE* src2p = src2->GetReadPtr();

  const int src1_pitch = src1->GetPitch();
  const int src2_pitch = src2->GetPitch();

  const int cyb = int(0.114*32768+0.5);
  const int cyg = int(0.587*32768+0.5);
  const int cyr = int(0.299*32768+0.5);

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16)) 
  {
    mask_sse2(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
  }
  else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) 
  {
    mask_mmx(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
  }
  else
#endif
  {
    mask_c(src1p, src2p, src1_pitch, src2_pitch, vi.width, vi.height, cyb, cyg, cyr);
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
  if (!vi.IsRGB32())
    env->ThrowError("ColorKeyMask: requires RGB32 input");
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
#pragma warning(disable: 4309)
  __m64 tolerance = _mm_set_pi8(0xFF, tolR, tolG, tolB, 0xFF, tolR, tolG, tolB);
#pragma warning(default: 4309)
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

static void colorkeymask_c(BYTE* pf, int pitch, int color, int height, int rowsize, int tolB, int tolG, int tolR) {
  const int R = (color >> 16) & 0xff;
  const int G = (color >> 8) & 0xff;
  const int B = color & 0xff;

  for (int y = 0; y< height; y++) {
    for (int x = 0; x < rowsize; x+=4) {
      if (IsClose(pf[x],B,tolB) && IsClose(pf[x+1],G,tolG) && IsClose(pf[x+2],R,tolR))
        pf[x+3]=0;
    }
    pf += pitch;
  }
}

PVideoFrame __stdcall ColorKeyMask::GetFrame(int n, IScriptEnvironment *env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  BYTE* pf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int rowsize = frame->GetRowSize();

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(pf, 16))
  {
    colorkeymask_sse2(pf, pitch, color, vi.height, rowsize, tolB, tolG, tolR);
  }
  else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX)
  {
    colorkeymask_mmx(pf, pitch, color, vi.height, rowsize, tolB, tolG, tolR);
  }
  else
#endif
  {
    colorkeymask_c(pf, pitch, color, vi.height, rowsize, tolB, tolG, tolR);
  } 

  return frame;
}

AVSValue __cdecl ColorKeyMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ColorKeyMask(args[0].AsClip(), args[1].AsInt(0),
                          args[2].AsInt(10),
                          args[3].AsInt(args[2].AsInt(10)),
                          args[4].AsInt(args[2].AsInt(10)), env);
}


/********************************
 ******  ResetMask filter  ******
 ********************************/


ResetMask::ResetMask(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if (!vi.IsRGB32())
    env->ThrowError("ResetMask: RGB32 data only");
}


PVideoFrame ResetMask::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);
  env->MakeWritable(&f);

  BYTE* pf = f->GetWritePtr();
  int pitch = f->GetPitch();
  int rowsize = f->GetRowSize();
  int height = f->GetHeight();

  for (int i=0; i<height; i++) {
    for (int j=3; j<rowsize; j+=4)
      pf[j] = 255;
    pf += pitch;
  }

  return f;
}


AVSValue ResetMask::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new ResetMask(args[0].AsClip(), env);
}


/********************************
 ******  Invert filter  ******
 ********************************/


Invert::Invert(PClip _child, const char * _channels, IScriptEnvironment* env)
  : GenericVideoFilter(_child), channels(_channels)
{

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
#pragma warning(disable: 4309)
  __m64 maskv = _mm_set1_pi8(0xFF);
#pragma warning(default: 4309)
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

static void invert_plane_c(BYTE* frame, int pitch, int width, int height) {
  int mod4_width = width / 4 * 4;
  for (int y = 0; y < height; ++y) {
    int* intptr = reinterpret_cast<int*>(frame);

    for (int x = 0; x < mod4_width / 4; ++x) {
      intptr[x] = intptr[x] ^ 0xFFFFFFFF;
    }

    for (int x = mod4_width; x < width; ++x) {
      frame[x] = frame[x] ^ 255;
    }
    frame += pitch;
  }
}

static void invert_frame(BYTE* frame, int pitch, int rowsize, int height, int mask, IScriptEnvironment *env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(frame, 16)) 
  {
    invert_frame_sse2(frame, pitch, rowsize, height, mask);
  }
#ifdef X86_32
  else if (env->GetCPUFlags() & CPUF_MMX)
  {
    invert_frame_mmx(frame, pitch, rowsize, height, mask);
  }
#endif
  else 
  {
    invert_frame_c(frame, pitch, rowsize, height, mask);
  }
}

static void invert_plane(BYTE* frame, int pitch, int rowsize, int height, IScriptEnvironment *env) {
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(frame, 16)) 
  {
    invert_frame_sse2(frame, pitch, rowsize, height, 0xffffffff);
  }
#ifdef X86_32
  else if (env->GetCPUFlags() & CPUF_MMX)
  {
    invert_plane_mmx(frame, pitch, rowsize, height);
  }
#endif
  else 
  {
    invert_plane_c(frame, pitch, rowsize, height);
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


  bool doB = false;
  bool doG = false;
  bool doR = false;
  bool doA = false;

  bool doY = false;
  bool doU = false;
  bool doV = false;
  char ch = 1;

  for (int k=0; ch!='\0'; ++k) {
    ch = tolower(channels[k]);
    if (ch == 'b')
      doB = true;
    if (ch == 'g')
      doG = true;
    if (ch == 'r')
      doR = true;
    if (ch == 'a')
      doA = true;

    if (ch == 'y')
      doY = true;
    if (ch == 'u')
      doU = !vi.IsY8();
    if (ch == 'v')
      doV = !vi.IsY8();
  }

  if (vi.IsYUY2()) {
    int mask = doY ? 0x00ff00ff : 0;
    mask |= doU ? 0x0000ff00 : 0;
    mask |= doV ? 0xFF000000 : 0;

    invert_frame(pf, pitch, rowsize, height, mask, env);
  }

  if (vi.IsRGB32()) {
    int mask = doB ? 0xff : 0;
    mask |= doG ? 0xff00 : 0;
    mask |= doR ? 0xff0000 : 0;
    mask |= doA ? 0xff000000 : 0;
    invert_frame(pf, pitch, rowsize, height, mask, env);
  }

  if (vi.IsPlanar()) {
    if (doY)
      invert_plane(pf, pitch, f->GetRowSize(PLANAR_Y_ALIGNED), height, env);
    if (doU)
      invert_plane(f->GetWritePtr(PLANAR_U), f->GetPitch(PLANAR_U), f->GetRowSize(PLANAR_U_ALIGNED), f->GetHeight(PLANAR_U), env);
    if (doV)
      invert_plane(f->GetWritePtr(PLANAR_V), f->GetPitch(PLANAR_V), f->GetRowSize(PLANAR_V_ALIGNED), f->GetHeight(PLANAR_V), env);
  }

  if (vi.IsRGB24()) {
    int rMask= doR ? 0xff : 0;
    int gMask= doG ? 0xff : 0;
    int bMask= doB ? 0xff : 0;
    for (int i=0; i<height; i++) {

      for (int j=0; j<rowsize; j+=3) {
        pf[j] = pf[j] ^ bMask;
        pf[j+1] = pf[j+1] ^ gMask;
        pf[j+2] = pf[j+2] ^ rMask;
      }
      pf += pitch;
    }
  }

  return f;
}


AVSValue Invert::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Invert(args[0].AsClip(), args[0].AsClip()->GetVideoInfo().IsRGB() ? args[1].AsString("RGBA") : args[1].AsString("YUV"), env);
}


/**********************************
 ******  ShowChannel filter  ******
 **********************************/


ShowChannel::ShowChannel(PClip _child, const char * pixel_type, int _channel, IScriptEnvironment* env)
  : GenericVideoFilter(_child), channel(_channel), input_type(_child->GetVideoInfo().pixel_type)
{
  static const char * const ShowText[4] = {"Blue", "Green", "Red", "Alpha"};

  if ((channel == 3) && !vi.IsRGB32())
    env->ThrowError("ShowAlpha: RGB32 data only");

  if (!vi.IsRGB())
    env->ThrowError("Show%s: RGB data only", ShowText[channel]);

  if (!lstrcmpi(pixel_type, "rgb")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
  }
  else if (!lstrcmpi(pixel_type, "rgb32")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
  }
  else if (!lstrcmpi(pixel_type, "rgb24")) {
    vi.pixel_type = VideoInfo::CS_BGR24;
  }
  else if (!lstrcmpi(pixel_type, "yuy2")) {
    if (vi.width & 1) {
      env->ThrowError("Show%s: width must be mod 2 for yuy2", ShowText[channel]);
    }
    vi.pixel_type = VideoInfo::CS_YUY2;
  }
  else if (!lstrcmpi(pixel_type, "yv12")) {
    if (vi.width & 1) {
      env->ThrowError("Show%s: width must be mod 2 for yv12", ShowText[channel]);
    }
    if (vi.height & 1) {
      env->ThrowError("Show%s: height must be mod 2 for yv12", ShowText[channel]);
    }
    vi.pixel_type = VideoInfo::CS_YV12;
  }
  else if (!lstrcmpi(pixel_type, "y8")) {
    vi.pixel_type = VideoInfo::CS_Y8;
  }
  else {
    env->ThrowError("Show%s supports the following output pixel types: RGB, Y8, YUY2, or YV12", ShowText[channel]);
  }
}


PVideoFrame ShowChannel::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame f = child->GetFrame(n, env);

  const BYTE* pf = f->GetReadPtr();
  const int height = f->GetHeight();
  const int pitch = f->GetPitch();
  const int rowsize = f->GetRowSize();

  if (input_type == VideoInfo::CS_BGR32) {
    if (vi.pixel_type == VideoInfo::CS_BGR32)
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=4) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
          }
          dstp += pitch;
        }
        return f;
      }
      else {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=4) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
            dstp[j + 3] = pf[j + 3];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
        return dst;
      }
    }
    else if (vi.pixel_type == VideoInfo::CS_BGR24)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();

      for (int i=0; i<height; ++i) {
        for (int j=0; j<rowsize/4; j++) {
          dstp[j*3 + 0] = dstp[j*3 + 1] = dstp[j*3 + 2] = pf[j*4 + channel];
        }
        pf   += pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else if (vi.pixel_type == VideoInfo::CS_YUY2)
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
    {
      if ((vi.pixel_type == VideoInfo::CS_YV12) || (vi.pixel_type == VideoInfo::CS_Y8))
      {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstrowsize = dst->GetRowSize();

        // RGB is upside-down
        pf += (height-1) * pitch;

        for (int i=0; i<height; ++i) {
          for (int j=0; j<dstrowsize; ++j) {
            dstp[j] = pf[j*4 + channel];
          }
          pf -= pitch;
          dstp += dstpitch;
        }
        if (vi.pixel_type == VideoInfo::CS_YV12)
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          dstrowsize = dst->GetRowSize(PLANAR_U_ALIGNED)/4;
          const int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstpu = dst->GetWritePtr(PLANAR_U);
          BYTE * dstpv = dst->GetWritePtr(PLANAR_V);
          for (int i=0; i<dstheight; ++i) {
            for (int j=0; j<dstrowsize; ++j) {
              ((unsigned int*) dstpu)[j] = ((unsigned int*) dstpv)[j] = 0x80808080;
            }
            dstpu += dstpitch;
            dstpv += dstpitch;
          }
        }
        return dst;
      }
    }
  }
  else if (input_type == VideoInfo::CS_BGR24) {
    if (vi.pixel_type == VideoInfo::CS_BGR24)
    {
      if (f->IsWritable()) {
        // we can do it in-place
        BYTE* dstp = f->GetWritePtr();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=3) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = dstp[j + channel];
          }
          dstp += pitch;
        }
        return f;
      }
      else {
        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        const int dstpitch = dst->GetPitch();

        for (int i=0; i<height; ++i) {
          for (int j=0; j<rowsize; j+=3) {
            dstp[j + 0] = dstp[j + 1] = dstp[j + 2] = pf[j + channel];
          }
          pf   += pitch;
          dstp += dstpitch;
        }
        return dst;
      }
    }
    else if (vi.pixel_type == VideoInfo::CS_BGR32)
    {
      PVideoFrame dst = env->NewVideoFrame(vi);
      BYTE * dstp = dst->GetWritePtr();
      const int dstpitch = dst->GetPitch();

      for (int i=0; i<height; ++i) {
        for (int j=0; j<rowsize/3; j++) {
          dstp[j*4 + 0] = dstp[j*4 + 1] = dstp[j*4 + 2] = dstp[j*4 + 3] = pf[j*3 + channel];
        }
        pf   += pitch;
        dstp += dstpitch;
      }
      return dst;
    }
    else if (vi.pixel_type == VideoInfo::CS_YUY2)
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
    {
      if ((vi.pixel_type == VideoInfo::CS_YV12) || (vi.pixel_type == VideoInfo::CS_Y8))
      {
        int i, j;  // stupid VC6

        PVideoFrame dst = env->NewVideoFrame(vi);
        BYTE * dstp = dst->GetWritePtr();
        int dstpitch = dst->GetPitch();
        int dstrowsize = dst->GetRowSize();

        // RGB is upside-down
        pf += (height-1) * pitch;

        for (i=0; i<height; ++i) {
          for (j=0; j<dstrowsize; ++j) {
            dstp[j] = pf[j*3 + channel];
          }
          pf -= pitch;
          dstp += dstpitch;
        }
        if (vi.pixel_type == VideoInfo::CS_YV12)
        {
          dstpitch = dst->GetPitch(PLANAR_U);
          dstrowsize = dst->GetRowSize(PLANAR_U_ALIGNED)/4;
          const int dstheight = dst->GetHeight(PLANAR_U);
          BYTE * dstpu = dst->GetWritePtr(PLANAR_U);
          BYTE * dstpv = dst->GetWritePtr(PLANAR_V);
          for (i=0; i<dstheight; ++i) {
            for (j=0; j<dstrowsize; ++j) {
              ((unsigned int*) dstpu)[j] = ((unsigned int*) dstpv)[j] = 0x80808080;
            }
            dstpu += dstpitch;
            dstpv += dstpitch;
          }
        }
        return dst;
      }
    }
  }
  env->ThrowError("ShowChannel: unexpected end of function");
  return f;
}


AVSValue ShowChannel::Create(AVSValue args, void* channel, IScriptEnvironment* env)
{
  return new ShowChannel(args[0].AsClip(), args[1].AsString("RGB"), (int)channel, env);
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

  if (!lstrcmpi(pixel_type, "rgb32")) {
    vi.pixel_type = VideoInfo::CS_BGR32;
    if (alpha && (viA.pixel_type == VideoInfo::CS_BGR24))
      env->ThrowError("MergeARGB: Alpha source channel may not be RGB24");
  }
  else if (!lstrcmpi(pixel_type, "rgb24")) {
    vi.pixel_type = VideoInfo::CS_BGR24;
  }
  else {
    env->ThrowError("MergeRGB: supports the following output pixel types: RGB24, or RGB32");
  }

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

  // RGB is upside-down, backscan any YUV to match
  const int Bpitch = (viB.IsYUV()) ? -(B->GetPitch()) : B->GetPitch();
  const int Gpitch = (viG.IsYUV()) ? -(G->GetPitch()) : G->GetPitch();
  const int Rpitch = (viR.IsYUV()) ? -(R->GetPitch()) : R->GetPitch();

  // Bump any RGB channels, move any YUV channels to last line
  const BYTE* Bp = B->GetReadPtr() + ((Bpitch < 0) ? Bpitch * (1-height) : 0);
  const BYTE* Gp = G->GetReadPtr() + ((Gpitch < 0) ? Gpitch * (1-height) : 1);
  const BYTE* Rp = R->GetReadPtr() + ((Rpitch < 0) ? Rpitch * (1-height) : 2);

  // Adjustment from the end of 1 line to the start of the next
  const int Bmodulo = Bpitch - B->GetRowSize();
  const int Gmodulo = Gpitch - G->GetRowSize();
  const int Rmodulo = Rpitch - R->GetRowSize();

  // Number of bytes per pixel (1, 2, 3 or 4)
  const int Bstride = viB.IsPlanar() ? 1 : (viB.BitsPerPixel()>>3);
  const int Gstride = viG.IsPlanar() ? 1 : (viG.BitsPerPixel()>>3);
  const int Rstride = viR.IsPlanar() ? 1 : (viR.BitsPerPixel()>>3);

  // End of VFB
  BYTE const * yend = dstp + pitch*height;

  if (alpha) { // ARGB mode
    const int Apitch = (viA.IsYUV()) ? -(A->GetPitch()) : A->GetPitch();
    const BYTE* Ap = A->GetReadPtr() + ((Apitch < 0) ? Apitch * (1-height) : 3);
    const int Amodulo = Apitch - A->GetRowSize();
    const int Astride = viA.IsPlanar() ? 1 : (viA.BitsPerPixel()>>3);

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
  }
  else if (vi.pixel_type == VideoInfo::CS_BGR32) { // RGB32 mode
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
  }
  else if (vi.pixel_type == VideoInfo::CS_BGR24) { // RGB24 mode
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
  }
  else
    env->ThrowError("%s: unexpected end of function", myname);

  return dst;
}


AVSValue MergeRGB::Create(AVSValue args, void* mode, IScriptEnvironment* env)
{
  if (mode) // ARGB
    return new MergeRGB(args[0].AsClip(), args[3].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), "RGB32", env);
  else      // RGB[type]
    return new MergeRGB(args[0].AsClip(), args[2].AsClip(), args[1].AsClip(), args[0].AsClip(), 0, args[3].AsString("RGB32"), env);
}


/*******************************
 *******   Layer Filter   ******
 *******************************/

Layer::Layer( PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y,
              int _t, bool _chroma, IScriptEnvironment* env )
  : child1(_child1), child2(_child2), levelB(_lev), ofsX(_x), ofsY(_y), Op(_op),
    T(_t), chroma(_chroma)
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();

  if (vi1.pixel_type != vi2.pixel_type)
    env->ThrowError("Layer: image formats don't match");

  if (! (vi1.IsRGB32() | vi1.IsYUY2()) )
    env->ThrowError("Layer only support RGB32 and YUY2 formats");

  vi = vi1;

  if (vi.IsRGB32()) ofsY = vi.height-vi2.height-ofsY; //RGB is upside down
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

__declspec(align(8)) static const __int64 oxooffooffooffooff=0x00ff00ff00ff00ff;  // Luma mask
__declspec(align(8)) static const __int64 oxffooffooffooffoo=0xff00ff00ff00ff00;  // Chroma mask
__declspec(align(8)) static const __int64 oxoo80oo80oo80oo80=0x0080008000800080;  // Null Chroma
__declspec(align(8)) static const __int64 ox7f7f7f7f7f7f7f7f=0x7f7f7f7f7f7f7f7f;  // FAST shift mask
__declspec(align(8)) static const __int64 ox0101010101010101=0x0101010101010101;  // FAST lsb mask
__declspec(align(8)) static const __int64 ox00000001        =0x0000000000000001;  // QWORD(1)

const int cyb = int(0.114*32768+0.5);
const int cyg = int(0.587*32768+0.5);
const int cyr = int(0.299*32768+0.5);
__declspec(align(8)) static const __int64 rgb2lum = ((__int64)cyr << 32) | (cyg << 16) | cyb;

enum
{
  LIGHTEN = 0,
  DARKEN = 1
};

#ifdef X86_32

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

static void layer_yuy2_fast_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
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
      if (mode == LIGHTEN) {
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
      if (mode == LIGHTEN) {
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

template<int mode>
static void layer_yuy2_lighten_darken_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; ++x) {
      int alpha_mask;
      if (mode == LIGHTEN) {
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
  __m128i temp = _mm_madd_epi16(src, rgb_coeffs); 
  __m128i low = _mm_shuffle_epi32(temp, _MM_SHUFFLE(3, 3, 1, 1));
  temp = _mm_add_epi32(low, temp);
  temp = _mm_srli_epi32(temp, 15);
  __m128i result = _mm_shufflelo_epi16(temp, _MM_SHUFFLE(0, 0, 0, 0));
  return _mm_shufflehi_epi16(result, _MM_SHUFFLE(0, 0, 0, 0));
}

static __forceinline __m64 calculate_luma_isse(const __m64 &src, const __m64 &rgb_coeffs, const __m64 &zero) {
  __m64 temp = _mm_madd_pi16(src, rgb_coeffs);
  __m64 low = _mm_unpackhi_pi32(temp, zero);
  temp = _mm_add_pi32(low, temp);
  temp = _mm_srli_pi32(temp, 15);
  return _mm_shuffle_pi16(temp, _MM_SHUFFLE(0, 0, 0, 0));
}

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

      __m128i alpha = calculate_monochrome_alpha_sse2(src, level_vector, one);

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

static void layer_rgb32_mul_chroma_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      dstp[x*4]   = dstp[x*4]   + (((((ovrp[x*4]   * dstp[x*4]) >> 8)   - dstp[x*4]  ) * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((((ovrp[x*4+1] * dstp[x*4+1]) >> 8) - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((((ovrp[x*4+2] * dstp[x*4+2]) >> 8) - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((((ovrp[x*4+3] * dstp[x*4+3]) >> 8) - dstp[x*4+3]) * alpha) >> 8);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

static void layer_rgb32_mul_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;
      int luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

      dstp[x*4]   = dstp[x*4]   + (((((luma * dstp[x*4]) >> 8)   - dstp[x*4]  ) * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((((luma * dstp[x*4+1]) >> 8) - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((((luma * dstp[x*4+2]) >> 8) - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((((luma * dstp[x*4+3]) >> 8) - dstp[x*4+3]) * alpha) >> 8);
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

      __m128i alpha = calculate_monochrome_alpha_sse2(src, level_vector, one);

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

static void layer_rgb32_add_chroma_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      dstp[x*4]   = dstp[x*4]   + (((ovrp[x*4]   - dstp[x*4])   * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> 8);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

static void layer_rgb32_add_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;
      int luma = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;

      dstp[x*4]   = dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> 8);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}


static void layer_rgb32_fast_sse2(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  layer_yuy2_fast_sse2(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
}

static void layer_rgb32_fast_isse(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) {
  layer_yuy2_fast_isse(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
}

static void layer_rgb32_fast_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  layer_yuy2_fast_c(dstp, ovrp, dst_pitch, overlay_pitch, width*2, height, level);
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

      __m128i alpha = calculate_monochrome_alpha_sse2(src, level_vector, one);

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

static void layer_rgb32_subtract_chroma_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;

      dstp[x*4]   = dstp[x*4]   + (((255 - ovrp[x*4]   - dstp[x*4])   * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((255 - ovrp[x*4+1] - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((255 - ovrp[x*4+2] - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((255 - ovrp[x*4+3] - dstp[x*4+3]) * alpha) >> 8);
    }
    dstp += dst_pitch;
    ovrp += overlay_pitch;
  }
}

static void layer_rgb32_subtract_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;
      int luma = (cyb * (255 - ovrp[x*4]) + cyg * (255 - ovrp[x*4+1]) + cyr * (255 - ovrp[x*4+2])) >> 15;

      dstp[x*4]   = dstp[x*4]   + (((luma - dstp[x*4])   * alpha) >> 8);
      dstp[x*4+1] = dstp[x*4+1] + (((luma - dstp[x*4+1]) * alpha) >> 8);
      dstp[x*4+2] = dstp[x*4+2] + (((luma - dstp[x*4+2]) * alpha) >> 8);
      dstp[x*4+3] = dstp[x*4+3] + (((luma - dstp[x*4+3]) * alpha) >> 8);
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

      __m128i alpha = calculate_monochrome_alpha_sse2(src, level_vector, one);

      src = _mm_unpacklo_epi8(src, zero);
      ovr = _mm_unpacklo_epi8(ovr, zero);

      __m128i luma_ovr = calculate_luma_sse2(ovr, rgb_coeffs, zero);
      __m128i luma_src = calculate_luma_sse2(src, rgb_coeffs, zero);

      __m128i tmp = _mm_add_epi16(threshold, luma_src);
      __m128i mask;
      if (mode == LIGHTEN) {
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

      if (mode == LIGHTEN) {
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

template<int mode>
static void layer_rgb32_lighten_darken_c(BYTE* dstp, const BYTE* ovrp, int dst_pitch, int overlay_pitch, int width, int height, int level, int thresh) { 
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width ; ++x) {
      int alpha = (ovrp[x*4+3] * level + 1) >> 8;
      int luma_ovr = (cyb * ovrp[x*4] + cyg * ovrp[x*4+1] + cyr * ovrp[x*4+2]) >> 15;
      int luma_src = (cyb * dstp[x*4] + cyg * dstp[x*4+1] + cyr * dstp[x*4+2]) >> 15;

      if (mode == LIGHTEN) {
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

#endif

PVideoFrame __stdcall Layer::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src1 = child1->GetFrame(n, env);

#ifdef X86_32
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

    int thresh= ((T & 0xFF) <<16)| (T & 0xFF);

    if (!lstrcmpi(Op, "Mul"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16)) 
        {
          layer_yuy2_mul_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_mul_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_mul_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_add_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_add_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_fast_isse(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else
        {
          layer_yuy2_fast_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
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
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_subtract_mmx<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_MMX) 
        {
          layer_yuy2_subtract_mmx<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
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
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_lighten_darken_isse<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
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
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_yuy2_lighten_darken_isse<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
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
  else if (vi.IsRGB32())
  {
    src1p += (src1_pitch * ydest) + (xdest * 4);
    src2p += (src2_pitch * ysrc) + (xsrc * 4);

    int thresh = T & 0xFF;

    if (!lstrcmpi(Op, "Mul"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_mul_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_mul_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else 
        {
          layer_rgb32_mul_chroma_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_mul_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_mul_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
        else 
        {
          layer_rgb32_mul_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Add"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_add_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_add_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else 
        {
          layer_rgb32_add_chroma_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_add_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_add_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
        else 
        {
          layer_rgb32_add_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
    if (!lstrcmpi(Op, "Lighten"))
    {
      if (chroma) 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_lighten_darken_sse2<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_lighten_darken_isse<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
        else 
        {
          layer_rgb32_lighten_darken_c<LIGHTEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
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
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_lighten_darken_sse2<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_lighten_darken_isse<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
        } 
        else 
        {
          layer_rgb32_lighten_darken_c<DARKEN>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel, thresh);
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
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_fast_sse2(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_fast_isse(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else 
        {
          layer_rgb32_fast_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
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
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_subtract_sse2<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_subtract_isse<true>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else 
        {
          layer_rgb32_subtract_chroma_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      } 
      else 
      {
        if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src1p, 16) && IsPtrAligned(src2p, 16))
        {
          layer_rgb32_subtract_sse2<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        } 
        else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) 
        {
          layer_rgb32_subtract_isse<false>(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
        else 
        {
          layer_rgb32_subtract_c(src1p, src2p, src1_pitch, src2_pitch, width, height, mylevel);
        }
      }
    }
  }
#else
  //TODO
  env->ThrowError("Layer::GetFrame is not yet ported to 64-bit.");
#endif
  return src1;
}


AVSValue __cdecl Layer::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Layer( args[0].AsClip(), args[1].AsClip(), args[2].AsString("Add"), args[3].AsInt(257),
                    args[4].AsInt(0), args[5].AsInt(0), args[6].AsInt(0), args[7].AsBool(true), env );
}



/**********************************
 *******   Subtract Filter   ******
 *********************************/
bool Subtract::DiffFlag = false;
BYTE Subtract::Diff[513];

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

  if (!DiffFlag) { // Init the global Diff table
    DiffFlag = true;
    for (int i=0; i<=512; i++) Diff[i] = max(0,min(255,i-129));
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

  if (vi.IsPlanar()) {
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<row_size; x++) {
        src1p[x] = Diff[src1p[x] - src2p[x] + 126 + 129];
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }

    row_size=src1->GetRowSize(PLANAR_U);
    if (row_size) {
      BYTE* src1p = src1->GetWritePtr(PLANAR_U);
      const BYTE* src2p = src2->GetReadPtr(PLANAR_U);
      BYTE* src1pV = src1->GetWritePtr(PLANAR_V);
      const BYTE* src2pV = src2->GetReadPtr(PLANAR_V);

      for (int y=0; y<src1->GetHeight(PLANAR_U); y++) {
        for (int x=0; x<row_size; x++) {
          src1p[x] = Diff[src1p[x] - src2p[x] + 128 + 129];
          src1pV[x] = Diff[src1pV[x] - src2pV[x] + 128 + 129];
        }
        src1p += src1->GetPitch(PLANAR_U);
        src2p += src2->GetPitch(PLANAR_U);
        src1pV += src1->GetPitch(PLANAR_V);
        src2pV += src2->GetPitch(PLANAR_V);
      }
    }
    return src1;
  } // End planar

  // For YUY2, 50% gray is about (126,128,128) instead of (128,128,128).  Grr...
  if (vi.IsYUY2()) {
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; x+=2) {
        src1p[x] = Diff[src1p[x] - src2p[x] + 126 + 129];
        src1p[x+1] = Diff[src1p[x+1] - src2p[x+1] + 128 + 129];
      }
      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
  }
  else { // RGB
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<row_size; ++x)
        src1p[x] = Diff[src1p[x] - src2p[x] + 128 + 129];

      src1p += src1->GetPitch();
      src2p += src2->GetPitch();
    }
  }
  return src1;
}



AVSValue __cdecl Subtract::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Subtract(args[0].AsClip(), args[1].AsClip(), env);
}
