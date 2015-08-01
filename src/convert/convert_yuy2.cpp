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

#include "stdafx.h"

#include "convert_yuy2.h"

//  const int cyb = int(0.114*219/255*32768+0.5);  // 0x0C88
//  const int cyg = int(0.587*219/255*32768+0.5);  // 0x4087
//  const int cyr = int(0.299*219/255*32768+0.5);  // 0x20DE

//  const int cyb = int(0.0722*219/255*32768+0.5); // 0x07F0
//  const int cyg = int(0.7152*219/255*32768+0.5); // 0x4E9F
//  const int cyr = int(0.2126*219/255*32768+0.5); // 0x175F

//  const int cyb = int(0.114*32768+0.5);          // 0x0E97
//  const int cyg = int(0.587*32768+0.5);          // 0x4B23
//  const int cyr = int(0.299*32768+0.5);          // 0x2646

//  const int cyb = int(0.0722*32768+0.5);         // 0x093E
//  const int cyg = int(0.7152*32768+0.5);         // 0x5B8C
//  const int cyr = int(0.2126*32768+0.5);         // 0x1B36

//__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
  __declspec(align(8)) static const __int64 cybgr_64[4]  ={0x000020DE40870C88,
                                                           0x0000175F4E9F07F0,
                                                           0x000026464B230E97,
                                                           0x00001B365B8C093E};

  __declspec(align(8)) static const __int64 y1y2_fpix[4] ={0x5033A29E3F74B61E,    //=(1/((1-0.299)*255/112)<<15+0.5),  (1/((1-0.114)*255/112)<<15+0.5)
                                                           0x4766ACDD3C6EB9A3,    //=(1/((1-0.2126)*255/112)<<15+0.5), (1/((1-0.0722)*255/112)<<15+0.5)
                                                           0x5AF1A50F47F4B80C,    //=(1/((1-0.299)*255/127)<<15+0.5),  (1/((1-0.114)*255/127)<<15+0.5)
                                                           0x50F6AF0A44B6BB4A};   //=(1/((1-0.2126)*255/127)<<15+0.5), (1/((1-0.0722)*255/127)<<15+0.5)

  __declspec(align(16)) static const __int64 fpix_add    = 0x0080800000808000;    //=(128.5) << 16
  __declspec(align(16)) static const __int64 fpix_rnd    = 0x0101000001010000;    //=(128.5) << 17

  __declspec(align(16)) static const __int64 sub_64      = 0x0000FFC00000FFC0; // -64

//static const int sub_32      = 0x0000FFE0;    // -32
  static const int sub_16      = 0x0000FFF0;    // -16

  static const int fraction[4] ={0x00084000,    //=(16.5) << 15 = 0x84000
                                 0x00084000,
                                 0x00004000,    //=(0.5) << 15 = 0x4000
                                 0x00004000};



/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, bool _dupl, bool _interlaced, const char *matrix, IScriptEnvironment* env)
  : GenericVideoFilter(_child), interlaced(_interlaced),src_cs(vi.pixel_type)
{
  if (vi.height&3 && vi.IsYV12() && interlaced)
    env->ThrowError("ConvertToYUY2: Cannot convert from interlaced YV12 if height is not multiple of 4. Use Crop!");

  if (vi.height&1 && vi.IsYV12() )
    env->ThrowError("ConvertToYUY2: Cannot convert from YV12 if height is not even. Use Crop!");

  if (vi.width & 1)
    env->ThrowError("ConvertToYUY2: Image width must be even. Use Crop!");

  theMatrix = Rec601;
  if (matrix) {
    if (!vi.IsRGB())
      env->ThrowError("ConvertToYUY2: invalid \"matrix\" parameter (RGB data only)");

    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("ConvertToYUY2: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"PC.601\" or \"PC.709\")");
  }

  if ((env->GetCPUFlags() & CPUF_MMX) && vi.IsRGB()) {  // Generate MMX
    this->GenerateAssembly(vi.IsRGB24(), _dupl, (theMatrix < 2), vi.width,
                           &cybgr_64[theMatrix], &y1y2_fpix[theMatrix],
                           &fraction[theMatrix], env);
  }

  vi.pixel_type = VideoInfo::CS_YUY2;
}

ConvertToYUY2::~ConvertToYUY2() {
  assembly.Free();
}

PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if (((src_cs&VideoInfo::CS_YV12)==VideoInfo::CS_YV12)||((src_cs&VideoInfo::CS_I420)==VideoInfo::CS_I420)) {
    PVideoFrame dst = env->NewVideoFrame(vi,32);  // We need a bit more pitch here.
    BYTE* yuv = dst->GetWritePtr();
    if (interlaced) {
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V),
                      src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V),
                      src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      }
    } else {
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V),
                      src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V),
                      src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      }
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();

  if (env->GetCPUFlags() & CPUF_MMX) {
    mmx_ConvertRGBtoYUY2(src->GetReadPtr(), yuv, src->GetPitch(), dst->GetPitch(), vi.height);
    return dst;
  }

