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

// ConvertPlanar (c) 2005 by Klaus Post


#include "stdafx.h"

#include "convert_planar.h"
#include "../filters/planeswap.h"
#include "../filters/field.h"


#define USE_DYNAMIC_COMPILER true


ConvertToY8::ConvertToY8(PClip src, int in_matrix, IScriptEnvironment* env) : GenericVideoFilter(src), matrix(0) {
  yuy2_input = blit_luma_only = rgb_input = false;

  if (vi.IsPlanar()) {
    blit_luma_only = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsYUY2()) {
    yuy2_input = true;
    vi.pixel_type = VideoInfo::CS_Y8;
    return;
  }

  if (vi.IsRGB()) {
    rgb_input = true;
    pixel_step = vi.BytesFromPixels(1);
    vi.pixel_type = VideoInfo::CS_Y8;
    matrix = (signed short*)_aligned_malloc(sizeof(short)*4, 64);
    signed short* m = matrix;
    if (in_matrix == Rec601) {
      *m++ = (signed short)((219.0/255.0)*0.114*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.587*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.299*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_601) {
      *m++ = (signed short)(0.114*32768.0+0.5);  //B
      *m++ = (signed short)(0.587*32768.0+0.5);  //G
      *m++ = (signed short)(0.299*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == Rec709) {
      *m++ = (signed short)((219.0/255.0)*0.0722*32768.0+0.5);  //B
      *m++ = (signed short)((219.0/255.0)*0.7152*32768.0+0.5);  //G
      *m++ = (signed short)((219.0/255.0)*0.2126*32768.0+0.5);  //R
      offset_y = 16;
    } else if (in_matrix == PC_709) {
      *m++ = (signed short)(0.0722*32768.0+0.5);  //B
      *m++ = (signed short)(0.7152*32768.0+0.5);  //G
      *m++ = (signed short)(0.2126*32768.0+0.5);  //R
      offset_y = 0;
    } else if (in_matrix == AVERAGE) {
      *m++ = (signed short)(32768.0/3 + 0.5);  //B
      *m++ = (signed short)(32768.0/3 + 0.5);  //G
      *m++ = (signed short)(32768.0/3 + 0.5);  //R
      offset_y = 0;
    } else {
      _aligned_free(matrix);
      matrix = 0;
      env->ThrowError("ConvertToY8: Unknown matrix.");
    }
    *m = 0;  // Alpha
    return;
  }

  env->ThrowError("ConvertToY8: Unknown input format");
}

ConvertToY8::~ConvertToY8() {
  if (matrix)
    _aligned_free(matrix);
  matrix = 0;
}

PVideoFrame __stdcall ConvertToY8::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (blit_luma_only) {
	// Abuse Subframe to snatch the Y plane
	return env->Subframe(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (yuy2_input) {

    const BYTE* srcP = src->GetReadPtr();
    const int srcPitch = src->GetPitch();
    const int awidth = min(srcPitch>>1, (vi.width+7) & -8);

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);

    if (!(awidth & 7)) {
      this->convYUV422toY8(srcP, dstY, srcPitch, dstPitch, awidth, vi.height);
      return dst;
    }

    const int w = dst->GetRowSize(PLANAR_Y);
    const int h = dst->GetHeight(PLANAR_Y);

    for (int y=0; y<h; y++) {
      for (int x=0; x<w; x+=2) {
        const int x2 = x<<1;
        dstY[x] = srcP[x2];
        dstY[x+1] = srcP[x2+2];
      }
      srcP+=srcPitch;
      dstY+=dstPitch;
    }
  }

  if (rgb_input) {
    const int srcPitch = src->GetPitch();
    const BYTE* srcp = src->GetReadPtr() + srcPitch * (vi.height-1);  // We start at last line

    BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
    const int dstPitch = dst->GetPitch(PLANAR_Y);

    if (pixel_step == 4) {
      convRGB32toY8(srcp, dstY, -srcPitch, dstPitch, vi.width, vi.height, offset_y);
      return dst;
    }

    const int srcMod = srcPitch + (vi.width * pixel_step);
    const int m0 = matrix[0];
    const int m1 = matrix[1];
    const int m2 = matrix[2];
    for (int y=0; y<vi.height; y++) {
      for (int x=0; x<vi.width; x++) {
        const int Y = offset_y + ((m0 * srcp[0] + m1 * srcp[1] + m2 * srcp[2] + 16384) >> 15);
        dstY[x] = PixelClip(Y);  // All the safety we can wish for.
        srcp += pixel_step;
      }
      srcp -= srcMod;
      dstY += dstPitch;
    }
  }
  return dst;
}


void ConvertToY8::convRGB32toY8(const unsigned char *src, unsigned char *py,
       int pitch1, int pitch2y, int width, int height, int offset_y)
{
	void *rest = (void *)matrix;

	__asm {
		mov			esi,src
		mov			edi,py
		mov			eax,rest			; this->matrix

		movd		mm1,offset_y
		pcmpeqd		mm2,mm2
		paddd		mm1,mm1
		pxor		mm0,mm0
		psubd		mm1,mm2				; +=0.5
		movq		mm2,[eax]			; [0|cyr|cyg|cyb]

		mov			edx,width
		mov			ecx,3
		mov			eax,height
		and			ecx,edx				; ecx remainder
		psllq		mm1,46				; 0x0008400000000000
		sub			edx,ecx				; edx width mod 4

		cmp			ecx,3
		mov			rest,offset do_3	; Program line tail processing routine
		je			start

		cmp			ecx,2
		mov			rest,offset do_2
		je			start

		cmp			ecx,1
		mov			rest,offset do_1
		je			start

		mov			rest,offset nexty	; None, fall straight thru
		jmp			start

		align		16
do_1:
		movd		mm6,[esi+ecx*4]		; Get 1 pixels
		punpcklbw	mm6,mm0				; [00la|00rr|00gg|00bb]
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]
		punpckldq	mm7,mm6				; [loDWmm6|junk]
		paddd		mm6,mm1				; +=0.5
		paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]
		psrad		mm6,15				; -> 8 bit result
		punpckhwd	mm6,mm0				; [....|....|....|..00]
		packuswb	mm6,mm0				; [..|..|..|..|..|..|..|00]
		movd		[edi+ecx],mm6		; write 1 pixel

		jmp			nexty

		align		16
do_2:
		movq		mm6,[esi+ecx*4]		; Get pixels 1 & 0
		movq		mm5,mm6				; duplicate pixels
		punpcklbw	mm6,mm0				; [00la|00rr|00gg|00bb]			-- 0
		punpckhbw	mm5,mm0	 			; [00ha|00rr|00gg|00bb]         -- 1
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- 0
		pmaddwd		mm5,mm2				; [0*a+cyr*r|cyg*g+cyb*b]       -- 1
		punpckldq	mm7,mm6				; [loDWmm6|junk]				-- 0
		punpckldq	mm3,mm5				; [loDWmm5|junk]				-- 1
		paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]	-- 0
		paddd		mm5,mm3				; [hiDWmm5+32768+loDWmm5|junk]	-- 1
		paddd		mm6,mm1				; +=0.5
		paddd		mm5,mm1				; +=0.5
		psrad		mm6,15				; -> 8 bit result				-- 0
		psrad		mm5,15				; -> 8 bit result				-- 1

		punpckhwd	mm6,mm5				; [....|....|..11|..00]
		packuswb	mm6,mm0				; [..|..|..|..|..|..|11|00]
		movd		[edi+ecx],mm6		; write 2 pixels

		jmp			nexty

		align		16
do_3:
		movq		mm6,[esi+ecx*4]		; Get pixels 1 & 0
		movd		mm4,[esi+ecx*4+8]	; Get pixel 2
		movq		mm5,mm6				; duplicate pixels

		punpcklbw	mm6,mm0				; [00la|00rr|00gg|00bb]			-- 0
		punpckhbw	mm5,mm0	 			; [00ha|00rr|00gg|00bb]         -- 1
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- 0
		pmaddwd		mm5,mm2				; [0*a+cyr*r|cyg*g+cyb*b]       -- 1
		punpckldq	mm7,mm6				; [loDWmm6|junk]				-- 0
		punpckldq	mm3,mm5				; [loDWmm5|junk]				-- 1
		paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]	-- 0
		punpcklbw	mm4,mm0				; [00la|00rr|00gg|00bb]			-- 2
		paddd		mm5,mm3				; [hiDWmm5+32768+loDWmm5|junk]	-- 1
		pmaddwd		mm4,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- 2
		paddd		mm6,mm1				; +=0.5
		punpckldq	mm7,mm4				; [loDWmm4|junk]				-- 2
		paddd		mm5,mm1				; +=0.5
		paddd		mm4,mm7				; [hiDWmm4+32768+loDWmm4|junk]	-- 2
		psrad		mm6,15				; -> 8 bit result				-- 0
		paddd		mm4,mm1				; +=0.5
		psrad		mm5,15				; -> 8 bit result				-- 1
		psrad		mm4,15				; -> 8 bit result				-- 2
		punpckhwd	mm6,mm5				; [....|....|..11|..00]
		punpckhwd	mm4,mm0				; [....|....|....|..22]

		punpckldq	mm6,mm4				; [....|..22|..11|..00]
		packuswb	mm6,mm0				; [..|..|..|..|..|22|11|00]
		movd		[edi+ecx],mm6		; write 3 pixels

		align		16
