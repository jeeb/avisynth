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

#include "resample.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Resample_filters[] = {  
  { "PointResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", Create_PointResize },
  { "BilinearResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", Create_BilinearResize },
  { "BicubicResize", "cii[b]f[c]f[src_left]f[src_top]f[src_width]f[src_height]f", Create_BicubicResize },
  { "LanczosResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", Create_Lanczos3Resize},
  /**
    * Resize(PClip clip, dst_width, dst_height [src_left, src_top, src_width, int src_height,] )
    *
    * src_left et al.   =  when these optional arguments are given, the filter acts just like
    *                      a Crop was performed with those parameters before resizing, only faster
   **/

  { 0 }
};





/****************************************
 ***** Filtered Resize - Horizontal *****
 ***************************************/

FilteredResizeH::FilteredResizeH( PClip _child, double subrange_left, double subrange_width, 
                                  int target_width, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child), tempY(0), tempUV(0)
{
  pattern_chroma = 0;
  original_width = _child->GetVideoInfo().width;
  if (target_width<8)
    env->ThrowError("Resize: Width must be bigger than or equal to 8.");

  if (vi.IsYUV())
  {
    if ((target_width&1) && (vi.IsYUY2()))
      env->ThrowError("Resize: YUY2 width must be even");
    if ((target_width&1) && (vi.IsYV12()))
      env->ThrowError("Resize: YV12 width must be mutiple of 2.");
    tempY = (BYTE*) _aligned_malloc(original_width*2+4+32, 64);   // aligned for Athlon cache line
    tempUV = (BYTE*) _aligned_malloc(original_width*4+8+32, 64);  // aligned for Athlon cache line
    if (vi.IsYV12()) {
      pattern_chroma = GetResamplingPatternYUV( vi.width>>1, subrange_left/2, subrange_width/2,
        target_width>>1, func, true, tempY );
    } else {
      pattern_chroma = GetResamplingPatternYUV( vi.width>>1, subrange_left/2, subrange_width/2,
        target_width>>1, func, false, tempUV );
    }
    pattern_luma = GetResamplingPatternYUV(vi.width, subrange_left, subrange_width, target_width, func, true, tempY);
  }
  else
    pattern_luma = GetResamplingPatternRGB(vi.width, subrange_left, subrange_width, target_width, func);
  vi.width = target_width;
}


