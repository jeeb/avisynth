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
  _mm_empty();
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

PVideoFrame __stdcall ColorKeyMask::GetFrame(int n, IScriptEnvironment *env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);

  BYTE* pf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int rowsize = frame->GetRowSize();

#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_MMX) && (vi.width!=1))
  { // MMX
    const int height = vi.height;
    const int col8 = color;
    const int tol8 = 0xff000000 | (tolR << 16) | (tolG << 8) | tolB;
    const int xloopcount = -(rowsize & -8);
    pf -= xloopcount;
    __asm {
      mov       esi, pf
      mov       edx, height
      pxor      mm0, mm0
      movd      mm1, col8
      movd      mm2, tol8
      punpckldq mm1, mm1
      punpckldq mm2, mm2

yloop:
      mov       ecx, xloopcount
xloop:
      movq      mm3, [esi+ecx]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      add       ecx, 8
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movq      [esi+ecx-8], mm4
      jnz       xloop

      mov       ecx, rowsize
      and       ecx, 7
      jz        not_odd
      ; process last pixel
      movd      mm3, [esi]
      movq      mm4, mm1
      movq      mm5, mm3
      psubusb   mm4, mm3
      psubusb   mm5, mm1
      por       mm4, mm5
      psubusb   mm4, mm2
      pcmpeqd   mm4, mm0
      pslld     mm4, 24
      pandn     mm4, mm3
      movd      [esi], mm4

not_odd:
      add       esi, pitch
      dec       edx
      jnz       yloop
      emms
    }
  }
  else