nexty:
		add			esi,pitch1
		dec			eax
		add			edi,pitch2y
start:
		xor			ecx,ecx
		test		eax,eax
		jz			done

		cmp			ecx,edx				; break >= myx & ~3
		jb			loop4

		jmp			[rest]				; do_{n} remainder

		align		16
loop4:
		movq		mm6,[esi+ecx*4]		; Get pixels 1 & 0
		movq		mm4,[esi+ecx*4+8]	; Get pixels 3 & 2
		movq		mm5,mm6				; duplicate pixels

		punpcklbw	mm6,mm0				; [00la|00rr|00gg|00bb]			-- 0
		punpckhbw	mm5,mm0	 			; [00ha|00rr|00gg|00bb]         -- 1
		pmaddwd		mm6,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- 0
		pmaddwd		mm5,mm2				; [0*a+cyr*r|cyg*g+cyb*b]       -- 1
		punpckldq	mm7,mm6				; [loDWmm6|junk]				-- 0
		punpckldq	mm3,mm5				; [loDWmm5|junk]				-- 1
		paddd		mm6,mm7				; [hiDWmm6+32768+loDWmm6|junk]	-- 0
		paddd		mm5,mm3				; [hiDWmm5+32768+loDWmm5|junk]	-- 1
		paddd		mm6,mm1				; +=0.5
		paddd		mm5,mm1				; +=0.5
		psrad		mm6,15				; -> 8 bit result				-- 0
		psrad		mm5,15				; -> 8 bit result				-- 1

		movq		mm3,mm4				; duplicate pixels
		punpckhwd	mm6,mm5				; [....|....|..11|..00]

		punpcklbw	mm4,mm0				; [00la|00rr|00gg|00bb]			-- 2
		punpckhbw	mm3,mm0	 			; [00ha|00rr|00gg|00bb]         -- 3
		pmaddwd		mm4,mm2				; [0*a+cyr*r|cyg*g+cyb*b]		-- 2
		pmaddwd		mm3,mm2				; [0*a+cyr*r|cyg*g+cyb*b]       -- 3
		punpckldq	mm7,mm4				; [loDWmm4|junk]				-- 2
		punpckldq	mm5,mm3				; [loDWmm3|junk]				-- 3
		paddd		mm4,mm7				; [hiDWmm4+32768+loDWmm4|junk]	-- 2
		paddd		mm3,mm5				; [hiDWmm3+32768+loDWmm3|junk]	-- 3
		paddd		mm4,mm1				; +=0.5
		paddd		mm3,mm1				; +=0.5
		psrad		mm4,15				; -> 8 bit result				-- 2
		psrad		mm3,15				; -> 8 bit result				-- 3

		punpckhwd	mm4,mm3				; [....|....|..33|..22]

		punpckldq	mm6,mm4				; [..33|..22|..11|..00]
		add			ecx,4				; loop counter
		packuswb	mm6,mm0				; [..|..|..|..|33|22|11|00]
		cmp			ecx,edx				; break >= myx & ~3
		movd		[edi+ecx-4],mm6		; write 4 pixels
		jb			loop4

		jmp			[rest]				; do_{n} remainder

		align		16
