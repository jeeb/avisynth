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


#include "convert_rgb.h"
#include <tmmintrin.h>
#include "avs/alignment.h"


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

#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) 
  {
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
  else
#else
  {
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
#endif
  return dst;
}




RGB32to24::RGB32to24(PClip src)
: GenericVideoFilter(src)
{
  vi.pixel_type = VideoInfo::CS_BGR24;
}

//todo: think how to port to sse2 without tons of shuffles or (un)packs
static void convert_rgb32_to_rgb24_ssse3(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height) {
  size_t mod8_width = (width / 8) * 8;
  __m128i pixels0123_mask = _mm_set_epi8(0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5, 4, 2, 1, 0);
  __m128i pixels4567_mask = _mm_set_epi8(4, 2, 1, 0, 0, 0, 0, 0, 14, 13, 12, 10, 9, 8, 6, 5);
  __m128i merge_mask = _mm_set_epi32(0xFFFFFFFF, 0, 0, 0);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x+= 8) {
      __m128i src0123 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //a3b3 g3r3 a2b2 g2r2 a1b1 g1r1 a0b0 g0r0
      __m128i src4567 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4+16)); //a7b7 g7r7 a6b6 g6r6 a5b5 g5r5 a4b4 g4r4

      //pshufb being the most useful instruction for rgb24<->32 conversions
      __m128i px0123 = _mm_shuffle_epi8(src0123, pixels0123_mask); //xxxx xxxx b3g3 r3b2 g2r2 b1g1 r1b0 g0r0
      __m128i dst567 = _mm_shuffle_epi8(src4567, pixels4567_mask); //r5b4 g4r4 xxxx xxxx b7g7 r7b6 g6r6 b5g5
      
      __m128i dst012345 = _mm_or_si128(
        _mm_andnot_si128(merge_mask, px0123),
        _mm_and_si128(merge_mask, dst567)
        ); //r5b4 g4r4 b3g3 r3b2 g2r2 b1g1 r1b0 g0r0


      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+x*3), dst012345);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*3+16), dst567);
    }

    for (size_t x = mod8_width; x < width; ++x) {
      dstp[x*3+0] = srcp[x*4+0];
      dstp[x*3+1] = srcp[x*4+1];
      dstp[x*3+2] = srcp[x*4+2];
    }

    srcp += src_pitch;
    dstp += dst_pitch;
  }
}

#ifdef X86_32

static void convert_rgb32_to_rgb24_mmx(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height) {
  size_t mod4_width = (width / 4) * 4;
  __m64 low_pixel_mask = _mm_set_pi32(0, 0x00FFFFFF);
  __m64 high_pixel_mask = _mm_set_pi32(0x00FFFFFF, 0);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < width; x+= 4) {
      __m64 src01 = *reinterpret_cast<const __m64*>(srcp+x*4); //a1r1 g1b1 a0r0 g0b0
      __m64 src23 = *reinterpret_cast<const __m64*>(srcp+x*4+8); //a3r3 g3b3 a2r2 g2b2

      __m64 p0 = _mm_and_si64(src01, low_pixel_mask); //0000 0000 00r0 g0b0
      __m64 p1 = _mm_and_si64(src01, high_pixel_mask); //00r1 g1b1 0000 0000
      __m64 p2 = _mm_and_si64(src23, low_pixel_mask); //0000 0000 00r2 g2b2
      __m64 p3 = _mm_and_si64(src23, high_pixel_mask); //00r3 g3b3 0000 0000

      __m64 dst01 = _mm_or_si64(p0, _mm_srli_si64(p1, 8)); //0000 r1g1 b1r0 g0b0
      p3 = _mm_srli_si64(p3, 24); //0000 0000 r3g3 b300

      __m64 dst012 = _mm_or_si64(dst01, _mm_slli_si64(p2, 48));  //g2b2 r1g1 b1r0 g0b0
      __m64 dst23 = _mm_or_si64(p3, _mm_srli_si64(p2, 16)); //0000 0000 r3g3 b3r2

      *reinterpret_cast<__m64*>(dstp+x*3) = dst012;
      *reinterpret_cast<int*>(dstp+x*3+8) = _mm_cvtsi64_si32(dst23);
    }

    for (size_t x = mod4_width; x < width; ++x) {
      dstp[x*3+0] = srcp[x*4+0];
      dstp[x*3+1] = srcp[x*4+1];
      dstp[x*3+2] = srcp[x*4+2];
    }

    srcp += src_pitch;
    dstp += dst_pitch;
  }

  _mm_empty();
}

#endif // X86_32

static void convert_rgb32_to_rgb24_c(const BYTE *srcp, BYTE *dstp, size_t src_pitch, size_t dst_pitch, size_t width, size_t height) {
  for (size_t y = height; y > 0; --y) {
    for (size_t x = 0; x < width; ++x) {
      dstp[x*3+0] = srcp[x*4+0];
      dstp[x*3+1] = srcp[x*4+1];
      dstp[x*3+2] = srcp[x*4+2];
    }
    srcp += src_pitch;
    dstp += dst_pitch;
  }
}

PVideoFrame __stdcall RGB32to24::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE *srcp = src->GetReadPtr();
  BYTE *dstp = dst->GetWritePtr();
  size_t src_pitch = src->GetPitch();
  size_t dst_pitch = dst->GetPitch();

  if ((env->GetCPUFlags() & CPUF_SSSE3) && IsPtrAligned(srcp, 16)) {
    convert_rgb32_to_rgb24_ssse3(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height);
  } else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX)
  {
    convert_rgb32_to_rgb24_mmx(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height);
  }
  else 
#endif
  {
	 convert_rgb32_to_rgb24_c(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height);
  }
  return dst;
}