// non MMX machines.

  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) ? 4 : 3;

  if (theMatrix == PC_601) {
    const int cyb = int(0.114*65536+0.5);
    const int cyg = int(0.587*65536+0.5);
    const int cyr = int(0.299*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.299))*65536+0.5);

    inline_rgbtoyuy2(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == PC_709) {
    const int cyb = int(0.0722*65536+0.5);
    const int cyg = int(0.7152*65536+0.5);
    const int cyr = int(0.2126*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.2126))*65536+0.5);

    inline_rgbtoyuy2(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == Rec709) {
    const int cyb = int(0.0722*219/255*65536+0.5);
    const int cyg = int(0.7152*219/255*65536+0.5);
    const int cyr = int(0.2126*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.2126))*65536+0.5);

    inline_rgbtoyuy2(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == Rec601) {
    const int cyb = int(0.114*219/255*65536+0.5);
    const int cyg = int(0.587*219/255*65536+0.5);
    const int cyr = int(0.299*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.299))*65536+0.5);

    inline_rgbtoyuy2(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  }

  return dst;
}

// 1-2-1 Kernel version

inline void ConvertToYUY2::inline_rgbtoyuy2(const bool pcrange, const int cyb, const int cyg, const int cyr,
                                            const int ku, const int kv, const BYTE* rgb,
                                            BYTE* yuv, const int yuv_offset,
                                            const int rgb_offset, const int rgb_inc) {

  const int bias = pcrange ? 0x8000 : 0x108000; //  0.5 * 65536 : 16.5 * 65536

  for (int y=vi.height; y>0; --y)
  {
    // Use left most pixel for edge condition
    int y0                 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + bias) >> 16;
    const BYTE* rgb_prev   = rgb;
    for (int x = 0; x < vi.width; x += 2)
    {
      const BYTE* const rgb_next = rgb + rgb_inc;
      // y1 and y2 can't overflow
      const int y1         = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + bias) >> 16;
      yuv[0]               = (BYTE)y1;
      const int y2         = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + bias) >> 16;
      yuv[2]               = (BYTE)y2;
      if (pcrange) { // This is okay, the compiler optimises out the unused path when pcrange is a constant
        const int scaled_y = y0+y1*2+y2;
        const int b_y      = (rgb_prev[0]+rgb[0]*2+rgb_next[0]) - scaled_y;
        yuv[1]             = PixelClip((b_y * ku + (128<<18) + (1<<17)) >> 18);  // u
        const int r_y      = (rgb_prev[2]+rgb[2]*2+rgb_next[2]) - scaled_y;
        yuv[3]             = PixelClip((r_y * kv + (128<<18) + (1<<17)) >> 18);  // v
      }
      else {
        const int scaled_y = (y0+y1*2+y2 - 64) * int(255.0/219.0*65536+0.5);
        const int b_y      = ((rgb_prev[0]+rgb[0]*2+rgb_next[0]) << 16) - scaled_y;
        yuv[1]             = PixelClip(((b_y >> 12) * ku + (128<<22) + (1<<21)) >> 22);  // u
        const int r_y      = ((rgb_prev[2]+rgb[2]*2+rgb_next[2]) << 16) - scaled_y;
        yuv[3]             = PixelClip(((r_y >> 12) * kv + (128<<22) + (1<<21)) >> 22);  // v
      }
      y0       = y2;

      rgb_prev = rgb_next;
      rgb      = rgb_next + rgb_inc;
      yuv     += 4;
    }
    rgb += rgb_offset;
    yuv += yuv_offset;
  }
}


AVSValue __cdecl ConvertToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;

  const bool haveOpts = args[3].Defined() || args[4].Defined();

  if (clip->GetVideoInfo().IsPlanar()) {
    if (haveOpts || !clip->GetVideoInfo().IsYV12()) {
      // We have no direct conversions. Go to YV16.
      AVSValue new_args[5] = { clip, args[1], args[2], args[3], args[4] };
      clip = ConvertToPlanarGeneric::CreateYV16(AVSValue(new_args, 5), NULL,  env).AsClip();
    }
  }

  if (clip->GetVideoInfo().IsYV16())
    return new ConvertYV16ToYUY2(clip,  env);

  if (haveOpts)
    env->ThrowError("ConvertToYUY2: ChromaPlacement and ChromaResample options are not supported.");

  const bool i=args[1].AsBool(false);
  return new ConvertToYUY2(clip, false, i, args[2].AsString(0), env);
}





/****************************************************
 ******* Convert back to YUY2                  ******
 ******* this only uses Chroma from left pixel ******
 ******* to be used, when signal already has   ******
 ******* been YUY2 to avoid deterioration      ******
 ****************************************************/