done:
		emms
    }
}


void ConvertToY8::convYUV422toY8(const unsigned char *src, unsigned char *py,
       int pitch1, int pitch2y, int width, int height)
{
	int widthdiv2 = width>>1;
	__int64 Ymask = 0x00FF00FF00FF00FFi64;
	__asm
	{
		mov edi,[src]
		mov edx,[py]
		mov ecx,widthdiv2
		movq mm5,Ymask
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		add eax,4
		packuswb mm0,mm1       ; YYYYYYYY
		cmp eax,ecx
		movq [edx+eax*2-8],mm0 ; store y
		jl xloop
		add edi,pitch1
		add edx,pitch2y
		dec height
		jnz yloop
		emms
	}
}


AVSValue __cdecl ConvertToY8::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsY8())
    return clip;
  return new ConvertToY8(clip,getMatrix(args[1].AsString("rec601"), env), env);
}


/*****************************************************
 * ConvertRGBToYV24
 *
 * (c) Klaus Post, 2005
 ******************************************************/

ConvertRGBToYV24::ConvertRGBToYV24(PClip src, int in_matrix, IScriptEnvironment* env)
  : GenericVideoFilter(src), matrix(0)  {

  if (!vi.IsRGB())
    env->ThrowError("ConvertRGBToYV24: Only RGB data input accepted");

  pixel_step = vi.BytesFromPixels(1);
  vi.pixel_type = VideoInfo::CS_YV24;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);

  const int shift = 15;

  if (in_matrix == Rec601) {
    /*
    Y'= 0.299*R' + 0.587*G' + 0.114*B'
    Cb=-0.169*R' - 0.331*G' + 0.500*B'
    Cr= 0.500*R' - 0.419*G' - 0.081*B'
    */
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);
  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
    /*
    Y'= 0.2126*R' + 0.7152*G' + 0.0722*B'
    Cb=-0.1145*R' - 0.3855*G' + 0.5000*B'
    Cr= 0.5000*R' - 0.4542*G' - 0.0458*B'
    */
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertRGBToYV24: Unknown matrix.");
  }

  dyn_matrix = (BYTE*)matrix;
  dyn_dest = (BYTE*)_aligned_malloc(vi.width * 4 +32, 64);
  this->src_pixel_step = pixel_step;
  this->post_add = (offset_y & 0xffff) | ((__int64)(128 & 0xffff)<<16) | ((__int64)(128 & 0xffff)<<32);
  this->GenerateAssembly(vi.width, shift, false, env);

  unpck_src = new const BYTE*[1];
  unpck_dst = new BYTE*[3];
  unpck_src[0] = dyn_dest;
  this->GenerateUnPacker(vi.width, env);
}

ConvertRGBToYV24::~ConvertRGBToYV24() {
  if (dyn_dest)
    _aligned_free(dyn_dest);
  dyn_dest = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

  delete[] unpck_src;
  delete[] unpck_dst;
}


void ConvertRGBToYV24::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  R = r/Srgb                     // 0..1
  G = g/Srgb
  B = b*Srgb

  Y = Kr*R + Kg*G + Kb*B         // 0..1
  U = B - (Kr*R + Kg*G)/(1-Kb)   //-1..1
  V = R - (Kg*G + Kb*B)/(1-Kr)

  y = Y*Sy  + Oy                 // 16..235, 0..255
  u = U*Suv + Ouv                // 16..240, 1..255
  v = V*Suv + Ouv
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Sy  * Kb        * mulfac / Srgb + 0.5); //B
  *m++ = (signed short)(Sy  * Kg        * mulfac / Srgb + 0.5); //G
  *m++ = (signed short)(Sy  * Kr        * mulfac / Srgb + 0.5); //R
  *m++ = (signed short)(           -0.5 * mulfac             ); //Rounder
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kr/(Kb-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)(Suv * Kb/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv * Kg/(Kr-1) * mulfac / Srgb + 0.5);
  *m++ = (signed short)(Suv             * mulfac / Srgb + 0.5);
  *m++ = (signed short)(           -0.5 * mulfac             );
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  offset_y = Oy;
}