#endif
  {
    const int R = (color >> 16) & 0xff;
    const int G = (color >> 8) & 0xff;
    const int B = color & 0xff;

    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<rowsize; x+=4) {
        if (IsClose(pf[x],B,tolB) && IsClose(pf[x+1],G,tolG) && IsClose(pf[x+2],R,tolR))
          pf[x+3]=0;
      }
      pf += pitch;
    }
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
	const int myy = ycount;


		__declspec(align(8)) static const __int64 oxooffooffooffooff=0x00ff00ff00ff00ff;  // Luma mask
		__declspec(align(8)) static const __int64 oxffooffooffooffoo=0xff00ff00ff00ff00;  // Chroma mask
		__declspec(align(8)) static const __int64 oxoo80oo80oo80oo80=0x0080008000800080;  // Null Chroma
		__declspec(align(8)) static const __int64 ox7f7f7f7f7f7f7f7f=0x7f7f7f7f7f7f7f7f;  // FAST shift mask
		__declspec(align(8)) static const __int64 ox0101010101010101=0x0101010101010101;  // FAST lsb mask
		__declspec(align(8)) static const __int64 ox00000001        =0x0000000000000001;  // QWORD(1)

	if(vi.IsYUY2()){

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		src1p += (src1_pitch * ydest) + (xdest * 2);
		src2p += (src2_pitch * ysrc) + (xsrc * 2);
		const int myx = xcount >> 1;

		int thresh= ((T & 0xFF) <<16)| (T & 0xFF);

		if (!lstrcmpi(Op, "Mul"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				mulyuy32loop:
						mov         edx, myx
						xor         ecx, ecx
		align 16
						mulyuy32xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							pmullw	mm6,mm7

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							psrlw		mm6,8
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							psrlw		mm4,8							;line up chroma
							psubsw	mm5, mm4

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         mulyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		mulyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				muly032loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						muly032xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							pmullw	mm6,mm7

							pand		mm4,mm3					;mask for chroma

							psrlw		mm6,8
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							psrlw		mm4,8							;line up chroma
							movq		mm5,oxoo80oo80oo80oo80			;get null chroma

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							movq		mm7,mm1
							psrlw		mm7,1
							psubsw	mm5, mm4
							pmullw	mm5, mm7		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         muly032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		muly032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Add"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				addyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						addyuy32xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							psrlw		mm4,8							;line up chroma
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         addyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		addyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				addy032loop:
						mov         edx, myx
						xor         ecx, ecx

				align 16
						addy032xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							movq		mm5,oxoo80oo80oo80oo80			;get null chroma
							psrlw		mm4,8							;line up chroma

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6
						jnz         addy032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		addy032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Fast"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movq			mm0, ox7f7f7f7f7f7f7f7f	;get shift mask
				movq			mm1, ox0101010101010101 ;lsb mask

				fastyuy32loop:
						mov         edx, myx
						xor         ecx, ecx
						shr			edx,1

				    align 16
						fastyuy32xloop:
							//---- fetch src1/dest

							movq		mm7, [edi + ecx*8] ;src1/dest;
							movq		mm6, [esi + ecx*8] ;src2
#if 1			// ------------------------------------------------
							movq		mm5, mm7
							pxor		mm7, mm6
# if 1			// ------------------------------------------------
						// Use (a + b + 1) >> 1 = (a | b) - ((a ^ b) >> 1)
							por			mm6, mm5
							psrlq		mm7, 1		// Fuck Intel! Where is psrlb
							inc         ecx
							pand		mm7, mm0
							psubb		mm6, mm7
# else			// ------------------------------------------------
						// Use (a + b) >> 1 = (a & b) + ((a ^ b) >> 1)
							pand		mm6, mm5
							psrlq		mm7, 1		// Fuck Intel! Where is psrlb
							inc         ecx
							pand		mm7, mm0
							paddb		mm6, mm7
# endif			// ------------------------------------------------
							cmp         ecx, edx
							movq        [edi + ecx*8 - 8],mm6
#else			// ------------------------------------------------
						// Use (a >> 1) + (b >> 1) + (a & 1)
							movq		mm3, mm1
							pand		mm3, mm7
							psrlq		mm6,1
							psrlq		mm7,1
							pand		mm6,mm0
							pand		mm7,mm0

						//----- begin the fun stuff

							paddb		mm6, mm7		  ;fast src1
							paddb		mm6, mm3		  ;fast lsb
						//----- end the fun stuff...

							movq        [edi + ecx*8],mm6

							inc         ecx
							cmp         ecx, edx
#endif			// ------------------------------------------------
						jnz         fastyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		fastyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
			}
		}
		if (!lstrcmpi(Op, "Subtract"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				subyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						subyuy32xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							pcmpeqb	mm5, mm5					;mm5 will be sacrificed
							psrlw		mm4,8							;line up chroma
							psubb		mm5, mm6					;mm5 = 255-mm6
							movq		mm6,mm5					;temp mm6=mm5
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							pand		mm5,mm3					;mask for chroma
							psrlw		mm5,8							;line'em up

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6
						jnz        subyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		subyuy32loop
				emms
				}
			} else {
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm1, mylevel			;alpha
				punpcklwd		mm1,mm1	  ;mm1= 0000|0000|00aa*|00aa*
				punpckldq		mm1, mm1	;mm1= 00aa*|00aa*|00aa*|00aa*
				pxor		mm0,mm0

				suby032loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						suby032xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma
							pand		mm4,mm3					;mask for chroma
							pcmpeqb	mm5, mm5					;mm5 will be sacrificed
							psubb		mm5, mm6					;mm5 = 255-mm6
							movq		mm6,mm5					;temp mm6=mm5
							pand		mm6,mm2					;mask for luma

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							psrlw		mm4,8							;line up chroma
							movq		mm5,oxoo80oo80oo80oo80			;get null chroma

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         suby032xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		suby032loop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Lighten"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo    ; Chroma mask
				movq mm2, oxooffooffooffooff    ; Luma mask
				movd		mm0, mylevel				;alpha
				punpcklwd		mm0,mm0			;mm0= 0000|0000|00aa*|00aa*
				punpckldq		mm0, mm0			;mm0= 00aa*|00aa*|00aa*|00aa*

				lightenyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						lightenyuy32xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movd		mm1, thresh				;we'll need this in a minute
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma	src1__YY__YY__YY__YY
							punpckldq	mm1,mm1					;mm1= 00th|00th|00th|00th
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma	src2__YY__YY__YY__YY

							paddw		mm1, mm6			;add threshold + lum into temporary home
							pand		mm5,mm3					;mask for chroma	src2VV__UU__VV__UU__
							psrlw		mm5,8							;line'em up	src2__VV__UU__VV__UU

							pcmpgtw	mm1, mm7				;see which is greater
							pand			mm1, mm0				;mm1 now has alpha mask

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							pand		mm4,mm3					;mask for chroma	src1VV__UU__VV__UU__
							psrlw		mm4,8							;line up chroma	src1__VV__UU__VV__UU

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         lightenyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		lightenyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome lighten illegal op");
			}
		}
		if (!lstrcmpi(Op, "Darken"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p

				movq mm3, oxffooffooffooffoo     ; Chroma mask
				movq mm2, oxooffooffooffooff     ; Luma mask
				movd		mm0, mylevel				;alpha
				punpcklwd		mm0,mm0			;mm0= 0000|0000|00aa*|00aa*
				punpckldq		mm0, mm0			;mm0= 00aa*|00aa*|00aa*|00aa*

				darkenyuy32loop:
						mov         edx, myx
						xor         ecx, ecx

				    align 16
						darkenyuy32xloop:
							//---- fetch src1/dest

							movd		mm7, [edi + ecx*4] ;src1/dest;
							movd		mm6, [esi + ecx*4] ;src2
							movd		mm1, thresh				;we'll need this in a minute
							movq		mm4,mm7					;temp mm4=mm7
							pand		mm7,mm2					;mask for luma	src1__YY__YY__YY__YY
							punpckldq	mm1,mm1					;mm1= 00th|00th|00th|00th
							pand		mm4,mm3					;mask for chroma	src1VV__UU__VV__UU__
							movq		mm5,mm6					;temp mm5=mm6
							pand		mm6,mm2					;mask for luma	src2__YY__YY__YY__YY
							psrlw		mm4,8							;line up chroma	src1__VV__UU__VV__UU

							paddw		mm1, mm7			;add threshold + lum into temporary home

							pcmpgtw	mm1, mm6				;see which is greater
							pand			mm1, mm0				;mm1 now has alpha mask

							//----- begin the fun (luma) stuff
							psubsw	mm6, mm7
							pmullw	mm6, mm1		;mm6=scaled difference*255

							pand		mm5,mm3					;mask for chroma	src2VV__UU__VV__UU__
							psrlw		mm5,8							;line'em up	src2__VV__UU__VV__UU

							psrlw		mm6, 8		    ;scale result
							paddb		mm6, mm7		  ;add src1
							//----- end the fun stuff...

							//----- begin the fun (chroma) stuff
							psubsw	mm5, mm4
							pmullw	mm5, mm1		;mm5=scaled difference*255

							mov		eax, ecx
							inc         ecx
							cmp         ecx, edx

							psrlw		mm5, 8		    ;scale result
							paddb		mm5, mm4		  ;add src1
							//----- end the fun stuff...

							psllw		mm5,8				;line up chroma
							por			mm6,mm5		;and merge'em back

							movd        [edi + eax*4],mm6

						jnz         darkenyuy32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		darkenyuy32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome darken illegal op");
			}
		}
	}
	else if (vi.IsRGB32())
	{
		const int cyb = int(0.114*32768+0.5);
		const int cyg = int(0.587*32768+0.5);
		const int cyr = int(0.299*32768+0.5);
		__declspec(align(8)) static const __int64 rgb2lum = ((__int64)cyr << 32) | (cyg << 16) | cyb;

		BYTE* src1p = src1->GetWritePtr();
		const BYTE* src2p = src2->GetReadPtr();
		const int myx = xcount;

		src1p += (src1_pitch * ydest) + (xdest * 4);
		src2p += (src2_pitch * ysrc) + (xsrc * 4);

		int thresh = T & 0xFF;

		if (!lstrcmpi(Op, "Mul"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel			;alpha
				pcmpeqd		mm4, mm4
				pxor		mm0, mm0
				psrlq		mm4, 63					; 00000000|00000001

		mul32loop:
				mov         edx, myx
				xor         ecx, ecx

				align 16
		mul32xloop:
				movd		mm6, [esi + ecx*4]		;src2
				 movd		mm7, [edi + ecx*4]		;src1/dest
				movq		mm2, mm6
				 punpcklbw	mm6, mm0				;mm6= 00aa|00rr|00gg|00bb [src2]
		//----- extract alpha into four channels
				psrld		mm2, 24					;mm2= 0000|0000|0000|00aa
				 punpcklbw	mm7, mm0				;mm7= 00aa|00rr|00gg|00bb [src1]
				pmullw		mm2, mm1				;mm2= pixel alpha * script alpha
		//----- begin the fun stuff
				 pmullw		mm6, mm7				;src2*=src1
				paddd		mm2, mm4				;mm2+=1
				 psrlw		mm6, 8					;scale multiply result
				psrld		mm2, 8					;mm2= 00000000|000000aa*
				 psubsw		mm6, mm7				;subtract src1
				punpcklwd	mm2, mm2				;mm2= 0000|0000|00aa*|00aa*
				punpckldq	mm2, mm2				;mm2= 00aa*|00aa*|00aa*|00aa*
		//----- alpha mask now in all four channels of mm2
				pmullw		mm6, mm2		;mm6=scaled difference*alpha
				psrlw		mm6, 8					;scale result
				 mov		eax, ecx
				paddb		mm6, mm7				;add src1
		//----- end the fun stuff...
				 inc		ecx
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd        [edi + eax*4], mm6
				 jnz		mul32xloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			mul32loop
				emms
				}

			} else { // Mul monochrome

				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel
				pcmpeqd		mm4, mm4
				pxor		mm0, mm0
				psrlq		mm4, 63				; 00000000|00000001

		mul32yloop:
				mov         edx, myx
				xor         ecx, ecx

				align 16
		mul32yxloop:
				movd		mm6, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest
				movq		mm2, mm6
				 movq		mm3, rgb2lum
		//----- extract alpha into four channels
				psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				 punpcklbw	mm6, mm0			;mm6= 00aa|00rr|00gg|00bb [src2]
				pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
		//----- start rgb -> monochrome
				pmaddwd		mm6, mm3			;partial monochrome result
				 paddd		mm2, mm4			;mm2+=1
				punpckldq	mm3, mm6			;ready to add
				 psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				paddd		mm6, mm3			;32 bit result
				 punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				psrlq		mm6, 47				;8 bit result
				 punpckldq	mm2, mm2			;mm2= 00aa*|00aa*|00aa*|00aa*
		//----- alpha mask now in all four channels of mm3
				punpcklwd	mm6, mm6			;propagate words
				 punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				punpckldq	mm6, mm6
		//----- end rgb -> monochrome
				pmullw		mm6, mm7
				psrlw		mm6, 8				;scale multiply result
		//----- begin the fun stuff
				psubsw		mm6, mm7
				pmullw		mm6, mm2			;mm6=scaled difference*alpha
				psrlw		mm6, 8				;scale result
				 mov		eax, ecx
				paddb		mm6, mm7			;add src1
				 inc		ecx
		//----- end the fun stuff...
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd        [edi + eax*4], mm6
				 jnz		mul32yxloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			mul32yloop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Add"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel		;alpha
				pcmpeqd		mm4, mm4
				pxor		mm0, mm0
				psrlq		mm4, 63				; 00000000|00000001

		add32loop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		add32xloop:
				movd		mm6, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest
				movq		mm2, mm6
				 punpcklbw	mm6, mm0			;mm6= 00aa|00rr|00gg|00bb [src2]
		//----- extract alpha into four channels
				psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				 punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
		//----- begin the fun stuff
				 psubsw		mm6, mm7
				paddd		mm2, mm4			;mm2+=1
				psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				punpckldq	mm2, mm2			;mm2= 00aa*|00aa*|00aa*|00aa*
		//----- alpha mask now in all four channels of mm2
				pmullw		mm6, mm2			;mm6=scaled difference*alpha
				psrlw		mm6, 8				;scale result
				 mov		eax, ecx
				paddb		mm6, mm7			;add src1
		//----- end the fun stuff...
				 inc		ecx
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd		[edi + eax*4], mm6
				 jnz		add32xloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			add32loop
				emms
				}

			} else { // Add monochrome

				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel
				pcmpeqd		mm4, mm4
				pxor		mm0, mm0
				psrlq		mm4, 63				; 00000000|00000001

		add32yloop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		add32yxloop:
				movd		mm6, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest
				movq		mm2, mm6
				 movq		mm3, rgb2lum
		//----- extract alpha into four channels
				psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				 punpcklbw	mm6, mm0			;mm6= 00aa|00rr|00gg|00bb [src2]
				pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
		//----- start rgb -> monochrome
				 pmaddwd	mm6, mm3			;partial monochrome result
				paddd		mm2, mm4			;mm2+=1
				 punpckldq	mm3, mm6			;ready to add
				psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				 paddd		mm6, mm3			;32 bit result
				punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				 psrlq		mm6, 47				;8 bit result
				punpckldq	mm2, mm2			;mm2= 00aa*|00aa*|00aa*|00aa*
				 punpcklwd	mm6, mm6			;propagate words
				punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				 punpckldq	mm6, mm6
		//----- end rgb -> monochrome
				psubsw		mm6, mm7
				pmullw		mm6, mm2			;mm6=scaled difference*255
				psrlw		mm6, 8				;scale result
				 mov		eax, ecx
				paddb		mm6, mm7			;add src1
		//----- end the fun stuff...
				 inc		ecx
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd		[edi + eax*4], mm6
				jnz			add32yxloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			add32yloop
				emms
				}
			}
		}
		if (!lstrcmpi(Op, "Lighten"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel		;alpha
				pxor		mm0, mm0

		lighten32loop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		lighten32xloop:
				movd		mm4, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest		;what a mess...
				movq		mm2, mm4
				 punpcklbw	mm4, mm0			;mm4= 00aa|00rr|00gg|00bb [src2]
				movq		mm3, rgb2lum
				 movq		mm6, mm4			;make a copy of this for conversion
		//----- start rgb -> monochrome - interleaved pixels: twice the fun!
				pmaddwd		mm4, mm3			;partial monochrome result src2
				 movq		mm5, mm3			;avoid refetching rgb2lum from mem
				punpckldq	mm3, mm4			;ready to add partial products
				 punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				paddd		mm4, mm3			;32 bit monochrome result src
		//----- extract alpha into four channels
				 psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				movq		mm3, mm7			;now get src1
				 pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
				pmaddwd		mm3, mm5			;partial monochrome result src1
				 paddd		mm2, ox00000001		;mm2+=1
				punpckldq	mm5, mm3			;ready to add partial products src2
				 psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				paddd		mm3, mm5			;32 bit result src2
				movd		mm5, thresh			;get threshold
				psrlq		mm3, 47				;8 bit result src1
				 psrlq		mm4, 47				;8 bit result src2
		//----- end rgb -> monochrome
		//----- now monochrome src2 in mm4, monochrome src1 in mm3 can be used for pixel compare
				paddw		mm3, mm5			;add threshold to src1
				 punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				pcmpgtd		mm4, mm3			;and see if src1 still greater
				 punpckldq	mm2, mm2			;mm2= 00aa*|00aa*|00aa*|00aa*
				punpckldq	mm4, mm4			;extend compare result to entire quadword
		//----- alpha mask now in all four channels of mm2
				pand		mm2, mm4
		//----- begin the fun stuff
				psubsw		mm6, mm7
				pmullw		mm6, mm2			;mm6=scaled difference*255
				psrlw		mm6, 8				;now scale result from multiplier
				 mov		eax, ecx			;remember where we are
				paddb		mm6, mm7			;and add src1
		//----- end the fun stuff...
				 inc		ecx					;point to where we are going
				packuswb	mm6, mm0
				 cmp		ecx, edx			;and see if we are done
				movd		[edi + eax*4], mm6
				 jnz		lighten32xloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			lighten32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome lighten illegal op");
			}
		}
		if (!lstrcmpi(Op, "Darken"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel		;alpha
				pxor		mm0, mm0

		darken32loop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		darken32xloop:
				movd		mm4, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest		;what a mess...
				movq		mm2, mm4
				 punpcklbw	mm4, mm0			;mm4= 00aa|00rr|00gg|00bb [src2]
				movq		mm3, rgb2lum
				 movq		mm6, mm4			;make a copy of this for conversion
		//----- start rgb -> monochrome - interleaved pixels: twice the fun!
				pmaddwd		mm4, mm3			;partial monochrome result src2
				 movq		mm5, mm3			;avoid refetching rgb2lum from mem
				punpckldq	mm3, mm4			;ready to add partial products
				 punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				paddd		mm4, mm3			;32 bit monochrome result src
		//----- extract alpha into four channels
				 psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				movq		mm3, mm7			;now get src1
				 pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
				pmaddwd		mm3, mm5			;partial monochrome result src1
				 paddd		mm2, ox00000001		;mm2+=1
				punpckldq	mm5, mm3			;ready to add partial products src2
				 psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				paddd		mm3, mm5			;32 bit result src2
				movd		mm5, thresh			;get threshold
				 psrlq		mm4, 47				;8 bit result src2
				psrlq		mm3, 47				;8 bit result src1
		//----- end rgb -> monochrome
		//----- now monochrome src2 in mm4, monochrome src1 in mm3 can be used for pixel compare
				 paddw		mm4, mm5			;add threshold to src2
				punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				 pcmpgtd	mm3, mm4			;and see if src1 less
				punpckldq	mm2, mm2			;mm2= 00aa*|00aa*|00aa*|00aa*
				 punpckldq	mm3, mm3			;extend compare result to entire quadword
		//----- alpha mask now in all four channels of mm2
				 pand		mm2, mm3
		//----- begin the fun stuff
				psubsw		mm6, mm7
				pmullw		mm6, mm2			;mm6=scaled difference*255
				psrlw		mm6, 8				;now scale result from multiplier
				 mov		eax, ecx			;remember where we are
				paddb		mm6, mm7			;and add src1
		//----- end the fun stuff...
				 inc		ecx					;point to where we are going
				packuswb	mm6, mm0
				 cmp		ecx, edx			;and see if we are done
				movd		[edi + eax*4], mm6
				 jnz		darken32xloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			darken32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: monochrome darken illegal op");
			}
		}
		if (!lstrcmpi(Op, "Fast"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movq			mm0, ox7f7f7f7f7f7f7f7f	;get shift mask
				movq			mm1, ox0101010101010101 ;lsb mask


				fastrgb32loop:
						mov         edx, myx
						xor         ecx, ecx
						shr			edx,1

				align 16
						fastrgb32xloop:
							//---- fetch src1/dest

							movq		mm7, [edi + ecx*8] ;src1/dest;
							movq		mm6, [esi + ecx*8] ;src2
#if 1			// ------------------------------------------------
							movq		mm5, mm7
							pxor		mm7, mm6
# if 1			// ------------------------------------------------
						// Use (a + b + 1) >> 1 = (a | b) - ((a ^ b) >> 1)
							por			mm6, mm5
							psrlq		mm7, 1		// Fuck Intel! Where is psrlb
							inc         ecx
							pand		mm7, mm0
							psubb		mm6, mm7
# else			// ------------------------------------------------
						// Use (a + b) >> 1 = (a & b) + ((a ^ b) >> 1)
							pand		mm6, mm5
							psrlq		mm7, 1		// Fuck Intel! Where is psrlb
							inc         ecx
							pand		mm7, mm0
							paddb		mm6, mm7
# endif			// ------------------------------------------------
							cmp         ecx, edx
							movq        [edi + ecx*8 - 8],mm6
#else			// ------------------------------------------------
						// Use (a >> 1) + (b >> 1) + (a & 1)
							movq		mm3, mm1
							pand		mm3, mm7
							psrlq		mm6,1
							psrlq		mm7,1
							pand		mm6,mm0
							pand		mm7,mm0

						//----- begin the fun stuff

							paddb		mm6, mm7		  ;fast src1
							paddb		mm6, mm3		  ;fast lsb
						//----- end the fun stuff...

							movq        [edi + ecx*8],mm6

							inc         ecx
							cmp         ecx, edx
#endif			// ------------------------------------------------
						jnz         fastrgb32xloop

						add			edi, src1_pitch
						add			esi, src2_pitch
				dec		myy
				jnz		fastrgb32loop
				emms
				}
			} else {
			      env->ThrowError("Layer: this mode not allowed in FAST; use ADD instead");
			}
		}
		if (!lstrcmpi(Op, "Subtract"))
		{
			if (chroma)
			{
				__asm {
				mov			edi, src1p
				mov			esi, src2p
				movd		mm1, mylevel
				pxor		mm0, mm0
				pcmpeqd		mm3, mm3
				pcmpeqb		mm4, mm4
				psrlq		mm3, 63				;00000000|00000001
				punpcklbw	mm4, mm0			;0x00ff00ff00ff00ff

		sub32loop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		sub32xloop:
				movd		mm6, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest
				movq		mm2, mm6
				 punpcklbw	mm6, mm0			;mm6= 00aa|00rr|00gg|00bb [src2]
		//----- extract alpha into four channels
				psrld		mm2, 24				;mm2= 0000|0000|0000|00aa
				 pandn		mm6, mm4			;mm6 =~mm6
				pmullw		mm2, mm1			;mm2= pixel alpha * script alpha
				 punpcklbw	mm7, mm0			;mm7= 00aa|00rr|00gg|00bb [src1]
				paddd		mm2, mm3			;mm2+=1
				psrld		mm2, 8				;mm2= 0000|0000|0000|00aa*
				punpcklwd	mm2, mm2			;mm2= 0000|0000|00aa*|00aa*
				punpckldq	mm2, mm2			;mm2=00aa*|00aa*|00aa*|00aa*
		//----- begin the fun stuff
				psubsw		mm6, mm7
				pmullw		mm6, mm2			;mm6=scaled difference*255
				psrlw		mm6, 8				;scale result
				 mov		eax, ecx
				paddb		mm6, mm7			;add src1
		//----- end the fun stuff...
				 inc		ecx
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd		[edi + eax*4], mm6
				 jnz		sub32xloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			sub32loop
				emms
				}

			} else { // Subtract monochrome

				__asm {
				mov			edi, src1p
				mov			esi, src2p
				mov			eax, src2p
				movd		mm1, mylevel
				pxor		mm0, mm0
				pcmpeqd		mm5, mm5
				pcmpeqb		mm4, mm4
				psrlq		mm5, 63				;00000000|00000001
				punpcklbw	mm4, mm0			;0x00ff00ff00ff00ff

		sub32yloop:
				mov			edx, myx
				xor			ecx, ecx

				align 16
		sub32yxloop:
				movd		mm6, [esi + ecx*4]	;src2
				 movd		mm7, [edi + ecx*4]	;src1/dest
				movq		mm2, mm6
				 movq		mm3, rgb2lum
		//----- extract alpha into four channels
				psrlq		mm2, 24			;mm2= 0000|0000|0000|00aa
				 punpcklbw	mm6, mm0		;mm6= 00aa|00rr|00gg|00bb [src2]
				pmullw		mm2, mm1		;mm2= pixel alpha * script alpha
				 pandn		mm6, mm4		;mm6 =~mm6
				paddd		mm2, mm5		;mm2+=1
		//----- start rgb -> monochrome
				 pmaddwd	mm6, mm3		;partial monochrome result
				psrld		mm2, 8			;mm2= 0000|0000|0000|00aa*
				 punpckldq	mm3, mm6		;ready to add
				punpcklwd	mm2, mm2		;mm2= 0000|0000|00aa*|00aa*
				 paddd		mm6, mm3		;32 bit result
				punpckldq	mm2, mm2		;mm2=00aa*|00aa*|00aa*|00aa*
				 psrlq		mm6, 47			;8 bit result
				punpcklbw	mm7, mm0		;mm7= 00aa|00rr|00gg|00bb [src1]
				 punpcklwd	mm6, mm6		;propagate words
				 punpckldq	mm6, mm6
		//----- end rgb -> monochrome
				psubsw		mm6, mm7
				pmullw		mm6, mm2		;mm6=scaled difference*255
				psrlw		mm6, 8			;scale result
				 mov		eax, ecx
				paddb		mm6, mm7		;add src1
		//----- end the fun stuff...
				 inc		ecx
				packuswb	mm6, mm0
				 cmp		ecx, edx
				movd		[edi + eax*4],mm6
				 jnz		sub32yxloop

				add			edi, src1_pitch
				add			esi, src2_pitch
				dec			myy
				jnz			sub32yloop

				emms
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
