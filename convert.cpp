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



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Convert_filters[] = {
{ "ConvertToRGB", "c[matrix]s", ConvertToRGB::Create },       // matrix can be "rec709"
  { "ConvertToRGB24", "c[matrix]s", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s", ConvertToRGB::Create32 },
  { "ConvertToYV12", "c", ConvertToYV12::Create },  
  { "ConvertToYUY2", "c", ConvertToYUY2::Create },  
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
  __int64 oxooooooooooffffff=0x0000000000ffffff;
  __int64 oxooffffffoooooooo=0x00ffffff00000000;

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


ConvertToYV12::ConvertToYV12(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYV12: image width must be multiple of 2");
  if (vi.IsFieldBased() && (vi.height & 3))
    env->ThrowError("ConvertToYV12: Interlaced image height must be multiple of 4");
  if ((!vi.IsFieldBased()) && (vi.height & 1))
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
  
  PVideoFrame dst = env->NewVideoFrame(vi,-8);
  yuyv_to_yv12_mmx(dst->GetWritePtr(PLANAR_Y),dst->GetWritePtr(PLANAR_U),dst->GetWritePtr(PLANAR_V),src->GetReadPtr(),src->GetRowSize()/2,src->GetHeight(),src->GetRowSize()/2);
  return dst;
/*
  int* srcp = (int*)src->GetReadPtr();
  int* dstpY = (int*)dst->GetWritePtr(PLANAR_Y);
  short* dstpV = (short*)dst->GetWritePtr(PLANAR_V);
  short* dstpU = (short*)dst->GetWritePtr(PLANAR_U);
  int xloops=src->GetRowSize()/8;  // One loop = four pixels = two int32
  int yloops=src->GetHeight()/2;   // One loops = 2 lines  (should be 4 for proper chroma interpolation on fieldbased)  
  int src_pitch=src->GetPitch()/4;
  int dst_pitchY=dst->GetPitch(PLANAR_Y)/4;
  int dst_pitchUV=dst->GetPitch(PLANAR_V)/2;
  for (int y=0;y<yloops;y++) {
    for (int x=0;x<xloops;x++) {
      unsigned int s0=srcp[x*2]; // 2 pixels
      unsigned int s1=srcp[x*2+1];
      unsigned int s2=srcp[x*2+src_pitch];
      unsigned int s3=srcp[x*2+src_pitch+1];
      dstpY[x] = (s0&0xff) | ((s0>>8)&0xff00) | ((s1<<16)&0xff0000) | ((s1<<8)&0xff000000);
      dstpY[x+dst_pitchY] = (s2&0xff) | ((s2>>8)&0xff00) | ((s3<<16)&0xff0000) | ((s3<<8)&0xff000000);
      dstpU[x] = (unsigned short)(((s0&0xff00) + (s2&0xff00))>>9) | ((((s1&0xff00)+(s3&0xff00))>>9)<<8);
      dstpV[x] = (unsigned short)(((s0>>24)+(s2>>24))>>1) | ((((s1>>24)+(s3>>24))>>1)<<8);
    }
    srcp+=(src_pitch*2);
    dstpY+=(dst_pitchY*2);
    dstpU+=(dst_pitchUV);
    dstpV+=(dst_pitchUV);
  }
  return dst;
  */

}

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();
  if (!vi.IsYV12()) {
    if (vi.width&7) {
      int xtra = 8 - (vi.width&7);
      return new Crop(0,0,-xtra,0, AlignPlanar::Create(new ConvertToYV12(new AddBorders(0,0,xtra,0,0,clip),env)),env);
    }
    return AlignPlanar::Create(new ConvertToYV12(clip,env));
  }
  else
    return clip;
}


/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child), src_cs(vi.pixel_type)
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYUY2: image width must be even");
  vi.pixel_type = VideoInfo::CS_YUY2;
}

	/*****************************	
	 * MMX code by Klaus Post
	 * - Notes on MMX:
	 * Fractions are one bit less than integer code,
	 *  but otherwise the algorithm is the same, except
	 *  r_y and b_y are calculated at the same time.
	 * Order of executin has been changed much for better pairing possibilities.
	 * It is important that the 64bit values are 8 byte-aligned
	 *  otherwise it will give a huge penalty when accessing them.
   * Instructions pair rather ok, instructions from the top is merged
	 *  into last part, to avoid dependency stalls.
	 *  (paired instrucions are indented by a space)
	 *****************************/