PVideoFrame __stdcall ConvertRGBToYV24::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  int awidth = dst->GetRowSize(PLANAR_Y_ALIGNED);
// FIXME fix code to make wide_enough redundant!
  bool wide_enough = (!(vi.width & 1)); // We need to check if input is wide enough

  if (!wide_enough) {
    if (src->GetPitch()-src->GetRowSize() >= (vi.BytesFromPixels(1)))
      wide_enough = true;  // We have one more pixel (at least) in width.
  }

  if (wide_enough && USE_DYNAMIC_COMPILER) {
    int* i_dyn_dest = (int*)dyn_dest;
    for (int y = 0; y < vi.height; y++) {
      dyn_src = (unsigned char*)&srcp[(vi.height-y-1)*src->GetPitch()];
      assembly.Call();
      if (awidth & 7) {  // Should never happend, as all planar formats must have mod8 pitch
        for (int x = 0; x < vi.width; x++) {
          int p = i_dyn_dest[x];
          dstY[x] = p&0xff;
          dstU[x] = (p>>8)&0xff;
          dstV[x] = (p>>16)&0xff;
        }
      } else {
        unpck_dst[0] = dstY;
        unpck_dst[1] = dstU;
        unpck_dst[2] = dstV;
        this->unpacker.Call();
      }
      dstY += dst->GetPitch(PLANAR_Y);
      dstU += dst->GetPitch(PLANAR_U);
      dstV += dst->GetPitch(PLANAR_V);
    }
    return dst;
  }

  //Slow C-code.

  signed short* m = (signed short*)matrix;
  srcp += src->GetPitch() * (vi.height-1);  // We start at last line
  for (int y = 0; y < vi.height; y++) {
    for (int x = 0; x < vi.width; x++) {
      int b = srcp[0];
      int g = srcp[1];
      int r = srcp[2];
      int Y = offset_y + (((int)m[0] * b + (int)m[1] * g + (int)m[2] * r + 16384)>>15);
      int U = 128+(((int)m[4] * b + (int)m[5] * g + (int)m[6] * r + 16384)>>15);
      int V = 128+(((int)m[8] * b + (int)m[9] * g + (int)m[10] * r + 16384)>>15);
      *dstY++ = PixelClip(Y);  // All the safety we can wish for.
      *dstU++ = PixelClip(U);
      *dstV++ = PixelClip(V);
      srcp += pixel_step;
    }
    srcp -= src->GetPitch() + (vi.width * pixel_step);
    dstY += dst->GetPitch(PLANAR_Y) - vi.width;
    dstU += dst->GetPitch(PLANAR_U) - vi.width;
    dstV += dst->GetPitch(PLANAR_V) - vi.width;
  }
  return dst;
}

AVSValue __cdecl ConvertRGBToYV24::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV24())
    return clip;
  return new ConvertRGBToYV24(clip, getMatrix(args[1].AsString("rec601"), env), env);
}


/*****************************************************
 * ConvertYV24ToRGB
 *
 * (c) Klaus Post, 2005
 ******************************************************/

ConvertYV24ToRGB::ConvertYV24ToRGB(PClip src, int in_matrix, int _pixel_step, IScriptEnvironment* env)
 : GenericVideoFilter(src), pixel_step(_pixel_step), matrix(0) {

  if (!vi.IsYV24())
    env->ThrowError("ConvertYV24ToRGB: Only YV24 data input accepted");

  vi.pixel_type = (pixel_step == 3) ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
  matrix = (signed short*)_aligned_malloc(sizeof(short)*16,64);
  const int shift = 13;

  if (in_matrix == Rec601) {
/*
    B'= Y' + 1.772*U' + 0.000*V'
    G'= Y' - 0.344*U' - 0.714*V'
    R'= Y' + 0.000*U' + 1.402*V'
*/
    BuildMatrix(0.299,  /* 0.587  */ 0.114,  219, 112, 16, shift);

  }
  else if (in_matrix == PC_601) {

    BuildMatrix(0.299,  /* 0.587  */ 0.114,  255, 127,  0, shift);
  }
  else if (in_matrix == Rec709) {
/*
    B'= Y' + 1.8558*Cb + 0.0000*Cr
    G'= Y' - 0.1870*Cb - 0.4678*Cr
    R'= Y' + 0.0000*Cb + 1.5750*Cr
*/
    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 219, 112, 16, shift);
  }
  else if (in_matrix == PC_709) {

    BuildMatrix(0.2126, /* 0.7152 */ 0.0722, 255, 127,  0, shift);
  }
  else if (in_matrix == AVERAGE) {

    BuildMatrix(1.0/3, /* 1.0/3 */ 1.0/3, 255, 127,  0, shift);
  }
  else {
    _aligned_free(matrix);
    matrix = 0;
    env->ThrowError("ConvertYV24ToRGB: Unknown matrix.");
  }
  dyn_matrix = (BYTE*)matrix;
  dyn_src = (BYTE*)_aligned_malloc(vi.width * 4 + 32, 64);
  this->dest_pixel_step = pixel_step;
  this->pre_add = (offset_y & 0xffff) | ((__int64)(-128 & 0xffff)<<16) | ((__int64)(-128 & 0xffff)<<32);
  this->GenerateAssembly(vi.width, shift, true, env);

  unpck_src = new const BYTE*[3];
  unpck_dst = new BYTE*[1];
  unpck_dst[0] = dyn_src;
  this->GeneratePacker(vi.width, env);
}

