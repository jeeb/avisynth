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


//  const int cyb = int(0.114*219/255*32768+0.5); // 0x0C88
//  const int cyg = int(0.587*219/255*32768+0.5); // 0x4087
//  const int cyr = int(0.299*219/255*32768+0.5); // 0x20DE
//__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
  __declspec(align(8)) static const __int64 cybgr_64[4] ={0x000020DE40870C88,
                                                          0x0000175C4EA507ED,
                                                          0x000026464B230E97,
                                                          0x00001B335B92093B};

  __declspec(align(8)) static const __int64 fpix_mul[4] ={0x0000503300003F74,    //=(1/((1-0.299)*255/112)<<15+0.5),  (1/((1-0.114)*255/112)<<15+0.5)
                                                          0x0000476400003C97,    //=(1/((1-0.2125)*255/112)<<15+0.5), (1/((1-0.0721)*255/112)<<15+0.5)
                                                          0x00005AF1000047F4,    //=(1/((1-0.299)*255/127)<<15+0.5),  (1/((1-0.114)*255/127)<<15+0.5)
                                                          0x000050F3000044B4};   //=(1/((1-0.2125)*255/127)<<15+0.5), (1/((1-0.0721)*255/127)<<15+0.5)

  __declspec(align(8)) static const __int64 rb_mask     = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 fpix_add    = 0x0080800000808000;    //=(128.5) << 16
  __declspec(align(8)) static const __int64 chroma_mask2= 0xffff0000ffff0000;

  static const int y1y2_mult[4]={0x00004A85,    //=(255./219.) << 14
                                 0x00004A85,
                                 0x00004000,    //=1 << 14
                                 0x00004000};

  static const int fraction[4] ={0x00084000,    //=(16.5) << 15 = 0x84000
                                 0x00084000,
                                 0x00004000,    //=(0.5) << 15 = 0x4000
                                 0x00004000};

  static const int sub_32[4]   ={0x0000FFE0,    //=-16*2
                                 0x0000FFE0,
                                 0x00000000,    //=0
                                 0x00000000};


/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, bool _interlaced, const char *matrix, IScriptEnvironment* env)
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

  vi.pixel_type = VideoInfo::CS_YUY2;
}

PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  if ((env->GetCPUFlags() & CPUF_MMX)) {
	if ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB32toYUY2((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height, theMatrix);
		  __asm { emms }
		  return dst;
    } else  if ((src_cs&VideoInfo::CS_BGR24)==VideoInfo::CS_BGR24) {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB24toYUY2((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height, theMatrix);
		  __asm { emms }
		  return dst;
    }
  }
  if (((src_cs&VideoInfo::CS_YV12)==VideoInfo::CS_YV12)||((src_cs&VideoInfo::CS_I420)==VideoInfo::CS_I420)) {
    PVideoFrame dst = env->NewVideoFrame(vi,32);  // We need a bit more pitch here.
    BYTE* yuv = dst->GetWritePtr();
    if (interlaced) {
		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                    yuv, dst->GetPitch() ,src->GetHeight());
      }
    } else {
		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
                      yuv, dst->GetPitch() ,src->GetHeight());
      }
    }
    return dst;
  }