void ConvertToYUY2::mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h) {
	__declspec(align(8)) static const __int64 rgb_mask = 0x00ffffff00ffffff;
	__declspec(align(8)) static const __int64 fraction = 0x0000000000084000;    //= 0x108000/2 = 0x84000
	__declspec(align(8)) static const __int64 add_32 = 0x000000000000000020;    //= 32 shifted 15 up
	__declspec(align(8)) static const __int64 rb_mask = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 y1y2_mult = 0x0000000000004A85;
	__declspec(align(8)) static const __int64 fpix_add =	0x0080800000808000;
	__declspec(align(8)) static const __int64 fpix_mul =  0x000000282000001fb;
	__declspec(align(8)) static const __int64 chroma_mask =  0x000000000ff00ff00;
	__declspec(align(8)) static const __int64 low32_mask =  0x000000000ffffffff;
  //  const int cyb = int(0.114*219/255*32768+0.5);
  //  const int cyg = int(0.587*219/255*32768+0.5);
  //  const int cyr = int(0.299*219/255*32768+0.5);
  //	__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
	__declspec(align(8)) const __int64 cybgr_64 = 0x000020DE40870c88;

	int lwidth_bytes = w<<2;    // Width in bytes
	src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)


#define SRC eax
#define DST edi
#define RGBOFFSET ecx
#define YUVOFFSET edx

	for (int y=0;y<h;y++) {
	__asm {
		mov SRC,src
		mov DST,dst
		mov RGBOFFSET,0
		mov YUVOFFSET,0
		cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop		; Jump out of loop if true (width==0?? - somebody brave should remove this test)
		movq mm3,[rgb_mask]
		movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1
		pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1
		punpcklbw mm1,mm0						; mm1= 0000 R100 G100 B100
		movq mm4,[cybgr_64]
		align 16
re_enter:
		punpckhbw mm2,mm0				; mm2= 0000 R200 G200 B200
		 movq mm3,[fraction]
		psrlw mm1,8							; mm1= 0000 00R1 00G1 00B1
	  psrlw mm2,8							; mm2= 0000 00R2 00G2 00B2 
		 movq mm6,mm1						; mm6= 0000 00R1 00G1 00B1 (shifter unit stall)
		pmaddwd mm1,mm4						; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
		 movq mm7,mm2						; mm7= 0000 00R2 00G2 00B2		 
		  movq mm0,[rb_mask]
		pmaddwd mm2,mm4						; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		 paddd mm1,mm3						; Add rounding fraction
		  paddw mm6,mm7					; mm6 = accumulated RGB values (for b_y and r_y) 
		paddd mm2,mm3						; Add rounding fraction (lower dword only)
		 movq mm4,mm1
		movq mm5,mm2
		pand mm1,[low32_mask]
		 psrlq mm4,32
		pand mm6,mm0						; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		pand mm2,[low32_mask]
		 psrlq mm5,32
		paddd mm1,mm4				    ;mm1 Contains final y1 value (shifted 15 up)
		 psllq mm6, 14					; Shift up accumulated R and B values (<<15 in C)
		paddd mm2,mm5					  ;mm2 Contains final y2 value (shifted 15 up)
		 psrlq mm1,15
		movq mm3,mm1
		 psrlq mm2,15
		movq mm4,[add_32]
		 paddd mm3,mm2					;mm3 = y1+y2
		movq mm5,[y1y2_mult]
		 psubd mm3,mm4					; mm3 = y1+y2-32 		mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add]			; Constant that should be added to final UV pixel
	   pmaddwd mm3,mm5				; mm3=scaled_y (latency 2 cycles)
      movq mm4,[fpix_mul]		; Constant that should be multiplied to final UV pixel	  
		  psllq mm2,16					; mm2 Y2 shifted up (to clear fraction) mm2 ready
		punpckldq mm3,mm3						; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		psubd mm6,mm3								; mm6 = b_y and r_y (stall)
		psrld mm6,9									; Shift down b_y and r_y (>>10 in C-code) 
		 por mm1,mm2								; mm1 = 0000 0000 00Y2 00Y1
		pmaddwd mm6,mm4							; Mult b_y and r_y 
		 pxor mm2,mm2
		 movq mm7,[chroma_mask]
		paddd mm6, mm0							; Add 0x800000 to r_y and b_y 
		 add RGBOFFSET,8
		psrld mm6,9									; Move down, so fraction is only 7 bits
		 cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop						; Jump out of loop if true
		packssdw mm6,mm2						; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		 movq mm3,[rgb_mask]				;														[From top (to get better pairing)]
		psllq mm6,1									; Move up, so fraction is 8 bit
		 movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1	[From top (to get better pairing)]
		pand mm6,mm7					; Clear out fractions
		 pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1  [From top (to get better pairing)]
		por mm6,mm1									; Or luma and chroma together			
		 movq mm4,[cybgr_64]        ;                           [From top (to get better pairing)]
		movd [DST+YUVOFFSET],mm6		; Store final pixel						
		 punpcklbw mm1,mm0					; mm1= 0000 R100 G100 B100
		add YUVOFFSET,4			// Two pixels (packed)
		jmp re_enter
outloop:
		// Do store without loading next pixel
		packssdw mm6,mm2			; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		psllq mm6,1						; Move up, so fraction is 8 bit
		pand mm6,mm7					; Clear out fractions
		por mm1,mm6						; Or luma and chroma together
		movd [DST+YUVOFFSET],mm1	; Store final pixel
		} // end asm
		src -= src_pitch;
		dst += dst_pitch;
	} // end for y

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET
 
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
    PVideoFrame dst = env->NewVideoFrame(vi);
    BYTE* yuv = dst->GetWritePtr();
    yv12_to_yuyv_mmx(yuv,dst->GetPitch()/2,src->GetReadPtr(PLANAR_Y),src->GetReadPtr(PLANAR_U),src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
    return dst;
    /*
    const BYTE* yp = src->GetReadPtr(PLANAR_Y);
    const BYTE* up = src->GetReadPtr(PLANAR_U);
    const BYTE* vp = src->GetReadPtr(PLANAR_V);

    int dst_pitch=dst->GetPitch();
    int src_pitchUV=src->GetPitch(PLANAR_U);
    int src_pitchY=src->GetPitch(PLANAR_Y);

  if (!vi.IsFieldBased()) {
    for (int y=0; y<(vi.height>>1); y++) {
      for (int x=0; x<(vi.width>>1); x++) {
        yuv[x*4]=yp[x*2];
        yuv[x*4+2]=yp[x*2+1];
        yuv[x*4+1]=up[x];
        yuv[x*4+3]=vp[x];

        yuv[x*4+dst_pitch]=yp[x*2+src_pitchY];
        yuv[x*4+2+dst_pitch]=yp[x*2+1+src_pitchY];
        yuv[x*4+1+dst_pitch]=up[x];
        yuv[x*4+3+dst_pitch]=vp[x];
      }
      yp += src_pitchY*2;
      up += src_pitchUV;
      vp += src_pitchUV;
      yuv += dst_pitch*2;
    }
  } else { // Fieldbased
    for (int y=0; y<(vi.height>>2); y++) {
      for (int x=0; x<(vi.width>>1); x++) {
        yuv[x*4]=yp[x*2];
        yuv[x*4+2]=yp[x*2+1];
        yuv[x*4+1]=up[x];
        yuv[x*4+3]=vp[x];

        yuv[x*4+dst_pitch*2]=yp[x*2+src_pitchY*2];  // Two lines down, same UV
        yuv[x*4+2+dst_pitch*2]=yp[x*2+1+src_pitchY*2];
        yuv[x*4+1+dst_pitch*2]=up[x];
        yuv[x*4+3+dst_pitch*2]=vp[x];

        yuv[x*4+dst_pitch]=yp[x*2+src_pitchY];
        yuv[x*4+2+dst_pitch]=yp[x*2+1+src_pitchY];
        yuv[x*4+1+dst_pitch]=up[x+src_pitchUV];
        yuv[x*4+3+dst_pitch]=vp[x+src_pitchUV];

        yuv[x*4+dst_pitch*3]=yp[x*2+src_pitchY*3];
        yuv[x*4+2+dst_pitch*3]=yp[x*2+1+src_pitchY*3];
        yuv[x*4+1+dst_pitch*3]=up[x+src_pitchUV];
        yuv[x*4+3+dst_pitch*3]=vp[x+src_pitchUV];
      }
      yp += src_pitchY*4;
      up += src_pitchUV*2;
      vp += src_pitchUV*2;
      yuv += dst_pitch*4;
    }
  }
    return dst;
    */
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
  const VideoInfo vi = clip->GetVideoInfo();
  if (vi.IsYV12()) {
    if (vi.IsFieldBased()) {
      if (vi.width&7) {
        int xtra = 8 - (vi.width&7);
        return new Crop(0,0,-xtra,0,(new DoubleWeaveFields(new ConvertToYUY2(new SeparateFields(new AddBorders(0,0,xtra,0,0,clip),env), env))),env);
      } else {
        return new DoubleWeaveFields(new ConvertToYUY2(new SeparateFields(clip,env), env));
      }
    } else {
      if (vi.width&7) {
        int xtra = 8 - (vi.width&7);
        return new Crop(0,0,-xtra,0,(new ConvertToYUY2(new AddBorders(0,0,xtra,0,0,clip),env)),env);
      }
    }
  }
  return new ConvertToYUY2(clip, env);
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
			mmx_ConvertRGB32toYUY2((unsigned int *)src->GetReadPtr(),(unsigned int *)yuv ,(src->GetPitch())>>2, (dst->GetPitch())>>2,vi.width, vi.height);
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

void ConvertBackToYUY2::mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h) {
	__declspec(align(8)) static const __int64 rgb_mask = 0x00ffffff00ffffff;
	__declspec(align(8)) static const __int64 fraction = 0x0000000000084000;    //= 0x108000/2 = 0x84000
	__declspec(align(8)) static const __int64 add_32 = 0x000000000000000020;    //= 32 shifted 15 up
	__declspec(align(8)) static const __int64 rb_mask = 0x0000ffff0000ffff;    //=Mask for unpacked R and B
  __declspec(align(8)) static const __int64 y1y2_mult = 0x0000000000004A85;
	__declspec(align(8)) static const __int64 fpix_add =	0x00807fff00807fff;
	__declspec(align(8)) static const __int64 fpix_mul =  0x000000282000001fb;
	__declspec(align(8)) static const __int64 chroma_mask =  0x000000000ff00ff00;
	__declspec(align(8)) static const __int64 low32_mask =  0x000000000ffffffff;
	__declspec(align(8)) const __int64 cybgr_64 = 0x000020DE40870c88;

	int lwidth_bytes = w<<2;    // Width in bytes
	src+=src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)