ConvertBackToYUY2::ConvertBackToYUY2(PClip _child, const char *matrix, IScriptEnvironment* env)
  : ConvertToYUY2(_child, true, false, matrix, env)
{
  if (!_child->GetVideoInfo().IsRGB() && !_child->GetVideoInfo().IsYV24())
    env->ThrowError("ConvertBackToYUY2: Use ConvertToYUY2 to convert non-RGB material to YUY2.");

  if (_child->GetVideoInfo().IsYV24()) { // vi.IsYUY2
    const int awidth = (vi.width+7) & -8;

    GenerateYV24toYUY2(awidth, vi.height, env);
  }
}


void ConvertBackToYUY2::GenerateYV24toYUY2(int awidth, int height, IScriptEnvironment* env)
{
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);

  enum {             // Argument offsets
    py      = 0,
    pu      = 4,
    pv      = 8,
    dst     = 12,
    pitch1Y = 16,
    pitch1UV= 20,
    pitch2  = 24,
  };

  Assembler x86;   // This is the class that assembles the code.

  // Store registers and get arg pointers
  x86.push(        ebp);
  x86.mov(         ebp, dword_ptr[esp+4+4]);           // Pointer to args list

  x86.push(        eax);
  x86.push(        ebx);
  x86.push(        ecx);
  x86.push(        edx);
  x86.push(        edi);
  x86.push(        esi);

  x86.mov(         ecx, dword_ptr[ebp+py]);
  x86.mov(         edx, dword_ptr[ebp+pu]);
  x86.mov(         esi, dword_ptr[ebp+pv]);
  x86.mov(         edi, dword_ptr[ebp+dst]);
  if (sse2) {
    x86.pcmpeqw(   xmm7, xmm7);                        // ffffffffffffffff
    x86.mov(       ebx, height);
    x86.psrlw(     xmm7, 8);                           // 00ff00ff00ff00ff
  }
  else {
    x86.pcmpeqw(   mm7, mm7);                          // ffffffffffffffff
    x86.mov(       ebx, height);
    x86.psrlw(     mm7, 8);                            // 00ff00ff00ff00ff
  }
  x86.align(    16);

x86.label("yloop");
  x86.xor(         eax, eax);
  x86.align(    16);

x86.label("xloop");
  if (sse2) {
    x86.movq(      xmm1, qword_ptr[edx+eax]);          // 00000000|uUuUuUuU
    x86.movq(      xmm2, qword_ptr[esi+eax]);          // 00000000|vVvVvVvV
    x86.pand(      xmm1, xmm7);                        // 00000000|.U.U.U.U
    x86.psllw(     xmm2, 8);                           // 00000000|V.V.V.V.
    x86.movq(      xmm0, qword_ptr[ecx+eax]);          // 00000000|YYYYYYYY
    x86.por(       xmm1, xmm2);                        // 00000000|VUVUVUVU
    x86.add(       eax, 8);
    x86.punpcklbw( xmm0, xmm1);                        // VYUYVYUYVYUYVYUY
    x86.cmp(       eax, awidth);
    x86.movdqa(    xmmword_ptr[edi+eax*2-16], xmm0);   // store
  }
  else {
    x86.movq(      mm1, qword_ptr[edx+eax]);           // uUuUuUuU
    x86.movq(      mm2, qword_ptr[esi+eax]);           // vVvVvVvV
    x86.pand(      mm1, mm7);                          // .U.U.U.U
    x86.psllw(     mm2, 8);                            // V.V.V.V.
    x86.movq(      mm0, qword_ptr[ecx+eax]);           // YYYYYYYY
    x86.por(       mm1, mm2);                          // VUVUVUVU
    x86.movq(      mm3, mm0);
    x86.punpcklbw( mm0, mm1);                          // VYUYVYUY
    x86.add(       eax, 8);
    x86.punpckhbw( mm3, mm1);                          // VYUYVYUY
    x86.movq(      qword_ptr[edi+eax*2-16], mm0);      // store
    x86.cmp(       eax, awidth);
    x86.movq(      qword_ptr[edi+eax*2-8],  mm3);      // store
  }
  x86.jl("xloop");

  x86.add(         ecx, dword_ptr[ebp+pitch1Y]);
  x86.add(         edx, dword_ptr[ebp+pitch1UV]);
  x86.add(         esi, dword_ptr[ebp+pitch1UV]);
  x86.add(         edi, dword_ptr[ebp+pitch2]);
  x86.dec(         ebx);
  x86.jnz("yloop");
  if (!sse2)
    x86.emms(      );

  x86.pop(         esi);
  x86.pop(         edi);
  x86.pop(         edx);
  x86.pop(         ecx);
  x86.pop(         ebx);
  x86.pop(         eax);
  x86.pop(         ebp);
  x86.ret();

  assembly = DynamicAssembledCode(x86, env, "ConvertBackToYUY2: Dynamic MMX code could not be compiled.");
}


