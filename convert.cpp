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


#include "convert.h"
#include "convert_xvid.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Convert_filters[] = {
{ "ConvertToRGB", "c[matrix]s", ConvertToRGB::Create },       // matrix can be "rec709"
  { "ConvertToRGB24", "c[matrix]s", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s", ConvertToRGB::Create32 },
  { "ConvertToYV12", "c[interlaced]b", ConvertToYV12::Create },  
  { "ConvertToYUY2", "c[interlaced]b", ConvertToYUY2::Create },  
  { "ConvertBackToYUY2", "c", ConvertBackToYUY2::Create },  
  { "Greyscale", "c", Greyscale::Create },
  { 0 }
};










/*************************************
 *******   RGB Helper Classes   ******
 ************************************/

RGB24to32::RGB24to32(PClip src) 
  : GenericVideoFilter(src) 
{
  vi.pixel_type = VideoInfo::CS_BGR32;
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
  vi.pixel_type = VideoInfo::CS_BGR24;
}


PVideoFrame __stdcall RGB32to24::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE *p = src->GetReadPtr();
  BYTE *q = dst->GetWritePtr();
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const int h=vi.height;
  int x_loops=vi.width; // 4 dwords/loop   read 16 bytes, write 12 bytes
  const int x_left=vi.width%4;
  x_loops-=x_left;
  x_loops*=4;
  __declspec(align(8)) static const __int64 oxooooooooooffffff=0x0000000000ffffff;
  __declspec(align(8)) static const __int64 oxooffffffoooooooo=0x00ffffff00000000;

  __asm {
    mov esi,p
    mov edi,q
    mov eax,[h]
    mov edx,[x_left];
    movq mm6,[oxooooooooooffffff];
    movq mm7,[oxooffffffoooooooo];
    align 16
yloop:
    mov ebx,0  ; src offset
    mov ecx,0  ; dst offset
xloop:
    movq mm0,[ebx+esi]    ; a1r1 g1b1 a0r0 g0b0
    movq mm1,[ebx+esi+8]  ; a3r3 g3b3 a2r2 g2b2
    
    movq mm2,mm0      ; a1r1 g1b1 a0r0 g0b0
    movq mm3,mm1      ; a3r3 g3b3 a2r2 g2b2
    
    pand mm0,mm6      ; 0000 0000 00r0 g0b0
    pand mm1,mm6      ; 0000 0000 00r2 g2b2 

    pand mm2,mm7      ; 00r1 g1b1 0000 0000
    pand mm3,mm7      ; 00r3 g3b3 0000 0000

    movq mm4,mm1      ; 0000 0000 00r2 g2b2 
    psrlq mm2,8       ; 0000 r1g1 b100 0000
    
    psllq mm4,48      ; g2b2 0000 0000 0000
    por mm0,mm2       ; 0000 r1g1 b1r0 g0b0

    psrlq mm1,16      ; 0000 0000 0000 00r2
    por mm0,mm4

    psrlq mm3,24      ; 0000 0000 b3g3 r300
    movq [ecx+edi],mm0

    por mm3,mm1
    movd [ecx+edi+8],mm3

    add ebx,16
    add ecx,12
    cmp ebx,[x_loops]
    jl xloop

    
    cmp edx,0
    je no_copy
    cmp edx,2
    je copy_2
    cmp edx,1
    je copy_1
//copy 3
    add esi,ebx
    add edi,ecx
    movsb 
    movsb
    movsb
    inc esi
    movsb
    movsb
    movsb
    inc esi
    movsb
    movsb
    movsb
    sub esi,ebx
    sub edi,ecx
    sub esi,11
    sub edi,9
    jmp no_copy
    align 16
copy_2:
    add esi,ebx
    add edi,ecx
    movsb
    movsb
    movsb
    inc esi
    movsb
    movsb
    movsb
    sub esi,ebx
    sub edi,ecx
    sub esi,7
    sub edi,6
    jmp no_copy
    align 16
copy_1:
    add esi,ebx
    add edi,ecx
    movsb
    movsb
    movsb
    sub esi,ebx
    sub edi,ecx
    sub esi,3
    sub edi,3
    align 16
no_copy:
    add esi,[src_pitch]
    add edi,[dst_pitch]    
    
    dec eax
    jnz yloop
    emms
  }
  