ConvertYV24ToRGB::~ConvertYV24ToRGB() {
  if (dyn_src)
    _aligned_free(dyn_src);
  dyn_src = 0;

  if (matrix)
    _aligned_free(matrix);
  matrix = 0;

  delete[] unpck_src;
  delete[] unpck_dst;
}


void ConvertYV24ToRGB::BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift)
{
/*
  Kr   = {0.299, 0.2126}
  Kb   = {0.114, 0.0722}
  Kg   = 1 - Kr - Kb // {0.587, 0.7152}
  Srgb = 255
  Sy   = {219, 255}
  Suv  = {112, 127}
  Oy   = {16, 0}
  Ouv  = 128

  Y =(y-Oy)  / Sy                         // 0..1
  U =(u-Ouv) / Suv                        //-1..1
  V =(v-Ouv) / Suv

  R = Y                  + V*(1-Kr)       // 0..1
  G = Y - U*(1-Kb)*Kb/Kg - V*(1-Kr)*Kr/Kg
  B = Y + U*(1-Kb)

  r = R*Srgb                              // 0..255
  g = G*Srgb
  b = B*Srgb
*/
  const double mulfac = double(1<<shift);

  const double Kg = 1.- Kr - Kb;
  const int Srgb = 255;

  signed short* m = matrix;

  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5); //Y
  *m++ = (signed short)(Srgb * (1-Kb)       * mulfac / Suv + 0.5); //U
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5); //V
  *m++ = (signed short)(                0.5 * mulfac            ); //Rounder
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * (Kb-1)*Kb/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (Kr-1)*Kr/Kg * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)(Srgb * 1.000        * mulfac / Sy  + 0.5);
  *m++ = (signed short)(Srgb * 0.000        * mulfac / Suv + 0.5);
  *m++ = (signed short)(Srgb * (1-Kr)       * mulfac / Suv + 0.5);
  *m++ = (signed short)(                0.5 * mulfac            );
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  *m++ = (signed short)0x0000;
  *m++ = (signed short)0xff00;
  offset_y = -Oy;
}

PVideoFrame __stdcall ConvertYV24ToRGB::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 8);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  BYTE* dstp = dst->GetWritePtr();

  int awidth = src->GetRowSize(PLANAR_Y_ALIGNED);

  if (USE_DYNAMIC_COMPILER) {
    int* i_dyn_src = (int*)dyn_src;
    for (int y = 0; y < vi.height; y++) {
      if (awidth & 7) { // This should be very safe to assume to never happend
        for (int x = 0; x < vi.width; x++) {
          i_dyn_src[x] = srcY[x] | (srcU[x] << 8 ) | (srcV[x] << 16) | (1<<24);
        }
      } else {
        unpck_src[0] = srcY;
        unpck_src[1] = srcU;
        unpck_src[2] = srcV;
        this->packer.Call();
      }
      dyn_dest = &dstp[(vi.height-y-1)*dst->GetPitch()];
      assembly.Call();
      srcY += src->GetPitch(PLANAR_Y);
      srcU += src->GetPitch(PLANAR_U);
      srcV += src->GetPitch(PLANAR_V);
    }
    return dst;
  }

  //Slow C-code.

  signed short* m = (signed short*)matrix;
  dstp += dst->GetPitch() * (vi.height-1);  // We start at last line
  if (pixel_step == 4) {
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[0] = PixelClip(b);  // All the safety we can wish for.
        dstp[1] = PixelClip(g);  // Probably needed here.
        dstp[2] = PixelClip(r);
        dstp[3] = 255; // alpha
        dstp += 4;
      }
      dstp -= dst->GetPitch() + (vi.width * pixel_step);
      srcY += src->GetPitch(PLANAR_Y);
      srcU += src->GetPitch(PLANAR_U);
      srcV += src->GetPitch(PLANAR_V);
    }
  } else {
    for (int y = 0; y < vi.height; y++) {
      for (int x = 0; x < vi.width; x++) {
        int Y = srcY[x] + offset_y;
        int U = srcU[x] - 128;
        int V = srcV[x] - 128;
        int b = (((int)m[0] * Y + (int)m[1] * U + (int)m[ 2] * V + 4096)>>13);
        int g = (((int)m[4] * Y + (int)m[5] * U + (int)m[ 6] * V + 4096)>>13);
        int r = (((int)m[8] * Y + (int)m[9] * U + (int)m[10] * V + 4096)>>13);
        dstp[0] = PixelClip(b);  // All the safety we can wish for.
        dstp[1] = PixelClip(g);  // Probably needed here.
        dstp[2] = PixelClip(r);
        dstp += pixel_step;
      }
      dstp -= dst->GetPitch() + (vi.width * pixel_step);
      srcY += src->GetPitch(PLANAR_Y);
      srcU += src->GetPitch(PLANAR_U);
      srcV += src->GetPitch(PLANAR_V);
    }
  }
  return dst;
}

AVSValue __cdecl ConvertYV24ToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString("rec601"), env), 4, env);
}

AVSValue __cdecl ConvertYV24ToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsRGB())
    return clip;
  return new ConvertYV24ToRGB(clip, getMatrix(args[1].AsString("rec601"), env), 3, env);
}

/************************************
 * YUY2 to YV16
 ************************************/

ConvertYUY2ToYV16::ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYUY2())
    env->ThrowError("ConvertYUY2ToYV16: Only YUY2 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YV16;

}

