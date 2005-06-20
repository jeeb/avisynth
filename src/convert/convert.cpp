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

#include "convert.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Convert_filters[] = {
{ "ConvertToRGB", "c[matrix]s[interlaced]b", ConvertToRGB::Create },       // matrix can be "rec709", "PC.601" or "PC.709"
  { "ConvertToRGB24", "c[matrix]s[interlaced]b", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s[interlaced]b", ConvertToRGB::Create32 },
  { "ConvertToYV12", "c[interlaced]b[matrix]s", ConvertToYV12::Create },  
  { "ConvertToYUY2", "c[interlaced]b[matrix]s", ConvertToYUY2::Create },  
  { "ConvertBackToYUY2", "c[matrix]s", ConvertBackToYUY2::Create },  
  { "Greyscale", "c[matrix]s", Greyscale::Create },       // matrix can be "rec709" or "Average"
  { 0 }
};


/*************************************
 *******   RGB Helper Classes   ******
 *************************************/

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
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();

  if (env->GetCPUFlags() & CPUF_MMX) {
	int h=vi.height;
	int x_loops=vi.width; // 4 dwords/loop   read 12 bytes, write 16 bytes
	const int x_left=vi.width%4;
	x_loops-=x_left;
	x_loops*=4;
	__declspec(align(8)) static const __int64 oxffooooooffoooooo=0xff000000ff000000;

	__asm {
	push		ebx					; daft compiler assumes this is preserved!!!
    mov			esi,p
    mov			edi,q
    mov			eax,255				; Alpha channel for unaligned stosb
    mov			edx,[x_left]		; Count of unaligned pixels
    movq		mm7,[oxffooooooffoooooo]
    align 16
yloop:
    mov			ebx,0				; src offset
    mov			ecx,0				; dst offset
xloop:
    movq		mm0,[ebx+esi]		; 0000 0000 b1r0 g0b0	get b1 & a0
     movd		mm1,[ebx+esi+4]		; 0000 0000 g2b2 r1g1	get b2 & t1
	movq		mm2,mm0				; 0000 0000 b1r0 g0b0	copy b1
	 punpcklwd	mm1,mm1				; g2b2 g2b2 r1g1 r1g1	b2 in top, t1 in bottom
	psrld		mm2,24				; 0000 0000 0000 00b1	b1 in right spot
	 pslld		mm1,8				; b2g2 b200 g1r1 g100	t1 in right spot
    movd		mm3,[ebx+esi+8]		; 0000 0000 r3g3 b3r2	get a3 & t2
	 por		mm2,mm1				; b2g2 b200 g1r1 g1b1	build a1 in low mm2
	pslld		mm1,8				; g2b2 0000 r1g1 b100	clean up b2
	 psllq		mm3,24				; 00r3 g3b3 r200 0000	a3 in right spot
	psrlq		mm1,40				; 0000 0000 00g2 b200	b2 in right spot
	 punpckldq	mm0,mm2				; g1r1 g1b1 b1r0 g0b0	build a1, a0 in mm0
	por			mm1,mm3				; 00r3 g3b3 r2g2 b200	build a2
	 por		mm0,mm7				; a1r1 g1b1 a0r0 g0b0	add alpha to a1, a0
	psllq		mm1,24				; b3r2 g2b2 0000 0000	a2 in right spot
     movq		[ecx+edi],mm0		; a1r1 g1b1 a0r0 g0b0	store a1, a0
	punpckhdq	mm1,mm3				; 00r3 g3b3 b3r2 g2b2	build a3, a2 in mm1
     add		ecx,16				; bump dst index
	por			mm1,mm7				; a3r3 g3b3 a2r2 g2b2	add alpha to a3, a2
     add		ebx,12				; bump src index
    movq		[ecx+edi-8],mm1		; a3r3 g3b3 a2r2 g2b2	store a3, a2

    cmp			ecx,[x_loops]
    jl			xloop

    cmp			edx,0				; Check unaligned move count
    je			no_copy				; None, do next row
    cmp			edx,2
    je			copy_2				; Convert 2 pixels
    cmp			edx,1
    je			copy_1				; Convert 1 pixel
//copy 3
    add			esi,ebx				; else Convert 3 pixels
    add			edi,ecx
    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a

    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a

    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a
    sub			esi,ebx
    sub			edi,ecx
    sub			esi,9
    sub			edi,12
    jmp			no_copy
    align 16
copy_2:
    add			esi,ebx
    add			edi,ecx
    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a

    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a
    sub			esi,ebx
    sub			edi,ecx
    sub			esi,6
    sub			edi,8
    jmp			no_copy
    align 16
copy_1:
    add			esi,ebx
    add			edi,ecx
    movsb 							; b
    movsb							; g
    movsb							; r
    stosb							; a
    sub			esi,ebx
    sub			edi,ecx
    sub			esi,3
    sub			edi,4
    align 16
no_copy:
    add			esi,[src_pitch]
    add			edi,[dst_pitch]    
    
    dec			[h]
    jnz			yloop
    emms
	pop			ebx
	}
  }
  else {
	for (int y = vi.height; y > 0; --y) {
	  for (int x = 0; x < vi.width; ++x) {
		q[x*4+0] = p[x*3+0];
		q[x*4+1] = p[x*3+1];
		q[x*4+2] = p[x*3+2];
		q[x*4+3] = 255;
	  }
	  p += src_pitch;
	  q += dst_pitch;
	}
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

  if (env->GetCPUFlags() & CPUF_MMX) {
	const int h=vi.height;
	int x_loops=vi.width; // 4 dwords/loop   read 16 bytes, write 12 bytes
	const int x_left=vi.width%4;
	x_loops-=x_left;
	x_loops*=4;
	__declspec(align(8)) static const __int64 oxooooooooooffffff=0x0000000000ffffff;
	__declspec(align(8)) static const __int64 oxooffffffoooooooo=0x00ffffff00000000;

	__asm {
	push ebx			; daft compiler assumes this is preserved!!!
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
    movq mm4,mm1      ; 0000 0000 00r2 g2b2 

    psrlq mm2,8       ; 0000 r1g1 b100 0000
    pand mm3,mm7      ; 00r3 g3b3 0000 0000
    
    psllq mm4,48      ; g2b2 0000 0000 0000
    por mm0,mm2       ; 0000 r1g1 b1r0 g0b0

    psrlq mm1,16      ; 0000 0000 0000 00r2
    por mm0,mm4       ; g2b2 r1g1 b1r0 g0b0

    psrlq mm3,24      ; 0000 0000 r3g3 b300
    movq [ecx+edi],mm0

    por mm3,mm1       ; 0000 0000 r3g3 b3r2
    add ebx,16
    movd [ecx+edi+8],mm3

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
	pop ebx
	}
  }
  else {
	for (int y = vi.height; y > 0; --y) {
	  for (int x = 0; x < vi.width; ++x) {
		q[x*3+0] = p[x*4+0];
		q[x*3+1] = p[x*4+1];
		q[x*3+2] = p[x*4+2];
	  }
	  p += src_pitch;
	  q += dst_pitch;
	}
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
  theMatrix = Rec601;
  is_yv12=false;
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else
      env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter (must be matrix=\"Rec709\", \"PC.601\" or \"PC.709\")");
  }
  use_mmx = (env->GetCPUFlags() & CPUF_MMX) != 0;

  if ((theMatrix != Rec601) && ((vi.width & 3) != 0) || !use_mmx)
    env->ThrowError("ConvertToRGB: Rec.709 and PC Levels support require MMX and horizontal width a multiple of 4");
  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
}


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();
  
  int src_rowsize = __min(src_pitch, (src->GetRowSize()+7) & -8);
  // assumption: is_yuy2
  if (use_mmx && ((src_rowsize & 7) == 0) && (src_rowsize >= 16)) {
	VideoInfo vi2 = vi;
	vi2.width=src_rowsize / 2;
	PVideoFrame dst = env->NewVideoFrame(vi2,-2); // force pitch == rowsize
	BYTE* dstp = dst->GetWritePtr();

    (vi.IsRGB24() ? mmx_YUY2toRGB24 : mmx_YUY2toRGB32)(srcp, dstp,
      srcp + vi.height * src_pitch, src_pitch, src_rowsize, theMatrix);
	  
	if (vi.width & 3) {  // Did we extend off the right edge of picture?
	  const int dst_pitch = dst->GetPitch();
	  const int x2 = (vi.width-2) * 2;
	  const int xE = (vi.width-1) * (vi2.BitsPerPixel()>>3);
	  srcp += vi.height * src_pitch;
	  for (int y=vi.height; y>0; --y) {
		srcp -= src_pitch;
		YUV2RGB(srcp[x2+2], srcp[x2+1], srcp[x2+3], &dstp[xE]);
		dstp += dst_pitch;
	  }
	}
	return env->Subframe(dst,0, dst->GetPitch(), vi2.BytesFromPixels(vi.width), vi.height);
  }
  else {
	PVideoFrame dst = env->NewVideoFrame(vi);
	const int dst_pitch = dst->GetPitch();
	BYTE* dstp = dst->GetWritePtr();

	if (vi.IsRGB32()) {    
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
	else if (vi.IsRGB24()) {
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
}


AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();
  if (vi.IsYUV()) {
    if (vi.IsYV12()) {
      return new ConvertToRGB(new ConvertToYUY2(clip,args[2].AsBool(false),NULL,env), false, matrix, env);
    }
    return new ConvertToRGB(clip, false, matrix, env);
  } else {
    return clip;
  }
}


AVSValue __cdecl ConvertToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo vi = clip->GetVideoInfo();
  if (vi.IsYUV()) {
    if (vi.IsYV12()) {
       return new ConvertToRGB(new ConvertToYUY2(clip,args[2].AsBool(false),NULL,env), false, matrix, env);
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
       return new ConvertToRGB(new ConvertToYUY2(clip,args[2].AsBool(false),NULL,env), true, matrix, env);
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
    env->ThrowError("ConvertToYV12: Image width must be multiple of 2");
  if (interlaced && (vi.height & 3))
    env->ThrowError("ConvertToYV12: Interlaced image height must be multiple of 4");
  if ((!interlaced) && (vi.height & 1))
    env->ThrowError("ConvertToYV12: Image height must be multiple of 2");
  isYUY2=isRGB32=isRGB24=false;
  if (vi.IsYUY2()) isYUY2 = true;
  if (vi.IsRGB32()) isRGB32 = true;
  if (vi.IsRGB24()) isRGB24 = true;
 
  vi.pixel_type = VideoInfo::CS_YV12;

  if ((env->GetCPUFlags() & CPUF_MMX) == 0)
    env->ThrowError("ConvertToYV12: YV12 support require a MMX capable processor.");
}

PVideoFrame __stdcall ConvertToYV12::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);  
  PVideoFrame dst = env->NewVideoFrame(vi);

  if (isYUY2) {
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
  if (vi.IsRGB()) {
  	return new ConvertToYV12(new ConvertToYUY2(clip,false,args[2].AsString(0),env),args[1].AsBool(false),env);
  } else {
    if (args[2].Defined())
      env->ThrowError("ConvertToYV12: invalid \"matrix\" parameter (RGB data only)");
    if (vi.IsYUY2())
      return  new ConvertToYV12(clip,args[1].AsBool(false),env);
  }
  return clip;
}


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
    else
      env->ThrowError("ConvertToYUY2: invalid \"matrix\" parameter (must be matrix=\"Rec709\", \"PC.601\" or \"PC.709\")");
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
    else
      env->ThrowError("ConvertBackToYUY2: invalid \"matrix\" parameter (must be matrix=\"Rec709\", \"PC.601\" or \"PC.709\")");
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





/*************************************
 *******   Convert to Greyscale ******
 ************************************/

Greyscale::Greyscale(PClip _child, const char* matrix, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
  if (matrix) {
    if (!vi.IsRGB())
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (RGB data only)");
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "Average"))
      theMatrix = Average;
    else
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (must be matrix=\"Rec709\" or \"Average\")");
  }
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
  }
  else if (vi.IsYUY2() && (env->GetCPUFlags() & CPUF_MMX)) {
	__declspec(align(8)) static const __int64 oxooffooffooffooff = 0x00ff00ff00ff00ff; 
	__declspec(align(8)) static const __int64 ox80oo80oo80oo80oo = 0x8000800080008000; 
                                                                                       
	  myx = __min(pitch>>1, (myx+3) & -4);	// Try for mod 8                           
	  __asm {
	  	push		ebx						; daft compiler assumes this is preserved!!!

		movq		mm7,oxooffooffooffooff
		movq		mm6,ox80oo80oo80oo80oo
		mov			esi,srcp				; data pointer
		mov			ebx,pitch				; pitch
		mov			edx,myy					; height
		mov			edi,myx					; aligned width
		sub			ebx,edi
		sub			ebx,edi					; modulo = pitch - rowsize
		shr			edi,1					; number of dwords

		align		16
yloop:
		mov			ecx,edi					; number of dwords

		test		esi,0x7					; qword aligned?
		jz			xloop1					; yes

		movd		mm5,[esi]				; process 1 dword
		pand		mm5,mm7					; keep luma
		 add		esi,4					; srcp++
		por			mm5,mm6					; set chroma = 128
		 dec		ecx						; count--
		movd		[esi-4],mm5				; update 2 pixels

		align		16
xloop1:
		cmp			ecx,16					; Try to do 16 pixels per loop
		jl			xlend1

		movq		mm0,[esi+0]				; process 1 qword
		movq		mm1,[esi+8]
		pand		mm0,mm7					; keep luma
		pand		mm1,mm7
		por			mm0,mm6					; set chroma = 128
		por			mm1,mm6
		movq		[esi+0],mm0				; update 2 pixels
		movq		[esi+8],mm1
		
		movq		mm2,[esi+16]
		movq		mm3,[esi+24]
		pand		mm2,mm7
		pand		mm3,mm7
		por			mm2,mm6
		por			mm3,mm6
		movq		[esi+16],mm2
		movq		[esi+24],mm3

		movq		mm0,[esi+32]
		movq		mm1,[esi+40]
		pand		mm0,mm7
		pand		mm1,mm7
		por			mm0,mm6
		por			mm1,mm6
		movq		[esi+32],mm0
		movq		[esi+40],mm1

		movq		mm2,[esi+48]
		movq		mm3,[esi+56]
		pand		mm2,mm7
		pand		mm3,mm7
		por			mm2,mm6
		por			mm3,mm6
		movq		[esi+48],mm2
		movq		[esi+56],mm3
		
		sub			ecx,16					; count-=16
		add			esi,64					; srcp+=16

		jmp			xloop1
xlend1:
		cmp			ecx,2
		jl			xlend2

		align		16
xloop3:
		movq		mm0,[esi]				; process 1 qword
		 sub		ecx,2					; count-=2
		pand		mm0,mm7					; keep luma
		 add		esi,8					; srcp+=2
		por			mm0,mm6					; set chroma = 128
		 cmp		ecx,2					; any qwords left
		movq		[esi-8],mm0				; update 4 pixels
		 jge		xloop3					; more qwords ?

		align		16
xlend2:
		cmp			ecx,1					; 1 dword left
		jl			xlend3					; no

		movd		mm0,[esi]				; process 1 dword
		pand		mm0,mm7					; keep luma
		por			mm0,mm6					; set chroma = 128
		movd		[esi-4],mm0				; update 2 pixels
		add			esi,4					; srcp++

xlend3:
		add			esi,ebx
		dec			edx
		jnle		yloop
		emms
		pop			ebx
	  }
  }
  else if (vi.IsYUY2()) {
	for (int y=0; y<myy; ++y) {
	  for (int x=0; x<myx; x++)
		srcp[x*2+1] = 128;
	  srcp += pitch;
	}
  }
  else if (vi.IsRGB32() && (env->GetCPUFlags() & CPUF_MMX)) {
	const int cyav = int(0.33333*32768+0.5);

	const int cyb = int(0.114*32768+0.5);
	const int cyg = int(0.587*32768+0.5);
	const int cyr = int(0.299*32768+0.5);

	const int cyb709 = int(0.0721*32768+0.5);
	const int cyg709 = int(0.7154*32768+0.5);
	const int cyr709 = int(0.2125*32768+0.5);

	__int64 rgb2lum;
    __declspec(align(8)) static const __int64 oxoooo4ooooooooooo=0x0000400000000000;
    __declspec(align(8)) static const __int64 oxffooooooffoooooo=0xff000000ff000000;
	
	if (theMatrix == Rec709)
	  rgb2lum = ((__int64)cyr709 << 32) | (cyg709 << 16) | cyb709;
	else if (theMatrix == Average)
	  rgb2lum = ((__int64)cyav << 32) | (cyav << 16) | cyav;
	else
	  rgb2lum = ((__int64)cyr << 32) | (cyg << 16) | cyb;

    __asm {
	  	push		ebx					; daft compiler assumes this is preserved!!!
		mov			edi,srcp
		pxor		mm0,mm0
		movq		mm1,oxoooo4ooooooooooo
		movq		mm2,rgb2lum
		movq		mm3,oxffooooooffoooooo

		xor			ecx,ecx
		mov			ebx,myy
		mov			edx,myx

		align		16
rgb2lum_mmxloop:
		movq		mm6,[edi+ecx*4]		; Get 2 pixels
		 movq		mm4,mm3				; duplicate alpha mask
		movq		mm5,mm6				; duplicate pixels
		 pand		mm4,mm6				; extract alpha channel [ha00000la000000]
		punpcklbw	mm6,mm0				; [00ha|00rr|00gg|00bb]		-- low
		 punpckhbw	mm5,mm0	 			;                      		-- high
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- low
		 pmaddwd	mm5,mm2				;                         		-- high
		punpckldq	mm7,mm6				; [loDWmm6|junk]				-- low
		 paddd		mm6,mm1				; +=0.5
		paddd		mm5,mm1				; +=0.5
		 paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]	-- low
		punpckldq	mm7,mm5				; [loDWmm5|junk]				-- high
		psrlq		mm6,47				; -> 8 bit result				-- low
		 paddd		mm5,mm7				; [hiDWmm5+32768+loDWmm5|junk]	-- high
		punpcklwd	mm6,mm6				; [0000|0000|grey|grey]		-- low
		 psrlq		mm5,47				; -> 8 bit result				-- high
		punpckldq	mm6,mm6				; [grey|grey|grey|grey]		-- low
		 punpcklwd	mm5,mm5				; [0000|0000|grey|grey]		-- high
		 punpckldq	mm5,mm5				; [grey|grey|grey|grey]		-- high
		 packuswb	mm6,mm5				; [hg|hg|hg|hg|lg|lg|lg|lg]
		 psrld		mm6,8				; [00|hg|hg|hg|00|lg|lg|lg]
		add			ecx,2				; loop counter
		 por		mm6,mm4				; [ha|hg|hg|hg|la|lg|lg|lg]
		cmp			ecx,edx				; loop >= myx
		 movq		[edi+ecx*4-8],mm6	; update 2 pixels
		jnge		rgb2lum_mmxloop

		test		edx,1				; Non-mod 2 width
		jz			rgb2lum_even

		movd		mm6,[edi+ecx*4]		; Get 1 pixels
		movq		mm4,mm3				; duplicate alpha mask
		pand		mm4,mm6				; extract alpha channel [xx00000la000000]
		punpcklbw	mm6,mm0				; [00ha|00rr|00gg|00bb]
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]
		punpckldq	mm7,mm6				; [loDWmm6|junk]
		paddd		mm6,mm1				; +=0.5
		paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]
		psrlq		mm6,47				; -> 8 bit result
		punpcklwd	mm6,mm6				; [0000|0000|grey|grey]
		punpckldq	mm6,mm6				; [grey|grey|grey|grey]
		packuswb	mm6,mm0				; [xx|xx|xx|xx|lg|lg|lg|lg]
		psrld		mm6,8				; [00|xx|xx|xx|00|lg|lg|lg]
		por			mm6,mm4				; [xx|xx|xx|xx|la|lg|lg|lg]
		movd		[edi+ecx*4],mm6	; update 1 pixels