/*  
  for (int y = vi.height; y > 0; --y) {
    for (int x = 0; x < vi.width; ++x) {
      q[x*3+0] = p[x*4+0];
      q[x*3+1] = p[x*4+1];
      q[x*3+2] = p[x*4+2];
    }
    p += src->GetPitch();
    q += dst->GetPitch();
  }
  */
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
  is_yv12=false;
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      rec709 = true;
    else
      env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter (must be matrix=\"Rec709\")");
  }
  use_mmx = (vi.width & 3) == 0 && (env->GetCPUFlags() & CPUF_MMX);
  if (vi.IsYV12()) {
    is_yv12=true;
    yv12_width=vi.width;
    if (vi.width&7) {
      vi.width += 8 - (vi.width & 7);
      yv12_width=vi.width;
    }
    vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
    return;
  }

  if ((rgb24 || rec709) && !use_mmx)
    env->ThrowError("ConvertToRGB: 24-bit RGB and Rec.709 support require MMX and horizontal width a multiple of 4");
  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;

}


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  if (is_yv12) {
    if (vi.IsRGB24()) {
      yv12_to_rgb24_mmx(dstp, dst_pitch/3,src->GetReadPtr(PLANAR_Y),src->GetReadPtr(PLANAR_U),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_Y),src->GetPitch(PLANAR_U),yv12_width,-src->GetHeight(PLANAR_Y));
    } else {
      yv12_to_rgb32_mmx(dstp, dst_pitch/4,src->GetReadPtr(PLANAR_Y),src->GetReadPtr(PLANAR_U),src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_Y),src->GetPitch(PLANAR_U),yv12_width,-src->GetHeight(PLANAR_Y));
    }
    return dst;
  }  
  // assumption: is_yuy2
  if (use_mmx) {
    (vi.IsRGB24() ? mmx_YUY2toRGB24 : mmx_YUY2toRGB32)(srcp, dstp,
      srcp + vi.height * src_pitch, src_pitch, src->GetRowSize(), rec709);
  } else if (vi.IsRGB32()) {    
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
  } else if (vi.IsRGB24()) {
    srcp += vi.height * src_pitch;
    for (int y=vi.height; y>0; --y) {
      srcp -= src_pitch;
      for (int x=0; x<vi.width; x+=2) {
        YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*3]);
        YUV2RGB(srcp[x*2+2], srcp[x*2+1], srcp[x*2+3], &dstp[x*3+3]);
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
  const VideoInfo vi = clip->GetVideoInfo();
  if (vi.IsYUV()) {
    if (vi.IsYV12()) {
      if (vi.width&7) {
        return new Crop(0,0,-(8-(vi.width&7)),0,new ConvertToRGB(clip, false, matrix, env),env);
      }
    }
    return new ConvertToRGB(clip, false, matrix, env);
  } else if (vi.IsRGB24())
    return new RGB24to32(clip);
  else
    return clip;
}


AVSValue __cdecl ConvertToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();
  if (vi.IsYUV()) {
    if (vi.IsYV12()) {
      if (vi.width&7) {
        return new Crop(0,0,-(8-(vi.width&7)),0,new ConvertToRGB(clip, true, matrix, env),env);
      }
    }
    return new ConvertToRGB(clip, true, matrix, env);
  } else if (vi.IsRGB32())
    return new RGB32to24(clip);
  else
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
    env->ThrowError("ConvertToYV12: image width must be multiple of 2");
  if (interlaced && (vi.height & 3))
    env->ThrowError("ConvertToYV12: Interlaced image height must be multiple of 4");
  if ((!interlaced) && (vi.height & 1))
    env->ThrowError("ConvertToYV12: image height must be multiple of 2");
  isYUY2=isRGB32=isRGB24=false;
  if (vi.IsYUY2()) isYUY2 = true;
  if (vi.IsRGB32()) isRGB32 = true;
  if (vi.IsRGB24()) isRGB24 = true;
 
  vi.pixel_type = VideoInfo::CS_YV12;

}