PVideoFrame __stdcall ConvertYUY2ToYV16::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcP = src->GetReadPtr();
  const int awidth = min(src->GetPitch()>>1, (vi.width+7) & -8);

  BYTE* dstY = dst->GetWritePtr(PLANAR_Y);
  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  if (!(awidth&7)) {  // Use MMX
    this->convYUV422to422(srcP, dstY, dstU, dstV, src->GetPitch(), dst->GetPitch(PLANAR_Y),
                          dst->GetPitch(PLANAR_U),  awidth, vi.height);
    return dst;
  }

  int w = vi.width/2;

  for (int y=0; y<vi.height; y++) { // ASM will probably not be faster here.
    for (int x=0; x<w; x++) {
      int x2 = x<<1;
      int x4 = x<<2;
      dstY[x2] = srcP[x4];
      dstY[x2+1] = srcP[x4+2];
      dstU[x] = srcP[x4+1];
      dstV[x] = srcP[x4+3];
    }
    srcP += src->GetPitch();
    dstY += dst->GetPitch(PLANAR_Y);
    dstU += dst->GetPitch(PLANAR_U);
    dstV += dst->GetPitch(PLANAR_V);
  }
  return dst;
}

void ConvertYUY2ToYV16::convYUV422to422(const unsigned char *src,
                                        unsigned char *py, unsigned char *pu, unsigned char *pv,
                                        int pitch1, int pitch2y, int pitch2uv, int width, int height)
{
	int widthdiv2 = width>>1;
	__int64 Ymask = 0x00FF00FF00FF00FFi64;
	__asm
	{
        push ebx
		mov edi,[src]
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov ecx,widthdiv2
		movq mm5,Ymask
	yloop:
		xor eax,eax
		align 16
	xloop:
		movq mm0,[edi+eax*4]   ; VYUYVYUY - 1
		movq mm1,[edi+eax*4+8] ; VYUYVYUY - 2
		movq mm2,mm0           ; VYUYVYUY - 1
		movq mm3,mm1           ; VYUYVYUY - 2
		pand mm0,mm5           ; 0Y0Y0Y0Y - 1
		psrlw mm2,8 	       ; 0V0U0V0U - 1
		pand mm1,mm5           ; 0Y0Y0Y0Y - 2
		psrlw mm3,8            ; 0V0U0V0U - 2
		packuswb mm0,mm1       ; YYYYYYYY
		packuswb mm2,mm3       ; VUVUVUVU
		movq [ebx+eax*2],mm0   ; store y
		movq mm4,mm2           ; VUVUVUVU
		pand mm2,mm5           ; 0U0U0U0U
		psrlw mm4,8            ; 0V0V0V0V
		add eax,4
		packuswb mm2,mm2       ; xxxxUUUU
		cmp eax,ecx
		packuswb mm4,mm4       ; xxxxVVVV
		movd [edx+eax-4],mm2   ; store u
		movd [esi+eax-4],mm4   ; store v
		jl xloop
		add edi,pitch1
		add ebx,pitch2y
		add edx,pitch2uv
		add esi,pitch2uv
		dec height
		jnz yloop
		emms
        pop ebx
	}
}


AVSValue __cdecl ConvertYUY2ToYV16::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYV16())
    return clip;
  return new ConvertYUY2ToYV16(clip, env);
}

/************************************
 * YV16 to YUY2
 ************************************/

ConvertYV16ToYUY2::ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src) {

  if (!vi.IsYV16())
    env->ThrowError("ConvertYV16ToYUY2: Only YV16 is allowed as input");

  vi.pixel_type = VideoInfo::CS_YUY2;

}

PVideoFrame __stdcall ConvertYV16ToYUY2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi, 16);

  const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);
  const int awidth = min(src->GetPitch(PLANAR_Y), (vi.width+7) & -8);

  BYTE* dstp = dst->GetWritePtr();

  if (!(awidth&7)) {  // Use MMX
    this->conv422toYUV422(srcY, srcU, srcV, dstp, src->GetPitch(PLANAR_Y), src->GetPitch(PLANAR_U),
      dst->GetPitch(), awidth, vi.height);
  }

  int w = vi.width/2;

  for (int y=0; y<vi.height; y++) { // ASM will probably not be faster here.
    for (int x=0; x<w; x++) {
      int x2 = x<<1;
      int x4 = x<<2;
      dstp[x4] = srcY[x2];
      dstp[x4+2] = srcY[x2+1];
      dstp[x4+1] = srcU[x];
      dstp[x4+3] = srcV[x];
    }
    srcY += src->GetPitch(PLANAR_Y);
    srcU += src->GetPitch(PLANAR_U);
    srcV += src->GetPitch(PLANAR_V);
    dstp += dst->GetPitch();
  }
  return dst;
}

void ConvertYV16ToYUY2::conv422toYUV422(const unsigned char *py, const unsigned char *pu, const unsigned char *pv,
                                        unsigned char *dst,
                                        int pitch1Y, int pitch1UV, int pitch2, int width, int height)
{
	int widthdiv2 = width >> 1;
	__asm
	{
        push ebx
		mov ebx,[py]
		mov edx,[pu]
		mov esi,[pv]
		mov edi,[dst]
		mov ecx,widthdiv2
yloop:
		xor eax,eax
		align 16
xloop:
		movd mm1,[edx+eax]     ;0000UUUU
		movd mm2,[esi+eax]     ;0000VVVV
		movq mm0,[ebx+eax*2]   ;YYYYYYYY
		punpcklbw mm1,mm2      ;VUVUVUVU
		movq mm3,mm0           ;YYYYYYYY
		punpcklbw mm0,mm1      ;VYUYVYUY
		add eax,4
		punpckhbw mm3,mm1      ;VYUYVYUY
		movq [edi+eax*4-16],mm0 ;store
		cmp eax,ecx
		movq [edi+eax*4-8],mm3   ;store
		jl xloop
		add ebx,pitch1Y
		add edx,pitch1UV
		add esi,pitch1UV
		add edi,pitch2
		dec height
		jnz yloop
		emms
        pop ebx
	}
}