#define SRC eax
#define DST edi
#define RGBOFFSET ecx
#define YUVOFFSET edx

	for (int y=0;y<h;y++) {
	__asm {
		mov SRC,src
		mov DST,dst
		mov RGBOFFSET,0
		mov YUVOFFSET,0
		cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop		; Jump out of loop if true (width==0?? - somebody brave should remove this test)
		movq mm3,[rgb_mask]
		movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1
		pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1
		punpcklbw mm1,mm0						; mm1= 0000 R100 G100 B100
		movq mm4,[cybgr_64]
		align 16
re_enter:
		punpckhbw mm2,mm0				; mm2= 0000 R200 G200 B200
		 movq mm3,[fraction]
		psrlw mm1,8							; mm1= 0000 00R1 00G1 00B1
	  psrlw mm2,8							; mm2= 0000 00R2 00G2 00B2 (shifter unit stall)
		 movq mm6,mm1						; mm6= 0000 00R1 00G1 00B1 
		pmaddwd mm1,mm4						; mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
			pand mm6,[rb_mask]			; Clear out accumulated G-value mm6= 0000 RRRR 0000 BBBB
		pmaddwd mm2,mm4						; mm1= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
		 paddd mm1,mm3						; Add rounding fraction
		paddd mm2,mm3						; Add rounding fraction (lower dword only)
		 movq mm4,mm1
		movq mm5,mm2
		pand mm1,[low32_mask]
		 psrlq mm4,32
		pand mm2,[low32_mask]
		 psrlq mm5,32
		paddd mm1,mm4				    ;mm1 Contains final y1 value (shifted 15 up)
		 psllq mm6, 15					; Shift up first pixe R and B values (<<16 in C)
		paddd mm2,mm5					  ;mm2 Contains final y2 value (shifted 15 up)
		 psrlq mm1,15
		movq mm3,mm1
		psrlq mm2,15
		movq mm4,[add_32]
			pslld mm3,1						; mm3=y1*2
		movq mm5,[y1y2_mult]
		 psubd mm3,mm4					; mm3 = y1+y2-32 		mm0,mm4,mm5,mm7 free
    movq mm0,[fpix_add]			; Constant that should be added to final UV pixel
	   pmaddwd mm3,mm5				; mm3=scaled_y (latency 2 cycles)
      movq mm4,[fpix_mul]		; Constant that should be multiplied to final UV pixel	  
		  psllq mm2,16					; mm2 Y2 shifted up (to clear fraction) mm2 ready
		punpckldq mm3,mm3						; Move scaled_y to upper dword mm3=SCAL ED_Y SCAL ED_Y 
		psubd mm6,mm3								; mm6 = b_y and r_y (stall)
		psrld mm6,9									; Shift down b_y and r_y (>>10 in C-code) 
		 por mm1,mm2								; mm1 = 0000 0000 00Y2 00Y1
		pmaddwd mm6,mm4							; Mult b_y and r_y 
		 pxor mm2,mm2
		 movq mm7,[chroma_mask]
		paddd mm6, mm0							; Add 0x800000 to r_y and b_y 
		 add RGBOFFSET,8
		psrld mm6,9									; Move down, so fraction is only 7 bits
		 cmp       RGBOFFSET,[lwidth_bytes]
		jge       outloop						; Jump out of loop if true
		packssdw mm6,mm2						; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		 movq mm3,[rgb_mask]				;														[From top (to get better pairing)]
		psllq mm6,1									; Move up, so fraction is 8 bit
		 movq mm0,[SRC+RGBOFFSET]		; mm0= XXR2 G2B2 XXR1 G1B1	[From top (to get better pairing)]
		pand mm6,mm7					; Clear out fractions
		 pand mm0,mm3								; mm0= 00R2 G2B2 00R1 G1B1  [From top (to get better pairing)]
		por mm6,mm1									; Or luma and chroma together			
		 movq mm4,[cybgr_64]        ;                           [From top (to get better pairing)]
		movd [DST+YUVOFFSET],mm6		; Store final pixel						
		 punpcklbw mm1,mm0					; mm1= 0000 R100 G100 B100
		add YUVOFFSET,4			// Two pixels (packed)
		jmp re_enter
outloop:
		// Do store without loading next pixel
		packssdw mm6,mm2			; mm6 = 0000 0000 VVVV UUUU (7 bits fraction) (values above 0xff are saturated)
		psllq mm6,1						; Move up, so fraction is 8 bit
		pand mm6,mm7					; Clear out fractions
		por mm1,mm6						; Or luma and chroma together
		movd [DST+YUVOFFSET],mm1	; Store final pixel
		} // end asm
		src -= src_pitch;
		dst += dst_pitch;
	} // end for y

#undef SRC
#undef DST
#undef RGBOFFSET
#undef YUVOFFSET
 
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
		    srcpUV[x] = 0x7f7f7f7f;  // mod 8
      }
		  srcpUV += pitch;
	  }
    pitch = frame->GetPitch(PLANAR_V)/4;
    srcpUV = (int*)frame->GetWritePtr(PLANAR_V);
    myx = frame->GetRowSize(PLANAR_V_ALIGNED)/4;
    myy = frame->GetHeight(PLANAR_V);
	  for (y=0; y<myy; ++y) {
      for (int x=0; x<myx; x++) {
		    srcpUV[x] = 0x7f7f7f7f;  // mod 8
      }
		  srcpUV += pitch;
	  }
  } else if (vi.IsYUY2())
	{
	  for (int y=0; y<myy; ++y) {
		for (int x=0; x<myx; x++)
		  srcp[x*2+1] = 127;
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