rgb2lum_even:
		add			edi,pitch
		mov			edx,myx
		xor			ecx,ecx
		dec			ebx
		jnle		rgb2lum_mmxloop

		emms
		pop			ebx
    }
  }
  else if (vi.IsRGB()) {  // RGB C
    BYTE* p_count = srcp;
    const int rgb_inc = vi.IsRGB32() ? 4 : 3;
	if (theMatrix == Rec709) {
//	  const int cyb709 = int(0.0721*65536+0.5); //  4725
//	  const int cyg709 = int(0.7154*65536+0.5); // 46884
//	  const int cyr709 = int(0.2125*65536+0.5); // 13927

	  for (int y=0; y<vi.height; ++y) {
		for (int x=0; x<vi.width; x++) {
		  int greyscale=((srcp[0]*4725)+(srcp[1]*46884)+(srcp[2]*13927)+32768)>>16; // This is the correct brigtness calculations (standardized in Rec. 709)
		  srcp[0]=srcp[1]=srcp[2]=greyscale;
		  srcp += rgb_inc;
		} 
		p_count+=pitch;
		srcp=p_count;
	  }
	}
	else if (theMatrix == Average) {
//	  const int cyav = int(0.333333*65536+0.5); //  21845

	  for (int y=0; y<vi.height; ++y) {
		for (int x=0; x<vi.width; x++) {
		  int greyscale=((srcp[0]+srcp[1]+srcp[2])*21845+32768)>>16; // This is the average of R, G & B
		  srcp[0]=srcp[1]=srcp[2]=greyscale;
		  srcp += rgb_inc;
		} 
		p_count+=pitch;
		srcp=p_count;
	  }
	}
	else {
//	  const int cyb = int(0.114*65536+0.5); //  7471
//	  const int cyg = int(0.587*65536+0.5); // 38470
//	  const int cyr = int(0.299*65536+0.5); // 19595

	  for (int y=0; y<vi.height; ++y) {
		for (int x=0; x<vi.width; x++) {
		  int greyscale=((srcp[0]*7471)+(srcp[1]*38470)+(srcp[2]*19595)+32768)>>16; // This produces similar results as YUY2 (luma calculation)
		  srcp[0]=srcp[1]=srcp[2]=greyscale;
		  srcp += rgb_inc;
		} 
		p_count+=pitch;
		srcp=p_count;
	  }
	}
  }
  return frame;
}


AVSValue __cdecl Greyscale::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Greyscale(args[0].AsClip(), args[1].AsString(0), env);
}