PVideoFrame __stdcall ConvertToYV12::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  if (isRGB32) {
    PVideoFrame dst = env->NewVideoFrame(vi,-8);
    rgb32_to_yv12_mmx(dst->GetWritePtr(PLANAR_Y),dst->GetWritePtr(PLANAR_U),dst->GetWritePtr(PLANAR_V),src->GetReadPtr(),src->GetRowSize()/4,src->GetHeight(),src->GetPitch()/4);
    return dst;
  } else if (isRGB24) {
    PVideoFrame dst = env->NewVideoFrame(vi,-8);
    rgb24_to_yv12_mmx(dst->GetWritePtr(PLANAR_Y),dst->GetWritePtr(PLANAR_U),dst->GetWritePtr(PLANAR_V),src->GetReadPtr(),src->GetRowSize()/3,src->GetHeight(),src->GetPitch()/3);
    return dst;
  }
  
  PVideoFrame dst = env->NewVideoFrame(vi);
  if (isYUY2) {
//  yuyv_to_yv12_mmx(dst->GetWritePtr(PLANAR_Y),dst->GetWritePtr(PLANAR_U),dst->GetWritePtr(PLANAR_V),src->GetReadPtr(),src->GetRowSize()/2,src->GetHeight(),src->GetRowSize()/2);
    if (interlaced) {
		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yuy2_i_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(), 
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_Y),dst->GetPitch(PLANAR_U),
          src->GetHeight());
      } else {
        mmx_yuy2_i_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(), 
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_Y),dst->GetPitch(PLANAR_U),
          src->GetHeight());
      }
    } else {
		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yuy2_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(), 
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_Y),dst->GetPitch(PLANAR_U),
          src->GetHeight());
      } else {
        mmx_yuy2_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(), 
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_Y),dst->GetPitch(PLANAR_U),
          src->GetHeight());
      }
    }
  }

  return dst;

}

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();
  if (!vi.IsYV12()) {
    if (vi.IsRGB()) {
      if (vi.width&7) {
        int xtra = 8 - (vi.width&7);
        return new Crop(0,0,-xtra,0, AlignPlanar::Create(new ConvertToYV12(new AddBorders(0,0,xtra,0,0,clip),args[1].AsBool(false),env)),env);
      }
      return AlignPlanar::Create(new ConvertToYV12(clip,args[1].AsBool(false),env));
    } else if (vi.IsYUY2()) {
      return  new ConvertToYV12(clip,args[1].AsBool(false),env);
    }
  }
  return clip;
}


/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, bool _interlaced, IScriptEnvironment* env)
  : GenericVideoFilter(_child), interlaced(_interlaced),src_cs(vi.pixel_type)
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYUY2: image width must be even");
  vi.pixel_type = VideoInfo::CS_YUY2;
}

PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
	if ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) {
		if ((env->GetCPUFlags() & CPUF_MMX)) {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB32toYUY2((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height);
			__asm { emms }
			return dst;
		}
	}
  if (((src_cs&VideoInfo::CS_YV12)==VideoInfo::CS_YV12)||((src_cs&VideoInfo::CS_I420)==VideoInfo::CS_I420)) {  
    PVideoFrame dst = env->NewVideoFrame(vi,16);
    BYTE* yuv = dst->GetWritePtr();
    if (interlaced) {
//      if (((src->GetPitch()&7)||(dst->GetPitch())&15)) 
//        env->ThrowError("ConvertToYUY2 (interlaced): Pitch was not properly aligned - please report this!");

		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), 
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_i_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), 
                    yuv, dst->GetPitch() ,src->GetHeight());
      }
    } else {
//      if (((src->GetPitch()&7)||(dst->GetPitch())&15)) 
//        env->ThrowError("ConvertToYUY2 (progressive): Pitch was not properly aligned - please report this!");

		  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
        isse_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), 
                      yuv, dst->GetPitch() ,src->GetHeight());
      } else {
        mmx_yv12_to_yuy2(src->GetReadPtr(PLANAR_Y), src->GetReadPtr(PLANAR_U), src->GetReadPtr(PLANAR_V), src->GetRowSize(PLANAR_Y_ALIGNED), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), 
                      yuv, dst->GetPitch() ,src->GetHeight());
      }
//      yv12_to_yuyv_mmx(yuv,dst->GetPitch()/2,src->GetReadPtr(PLANAR_Y),src->GetReadPtr(PLANAR_U),src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();
  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int cyb = int(0.114*219/255*65536+0.5);
  const int cyg = int(0.587*219/255*65536+0.5);
  const int cyr = int(0.299*219/255*65536+0.5);

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) ? 4 : 3;

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
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  bool i=args[1].AsBool(false);
  return new ConvertToYUY2(clip, i, env);
}


/****************************************************
 ******* Convert back to YUY2                  ******
 ******* this only uses Chroma from left pixel ******
 ******* to be used, when signal already has   ******
 ******* been YUY2 to avoid deterioration      ******
 ****************************************************/