AVSValue __cdecl ConvertYV16ToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;
  return new ConvertYV16ToYUY2(clip, env);
}

/**********************************************
 * Converter between arbitrary planar formats
 *
 * This uses plane copy for luma, and the
 * bicubic resizer for chroma (could be
 * customizable later)
 *
 * (c) Klaus Post, 2005
 **********************************************/



ConvertToPlanarGeneric::ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                                               AVSValue* UsubSampling, AVSValue* VsubSampling,
                                               int cp,  const AVSValue* chromaResampler,
                                               IScriptEnvironment* env) : GenericVideoFilter(src) {
  Y8input = vi.IsY8();

  if (! (vi.IsYV12() || dst_space == VideoInfo::CS_YV12 || dst_space == VideoInfo::CS_I420)) {
    interlaced = false;  // Ignore, if YV12 is not involved.
  }

  if (interlaced)
    vi.SetFieldBased(false);

  if (!Y8input) {
    Usource = new SwapUVToY(child, SwapUVToY::UToY8, env);
    Vsource = new SwapUVToY(child, SwapUVToY::VToY8, env);
    if (interlaced) Usource = new SeparateFields(Usource, env);
    if (interlaced) Vsource = new SeparateFields(Vsource, env);
  }

  int uv_width = vi.width;
  int uv_height = vi.height;

  if (interlaced && (vi.IsYV12() || dst_space == VideoInfo::CS_YV12 || dst_space == VideoInfo::CS_I420)) {
    uv_height /=  2;
  }

  switch (dst_space) {
    case VideoInfo::CS_YV12:
    case VideoInfo::CS_I420:
      uv_width /= 2;
      uv_height /=  2;
      switch (cp) {
        case PLACEMENT_MPEG2:
          UsubSampling[1] = AVSValue(0.5); // Override U chroma placement if source is YV12
          VsubSampling[1] = AVSValue(0.5); // Override V chroma placement if source is YV12
          break;
        case PLACEMENT_DV:
          UsubSampling[1] = AVSValue(0.5); // Override U chroma placement if source is YV12
          VsubSampling[1] = AVSValue(1.5); // Override V chroma placement if source is YV12
          break;
        case PLACEMENT_MPEG1:
          UsubSampling[0] = AVSValue(0.5); // Override U chroma placement if source is YV12
          VsubSampling[0] = AVSValue(0.5); // Override V chroma placement if source is YV12
          UsubSampling[1] = AVSValue(0.5); // Override U chroma placement if source is YV12
          VsubSampling[1] = AVSValue(0.5); // Override V chroma placement if source is YV12
      }
      break;
    case VideoInfo::CS_YV24:
      uv_width /= 1; uv_height /= 1;
      break;
    case VideoInfo::CS_YV16:
      uv_width /= 2; uv_height /= 1;
      break;
    case VideoInfo::CS_YV411:
      uv_width /= 4; uv_height /= 1;
      break;
    default:
      env->ThrowError("Convert: Cannot convert to destination format.");

  }

  vi.pixel_type = dst_space;

  if (!Y8input) {
    const int xmod  = 1 << vi.GetPlaneWidthSubsampling(PLANAR_U);
    const int xmask = xmod - 1;
    if (vi.width & xmask)
      env->ThrowError("Convert: Cannot convert if width isn't mod%d!", xmod);

    const int ymod  = 1 << vi.GetPlaneHeightSubsampling(PLANAR_U);
    const int ymask = ymod - 1;
    if (vi.height & ymask)
      env->ThrowError("Convert: Cannot convert if height isn't mod%d!", ymod);

    ResamplingFunction *filter = getResampler(chromaResampler->AsString("bicubic"), env);
    UsubSampling[2] = AVSValue(Usource->GetVideoInfo().width+UsubSampling[0].AsFloat());
    UsubSampling[3] = AVSValue(Usource->GetVideoInfo().height+UsubSampling[1].AsFloat());
    VsubSampling[2] = AVSValue(Vsource->GetVideoInfo().width+VsubSampling[0].AsFloat());
    VsubSampling[3] = AVSValue(Vsource->GetVideoInfo().height+VsubSampling[1].AsFloat());
    Usource = FilteredResize::CreateResize(Usource, uv_width, uv_height, UsubSampling, filter, env);
    Vsource = FilteredResize::CreateResize(Vsource, uv_width, uv_height, VsubSampling, filter, env);
    if (interlaced) Usource = new SelectEvery(new DoubleWeaveFields(Usource), 2, 0);
    if (interlaced) Vsource = new SelectEvery(new DoubleWeaveFields(Vsource), 2, 0);
    delete filter;
  }
}

