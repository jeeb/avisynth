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


#include "greyscale.h"
#include <avs/win.h>
#include <cstdlib>
#include "../core/internal.h"
#include <emmintrin.h>
#include "avs/alignment.h"
#include "avs/minmax.h"


/*************************************
 *******   Convert to Greyscale ******
 ************************************/

extern const AVSFunction Greyscale_filters[] = {
  { "Greyscale", "c[matrix]s", Greyscale::Create },       // matrix can be "rec601", "rec709" or "Average"
  { "Grayscale", "c[matrix]s", Greyscale::Create },
  { 0 }
};

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
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\" or \"Average\")");
  }
}

//this is not really faster than MMX but a lot cleaner
static void greyscale_yuy2_sse2(BYTE *srcp, size_t /*width*/, size_t height, size_t pitch) {
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
  __m128i chroma_value = _mm_set1_epi16(0x8000);
  BYTE* end_point = srcp + pitch * height;

  while(srcp < end_point) {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp));
    src = _mm_and_si128(src, luma_mask);
    src = _mm_or_si128(src, chroma_value);
    _mm_store_si128(reinterpret_cast<__m128i*>(srcp), src);

    srcp += 16;
  }
}

#ifdef X86_32
static void greyscale_yuy2_mmx(BYTE *srcp, size_t width, size_t height, size_t pitch) {
  bool not_mod8 = false;
  size_t loop_limit = min((pitch / 8) * 8, ((width*4 + 7) / 8) * 8);


  __m64 luma_mask = _mm_set1_pi16(0x00FF);
  __m64 chroma_value = _mm_set1_pi16(0x8000);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
     __m64 src = *reinterpret_cast<const __m64*>(srcp+x);
     src = _mm_and_si64(src, luma_mask);
     src = _mm_or_si64(src, chroma_value);
     *reinterpret_cast<__m64*>(srcp+x) = src;
    }

    if (loop_limit < width) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+width-8);
      src = _mm_and_si64(src, luma_mask);
      src = _mm_or_si64(src, chroma_value);
      *reinterpret_cast<__m64*>(srcp+width-8) = src;
    }

    srcp += pitch;
  }
 _mm_empty();
}
#endif


PVideoFrame Greyscale::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  if (vi.IsY8())
    return frame;

  env->MakeWritable(&frame);
  BYTE* srcp = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  int height = vi.height;
  int width = vi.width;

  if (vi.IsPlanar()) {
    pitch = frame->GetPitch(PLANAR_U)/4;
    int *srcpUV = (int*)frame->GetWritePtr(PLANAR_U);
    width = frame->GetRowSize(PLANAR_U_ALIGNED)/4;
    height = frame->GetHeight(PLANAR_U);
    for (int y=0; y<height; y++) {
      for (int x=0; x<width; x++) {
        srcpUV[x] = 0x80808080;  // mod 8
      }
      srcpUV += pitch;
    }
    pitch = frame->GetPitch(PLANAR_V)/4;
    srcpUV = (int*)frame->GetWritePtr(PLANAR_V);
    width = frame->GetRowSize(PLANAR_V_ALIGNED)/4;
    height = frame->GetHeight(PLANAR_V);
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; x++) {
        srcpUV[x] = 0x80808080;  // mod 8
      }
      srcpUV += pitch;
    }
  }

  else if (vi.IsYUY2()) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && width > 4 && IsPtrAligned(srcp, 16)) {
      greyscale_yuy2_sse2(srcp, width, height, pitch);
    } else
#ifdef X86_32
    if ((env->GetCPUFlags() & CPUF_MMX) && width > 2) {
      greyscale_yuy2_mmx(srcp, width, height, pitch);
    } else
#endif
    {
      for (int y=0; y<height; ++y)
      {
        for (int x=0; x<width; x++)
          srcp[x*2+1] = 128;
        srcp += pitch;
      }
    }
  }

#ifdef X86_32
  else if (vi.IsRGB32() && (env->GetCPUFlags() & CPUF_MMX))
  {
	  const int cyav = int(0.33333*32768+0.5);

	  const int cyb = int(0.114*32768+0.5);
	  const int cyg = int(0.587*32768+0.5);
	  const int cyr = int(0.299*32768+0.5);

	  const int cyb709 = int(0.0722*32768+0.5);
	  const int cyg709 = int(0.7152*32768+0.5);
	  const int cyr709 = int(0.2126*32768+0.5);

	  __int64 rgb2lum;

	  if (theMatrix == Rec709)
	    rgb2lum = ((__int64)cyr709 << 32) | (cyg709 << 16) | cyb709;
	  else if (theMatrix == Average)
	    rgb2lum = ((__int64)cyav << 32) | (cyav << 16) | cyav;
	  else
	    rgb2lum = ((__int64)cyr << 32) | (cyg << 16) | cyb;

    __asm {
		mov			edi,srcp
		pcmpeqd		mm1,mm1
		pxor		mm0,mm0
		psrlq		mm1,63				; 0x0000000000000001
		pcmpeqd		mm3,mm3
		psllq		mm1,46				; 0x0000400000000000
		movq		mm2,rgb2lum
		pslld		mm3,24				; 0xff000000ff000000

		xor			ecx,ecx
		mov			eax,height
		mov			edx,width

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
		cmp			ecx,edx				; loop >= width
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
		mov			edx,width
		xor			ecx,ecx
		dec			eax
		jnle		rgb2lum_mmxloop

		emms
    }
  }
#endif
  else if (vi.IsRGB())
  {  // RGB C
    BYTE* p_count = srcp;
    const int rgb_inc = vi.IsRGB32() ? 4 : 3;
	if (theMatrix == Rec709) {
//	  const int cyb709 = int(0.0722*65536+0.5); //  4732
//	  const int cyg709 = int(0.7152*65536+0.5); // 46871
//	  const int cyr709 = int(0.2126*65536+0.5); // 13933

	  for (int y=0; y<vi.height; ++y) {
		for (int x=0; x<vi.width; x++) {
		  int greyscale=((srcp[0]*4732)+(srcp[1]*46871)+(srcp[2]*13933)+32768)>>16; // This is the correct brigtness calculations (standardized in Rec. 709)
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
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsY8())
    return clip;

  return new Greyscale(clip, args[1].AsString(0), env);
}