PVideoFrame __stdcall ConvertBackToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if ((src_cs&VideoInfo::CS_YV24)==VideoInfo::CS_YV24) {
    PVideoFrame dst = env->NewVideoFrame(vi, 16); // YUY2 8 pixel aligned
    BYTE* dstp = dst->GetWritePtr();
    const int dpitch  = dst->GetPitch();

    const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
    const BYTE* srcU = src->GetReadPtr(PLANAR_U);
    const BYTE* srcV = src->GetReadPtr(PLANAR_V);

    const int pitchY  = src->GetPitch(PLANAR_Y);
    const int pitchUV = src->GetPitch(PLANAR_U);

    const int awidth = (vi.width+7) & -8;

    if ((pitchY >= awidth) && (env->GetCPUFlags() & CPUF_MMX)) {  // Use MMX
      assembly.Call(srcY, srcU, srcV, dstp, pitchY, pitchUV, dpitch);
      return dst;
    }

    for (int y=0; y<vi.height; y++) {
      for (int x2=0; x2<vi.width; x2+=2) {
        dstp[x2*2+0] = srcY[x2];
        dstp[x2*2+1] = srcU[x2];
        dstp[x2*2+2] = srcY[x2+1];
        dstp[x2*2+3] = srcV[x2];
      }
      srcY += pitchY;
      srcU += pitchUV;
      srcV += pitchUV;
      dstp += dpitch;
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();

  if (env->GetCPUFlags() & CPUF_MMX) {
      mmx_ConvertRGBtoYUY2(src->GetReadPtr(),yuv ,src->GetPitch(), dst->GetPitch(), vi.height);
      return dst;
  }

  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch(); // Last line

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize(); // moving upwards
  const int rgb_inc = (src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32 ? 4 : 3;

/* Existing 0-1-0 Kernel version */
  if (theMatrix == PC_601) {
    const int cyb = int(0.114*65536+0.5);
    const int cyg = int(0.587*65536+0.5);
    const int cyr = int(0.299*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.299))*65536+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[0] = (BYTE)y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = (BYTE)y2;
        const int scaled_y = y1;
        const int b_y = rgb[0] - scaled_y;
        yuv[1] = ScaledPixelClip(b_y * ku + 0x800000);  // u
        const int r_y = rgb[2] - scaled_y;
        yuv[3] = ScaledPixelClip(r_y * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else if (theMatrix == PC_709) {
    const int cyb = int(0.0722*65536+0.5);
    const int cyg = int(0.7152*65536+0.5);
    const int cyr = int(0.2126*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.2126))*65536+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[0] = (BYTE)y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = (BYTE)y2;
        const int scaled_y = y1;
        const int b_y = rgb[0] - scaled_y;
        yuv[1] = ScaledPixelClip(b_y * ku + 0x800000);  // u
        const int r_y = rgb[2] - scaled_y;
        yuv[3] = ScaledPixelClip(r_y * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else if (theMatrix == Rec709) {
    const int cyb = int(0.0722*219/255*65536+0.5);
    const int cyg = int(0.7152*219/255*65536+0.5);
    const int cyr = int(0.2126*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0722))*32768+0.5);
    const int kv  = int(112./(255.*(1.0-0.2126))*32768+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000) >> 16;
        yuv[0] = (BYTE)y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = (BYTE)y2;
        const int scaled_y = (y1 - 16) * int(255.0/219.0*65536+0.5);
        const int b_y = ((rgb[0]) << 16) - scaled_y;
        yuv[1] = ScaledPixelClip((b_y >> 15) * ku + 0x800000);  // u
        const int r_y = ((rgb[2]) << 16) - scaled_y;
        yuv[3] = ScaledPixelClip((r_y >> 15) * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else if (theMatrix == Rec601) {
    const int cyb = int(0.114*219/255*65536+0.5);
    const int cyg = int(0.587*219/255*65536+0.5);
    const int cyr = int(0.299*219/255*65536+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000) >> 16;
        yuv[0] = (BYTE)y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = (BYTE)y2;
        const int scaled_y = (y1 - 16) * int(255.0/219.0*65536+0.5);
        const int b_y = ((rgb[0]) << 16) - scaled_y;
        yuv[1] = ScaledPixelClip((b_y >> 15) * int(1/2.018*32768+0.5) + 0x800000);  // u
        const int r_y = ((rgb[2]) << 16) - scaled_y;
        yuv[3] = ScaledPixelClip((r_y >> 15) * int(1/1.596*32768+0.5) + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  }

  return dst;
}

AVSValue __cdecl ConvertBackToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (!clip->GetVideoInfo().IsYUY2())
    return new ConvertBackToYUY2(clip, args[1].AsString(0), env);

  return clip;
}

  /********************************
   * Dynamic compiled RGB to YUY2 convertion.
   *
   * (c) 2002- 2005 Klaus Post and Ian Brabham
   * (c) 2011       Ian Brabham added 1-2-1 mode
   *
   * rgb24: If true, BGR24 is assumed, otherwise BGR32 is assumed.
   * dupl: Only calculate chroma from the leftmost pixel. Use if material has already been 4:2:2 subsampled.
   * sub: Set to true, if sub_32 has to be subtracted.
   ********************************/

  /********************************
   * - Notes on MMX:
   * Fractions are one bit less than integer code,
   *  but otherwise the algorithm is the same, except
   *  r_y and b_y are calculated at the same time.
   * Order of executin has been changed much for better pairing possibilities.
   * It is important that the 64bit values are 8 byte-aligned
   *  otherwise it will give a huge penalty when accessing them.
   * Instructions pair rather ok, instructions from the top is merged
   *  into last part, to avoid dependency stalls.
   *****************************/


void ConvertToYUY2::mmx_ConvertRGBtoYUY2(const BYTE *src,BYTE *dst,int src_pitch, int dst_pitch, int h) {

  src += src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0;y<h;y++) {
    assembly.Call(src, dst);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
}

/* Code for 1-2-1 & 0-1-0 kernels */

void ConvertToYUY2::GenerateAssembly(bool rgb24, bool dupl, bool sub, int w,
                                     const __int64* ptr_cybgr, const __int64* ptr_y1y2_fpix,
                                     const int* ptr_fraction, IScriptEnvironment* env)  {

  bool sse  = !!(env->GetCPUFlags() & CPUF_SSE);
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool fast128 = !!(env->GetCPUFlags() & (CPUF_SSE3|CPUF_SSSE3|CPUF_SSE4_1|CPUF_SSE4_2));

  if (!fast128 && !dupl)
    sse2 = false; // 1-2-1 SSE2 code is slower than MMX on P4 etc.

  int lwidth_bytes = w;
  lwidth_bytes *= (rgb24) ? 3 : 4;    // Width in bytes

#define SRC esi
#define DST edi
#define RGBEND eax

  Assembler x86;   // This is the class that assembles the code.

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7

  if (!sse2) { // MMX Code

    // Store registers and get arg pointers
    x86.push(        eax);
    x86.mov(         eax,dword_ptr[esp+4+4]);      // Pointer to args list
    x86.push(        SRC);
    x86.mov(         SRC,dword_ptr[eax+0]);        // arg1
    x86.push(        DST);
    x86.mov(         DST,dword_ptr[eax+4]);      // arg2

    x86.movq(        mm2,qword_ptr[SRC]);        //mm2= XXR2 G2B2 XXR1 G1B1
    x86.movq(        mm7,qword_ptr[ptr_cybgr]);
    x86.mov(         RGBEND,lwidth_bytes);
    x86.movd(        mm0,dword_ptr[ptr_fraction]);
    x86.add(         RGBEND,SRC);
    x86.sub(         DST,4);                     // Compensate for early +=4 below
    x86.punpcklbw(   mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
    if (rgb24) {     
      x86.psllq(     mm2,8);                     // Compensate for RGB24
    }                
    x86.cmp(         SRC,RGBEND);
    x86.psrlw(       mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
    x86.jae(         "outloop");                 // Jump out of loop if true (width==0)

    if(!dupl) { // 1-2-1 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
      x86.punpckhbw( mm2,mm0);                   // mm2= 00XX 00R2 00G2 00B2
      x86.movq(      mm6,mm1);                   // mm6 = 00XX 00R0 00G0 00B0
      x86.movq(      mm5,mm2);                   // mm5 = 00XX 00R0 00G0 00B0 for next time
      x86.pmaddwd(   mm2,mm7);                   // mm2 = partial y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1 = partial y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.punpckldq( mm4,mm2);                   // mm4 = 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm4);                   // mm2 = 0000y2.. 0000???? 
      x86.punpckldq( mm4,mm1);                   // mm4 = 00G1+00B1 xxxx xxxx
      x86.psrld(     mm2,15);                    // mm2 = 000000y2 00000000 final value
      x86.paddd(     mm1,mm4);                   // mm1 = 0000y1.. 0000???? 
      x86.movq(      mm3,mm2);                   // mm3 = y2
      x86.psrld(     mm1,15);                    // mm1 = 000000y1 00000000 final value
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm4,qword_ptr[&sub_64]);    // mm4 = -64's
      } else { // PC scale mode
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y2

      x86.align(     16);
      x86.label("loop");

      if (sse)
        x86.prefetchnta(byte_ptr[SRC+64]);

      x86.paddw(     mm6,mm5);                   // mm6 += 00XX 00R2 00G2 00B2
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y1 + y2
      x86.pslld(     mm6,16);                    // Clear out G-value mm6= RRRR 0000 BBBB 0000
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y1*2 + y2
      x86.punpckhdq( mm1,mm2);                   // mm1= 000000y2 000000y1
      x86.movq(      mm2,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm4);                   // mm3 = y0 + y1*2 - 64
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.punpckhdq( mm3,mm3);                   // Copy scaled_y to lower dword mm3=SCALED_Y SCALED_Y
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.por(       mm6,mm3);                   // mm6 = 00r1 00y1 00b1 00y1
      x86.cmp(       SRC,RGBEND);
      x86.pmaddwd(   mm6,mm2);                   // Mult b_y and r_y 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      x86.pcmpeqd(   mm3,mm3);                   // mm3  = ffff ffff ffff ffff
      x86.psrad(     mm6,1);                     // mm6 /= 2
      x86.pslld(     mm3,16);                    // mm3  = ffff 0000 ffff 0000
      x86.paddd(     mm6,mm4);                   // Add 0x808000 to r_y and b_y
      x86.movq(      mm2,qword_ptr[SRC]);        // mm2= XXR2 G2B2 XXR1 G1B1
      x86.pand(      mm6,mm3);                   // Clear out fractions
      x86.movq(      mm3,mm1);                   // Y0 = Y2 for next time
      x86.por(       mm6,mm1);                   // mm6 = 00vv 00Y2 00uu 00Y1
      x86.punpcklbw( mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
      x86.packuswb(  mm6,mm6);                   // mm6 = VVY2 UUY1 VVY2 UUY1
      x86.psrlw(     mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      if (rgb24) {
        x86.psllq(   mm2,8);                     // Compensate for RGB24
      }
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.punpckhbw( mm2,mm0);                   // mm2 = 00XX 00R2 00G2 00B2
      x86.movq(      mm6,mm5);                   // mm6 = 00XX 00R0 00G0 00B0
      x86.movq(      mm5,mm2);                   // mm5 = 00XX 00R0 00G0 00B0 for next time
      x86.pmaddwd(   mm2,mm7);                   // mm2 = partial y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1 = partial y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.punpckldq( mm4,mm2);                   // mm4 = 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm4);                   // mm2 = 0000y2.. 0000???? 
      x86.punpckldq( mm4,mm1);                   // mm4 = 00G1+00B1 xxxx xxxx
      x86.psrld(     mm2,15);                    // mm2 = 000000y2 00000000 final value
      x86.paddd(     mm1,mm4);                   // mm1 = 0000y1.. 0000???? 
      x86.paddw(     mm3,mm2);                   // mm3 = y0 + y2
      x86.psrld(     mm1,15);                    // mm1 = 000000y1 00000000 final value
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm4,qword_ptr[&sub_64]);    // mm4 = -64's
      } else { // PC scale mode
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.jmp(       "loop");                // loop if true

    } else { // 0-1-0 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
      x86.movq(      mm5,qword_ptr[&fpix_add]);  // 0x808000 for add to r_y and b_y

      x86.align(     16);
      x86.label("loop");

      if (sse)
        x86.prefetchnta(byte_ptr[SRC+64]);

      x86.punpckhbw( mm2,mm0);                   // mm2= 00XX 00R2 00G2 00B2

      x86.movq(      mm6,mm1);                   // mm6  = 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.pmaddwd(   mm2,mm7);                   // mm2= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.punpckldq( mm3,mm1);                   // mm3= 00G1+00B1 xxxx xxxx
      x86.punpckldq( mm4,mm2);                   // mm4= 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm3);
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movd(    mm3,dword_ptr[&sub_16]);    // mm3 = -16
      }
      x86.paddd(     mm2,mm4);
      x86.pslld(     mm6,16);                    // Clear out G-value mm6= RRRR 0000 BBBB 0000
      x86.punpckhdq( mm1,mm2);                   // mm1= 000y2... 000y1...
      x86.movq(      mm4,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E
      x86.psrld(     mm1,15);                    // mm1= 000000y2 000000y1 final value

      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm1);                   // mm3 = y1 - 16
      } else { // PC scale mode
        x86.movq(    mm3,mm1);                   // mm3 = y1
      }

      x86.punpckldq( mm3,mm3);                   // Copy scaled_y to upper dword mm3=SCALED_Y SCALED_Y
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.por(       mm6,mm3);                   // mm6 = 00r1 00y1 00b1 00y1
      x86.pcmpeqd(   mm3,mm3);                   // mm3  = ffff ffff ffff ffff
      x86.paddw(     mm6,mm6);                   // mm6 = 0r10 0y10 0b10 0y10 -- Shift up
      x86.pslld(     mm3,16);                    // mm3  = ffff 0000 ffff 0000
      x86.pmaddwd(   mm6,mm4);                   // Mult b_y and r_y 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddd(     mm6,mm5);                   // Add 0x808000 to r_y and b_y
      x86.movq(      mm2,qword_ptr[SRC]);        // mm2= XXR2 G2B2 XXR1 G1B1
      x86.pand(      mm6,mm3);                   // Clear out fractions
      x86.cmp(       SRC,RGBEND);
      x86.por(       mm6,mm1);                   // mm6 = 00vv 00Y2 00uu 00Y1
      x86.punpcklbw( mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
      x86.packuswb(  mm6,mm6);                   // mm6 = VVY2 UUY1 VVY2 UUY1
      x86.psrlw(     mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      if (rgb24) {
        x86.psllq(   mm2,8);                     // Compensate for RGB24
      }
      x86.jb(        "loop");                    // Jump loop if SRC below RGBEND
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
    }

  } else { // SSE2 code

    // Store registers
    x86.push(        eax);
    x86.mov(         eax,dword_ptr[esp+4+4]);    // Pointer to args list
    x86.push(        SRC);
    x86.push(        DST);

    x86.mov(         SRC,dword_ptr[eax+0]);      // arg1
    x86.mov(         DST,dword_ptr[eax+4]);      // arg2
    if (rgb24) {
      x86.sub(       SRC,1);                     // Compensate for RGB24
    }
    x86.sub(         DST,4);                     // Compensate for early +=4 below

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
    x86.movq(        xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 xxr1 g1b1
    x86.movq(        xmm7,qword_ptr[ptr_cybgr]);
    x86.mov(         RGBEND,lwidth_bytes);
    x86.movd(        xmm0,dword_ptr[ptr_fraction]);
    x86.add(         RGBEND,SRC);
    x86.movq(        mm5,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
    x86.prefetchnta( byte_ptr[SRC+64]);
//_______________________________________
    if(!dupl) { // 1-2-1 mode
      x86.cmp(       SRC,RGBEND);
      x86.pxor(      xmm1,xmm1);
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)

      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm0,qword_ptr[&sub_64]);    // mm0  = -64
      }
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.punpcklqdq(xmm7,xmm7);                 // Copy to high qword
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pshufd(    xmm0,xmm0,0x00); // 0000    // Copy to all dwords
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+cyr*R|cyg*G+cyb*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.movq(      mm4,mm3);                   // mm4  = 00XX 00R0 00G0 00B0 first time
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [0000|0000|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.movdq2q(   mm4,xmm3);                  // mm4  = 00XX 00R2 00G2 00B2
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.movq(      mm7,qword_ptr[&fpix_rnd]);  // mm7   = 128.5 << 17 -- 0x0101000001010000
      x86.paddd(     xmm2,xmm4);
      x86.pcmpeqd(   mm6,mm6);                   // mm6  = ffffffff ffffffff
      x86.paddw(     mm3,mm4);                   // mm3  = accumulated RGB values (for b_y and r_y)
      x86.psrld(     xmm2,15);                   // xmm2 = 0000 00y2 0000 00y2 0000 00y1 0000 00y1 final value
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 0000 00y1 0000 00y1
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.pslld(     mm6,16);                    // mm6  = ffff0000 ffff0000
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2
      x86.paddw(     mm3,mm1);                   // Y0 = Y1 for first time

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 XMM6 xmm7
      x86.align(     16);
      x86.label("loop");
//-----------------------------------------------   Unroll 1 mm4, mm6------------------------
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm0);                   // mm3  = y0 - 64
      }
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1 - 64
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 XXr1 g1b1
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1*2 - 64
      x86.punpckldq( mm1,mm2);                   // mm1  = 000000y2 000000y1
      x86.paddw(     mm3,mm2);                   // mm3  = y0 + y1*2 + y2 - 64
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.cmp(       SRC,RGBEND);
      x86.paddd(     mm3,mm7);                   // Add  128.5 to r_y and b_y -- (r_y ?? b_y ?? + 256 0 256 0 + 1 1 1 1) >> 1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00XX 00r1 00g1 00b1
      x86.psrad(     mm3,1);                     // mm3 /= 2

      x86.prefetchnta(byte_ptr[SRC+128]);
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pand(      mm6,mm3);                   // mm6  = .r_y0000 .b_y0000
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.por(       mm6,mm1);                   // mm6  = VVVV 00y2 UUUU 00y1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.packuswb(  mm6,mm6);                   // mm6  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      x86.movdq2q(   mm6,xmm3);                  // mm6  = 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+CYR*R|CYG*G+CYB*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [CYG*G+CYB*B|0*A+CYR*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.paddw(     mm3,mm6);                   // mm3 += 00XX 00R2 00G2 00B2 accumulated RGB values (for b_y and r_y)
      x86.pcmpeqd(   mm4,mm4);                   // mm4  = ffffffff ffffffff
      x86.paddd(     xmm2,xmm4);
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.pslld(     mm4,16);                    // mm4  = ffff0000 ffff0000
      x86.psrld(     xmm2,15);                   // xmm2 = 000000y2 000000y2 000000y1 000000y1 final value
      x86.paddw(     mm3,mm2);                   // mm3  = RRRR 00Y0 BBBB 00Y0
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 000000y1 000000y1
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2

//-----------------------------------------------   Unroll 2 mm6, mm4------------------------
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm0);                   // mm3  = y0 - 64
      }
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1 - 64
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 XXr1 g1b1
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1*2 - 64
      x86.punpckldq( mm1,mm2);                   // mm1  = 000000y2 000000y1
      x86.paddw(     mm3,mm2);                   // mm3  = y0 + y1*2 + y2 - 64
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.cmp(       SRC,RGBEND);
      x86.paddd(     mm3,mm7);                   // Add  128.5 to r_y and b_y -- (r_y ?? b_y ?? + 256 0 256 0 + 1 1 1 1) >> 1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00XX 00r1 00g1 00b1
      x86.psrad(     mm3,1);                     // mm3 /= 2

      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pand(      mm4,mm3);                   // mm4  = .r_y0000 .b_y0000
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.por(       mm4,mm1);                   // mm4  = VVVV 00y2 UUUU 00y1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.packuswb(  mm4,mm4);                   // mm4  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm4);        // Store final pixel
      x86.movdq2q(   mm4,xmm3);                  // mm4  = 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+CYR*R|CYG*G+CYB*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.paddw(     mm3,mm6);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [CYG*G+CYB*B|0*A+CYR*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R2 00G2 00B2 accumulated RGB values (for b_y and r_y)
      x86.pcmpeqd(   mm6,mm6);                   // mm6  = ffffffff ffffffff
      x86.paddd(     xmm2,xmm4);
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.pslld(     mm6,16);                    // mm6  = ffff0000 ffff0000
      x86.psrld(     xmm2,15);                   // xmm2 = 000000y2 000000y2 000000y1 000000y1 final value
      x86.paddw(     mm3,mm2);                   // mm3  = RRRR 00Y0 BBBB 00Y0
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 000000y1 000000y1
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2
//-------------------------------------------------------------------------------------------
      x86.jmp(       "loop");                    // loop if true

    } else { // 0-1-0 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.pcmpeqd( mm1,mm1);                   // 0xFFFFFFFF
        x86.movq(    mm6,qword_ptr[&fpix_add]);  // Add 128.5 to r_y and b_y -- 0x0080800000808000
        x86.pslld(   mm1,21);                    // 0xFFE00000
        x86.cmp(     SRC,RGBEND);
        x86.psrld(   mm1,16);                    // 0x0000FFE0 = 0 -32 0 -32
      } else {
        x86.movq(    mm6,qword_ptr[&fpix_add]);  // Add 128.5 to r_y and b_y -- 0x0080800000808000
        x86.cmp(     SRC,RGBEND);
      }
      x86.pxor(      xmm1,xmm1);
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.pcmpeqd(   mm4,mm4);                   // mm4 = ffffffff ffffffff
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.punpcklqdq(xmm7,xmm7);                 // Copy to high qword
      x86.pshufd(    xmm0,xmm0,0x00); // 0000    // Copy to all dwords
      x86.pslld(     mm4,16);                    // mm4 = ffff0000 ffff0000
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }

      x86.align(     16);
      x86.label("loop");

      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+cyr*R|cyg*G+cyb*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.pslld(     mm3,17);                    // mm3  = 00r1 0000 00b1 0000 *2
      x86.pshufd(    xmm3,xmm2,0xB1); // 2301    // xmm3 = [cyg*G+cyb*B|0*A+cyr*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm1);                   // mm3  += 0 -32 0 -32
      }
      x86.paddd(     xmm2,xmm3);
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.movdq2q(   mm2,xmm2);                  // mm2  = 00y1.... 00y1....
      x86.psrldq(    xmm2,4);                    // xmm2 = 00000000 00y2.... 00y2.... 00y1....
      x86.psrld(     mm2,14);                    // mm2  = 000000y1 000000y1 *2
      x86.movdq2q(   mm0,xmm2);                  // mm0  = 00y2.... 00y1....
      x86.paddw(     mm3,mm2);                   // mm3  = 00r1 00y1 00b1 00y1 *2
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 xxr1 g1b1
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.psrld(     mm0,15);                    // mm0  = 0000 00y1 0000 00y1 final value
      x86.paddd(     mm3,mm6);                   // Add 128.5 to r_y and b_y -- 0x0080800000808000
      x86.prefetchnta(byte_ptr[SRC+128]);
      x86.pand(      mm3,mm4);                   // mm3  = .r_y0000 .b_y0000
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.por(       mm3,mm0);                   // mm3  = VVVV 00y2 UUUU 00y1
      x86.cmp(       SRC,RGBEND);
      x86.packuswb(  mm3,mm3);                   // mm3  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm3);        // Store final pixel
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.jb(        "loop");                    // Jump loop if SRC below RGBEND
    }

  }
  x86.align(     16);
  x86.label("outloop");
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7

  x86.emms(        );
  // Restore registers
  x86.pop(         DST);
  x86.pop(         SRC);
  x86.pop(         eax);
  x86.ret(         );

  assembly = DynamicAssembledCode(x86, env, "ConvertToYUY2: Dynamic MMX code could not be compiled.");
}