PVideoFrame __stdcall FilteredResizeH::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  if (vi.IsYV12()) {
      int plane = 0;
        while (plane++<3) {
//        int org_width = (plane==1) ? original_width : (original_width+1)>>1;
        int org_width = (plane==1) ? src->GetRowSize(PLANAR_Y_ALIGNED) : src->GetRowSize(PLANAR_V_ALIGNED);
        int dst_height= (plane==1) ? dst->GetHeight() : dst->GetHeight(PLANAR_U);
        int* array = (plane==1) ? pattern_luma : pattern_chroma;
        int dst_width = (plane==1) ? dst->GetRowSize(PLANAR_Y_ALIGNED) : dst->GetRowSize(PLANAR_U_ALIGNED);
        switch (plane) {
      case 2:
        srcp = src->GetReadPtr(PLANAR_U);
        dstp = dst->GetWritePtr(PLANAR_U);
        src_pitch = src->GetPitch(PLANAR_U);
        dst_pitch = dst->GetPitch(PLANAR_U);
        break;
      case 3:
        srcp = src->GetReadPtr(PLANAR_V);
        dstp = dst->GetWritePtr(PLANAR_V);
        src_pitch = src->GetPitch(PLANAR_U);
        dst_pitch = dst->GetPitch(PLANAR_U);
        }
      int fir_filter_size_luma = array[0];
      int* temp_dst;
      int loopc;
      static const __int64 x0000000000FF00FF = 0x0000000000FF00FF;
      static const __int64 xFFFF0000FFFF0000 = 0xFFFF0000FFFF0000;
      static const __int64 FPround =           0x0000200000002000;  // 16384/2
      int filter_offset=fir_filter_size_luma*8+8;
    __asm {
      pxor        mm0, mm0
      movq        mm7, x0000000000FF00FF
      movq        mm6, FPround
      movq        mm5, xFFFF0000FFFF0000
    }
    if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
     for (int y=0; y<dst_height; ++y)
      {
        int* cur_luma = array+2;
        int x = dst_width / 4;

        __asm {  // 
        mov         edi, this
        mov         ecx, org_width
        mov         edx, [edi].tempY;
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line to 00yy 00yy 00yy 00yy
        align 16

      yv_deintloop:
        prefetchnta [esi+256]
        movd        mm1, [esi]          ;mm1 = 00 00 YY YY
        inc         eax
        punpcklbw   mm1, mm0            ;mm1 = 0Y 0Y 0Y 0Y
        movq        [edx+eax*8], mm1    
        add         esi, 4
        sub         ecx, 4

        jnz         yv_deintloop
      // use this as source from now on
        mov         edx, dstp
        mov         eax, cur_luma
        mov         temp_dst,edx
        align 16
      yv_xloopYUV:
        mov         ebx, [filter_offset]  ; Offset to next pixel pair
        mov         ecx, fir_filter_size_luma
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0            ;Clear mm0
        movq        mm3, mm0            ;Clear mm3
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        mov         loopc,ecx
        mov         edx, [eax+ebx]      ;edx = next &tempY[ofs0]
        mov         ecx, [eax+ebx+4]    ;ecx = next &tempY[ofs1]
        add         eax, 8              ;cur_luma++
        align 16
      yv_aloopY:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        // Equivalent pixels of next two pixels are in mm4
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll1
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll2
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll3
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll4
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll5
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll6
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jz         out_yv_aloopY
//unroll7
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate

        jnz         yv_aloopY
        align 16
out_yv_aloopY:

        add eax,ebx;
        mov         edx,temp_dst
        paddd       mm1, mm6            ;Y1|Y1|Y0|Y0  (round)
        paddd       mm3, mm6            ;Y1|Y1|Y0|Y0  (round)
        psrld       mm1, 14             ;mm1 = 0|y1|0|y0
        psrld       mm3, 14             ;mm1 = 0|y1|0|y0
        pshufw mm1,mm1,11111000b        ;mm1 = 0|0|y1|y0
        pshufw mm3,mm3,10001111b        ;mm1 = y1|y0|0|0

        packuswb    mm1, mm1            ;mm1 = ...|0|0|0|Y1y0
        packuswb    mm3, mm3            ;mm3 = ...|0|0|Y1y0|0
        por mm1,mm3
        movd        [edx], mm1
        add         temp_dst,4
        dec         x
        jnz         yv_xloopYUV
        }// end asm
        srcp += src_pitch;
        dstp += dst_pitch;
      }// end for y
    } else {  // end if isse, else do mmx
      for (int y=0; y<dst_height; ++y) {
        int* cur_luma = array+2;
        int x = dst_width / 4;

        __asm {  // 
        mov         edi, this
        mov         ecx, org_width
        mov         edx, [edi].tempY;
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line to 00yy 00yy 00yy 00yy
        align 16

      yv_deintloop_mmx:
        movd        mm1, [esi]          ;mm1 = 00 00 YY YY
        inc         eax
        punpcklbw   mm1, mm0            ;mm1 = 0Y 0Y 0Y 0Y
        movq        [edx+eax*8], mm1    
        add         esi, 4
        sub         ecx, 4

        jnz         yv_deintloop_mmx
      // use this as source from now on
        mov         edx, dstp
        mov         eax, cur_luma
        mov         temp_dst,edx
        align 16
      yv_xloopYUV_mmx:
        mov         ebx, [filter_offset]  ; Offset to next pixel pair
        mov         ecx, fir_filter_size_luma
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0            ;Clear mm0
        movq        mm3, mm0            ;Clear mm3
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        mov         loopc,ecx
        mov         edx, [eax+ebx]      ;edx = next &tempY[ofs0]
        mov         ecx, [eax+ebx+4]    ;ecx = next &tempY[ofs1]
        add         eax, 8              ;cur_luma++
        align 16
      yv_aloopY_mmx:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        // Equivalent pixels of next two pixels are in mm4
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
         movd        mm4, [edx]
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
         add         esi, 4
         add         edx, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
         punpckldq   mm4, [ecx]         ;[eax] = COn|COm|COb|COa
         add         edi, 4
         add         ecx, 4
        pmaddwd     mm4, [eax+ebx]      ;mm4 = Y1|Y0 (DWORDs)
         add         eax, 8              ;cur_luma++
         dec         loopc
        paddd       mm1, mm2            ;accumulate
         paddd       mm3, mm4            ;accumulate
        jnz         yv_aloopY_mmx

        // we don't bother unrolling, since non-isse machines have small branch-misprediction penalties.
        add eax,ebx;
        mov         edx,temp_dst
        paddd       mm1, mm6            ;Y1|Y1|Y0|Y0  (round)
        paddd       mm3, mm6            ;Y1|Y1|Y0|Y0  (round)
        psrld       mm1, 14             ;mm1 = 0|y1|0|y0
        psrld       mm3, 14             ;mm1 = 0|y1|0|y0
        packuswb    mm1, mm0            ;mm1 = ...|0|0|y1|y0 
        packuswb    mm3, mm0            ;mm3 = ...|0|0|y1|y0
        packuswb    mm1, mm0            ;mm1 = ...|0|0|0|Y1y0
        packuswb    mm3, mm0            ;mm3 = ...|0|0|0|Y1y0
        psllq       mm3,16              ;mm3= 0|0|y1y0|0
        por mm1,mm3  

        movd        [edx], mm1
        add         temp_dst,4
        dec         x
        jnz         yv_xloopYUV_mmx
        }// end asm
        srcp += src_pitch;
        dstp += dst_pitch;
      } // end for y
    } // end mmx
  } // end while plane.
   __asm { emms }
  } else 
  if (vi.IsYUY2())
  {  
    int fir_filter_size_luma = pattern_luma[0];
    int fir_filter_size_chroma = pattern_chroma[0];
    static const __int64 x0000000000FF00FF = 0x0000000000FF00FF;
    static const __int64 xFFFF0000FFFF0000 = 0xFFFF0000FFFF0000;
    static const __int64 FPround =           0x0000200000002000;  // 16384/2

    __asm {
      pxor        mm0, mm0
      movq        mm7, x0000000000FF00FF
      movq        mm6, FPround
      movq        mm5, xFFFF0000FFFF0000
    }
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
     for (int y=0; y<vi.height; ++y)
      {
        int* cur_luma = pattern_luma+2;
        int* cur_chroma = pattern_chroma+2;
        int x = vi.width / 2;

        __asm {
        mov         edi, this
        mov         ecx, [edi].original_width
        mov         edx, [edi].tempY
        mov         ebx, [edi].tempUV
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line
        align 16
      i_deintloop:
        prefetchnta [esi+256]
        movd        mm1, [esi]          ;mm'1 = 00 00 VY UY
        inc         eax
        movq        mm2, mm1
        punpcklbw   mm2, mm0            ;mm2 = 0V 0Y 0U 0Y
        pand        mm1, mm7            ;mm1 = 00 00 0Y 0Y
        movd        [edx+eax*4], mm1
        psrld       mm2, 16             ;mm2 = 00 0V 00 0U
        add         esi, 4
        movq        [ebx+eax*8], mm2
        sub         ecx, 2
        jnz         i_deintloop
      // use this as source from now on
        mov         eax, cur_luma
        mov         ebx, cur_chroma
        mov         edx, dstp
        align 16
      i_xloopYUV:
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        movq        mm3, mm0
        mov         ecx, fir_filter_size_luma
        add         eax, 8              ;cur_luma++
        align 16
      i_aloopY:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll1
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll2
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll3
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll4
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll5
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll6
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jz         out_i_aloopY
//unroll7
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jnz         i_aloopY
        align 16
out_i_aloopY:
        mov         esi, [ebx]          ;esi=&tempUV[ofs]
        add         ebx, 8              ;cur_chroma++
        mov         ecx, fir_filter_size_chroma
        align 16
      i_aloopUV:
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll1
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll2
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll3
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll4
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll5
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll6
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jz         out_i_aloopUV
//unroll7
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jnz         i_aloopUV
        align 16
out_i_aloopUV:
        paddd       mm3, mm6            ; V| V| U| U  (round)
         paddd       mm1, mm6            ;Y1|Y1|Y0|Y0  (round)
        pslld       mm3, 2              ; Shift up from 14 bits fraction to 16 bit fraction
         pxor        mm4,mm4             ;Clear mm4 - utilize shifter stall
        psrld       mm1, 14             ;mm1 = 0|y1|0|y0
         pmaxsw      mm3,mm4             ;Clamp at 0

        pand        mm3, mm5            ;mm3 = v| 0|u| 0
        por         mm3,mm1
        packuswb    mm3, mm3            ;mm3 = ...|v|y1|u|y0
        movd        [edx], mm3
        add         edx, 4
        dec         x
        jnz         i_xloopYUV
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
    } else {  // MMX 
      for (int y=0; y<vi.height; ++y)
      {
        int* cur_luma = pattern_luma+2;
        int* cur_chroma = pattern_chroma+2;
        int x = vi.width / 2;

        __asm {
        mov         edi, this
        mov         ecx, [edi].original_width
        mov         edx, [edi].tempY
        mov         ebx, [edi].tempUV
        mov         esi, srcp
        mov         eax, -1
      // deinterleave current line
        align 16
      deintloop:
        inc         eax
        movd        mm1, [esi]          ;mm1 = 0000VYUY
        movq        mm2, mm1
        punpcklbw   mm2, mm0            ;mm2 = 0V0Y0U0Y
         pand        mm1, mm7            ;mm1 = 00000Y0Y
        movd        [edx+eax*4], mm1
         psrld       mm2, 16             ;mm2 = 000V000U
        add         esi, 4
        movq        [ebx+eax*8], mm2
        sub         ecx, 2
        jnz         deintloop
      // use this as source from now on
        mov         eax, cur_luma
        mov         ebx, cur_chroma
        mov         edx, dstp
        align 16
      xloopYUV:
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        movq        mm3, mm0
        mov         ecx, fir_filter_size_luma
        add         eax, 8              ;cur_luma++
        align 16
      aloopY:
        // Identifiers:
        // Ya, Yb: Y values in srcp[ofs0]
        // Ym, Yn: Y values in srcp[ofs1]
        movd        mm2, [esi]          ;mm2 =  0| 0|Yb|Ya
        add         esi, 4
        punpckldq   mm2, [edi]          ;mm2 = Yn|Ym|Yb|Ya
                                        ;[eax] = COn|COm|COb|COa
        add         edi, 4
        pmaddwd     mm2, [eax]          ;mm2 = Y1|Y0 (DWORDs)
        add         eax, 8              ;cur_luma++
        dec         ecx
        paddd       mm1, mm2            ;accumulate
        jnz         aloopY

        mov         esi, [ebx]          ;esi=&tempUV[ofs]
        add         ebx, 8              ;cur_chroma++
        mov         ecx, fir_filter_size_chroma
        align 16
      aloopUV:
        movq        mm2, [esi]          ;mm2 = 0|V|0|U
                                        ;[ebx] = 0|COv|0|COu
        add         esi, 8
        pmaddwd     mm2, [ebx]          ;mm2 = V|U (DWORDs)
        add         ebx, 8              ;cur_chroma++
        dec         ecx
        paddd       mm3, mm2            ;accumulate
        jnz         aloopUV

        movq        mm4, mm3            ; clip chroma at 0
        psrad       mm3, 31
        pandn       mm3, mm4        
 
        paddd       mm1, mm6            ;Y1|Y1|Y0|Y0  (round)
        paddd       mm3, mm6            ; V| V| U| U  (round)
        pslld       mm3, 2              ; Shift up from 14 bits fraction to 16 bit fraction
                                
        pand        mm3, mm5            ;mm3 = v| 0|u| 0

        psrld       mm1, 14             ;mm1 = 0|y1|0|y0
        por         mm3, mm1
        packuswb    mm3, mm3            ;mm3 = ...|v|y1|u|y0
        movd        [edx], mm3
        add         edx, 4
        dec         x
        jnz         xloopYUV
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
    }
    __asm { emms }
  }
  else
  if (vi.IsRGB24())
  {
    // RGB24 is not recommended. 75% of all pixels are not aligned.
    int y = vi.height;
    int w = vi.width * 3;
    int fir_filter_size = pattern_luma[0];
    int* pattern_lumaP1 = pattern_luma+1 - fir_filter_size;
    static const __int64 xFF000000 = 0xFF000000;
    __asm {
      mov         esi, srcp
      mov         edi, dstp
      pxor        mm2, mm2
      movq        mm4, xFF000000
      align 16
    yloop24:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
      align 16
    xloop24:
      mov         eax, fir_filter_size
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      mov         ebx, [edx]
      lea         ebx, [ebx+ebx*2]          ;ebx = ofs = *cur * 3
      add         edx, 4                    ;cur++
      pxor        mm0, mm0                  ;btotal, gtotal
      pxor        mm1, mm1                  ;rtotal
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      add         ebx, esi                  ;ebx = srcp + ofs*3
      lea         eax, [eax+eax*2]          ;eax = a = fir_filter_size*3
      align 16
    aloop24:
      sub         edx, 4                    ;cur--
      sub         eax, 3
      movd        mm7, [ebx+eax]            ;mm7 = srcp[ofs+a] = 0|0|0|0|x|r|g|b
      punpcklbw   mm7, mm2                  ;mm7 = 0x|0r|0g|0b
      movq        mm6, mm7
      punpcklwd   mm7, mm2                  ;mm7 = 00|0g|00|0b
      punpckhwd   mm6, mm2                  ;mm6 = 00|0x|00|0r
      movd        mm5, [edx]                ;mm5 =    00|co (co = coefficient)
      packssdw    mm5, mm2
      punpckldq   mm5, mm5                  ;mm5 =    co|co
      pmaddwd     mm7, mm5                  ;mm7 =  g*co|b*co
      pmaddwd     mm6, mm5                  ;mm6 =  x*co|r*co
      paddd       mm0, mm7
      paddd       mm1, mm6
      jnz         aloop24
      pslld       mm0, 2
      pslld       mm1, 2                    ;compensate the fact that FPScale = 16384
      packuswb    mm0, mm1                  ;mm0 = x|_|r|_|g|_|b|_
      psrlw       mm0, 8                    ;mm0 = 0|x|0|r|0|g|0|b
      packuswb    mm0, mm2                  ;mm0 = 0|0|0|0|x|r|g|b
      pslld       mm0, 8
      psrld       mm0, 8                    ;mm0 = 0|0|0|0|0|r|g|b
      movd        mm3, [edi+ecx]            ;mm3 = 0|0|0|0|x|r|g|b (dst)
      pand        mm3, mm4                  ;mm3 = 0|0|0|0|x|0|0|0 (dst)
      por         mm3, mm0
      movd        [edi+ecx], mm3

      add         ecx, 3
      cmp         ecx, w
      jnz         xloop24

      add         esi, src_pitch
      add         edi, dst_pitch
      dec         y
      jnz         yloop24
      emms
    }
  }
  else
  {
    // RGB32
    int y = vi.height;
    int w = vi.width;
    int fir_filter_size = pattern_luma[0];
    int* pattern_lumaP1 = &pattern_luma[1] - fir_filter_size;

    __asm {
      mov         esi, srcp
      mov         edi, dstp
      pxor        mm2, mm2
      align 16
    yloop32:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
      align 16
    xloop32:
      mov         eax, fir_filter_size
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      mov         ebx, [edx]
      shl         ebx, 2                    ;ebx = ofs = *cur * 4
      add         edx, 4                    ;cur++
      pxor        mm0, mm0                  ;btotal, gtotal
      pxor        mm1, mm1                  ;atotal, rtotal
      lea         edx, [edx+eax*4]          ;cur += fir_filter_size
      add         ebx, esi                  ;ebx = srcp + ofs*4
      align 16
    aloop32:
      sub         edx, 4                    ;cur--
      dec         eax
      movd        mm7, [ebx+eax*4]          ;mm7 = srcp[ofs+a] = 0|0|0|0|a|r|g|b
      punpcklbw   mm7, mm2                  ;mm7 = 0a|0r|0g|0b
      movq        mm6, mm7
      punpcklwd   mm7, mm2                  ;mm7 = 00|0g|00|0b
      punpckhwd   mm6, mm2                  ;mm6 = 00|0a|00|0r
      movd        mm5, [edx]                ;mm5 =    00|co (co = coefficient)
      packssdw    mm5, mm2
      punpckldq   mm5, mm5                  ;mm5 =    co|co
      pmaddwd     mm7, mm5                  ;mm7 =  g*co|b*co
      pmaddwd     mm6, mm5                  ;mm6 =  a*co|r*co
      paddd       mm0, mm7
      paddd       mm1, mm6
      jnz         aloop32
      pslld       mm0, 2
      pslld       mm1, 2                    ;compensate the fact that FPScale = 16384
      packuswb    mm0, mm1                  ;mm0 = a|_|r|_|g|_|b|_
      psrlw       mm0, 8                    ;mm0 = 0|a|0|r|0|g|0|b
      packuswb    mm0, mm2                  ;mm0 = 0|0|0|0|a|r|g|b
      movd        [edi+ecx*4], mm0

      inc         ecx
      cmp         ecx, w
      jnz         xloop32

      add         esi, src_pitch
      add         edi, dst_pitch
      dec         y
      jnz         yloop32
      emms
    }
  }
  return dst;
} 


FilteredResizeH::~FilteredResizeH(void) 
{
  _aligned_free(pattern_luma);
  _aligned_free(pattern_chroma);
  if (tempY)
  {
    _aligned_free(tempUV);
    _aligned_free(tempY);
  }
}

 



/***************************************
 ***** Filtered Resize - Vertical ******
 ***************************************/

FilteredResizeV::FilteredResizeV( PClip _child, double subrange_top, double subrange_height, 
                                  int target_height, ResamplingFunction* func, IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  if (target_height<4)
    env->ThrowError("Resize: Height must be bigger than or equal to 4.");
  if (vi.IsYV12() && (target_height&3) && vi.IsFieldBased())
    env->ThrowError("Resize: Fieldbased YV12 destination height must be multiple of 4.");
  if (vi.IsYV12() && (target_height&1))
    env->ThrowError("Resize: YV12 destination height must be multiple of 2.");
  if (vi.IsRGB())
    subrange_top = vi.height - subrange_top - subrange_height;
  resampling_pattern = GetResamplingPatternRGB(vi.height, subrange_top, subrange_height, target_height, func);
  resampling_patternUV = GetResamplingPatternRGB(vi.height>>1, subrange_top/2.0f, subrange_height/2.0f, target_height>>1, func);
  vi.height = target_height;

  PVideoFrame src = child->GetFrame(0, env);
  int sh = src->GetHeight();
  pitch_gY = src->GetPitch(PLANAR_Y);
  yOfs = new int[sh];
  for (int i = 0; i < sh; i++) yOfs[i] = src->GetPitch() * i;

  int shUV = src->GetHeight(PLANAR_U);
  pitch_gUV = src->GetPitch(PLANAR_U);
  yOfsUV = new int[shUV];
  for (i = 0; i < shUV; i++) {
    yOfsUV[i] = src->GetPitch(PLANAR_U) * i;
  }
}


PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  static const __int64 FPround =           0x0000200000002000;  // 16384/2
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  int* cur = resampling_pattern;
  int fir_filter_size = *cur++;
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  int xloops = src->GetRowSize() / 4;
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int y = vi.height;
  int plane = vi.IsPlanar() ? 4:1;
  int *yOfs2 = this->yOfs;
  if (pitch_gUV != src->GetPitch(PLANAR_U)) {
    int shUV = src->GetHeight(PLANAR_U);
    for (int i = 0; i < shUV; i++) yOfsUV[i] = src->GetPitch(PLANAR_U) * i;
    pitch_gUV = src->GetPitch(PLANAR_U);
  }
  if (pitch_gY != src->GetPitch(PLANAR_Y))  {
    int sh = src->GetHeight();
    pitch_gY = src->GetPitch(PLANAR_Y);
    for (int i = 0; i < sh; i++) yOfs[i] = src->GetPitch() * i;
  }
  while (plane-->0){
    switch (plane) {
      case 2:  // take V plane
        cur = resampling_patternUV;
        fir_filter_size = *cur++;
        src_pitch = src->GetPitch(PLANAR_V);
        dst_pitch = dst->GetPitch(PLANAR_V);
        xloops = src->GetRowSize(PLANAR_V_ALIGNED) / 4;  // Means mod 8 for planar
        dstp = dst->GetWritePtr(PLANAR_V);
        srcp = src->GetReadPtr(PLANAR_V);
        y = dst->GetHeight(PLANAR_V);
        yOfs2 = this->yOfsUV;
        break;
      case 1: // U Plane
        cur = resampling_patternUV;
        fir_filter_size = *cur++;
        dstp = dst->GetWritePtr(PLANAR_U);
        srcp = src->GetReadPtr(PLANAR_U);
        y = dst->GetHeight(PLANAR_U);
        src_pitch = src->GetPitch(PLANAR_U);
        dst_pitch = dst->GetPitch(PLANAR_U);
        xloops = src->GetRowSize(PLANAR_U_ALIGNED) / 4 ;  // Means mod 8 for planar
        yOfs2 = this->yOfsUV;
        plane--; // skip case 0
        break;
      case 3: // Y plane for planar
      case 0: // Default for interleaved
        break;
    }
  __asm {
//    emms
    mov         edx, cur
    pxor        mm0, mm0
    mov         edi, fir_filter_size
    movq        mm6,[FPround]
    align 16
  yloop:
    mov         esi, yOfs2
    mov         eax, [edx]              ;eax = *cur
    mov         esi, [esi+eax*4]
    add         edx, 4                  ;cur++
    add         esi, srcp               ;esi = srcp + yOfs[*cur]
    xor         ecx, ecx                ;ecx = x = 0
    pxor        mm7, mm7
    pxor        mm1, mm1                ;total = 0
    align 16
  xloop:
    lea         eax, [esi+ecx*4]        ;eax = srcp2 = srcp + x
    xor         ebx, ebx                ;ebx = b = 0
    align 16
  bloop:
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll1
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll2
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll3
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll4
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll5
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll6
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    cmp         ebx, edi
    jz         out_bloop
//unroll7
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a     
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
     inc         ebx
    punpckldq   mm3, mm3                ;mm3 = co|co
     movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
     add         eax, src_pitch          ;srcp2 += src_pitch
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
     punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
     paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate

    cmp         ebx, edi
    jnz         bloop
align 16
out_bloop:
    paddd       mm1,mm6
    paddd       mm7,mm6
    mov         eax, dstp
    pslld       mm1, 2                  ;14 bits -> 16bit fraction
    pslld       mm7, 2                  ;compensate the fact that FPScale = 16384
    packuswb    mm1, mm7                ;mm1 = d|_|c|_|b|_|a|_
    psrlw       mm1, 8                  ;mm1 = 0|d|0|c|0|b|0|a
    packuswb    mm1, mm2                ;mm1 = 0|0|0|0|d|c|b|a
    movd        [eax+ecx*4], mm1
    pxor        mm7, mm7
    inc         ecx
    pxor        mm1, mm1                ;total = 0
    cmp         ecx, xloops
    jnz         xloop
    add         eax, dst_pitch
    lea         edx, [edx+edi*4]        ;cur += fir_filter_size
    mov         dstp, eax
    dec         y
    jnz         yloop
    emms
  }
  } // end while
  return dst;
}


FilteredResizeV::~FilteredResizeV(void)
{
  _aligned_free(resampling_pattern);
  delete[] yOfs;
}






/**********************************************
 *******   Resampling Factory Methods   *******
 **********************************************/

PClip CreateResizeH(PClip clip, double subrange_left, double subrange_width, int target_width, 
                    ResamplingFunction* func, IScriptEnvironment* env) 
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_left == 0 && subrange_width == target_width && subrange_width == vi.width) {
    return clip;
  } else if (subrange_left == int(subrange_left) && subrange_width == target_width
             && subrange_left >= 0 && subrange_left + subrange_width <= vi.width) 
  {
    return new Crop(int(subrange_left), 0, int(subrange_width), vi.height, clip, env);
  } else {
    return new FilteredResizeH(clip, subrange_left, subrange_width, target_width, func, env);
  }
}


