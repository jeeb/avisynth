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


#include "resize.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Resize_filters[] = {
  { "VerticalReduceBy2", "c", VerticalReduceBy2::Create },        // src clip
  { "HorizontalReduceBy2", "c", HorizontalReduceBy2::Create },    // src clip
  { "ReduceBy2", "c", Create_ReduceBy2 },                         // src clip
  { 0 }
};





/*************************************
 ******* Vertical 2:1 Reduction ******
 ************************************/


VerticalReduceBy2::VerticalReduceBy2(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  original_height = vi.height;
  vi.height >>= 1;
  if (vi.height<3) {
    env->ThrowError("VerticalReduceBy2: Image too small to be reduced by 2.");    
  }
}


PVideoFrame VerticalReduceBy2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const int row_size = src->GetRowSize();
  BYTE* dstp = dst->GetWritePtr();

  if ((env->GetCPUFlags() & CPUF_MMX)) {
    if ((row_size&3)==0) {  // row width divideable with 4 (one dword per loop)
      mmx_process(src,dstp,dst_pitch);
      return dst;
    }
  } 
    
  for (int y=0; y<vi.height; ++y) {
    const BYTE* line0 = src->GetReadPtr() + (y*2)*src_pitch;
    const BYTE* line1 = line0 + src_pitch;
    const BYTE* line2 = (y*2 < original_height-2) ? (line1 + src_pitch) : line0;
    for (int x=0; x<row_size; ++x)
      dstp[x] = (line0[x] + 2*line1[x] + line2[x] + 2) >> 2;
    dstp += dst_pitch;
  }

  return dst;
}

/*************************************
 ******* Vertical 2:1 Reduction ******
 ******* MMX Optimized          ******
 ******* by Klaus Post          ******
 ************************************/


#define R_SRC edx
#define R_DST edi
#define R_XOFFSET eax
#define R_YLEFT ebx
#define R_SRC_PITCH ecx
#define R_DST_PITCH esi

void VerticalReduceBy2::mmx_process(PVideoFrame src,BYTE* dstp, int dst_pitch) {
  
  const BYTE* srcp = src->GetReadPtr();
  const int src_pitch = src->GetPitch();
  int row_size = src->GetRowSize();
  const int height = vi.height-1;
  static const __int64 add_2=0x0002000200020002;
  __asm {
    movq mm7,[add_2];
  }
  if ((row_size&3)==0) {  // row width divideable with 4 (one dword per loop)
    __asm {
				add [srcp],-4
        mov R_XOFFSET,0
        mov R_SRC,srcp
        mov R_DST,dstp
        mov R_SRC_PITCH,[src_pitch]
        mov R_DST_PITCH,[dst_pitch]
        mov R_YLEFT,[height]
loopback:
      pxor mm1,mm1
        punpckhbw mm0,[R_SRC]  // line0
        punpckhbw mm1,[R_SRC+R_SRC_PITCH]  // line1
        punpckhbw mm2,[R_SRC+R_SRC_PITCH*2]  // line2
        psrlw mm0,8
        psrlw mm1,7
        paddw mm0,mm7
        psrlw mm2,8
        paddw mm0,mm1
        paddw mm0,mm2
        psrlw mm0,2
        packuswb mm0,mm1
        movd [R_DST+R_XOFFSET],mm0
        add R_SRC,4
        add R_XOFFSET,4
        cmp  R_XOFFSET,[row_size]
        jl loopback						; Jump back
        add srcp, R_SRC_PITCH
        mov R_XOFFSET,0
        add srcp, R_SRC_PITCH
        add R_DST,R_DST_PITCH
        mov R_SRC,srcp
        dec R_YLEFT
        jnz loopback
        
        // last line 
loopback_last:
      pxor mm1,mm1
        punpckhbw mm0,[R_SRC]  // line0
        punpckhbw mm1,[R_SRC+R_SRC_PITCH]  // line1
        psrlw mm0,8
        movq mm2,mm1  // dupe line 1
        psrlw mm1,7
        paddw mm0,mm7
        psrlw mm2,8
        paddw mm0,mm1
        paddw mm0,mm2
        psrlw mm0,2
        packuswb mm0,mm1
        movd [R_DST+R_XOFFSET],mm0
        add R_XOFFSET,4
        add R_SRC,4
        cmp  R_XOFFSET,[row_size]
        jl loopback_last						; Jump back
        emms
    }
  }
}

#undef R_SRC
#undef R_DST
#undef R_XOFFSET
#undef R_YLEFT
#undef R_SRC_PITCH
#undef R_DST_PITCH





/************************************
 **** Horizontal 2:1 Reduction ******
 ***********************************/

HorizontalReduceBy2::HorizontalReduceBy2(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child), mybuffer(0)
{
  if (vi.IsYUY2() && (vi.width & 3))
    env->ThrowError("HorizontalReduceBy2: YUY2 image width must be even");
  source_width = vi.width;
  vi.width >>= 1;
}