ConvertBackToYUY2::ConvertBackToYUY2(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child), rgb32(vi.IsRGB32())
{
  if (vi.width & 1)
    env->ThrowError("ConvertBackToYUY2: image width must be even");
  vi.pixel_type = VideoInfo::CS_YUY2;
}


PVideoFrame __stdcall ConvertBackToYUY2::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
	if (rgb32) {
		if ((env->GetCPUFlags() & CPUF_MMX)) {
			PVideoFrame dst = env->NewVideoFrame(vi);
			BYTE* yuv = dst->GetWritePtr();
			mmx_ConvertRGB32toYUY2_Dup((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height);
			__asm { emms }
			return dst;
		}
	}

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
      const int scaled_y = (y1*2 - 32) * int(255.0/219.0*32768+0.5);
      const int b_y = ((rgb[0]) << 16) - scaled_y;
      yuv[1] = ScaledPixelClip((b_y >> 10) * int(1/2.018*1024+0.5) + 0x800000);  // u
      const int r_y = ((rgb[2]) << 16) - scaled_y;
      yuv[3] = ScaledPixelClip((r_y >> 10) * int(1/1.596*1024+0.5) + 0x800000);  // v
      rgb = rgb_next + rgb_inc;
      yuv += 4;
    }
    rgb += rgb_offset;
    yuv += yuv_offset;
  }

  return dst;
}

AVSValue __cdecl ConvertBackToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return new ConvertBackToYUY2(clip, env);
  else
    return clip;
}





/*************************************
 *******   Convert to Greyscale ******
 ************************************/

Greyscale::Greyscale(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
}


PVideoFrame Greyscale::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* srcp = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  int myy = vi.height;
  int myx = vi.width;
  if (vi.IsYV12()) {
    pitch = frame->GetPitch(PLANAR_U)/4;
    int *srcpUV = (int*)frame->GetWritePtr(PLANAR_U);
    myx = frame->GetRowSize(PLANAR_U_ALIGNED)/4;
    myy = frame->GetHeight(PLANAR_U);
	  for (int y=0; y<myy; y++) {
      for (int x=0; x<myx; x++) {
		    srcpUV[x] = 0x80808080;  // mod 8
      }
		  srcpUV += pitch;
	  }
    pitch = frame->GetPitch(PLANAR_V)/4;
    srcpUV = (int*)frame->GetWritePtr(PLANAR_V);
    myx = frame->GetRowSize(PLANAR_V_ALIGNED)/4;
    myy = frame->GetHeight(PLANAR_V);
	  for (y=0; y<myy; ++y) {
      for (int x=0; x<myx; x++) {
		    srcpUV[x] = 0x80808080;  // mod 8
      }
		  srcpUV += pitch;
	  }
  } else if (vi.IsYUY2())
	{
	  for (int y=0; y<myy; ++y) {
		for (int x=0; x<myx; x++)
		  srcp[x*2+1] = 128;
		srcp += pitch;
	  }
	} else if (vi.IsRGB32() && (env->GetCPUFlags() & CPUF_MMX) && (!myx&1) ) {
		const int cyb = int(0.114*32768+0.5);
		const int cyg = int(0.587*32768+0.5);
		const int cyr = int(0.299*32768+0.5);
		myx = myx >> 1;
		__int64 rgb2lum = ((__int64)cyb << 32) | (cyg << 16) | cyr;

    __asm {
      mov			edi, srcp
        pxor		mm0,mm0
        movq		mm7,rgb2lum
        
        xor     ecx, ecx
        movq		mm2, [edi + ecx*8]
        mov			ebx, myy
        mov     edx, myx
        
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
  } else if (vi.IsRGB()) {  // RGB C
    BYTE* p_count = srcp;
    const int rgb_inc = vi.IsRGB32() ? 4 : 3;
    for (int y=0; y<vi.height; ++y) {
      for (int x=0; x<vi.width; x++) {
        //				int greyscale=((p[0]*4725)+(p[1]*46884)+(p[2]*13926))>>16;              // This is the correct brigtness calculations (standardized in Rec. 709)
        int greyscale=((srcp[0]*7471)+(srcp[1]*38469)+(srcp[2]*19595))>>16;      // This produces similar results as YUY2 (luma calculation)
        srcp[0]=srcp[1]=srcp[2]=greyscale;
        srcp += rgb_inc;
      } 
      p_count+=pitch;
      srcp=p_count;
    }
    
  }
  return frame;
}


AVSValue __cdecl Greyscale::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Greyscale(args[0].AsClip(),env);
}
