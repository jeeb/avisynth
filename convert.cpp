// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "convert.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Convert_filters[] = {
{ "ConvertToRGB", "c[matrix]s", ConvertToRGB::Create },       // matrix can be "rec709"
  { "ConvertToRGB24", "c[matrix]s", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s", ConvertToRGB::Create32 },
  { "ConvertToYUY2", "c", ConvertToYUY2::Create },  
  { "Greyscale", "c", Greyscale::Create },
  { 0 }
};










/*************************************
 *******   RGB Helper Classes   ******
 ************************************/

RGB24to32::RGB24to32(PClip src) 
  : GenericVideoFilter(src) 
{
  vi.pixel_type = VideoInfo::BGR32;
}


PVideoFrame __stdcall RGB24to32::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE *p = src->GetReadPtr();
  BYTE *q = dst->GetWritePtr();
  for (int y = vi.height; y > 0; --y) {
    for (int x = 0; x < vi.width; ++x) {
      q[x*4+0] = p[x*3+0];
      q[x*4+1] = p[x*3+1];
      q[x*4+2] = p[x*3+2];
      q[x*4+3] = 255;
    }
    p += src->GetPitch();
    q += dst->GetPitch();
  }
  return dst;
}




RGB32to24::RGB32to24(PClip src) 
: GenericVideoFilter(src) 
{
  vi.pixel_type = VideoInfo::BGR24;
}


PVideoFrame __stdcall RGB32to24::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE *p = src->GetReadPtr();
  BYTE *q = dst->GetWritePtr();
  for (int y = vi.height; y > 0; --y) {
    for (int x = 0; x < vi.width; ++x) {
      q[x*3+0] = p[x*4+0];
      q[x*3+1] = p[x*4+1];
      q[x*3+2] = p[x*4+2];
    }
    p += src->GetPitch();
    q += dst->GetPitch();
  }
  return dst;
}







/****************************************
 *******   Convert to RGB / RGBA   ******
 ***************************************/

ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix, 
                            IScriptEnvironment* env ) 
  : GenericVideoFilter(_child) 
{
  rec709 = false;
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      rec709 = true;
    else
      env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter (must be matrix=\"Rec709\")");
  }
  use_mmx = (vi.width & 3) == 0 && (env->GetCPUFlags() & CPUF_MMX);
  if ((rgb24 || rec709) && !use_mmx)
    env->ThrowError("ConvertToRGB: 24-bit RGB and Rec.709 support require MMX and horizontal width a multiple of 4");
  vi.pixel_type = rgb24 ? VideoInfo::BGR24 : VideoInfo::BGR32;
}


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  if (use_mmx) {
    (vi.IsRGB24() ? mmx_YUY2toRGB24 : mmx_YUY2toRGB32)(srcp, dstp,
      srcp + vi.height * src_pitch, src_pitch, src->GetRowSize(), rec709);
  } else {
    srcp += vi.height * src_pitch;
    for (int y=vi.height; y>0; --y) {
      srcp -= src_pitch;
      for (int x=0; x<vi.width; x+=2) {
        YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*4]);
        dstp[x*4+3] = 255;
        YUV2RGB(srcp[x*2+2], srcp[x*2+1], srcp[x*2+3], &dstp[x*4+4]);
        dstp[x*4+7] = 255;
      }
      dstp += dst_pitch;
    }
  }
  return dst;
}


AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  if (clip->GetVideoInfo().IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);
  else
    return clip;
}


AVSValue __cdecl ConvertToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();
  if (vi.IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);
  else if (vi.IsRGB24())
    return new RGB24to32(clip);
  else
    return clip;
}


AVSValue __cdecl ConvertToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();
  if (vi.IsYUV())
    return new ConvertToRGB(clip, true, matrix, env);
  else if (vi.IsRGB32())
    return new RGB32to24(clip);
  else
    return clip;
}












/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child), rgb32(vi.IsRGB32())
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYUY2: image width must be even");
  vi.pixel_type = VideoInfo::YUY2;
}


PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();
  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = rgb32 ? 4 : 3;

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
      yuv[1] = ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);  // u
      const int r_y = ((rgb[2]+rgb_next[2]) << 15) - scaled_y;
      yuv[3] = ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);  // v
      rgb = rgb_next + rgb_inc;
      yuv += 4;
    }
    rgb += rgb_offset;
    yuv += yuv_offset;
  }

  return dst;
}


AVSValue __cdecl ConvertToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return new ConvertToYUY2(clip, env);
  else
    return clip;
}







/*************************************
 *******   Convert to Greyscale ******
 ************************************/

Greyscale::Greyscale(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
//  if (!vi.IsYUY2())
//    env->ThrowError("Greyscale: requires YUY2 input");
}


PVideoFrame Greyscale::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* srcp = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int myy = vi.height;
  int myx = vi.width;
  if (vi.IsYUY2())
	{
	  for (int y=0; y<myy; ++y) {
		for (int x=0; x<myx; x++)
		  srcp[x*2+1] = 127;
		srcp += pitch;
	  }
	} else {
		const int cyb = int(0.114*32768+0.5);
		const int cyg = int(0.587*32768+0.5);
		const int cyr = int(0.299*32768+0.5);
		myx = myx >> 1;
		__int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;

		__asm {
		mov			edi, srcp
		pxor		mm0,mm0
		movq		mm7,rgb2lum

		xor         ecx, ecx
		movq		mm2, [edi + ecx*8]
		mov			ebx, myy
		mov         edx, myx

		rgb2lum_mmxloop:

						movq		mm6, mm2
						movq		mm3,mm7	;get rgb2lum
							movq		mm5,mm6
						punpcklbw		mm6,mm0		;mm6= 00aa|00bb|00gg|00rr [src2]
							punpckhbw		mm5, mm0	 
				//----- start rgb -> monochrome


						pmaddwd			mm6,mm3			;partial monochrome result

						mov		eax, ecx								;pipeline overhead - pointer to ecx-1
						inc         ecx		;loop counter

							movq		mm4, mm7	 ;get rgb2lum again

						movq		mm2, [edi + ecx*8] ; split up otherwise sequential memory access

							pmaddwd			mm5, mm4

						
						punpckldq		mm3,mm6			;ready to add
							punpckldq			mm4, mm5
						paddd			mm6, mm3		  ;32 bit result
							paddd			mm5, mm4
						psrlq			mm6, 47				;8 bit result
							psrlq			mm5, 47
						punpcklwd		mm6, mm6		;propogate words
						punpckldq		mm6, mm6
							punpcklwd		mm5, mm5
							punpckldq		mm5, mm5

						cmp         ecx, edx

						packuswb		mm6,mm0
							packuswb		mm5,mm0
							psllq				mm5,32
							por					mm6, mm5
						movq        [edi + eax*8],mm6

				jnz         rgb2lum_mmxloop

		add			edi, pitch
		mov         edx, myx
		xor         ecx, ecx
		dec		ebx
		jnz		rgb2lum_mmxloop
		emms
		}
	}
  return frame;
}


AVSValue __cdecl Greyscale::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Greyscale(args[0].AsClip(),env);
}