PVideoFrame HorizontalReduceBy2::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const int src_gap = src->GetPitch() - src->GetRowSize();  //aka 'modulo' in VDub filter terminology
  const int dst_gap = dst->GetPitch() - dst->GetRowSize();
  const int dst_pitch = dst->GetPitch();

  BYTE* dstp = dst->GetWritePtr();

  if (vi.IsYUY2()) {
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
			isse_process_yuy2(src,dstp,dst_pitch);
			return dst;
		}
  const BYTE* srcp = src->GetReadPtr();
    for (int y = vi.height; y>0; --y) {
      for (int x = (vi.width>>1)-1; x; --x) {
        dstp[0] = (srcp[0] + 2*srcp[2] + srcp[4] + 2) >> 2;
        dstp[1] = (srcp[1] + 2*srcp[5] + srcp[9] + 2) >> 2;
        dstp[2] = (srcp[4] + 2*srcp[6] + srcp[8] + 2) >> 2;
        dstp[3] = (srcp[3] + 2*srcp[7] + srcp[11] + 2) >> 2;
        dstp += 4;
        srcp += 8;
      }
      dstp[0] = (srcp[0] + 2*srcp[2] + srcp[4] + 2) >> 2;
      dstp[1] = (srcp[1] + srcp[5] + 1) >> 1;
      dstp[2] = (srcp[4] + srcp[6] + 1) >> 1;
      dstp[3] = (srcp[3] + srcp[7] + 1) >> 1;
      dstp += dst_gap+4;
      srcp += src_gap+8;

    }
  } else if (vi.IsRGB24()) {
  const BYTE* srcp = src->GetReadPtr();
    for (int y = vi.height; y>0; --y) {
      for (int x = (source_width-1)>>1; x; --x) {
        dstp[0] = (srcp[0] + 2*srcp[3] + srcp[6] + 2) >> 2;
        dstp[1] = (srcp[1] + 2*srcp[4] + srcp[7] + 2) >> 2;
        dstp[2] = (srcp[2] + 2*srcp[5] + srcp[8] + 2) >> 2;
        dstp += 3;
        srcp += 6;
      }
      if (source_width&1) {
        dstp += dst_gap;
        srcp += src_gap+3;
      } else {
        dstp[0] = (srcp[0] + srcp[3] + 1) >> 1;
        dstp[1] = (srcp[1] + srcp[4] + 1) >> 1;
        dstp[2] = (srcp[2] + srcp[5] + 1) >> 1;
        dstp += dst_gap+3;
        srcp += src_gap+6;
      }
    }
  } else {  //rgb32
    for (int y = vi.height; y>0; --y) {
  const BYTE* srcp = src->GetReadPtr();
      for (int x = (source_width-1)>>1; x; --x) {
        dstp[0] = (srcp[0] + 2*srcp[4] + srcp[8] + 2) >> 2;
        dstp[1] = (srcp[1] + 2*srcp[5] + srcp[9] + 2) >> 2;
        dstp[2] = (srcp[2] + 2*srcp[6] + srcp[10] + 2) >> 2;
        dstp[3] = (srcp[3] + 2*srcp[7] + srcp[11] + 2) >> 2;
        dstp += 4;
        srcp += 8;
      }
      if (source_width&1) {
        dstp += dst_gap;
        srcp += src_gap+4;
      } else {
        dstp[0] = (srcp[0] + srcp[4] + 1) >> 1;
        dstp[1] = (srcp[1] + srcp[5] + 1) >> 1;
        dstp[2] = (srcp[2] + srcp[6] + 1) >> 1;
        dstp[3] = (srcp[3] + srcp[7] + 1) >> 1;
        dstp += dst_gap+4;
        srcp += src_gap+8;
      }
    }
  }
  return dst;
}

/************************************
 **** Horizontal 2:1 Reduction ******
 **** YUY2 Integer SSE Optimized ****
 **** by Klaus Post              ****
 ***********************************/


#define R_SRC esi
#define R_DST edi
#define R_XOFFSET edx
#define R_YLEFT ebx
#define R_TEMP1 eax
#define R_TEMP2 ecx