PVideoFrame __stdcall ConvertToPlanarGeneric::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
              src->GetRowSize(PLANAR_Y_ALIGNED), src->GetHeight(PLANAR_Y));
  if (Y8input) {
    memset(dst->GetWritePtr(PLANAR_U), 0x80, dst->GetHeight(PLANAR_U)*dst->GetPitch(PLANAR_U));
    memset(dst->GetWritePtr(PLANAR_V), 0x80, dst->GetHeight(PLANAR_V)*dst->GetPitch(PLANAR_V));
  } else {
    src = Usource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_U));
    src = Vsource->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y),
                src->GetRowSize(PLANAR_Y_ALIGNED), dst->GetHeight(PLANAR_V));
  }
  return dst;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV12(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV12() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString("rec601"), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV12: Can only convert from Planar YUV.");

  bool interlaced = args[1].AsBool(false);
  AVSValue Usubs[4] = { 0.0, -0.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_U)), 0.0, 0.0 }; // Move chroma down 0.5 lines
  AVSValue Vsubs[4] = { 0.0, -0.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_V)), 0.0, 0.0 }; // Move chroma down 0.5 lines
  int cp = getPlacement(args[3].AsString("MPEG2"), env);
  switch (cp) {
        case PLACEMENT_DV:
          Usubs[1] = AVSValue(-0.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_U))); // Override U chroma placement if source is YV12
          Vsubs[1] = AVSValue(-1.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_V))); // Override V chroma placement if source is YV12
          break;
        case PLACEMENT_MPEG1:
          Usubs[0] = AVSValue(-0.5/(1 << clip->GetVideoInfo().GetPlaneWidthSubsampling(PLANAR_U))); // Override U chroma placement if source is YV12
          Vsubs[0] = AVSValue(-0.5/(1 << clip->GetVideoInfo().GetPlaneWidthSubsampling(PLANAR_V))); // Override V chroma placement if source is YV12
          Usubs[1] = AVSValue(-0.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_U))); // Override U chroma placement if source is YV12
          Vsubs[1] = AVSValue(-0.5/(1 << clip->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_V))); // Override V chroma placement if source is YV12
        case PLACEMENT_MPEG2:
          break;  // Already set (default)
  }
  clip = new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV12, interlaced, Usubs, Vsubs, cp, &args[4], env);
  return clip;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV16(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV16())
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString("rec601"), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    return new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV16: Can only convert from Planar YUV.");

  AVSValue Usubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  AVSValue Vsubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  int cp = getPlacement(args[3].AsString("MPEG2"), env);
  clip = new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV16, args[1].AsBool(false), Usubs, Vsubs, cp, &args[4], env);
  return clip;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV24(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV24() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    return new ConvertRGBToYV24(clip, getMatrix(args[2].AsString("rec601"), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV24: Can only convert from Planar YUV.");

  AVSValue Usubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  AVSValue Vsubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  int cp = getPlacement(args[3].AsString("MPEG2"), env);
  clip = new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV24, args[1].AsBool(false), Usubs, Vsubs, cp, &args[4], env);
  return clip;
}

AVSValue __cdecl ConvertToPlanarGeneric::CreateYV411(AVSValue args, void*, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();

  if (clip->GetVideoInfo().IsYV411() )
    return clip;

  if (clip->GetVideoInfo().IsRGB())
    clip = new ConvertRGBToYV24(clip, getMatrix(args[2].AsString("rec601"), env), env);

  if (clip->GetVideoInfo().IsYUY2())
    clip = new ConvertYUY2ToYV16(clip,  env);

  if (!clip->GetVideoInfo().IsPlanar())
    env->ThrowError("ConvertToYV411: Can only convert from Planar YUV.");

  AVSValue Usubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  AVSValue Vsubs[4] = { 0.0, 0.0, 0.0, 0.0 };
  int cp = getPlacement(args[3].AsString("MPEG2"), env);
  clip = new ConvertToPlanarGeneric(clip, VideoInfo::CS_YV411, args[1].AsBool(false), Usubs, Vsubs, cp, &args[4], env);
  return clip;
}


static int getPlacement( const char* placement, IScriptEnvironment* env) {
  if (placement) {
    if (!lstrcmpi(placement, "mpeg2"))
      return PLACEMENT_MPEG2;
    if (!lstrcmpi(placement, "mpeg1"))
      return PLACEMENT_MPEG1;
    if (!lstrcmpi(placement, "dv"))
      return PLACEMENT_DV;
  }
  env->ThrowError("Convert: Unknown chromaplacement");
  return PLACEMENT_MPEG2;
}


static ResamplingFunction* getResampler( const char* resampler, IScriptEnvironment* env) {
  if (resampler) {
    if      (!lstrcmpi(resampler, "point"))
      return new PointFilter();
    else if (!lstrcmpi(resampler, "bilinear"))
      return new TriangleFilter();
    else if (!lstrcmpi(resampler, "bicubic"))
      return new MitchellNetravaliFilter(1./3,1./3); // Parse out optional B= and C= from string
    else if (!lstrcmpi(resampler, "lanczos"))
      return new LanczosFilter(3); // Parse out optional Taps= from string
    else if (!lstrcmpi(resampler, "lanczos4"))
      return new LanczosFilter(4);
    else if (!lstrcmpi(resampler, "blackman"))
      return new BlackmanFilter(4);
    else if (!lstrcmpi(resampler, "spline16"))
      return new Spline16Filter();
    else if (!lstrcmpi(resampler, "spline36"))
      return new Spline36Filter();
    else if (!lstrcmpi(resampler, "spline64"))
      return new Spline64Filter();
    else if (!lstrcmpi(resampler, "gauss"))
      return new GaussianFilter(30.0); // Parse out optional P= from string
    else if (!lstrcmpi(resampler, "sinc"))
      return new SincFilter(4); // Parse out optional Taps= from string
    else
      env->ThrowError("Convert: Unknown chroma resampler, '%s'", resampler);
  }
  return new MitchellNetravaliFilter(1./3,1./3); // Default colorspace conversion for AviSynth
}