// non MMX machines.

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();
  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) ? 4 : 3;

  if (theMatrix == PC_601) {
    const int cyb = int(0.114*65536+0.5);
    const int cyg = int(0.587*65536+0.5);
    const int cyr = int(0.299*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.114))*32768+0.5);
    const int kv  = int(127./(255.*(1.0-0.299))*32768+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = y2;
        const int scaled_y = y1+y2;
        const int b_y = (rgb[0]+rgb_next[0]) - scaled_y;
        yuv[1] = ScaledPixelClip(b_y * ku + 0x800000);  // u
        const int r_y = (rgb[2]+rgb_next[2]) - scaled_y;
        yuv[3] = ScaledPixelClip(r_y * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else if (theMatrix == PC_709) {
    const int cyb = int(0.0721*65536+0.5);
    const int cyg = int(0.7154*65536+0.5);
    const int cyr = int(0.2125*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0721))*32768+0.5);
    const int kv  = int(127./(255.*(1.0-0.2125))*32768+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = y2;
        const int scaled_y = y1+y2;
        const int b_y = (rgb[0]+rgb_next[0]) - scaled_y;
        yuv[1] = ScaledPixelClip(b_y * ku + 0x800000);  // u
        const int r_y = (rgb[2]+rgb_next[2]) - scaled_y;
        yuv[3] = ScaledPixelClip(r_y * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else if (theMatrix == Rec709) {
    const int cyb = int(0.0721*219/255*65536+0.5);
    const int cyg = int(0.7154*219/255*65536+0.5);
    const int cyr = int(0.2125*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0721))*32768+0.5);
    const int kv  = int(112./(255.*(1.0-0.2125))*32768+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000) >> 16;
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = y2;
        const int scaled_y = (y1+y2 - 32) * int(255.0/219.0*32768+0.5);
        const int b_y = ((rgb[0]+rgb_next[0]) << 15) - scaled_y;
        yuv[1] = ScaledPixelClip((b_y >> 15) * ku + 0x800000);  // u
        const int r_y = ((rgb[2]+rgb_next[2]) << 15) - scaled_y;
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
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = y2;
        const int scaled_y = (y1+y2 - 32) * int(255.0/219.0*32768+0.5);
        const int b_y = ((rgb[0]+rgb_next[0]) << 15) - scaled_y;
        yuv[1] = ScaledPixelClip((b_y >> 15) * int(1/2.018*32768+0.5) + 0x800000);  // u
        const int r_y = ((rgb[2]+rgb_next[2]) << 15) - scaled_y;
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


AVSValue __cdecl ConvertToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  bool i=args[1].AsBool(false);
  return new ConvertToYUY2(clip, i, args[2].AsString(0), env);
}


/****************************************************
 ******* Convert back to YUY2                  ******
 ******* this only uses Chroma from left pixel ******
 ******* to be used, when signal already has   ******
 ******* been YUY2 to avoid deterioration      ******
 ****************************************************/

ConvertBackToYUY2::ConvertBackToYUY2(PClip _child, const char *matrix, IScriptEnvironment* env)
  : GenericVideoFilter(_child), rgb32(vi.IsRGB32())
{
  if (!vi.IsRGB())
    env->ThrowError("ConvertBackToYUY2: Use ConvertToYUY2 to convert non-RGB material to YUY2.");
  if (vi.width & 1)
    env->ThrowError("ConvertBackToYUY2: image width must be even");

  theMatrix = Rec601;
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("ConvertBackToYUY2: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"PC.601\" or \"PC.709\")");
  }

  vi.pixel_type = VideoInfo::CS_YUY2;
}


PVideoFrame __stdcall ConvertBackToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
	if ((env->GetCPUFlags() & CPUF_MMX)) {
		if (rgb32) {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB32toYUY2_Dup((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height, theMatrix);
			__asm { emms }
			return dst;
		}
		else {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB24toYUY2_Dup((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height, theMatrix);
			__asm { emms }
			return dst;
		}
	}

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();
  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = rgb32 ? 4 : 3;

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
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = y2;
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
    const int cyb = int(0.0721*65536+0.5);
    const int cyg = int(0.7154*65536+0.5);
    const int cyr = int(0.2125*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0721))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.2125))*65536+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;
        yuv[2] = y2;
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
    const int cyb = int(0.0721*219/255*65536+0.5);
    const int cyg = int(0.7154*219/255*65536+0.5);
    const int cyr = int(0.2125*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0721))*32768+0.5);
    const int kv  = int(112./(255.*(1.0-0.2125))*32768+0.5);

    for (int y=vi.height; y>0; --y)
    {
      for (int x = 0; x < vi.width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        const int y1 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000) >> 16;
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = y2;
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
        yuv[0] = y1;
        const int y2 = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;
        yuv[2] = y2;
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


/*******************************
 * MMX RGB32 version
 *******************************/

void mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 0
#define DUPL 0

#include "convert_yuy2.inc"

}


/*******************************
 * MMX RGB24 version
 *******************************/

void mmx_ConvertRGB24toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 1
#define DUPL 0

#include "convert_yuy2.inc"
 
}


/*******************************
 * MMX RGB32 left pixel only version
 *******************************/

void mmx_ConvertRGB32toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 0
#define DUPL 1

#include "convert_yuy2.inc"
 
}


/*******************************
 * MMX RGB24 left pixel only version
 *******************************/

void mmx_ConvertRGB24toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix) {
#define RGB24 1
#define DUPL 1

#include "convert_yuy2.inc"
 
}