void HorizontalReduceBy2::isse_process_yuy2(PVideoFrame src,BYTE* dstp, int dst_pitch) {
  
  const BYTE* srcp = src->GetReadPtr();
  const int src_pitch = src->GetPitch();
  int row_size = ((src->GetRowSize())>>1)-4;
	
  const int height = vi.height;
  __declspec(align(8)) static const __int64 add_2=0x0002000200020002;
  __declspec(align(8)) static const __int64 zero_mask=0x000000000000ffff;
  __declspec(align(8)) static const __int64 three_mask=0x0000ffff00000000;
  __declspec(align(8)) static const __int64 inv_0_mask=0xffffffffffff0000;
  __declspec(align(8)) static const __int64 inv_3_mask=0xffff0000ffffffff;
  __asm {
    movq mm6,[zero_mask];
    movq mm7,[inv_0_mask];
  }
/**
 * The matrix is flipped, so the mmx registers are equivalent to
 *  the downward column, and pixels are then added sideways in parallel.
 * The last two pixels are handled seperately (as in the C-code), 
 *  with naiive code - but it's only the last pixel.
 **/
  if ((row_size&3)==0) {  // row width divideable with 8 (one qword per loop)
    __asm {
      mov R_XOFFSET,0
				add [srcp],-4
				prefetchnta [srcp]
				prefetchnta [srcp+64]
        mov R_SRC,srcp
        mov R_DST,dstp
        mov R_YLEFT,[height]
loop_nextline:
				mov R_TEMP2,[row_size]
loopback:
				prefetchnta [R_SRC+64]
        punpckhbw mm0,[R_SRC]  // pixel 1
        punpckhbw mm1,[R_SRC+4]  // pixel 2
        punpckhbw mm2,[R_SRC+8]  // pixel 3
				psrlw mm0,8				;mm0=00VV 00Y2 00UU 00Y1
				psrlw mm1,8				;mm1=00VV 00Y2 00UU 00Y1
				psrlw mm2,8				;mm2=00VV 00Y2 00UU 00Y1
				movq mm3,mm0								; word 3 [4] word 0 in mm1 (ok)
				movq mm4,mm1								; word 0 [2] must be exchanged with word 2 in mm0
				pand mm3,[inv_3_mask]
				pshufw mm5,mm2,11000100b		; word 0 [4] must be exhanged (word0 in mm1) (ok)
				pand mm4,mm7
				pshufw mm1,mm1,0
				pand mm5,mm7
				movq mm2,mm1
        pand mm1,mm6
				pshufw mm0,mm0,10101010b
        pand mm2,[three_mask]
        pand mm0,mm6
				por mm4,mm0
				por mm3,mm2
				psllw mm4,1
				por mm5,mm1
				paddw mm3,mm4
				paddw mm5,[add_2]
				paddw mm3,mm5
        psrlw mm3,2
        packuswb mm3,mm1
        movd [R_DST+R_XOFFSET],mm3

				add R_SRC,8
        add R_XOFFSET,4
        cmp  R_XOFFSET,R_TEMP2
        jl loopback						; Jump back
				// Last two pixels
				// Could be optimized greatly, but it only runs once per line

			mov R_TEMP1,[R_SRC]
			mov R_TEMP2,[R_SRC+2]
			and R_TEMP1,255
			and R_TEMP2,255
			add R_TEMP1,R_TEMP2
			add R_TEMP1,R_TEMP2
			mov R_TEMP2,[R_SRC+4]
			add R_TEMP1,2
			and R_TEMP2,255
			add R_TEMP1,R_TEMP2
			shr R_TEMP1,2
			mov [R_DST+R_XOFFSET],R_TEMP1

			mov R_TEMP1,[R_SRC+1]
			mov R_TEMP2,[R_SRC+5]
			and R_TEMP1,255
			and R_TEMP2,255
			add R_TEMP1,1
			add R_TEMP1,R_TEMP2
			shr R_TEMP1,1
			shl R_TEMP1,8
			or [R_DST+R_XOFFSET],R_TEMP1

			mov R_TEMP1,[R_SRC+4]
			mov R_TEMP2,[R_SRC+6]
			and R_TEMP1,255
			and R_TEMP2,255
			add R_TEMP1,1
			add R_TEMP1,R_TEMP2
			shr R_TEMP1,1
			shl R_TEMP1,16
			or [R_DST+R_XOFFSET],R_TEMP1

			mov R_TEMP1,[R_SRC+3]
			mov R_TEMP2,[R_SRC+7]
			and R_TEMP1,255
			and R_TEMP2,255
			add R_TEMP1,1
			add R_TEMP1,R_TEMP2
			shr R_TEMP1,1
			shl R_TEMP1,24
			or [R_DST+R_XOFFSET],R_TEMP1

				mov R_TEMP2,[src_pitch]
        add srcp, R_TEMP2
        mov R_XOFFSET,0
        add R_DST,[dst_pitch]
        mov R_SRC,srcp
        dec R_YLEFT
        jnz loop_nextline        
        emms
    }
  }
}

#undef R_SRC
#undef R_DST
#undef R_XOFFSET
#undef R_YLEFT
#undef R_SRC_PITCH
#undef R_DST_PITCH




/**************************************
 *****  ReduceBy2 Factory Method  *****
 *************************************/
 

AVSValue __cdecl Create_ReduceBy2(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new HorizontalReduceBy2(new VerticalReduceBy2(args[0].AsClip(), env),env);
}