PClip CreateResizeV(PClip clip, double subrange_top, double subrange_height, int target_height, 
                    ResamplingFunction* func, IScriptEnvironment* env) 
{
  const VideoInfo& vi = clip->GetVideoInfo();
  if (subrange_top == 0 && subrange_height == target_height && subrange_height == vi.height) {
    return clip;
  } else if (subrange_top == int(subrange_top) && subrange_height == target_height
             && subrange_top >= 0 && subrange_top + subrange_height <= vi.height) 
  {
    return new Crop(0, int(subrange_top), vi.width, int(subrange_height), clip, env);
  } else {
    return new FilteredResizeV(clip, subrange_top, subrange_height, target_height, func, env);
  }
}


PClip CreateResize(PClip clip, int target_width, int target_height, const AVSValue* args, 
                   ResamplingFunction* f, IScriptEnvironment* env) 
{
  const VideoInfo& vi = clip->GetVideoInfo();
  const double subrange_left = args[0].AsFloat(0), subrange_top = args[1].AsFloat(0);
  const double subrange_width = args[2].AsFloat(vi.width), subrange_height = args[3].AsFloat(vi.height);
  PClip result = clip;
  bool H = ((subrange_width != vi.width) || (target_width != vi.width));
  bool V = ((subrange_height != vi.height) || (target_height != vi.height));
  // ensure that the intermediate area is maximal
  const double area_FirstH = subrange_height * target_width;
  const double area_FirstV = subrange_width * target_height;
  if (area_FirstH < area_FirstV)
  {
    if (V)
      result = CreateResizeV(clip, subrange_top, subrange_height, target_height, f, env);
    if (H)
      result = CreateResizeH(result, subrange_left, subrange_width, target_width, f, env);
  }
  else
  {
    if (H)
      result = CreateResizeH(clip, subrange_left, subrange_width, target_width, f, env);
    if (V)
      result = CreateResizeV(result, subrange_top, subrange_height, target_height, f, env);
  }
  return result;
}

AVSValue __cdecl Create_PointResize(AVSValue args, void*, IScriptEnvironment* env) 
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], 
                       &PointFilter(), env );
}


AVSValue __cdecl Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env) 
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], 
                       &TriangleFilter(), env );
}


AVSValue __cdecl Create_BicubicResize(AVSValue args, void*, IScriptEnvironment* env) 
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[5], 
                       &MitchellNetravaliFilter(args[3].AsFloat(1./3.), args[4].AsFloat(1./3.)), env );
}

// 09-14-2002 - Vlad59 - Lanczos3Resize - Added Lanczos3Resize
AVSValue __cdecl Create_Lanczos3Resize(AVSValue args, void*, IScriptEnvironment* env) 
{
  return CreateResize( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), &args[3], 
                       &Lanczos3Filter(), env );
}
