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

#include "resample.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Resample_filters[] = {  
  { "BilinearResize", "cii[src_left]f[src_top]f[src_width]f[src_height]f", Create_BilinearResize },
  { "BicubicResize", "cii[b]f[c]f[src_left]f[src_top]f[src_width]f[src_height]f", Create_BicubicResize },
  /**
    * Resize(PClip clip, [src_left, src_top, src_width, int src_height,] dst_width, dst_height)
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
  if (vi.IsYUY2())
  {
    if (target_width&1)
      env->ThrowError("Resize: YUY2 width must be even");
    tempY = (BYTE*) _aligned_malloc(original_width*2+4, 64);   // aligned for Athlon cache line
    tempUV = (BYTE*) _aligned_malloc(original_width*4+8, 64);  // aligned for Athlon cache line
    pattern_chroma = GetResamplingPatternYUV( vi.width>>1, subrange_left/2, subrange_width/2,
                                              target_width>>1, func, false, tempUV );
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
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();

  if (vi.IsYUY2())
  {  
    int fir_filter_size_luma = pattern_luma[0];
    int fir_filter_size_chroma = pattern_chroma[0];
    __declspec(align(8)) static const __int64 x0000000000FF00FF = 0x0000000000FF00FF;
    __declspec(align(8)) static const __int64 x00FF000000FF0000 = 0x00FF000000FF0000;
    __declspec(align(8)) static const __int64 FPround =           0x0000200000002000;  // 16384/2

    __asm {
      pxor        mm0, mm0
      movq        mm7, x0000000000FF00FF
      movq        mm6, FPround
      movq        mm5, x00FF000000FF0000
    }
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
      deintloop:
        inc         eax
        movd        mm1, [esi]          ;mm1 = 0000VYUY
        movq        mm2, mm1
        punpcklbw   mm2, mm0            ;mm2 = 0V0Y0U0Y
        pand        mm1, mm7            ;mm1 = 00000Y0Y
        psrld       mm2, 16             ;mm2 = 000V000U
        movd        [edx+eax*4], mm1
        add         esi, 4
        movq        [ebx+eax*8], mm2
        sub         ecx, 2
        jnz         deintloop
      // use this as source from now on
        mov         eax, cur_luma
        mov         ebx, cur_chroma
        mov         edx, dstp
      xloopYUV:
        mov         esi, [eax]          ;esi=&tempY[ofs0]
        movq        mm1, mm0
        mov         edi, [eax+4]        ;edi=&tempY[ofs1]
        movq        mm3, mm0
        mov         ecx, fir_filter_size_luma
        add         eax, 8              ;cur_luma++
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

        paddd       mm1, mm6            ;Y1|Y1|Y0|Y0
        paddd       mm3, mm6            ; V| V| U| U
        pslld       mm3, 2
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
    __declspec(align(8)) static const __int64 xFF000000 = 0xFF000000;
    __asm {
      mov         esi, srcp
      mov         edi, dstp
      pxor        mm2, mm2
      movq        mm4, xFF000000

    yloop24:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
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
    aloop24:
      sub         edx, 4                    ;cur--
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
      sub         eax, 3
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
    int* pattern_lumaP1 = pattern_luma+1 - fir_filter_size;

    __asm {
      mov         esi, srcp
      mov         edi, dstp
      pxor        mm2, mm2

    yloop32:
      xor         ecx, ecx
      mov         edx, pattern_lumaP1       ;cur - fir_filter_size
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
    aloop32:
      sub         edx, 4                    ;cur--
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
      dec         eax
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
  if (vi.IsRGB())
    subrange_top = vi.height - subrange_top - subrange_height;
  resampling_pattern = GetResamplingPatternRGB(vi.height, subrange_top, subrange_height, target_height, func);
  vi.height = target_height;

  PVideoFrame src = child->GetFrame(0, env);
  int sh = src->GetHeight();
  yOfs = new int[sh];
  for (int i = 0; i < sh; i++) yOfs[i] = src->GetPitch() * i;
}


PVideoFrame __stdcall FilteredResizeV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const int* cur = resampling_pattern;
  const int fir_filter_size = *cur++;
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const int xloops = src->GetRowSize() / 4;
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int y = vi.height;
  int *yOfs = this->yOfs;

  __asm {
    emms
    mov         edx, cur
    pxor        mm0, mm0
    mov         edi, fir_filter_size
  yloop:
    mov         esi, yOfs
    mov         eax, [edx]              ;eax = *cur
    mov         esi, [esi+eax*4]
    add         edx, 4                  ;cur++
    add         esi, srcp               ;esi = srcp + yOfs[*cur]
    xor         ecx, ecx                ;ecx = x = 0
  xloop:
    pxor        mm7, mm7
    pxor        mm1, mm1                ;total = 0
    lea         eax, [esi+ecx*4]        ;eax = srcp2 = srcp + x
    xor         ebx, ebx                ;ebx = b = 0
  bloop:
    movd        mm2, [eax]              ;mm2 = *srcp2 = 0|0|0|0|d|c|b|a
    movd        mm3, [edx+ebx*4]        ;mm3 = cur[b] = 0|co
    punpcklbw   mm2, mm0                ;mm2 = 0d|0c|0b|0a
    punpckldq   mm3, mm3                ;mm3 = co|co
    movq        mm4, mm2
    punpcklwd   mm2, mm0                ;mm2 = 00|0b|00|0a
    punpckhwd   mm4, mm0                ;mm4 = 00|0d|00|0c
    pmaddwd     mm2, mm3                ;mm2 =  b*co|a*co
    pmaddwd     mm4, mm3                ;mm4 =  d*co|c*co
    paddd       mm1, mm2
    paddd       mm7, mm4                ;accumulate
    inc         ebx
    add         eax, src_pitch          ;srcp2 += src_pitch
    cmp         ebx, edi
    jnz         bloop
    mov         eax, dstp
    pslld       mm1, 2
    pslld       mm7, 2                  ;compensate the fact that FPScale = 16384
    packuswb    mm1, mm7                ;mm1 = d|_|c|_|b|_|a|_
    psrlw       mm1, 8                  ;mm1 = 0|d|0|c|0|b|0|a
    packuswb    mm1, mm2                ;mm1 = 0|0|0|0|d|c|b|a
    movd        [eax+ecx*4], mm1
    inc         ecx
    cmp         ecx, xloops
    jnz         xloop
    add         eax, dst_pitch
    lea         edx, [edx+edi*4]        ;cur += fir_filter_size
    mov         dstp, eax
    dec         y
    jnz         yloop
    emms
  }
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
