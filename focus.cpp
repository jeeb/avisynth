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


#include "focus.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Focus_filters[] = {
  { "Blur", "cf[]f", Create_Blur },                     // amount [0 - 1]
  { "Sharpen", "cf[]f", Create_Sharpen },               // amount [0 - 1]
  { "TemporalSoften", "ciii", TemporalSoften::Create }, // radius, luma_threshold, chroma_threshold
  { "SpatialSoften", "ciii", SpatialSoften::Create },   // radius, luma_threshold, chroma_threshold
  { 0 }
};





/****************************************
 ****  AdjustFocus helper classes   *****
 ***************************************/

AdjustFocusV::AdjustFocusV(double _amount, PClip _child)
  : GenericVideoFilter(_child), amount(int(32768*pow(2.0, _amount)+0.5)) , line(NULL) {}

AdjustFocusV::~AdjustFocusV(void) 
{ 
  delete[] line; 
}

PVideoFrame __stdcall AdjustFocusV::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  if (!line)
      line = new BYTE[frame->GetRowSize()];
  BYTE* buf = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  const int center_weight = amount*2;
  const int outer_weight = 32768-amount;
  int row_size = vi.RowSize();
  memcpy(line, buf, row_size);
  BYTE* p = buf + pitch;
  for (int y = vi.height-2; y>0; --y) 
  {
    for (int x = 0; x < row_size; ++x) 
    {
      BYTE a = ScaledPixelClip(p[x] * center_weight + (line[x] + p[x+pitch]) * outer_weight);
      line[x] = p[x];
      p[x] = a;
    }
    p += pitch;
  } 
  return frame;
}



AdjustFocusH::AdjustFocusH(double _amount, PClip _child)
  : GenericVideoFilter(_child), amount(int(32768*pow(2.0, _amount)+0.5)) {}

PVideoFrame __stdcall AdjustFocusH::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  const int center_weight = amount*2;
  const int outer_weight = 32768-amount;
  const int row_size = vi.RowSize();
  BYTE* q = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  if (vi.IsYUY2()) {
    for (int y = vi.height; y>0; --y) 
    {
      BYTE uv = q[1];
      BYTE yy = q[2];
      BYTE vu = q[3];
      q[2] = ScaledPixelClip(q[2] * center_weight + (q[0] + q[4]) * outer_weight);
      for (int x = 2; x < vi.width-2; ++x) 
      {
        BYTE w = ScaledPixelClip(q[x*2+1] * center_weight + (uv + q[x*2+5]) * outer_weight);
        uv = vu; vu = q[x*2+1]; q[x*2+1] = w;
        BYTE y = ScaledPixelClip(q[x*2+0] * center_weight + (yy + q[x*2+2]) * outer_weight);
        yy = q[x*2+0]; q[x*2+0] = y;
      }
      q[vi.width*2-4] = ScaledPixelClip(q[vi.width*2-4] * center_weight + (yy + q[vi.width*2-2]) * outer_weight);
      q += pitch;
    }
  } 
  else if (vi.IsRGB32()) {
  for (int y = vi.height; y>0; --y) 
  {
      BYTE bb = q[0];
      BYTE gg = q[1];
      BYTE rr = q[2];
      BYTE aa = q[3];
      for (int x = 1; x < vi.width-1; ++x) 
      {
        BYTE b = ScaledPixelClip(q[x*4+0] * center_weight + (bb + q[x*4+4]) * outer_weight);
        bb = q[x*4+0]; q[x*4+0] = b;
        BYTE g = ScaledPixelClip(q[x*4+1] * center_weight + (gg + q[x*4+5]) * outer_weight);
        gg = q[x*4+1]; q[x*4+1] = g;
        BYTE r = ScaledPixelClip(q[x*4+2] * center_weight + (rr + q[x*4+6]) * outer_weight);
        rr = q[x*4+2]; q[x*4+2] = r;
        BYTE a = ScaledPixelClip(q[x*4+3] * center_weight + (aa + q[x*4+7]) * outer_weight);
        aa = q[x*4+3]; q[x*4+3] = a;
      }
      q += pitch;
    }
  } 
  else { //rgb24
    for (int y = vi.height; y>0; --y) 
    {
      BYTE bb = q[0];
      BYTE gg = q[1];
      BYTE rr = q[2];
      for (int x = 1; x < vi.width-1; ++x) 
      {
        BYTE b = ScaledPixelClip(q[x*3+0] * center_weight + (bb + q[x*3+3]) * outer_weight);
        bb = q[x*3+0]; q[x*3+0] = b;
        BYTE g = ScaledPixelClip(q[x*3+1] * center_weight + (gg + q[x*3+4]) * outer_weight);
        gg = q[x*3+1]; q[x*3+1] = g;
        BYTE r = ScaledPixelClip(q[x*3+2] * center_weight + (rr + q[x*3+5]) * outer_weight);
        rr = q[x*3+2]; q[x*3+2] = r;
      }
      q += pitch;
    }
  }
  return frame;
}





/************************************************
 *******   Sharpen/Blur Factory Methods   *******
 ***********************************************/

AVSValue __cdecl Create_Sharpen(AVSValue args, void*, IScriptEnvironment* env) 
{
  const double amountH = args[1].AsFloat(), amountV = args[2].AsFloat(amountH);
  if (amountH < -1.5849625 || amountH > 1.0 || amountV < -1.5849625 || amountV > 1.0)
    env->ThrowError("Sharpen: arguments must be in the range -1.58 to 1.0");
  return new AdjustFocusH(amountH, new AdjustFocusV(amountV, args[0].AsClip()));
}

AVSValue __cdecl Create_Blur(AVSValue args, void*, IScriptEnvironment* env) 
{
  const double amountH = args[1].AsFloat(), amountV = args[2].AsFloat(amountH);
  if (amountH < -1.0 || amountH > 1.5849625 || amountV < -1.0 || amountV > 1.5849625)
    env->ThrowError("Blur: arguments must be in the range -1.0 to 1.58");
  return new AdjustFocusH(-amountH, new AdjustFocusV(-amountV, args[0].AsClip()));
}






/***************************
 ****  TemporalSoften  *****
 **************************/


#define SCALE(i)    (int)((double)32768.0/(i)+0.5)
const short TemporalSoften::scaletab[] = {
  0,
  32767,     // because of signed MMX multiplications
  SCALE(2),
  SCALE(3),
  SCALE(4),
  SCALE(5),
  SCALE(6),
  SCALE(7),
  SCALE(8),
  SCALE(9),
  SCALE(10),
  SCALE(11),
  SCALE(12),
  SCALE(13),
  SCALE(14),
  SCALE(15),
};
#undef SCALE



TemporalSoften::TemporalSoften( PClip _child, unsigned radius, unsigned luma_thresh, 
                                unsigned chroma_thresh, IScriptEnvironment* env )
  : GenericVideoFilter  (_child),
    chroma_threshold    (min(chroma_thresh,255)),
    luma_threshold      (min(luma_thresh,255)),
    kernel              (2*min(radius,MAX_RADIUS)+1),
    scaletab_MMX        (NULL)
{
  if (!vi.IsYUY2())
    env->ThrowError("TemporalSoften: requires YUY2 input");

  accu = (DWORD*)new DWORD[(vi.width * vi.height * kernel * vi.BytesFromPixels(1)) >> 2];
  if (!accu)
    env->ThrowError("TemporalSoften: memory allocation error");
  memset(accu, 0, vi.width * vi.height * kernel * vi.BytesFromPixels(1));

  nprev = -100;

  if (env->GetCPUFlags() & CPUF_MMX) {
    scaletab_MMX = new __int64[65536]; 
    if (!scaletab_MMX) {
      delete[] accu;
      env->ThrowError("TemporalSoften: memory allocation error");
    }
    for(int i=0; i<65536; i++)
      scaletab_MMX[i] = ( (__int64) scaletab[ i       & 15]        ) |
                        (((__int64) scaletab[(i >> 4) & 15]) << 16 ) |
                        (((__int64) scaletab[(i >> 8) & 15]) << 32 ) |
                        (((__int64) scaletab[(i >>12) & 15]) << 48 );
  }

}



TemporalSoften::~TemporalSoften(void) 
{
  delete[] accu;
  delete[] scaletab_MMX;
}



PVideoFrame TemporalSoften::GetFrame(int n, IScriptEnvironment* env) 
{
  int noffset = n % kernel;                             // offset of the frame not yet in the buffer
  int coffset = (noffset + (kernel / 2) + 1) % kernel;  // center offset, offset of the frame to be returned 
  
  if (n != nprev+1)
    FillBuffer(n, noffset, env);                        // refill buffer in case of non-linear access (slow)

  nprev = n;

  int i = min(n + (kernel/2), vi.num_frames-1);         // do not read past the end
  PVideoFrame frame = child->GetFrame(i, env);          // get the new frame
  env->MakeWritable(&frame);
  DWORD* pframe = (DWORD*)frame->GetWritePtr();
  const int rowsize = frame->GetRowSize();
  int height = frame->GetHeight();
  const int modulo = frame->GetPitch() - rowsize;

  if (env->GetCPUFlags() & CPUF_MMX)
    run_MMX(pframe, rowsize, height, modulo, noffset, coffset);
  else
    run_C(pframe, rowsize, height, modulo, noffset, coffset);

  return frame;
}



void TemporalSoften::run_C(DWORD *pframe, int rowsize, int height, int modulo, int noffset, int coffset)
{
  DWORD* pbuf = accu;
  do {
    int x = rowsize >> 2;
    do {
      const DWORD center = pbuf[coffset];
      const int v  =  center >> 24        ;
      const int y2 = (center >> 16) & 0xff;
      const int u  = (center >>  8) & 0xff;
      const int y1 =  center        & 0xff;
      unsigned int y1accum = 0, uaccum = 0, y2accum = 0, vaccum = 0;
      unsigned int county1 = 0, county2 = 0, countu = 0, countv = 0;

      pbuf[noffset] = *pframe;
    
      for(int i=0; i<kernel; ++i) {
        const DWORD c = *pbuf++;
        const int cv  =  c >> 24        ;
        const int cy2 = (c >> 16) & 0xff;
        const int cu  = (c >>  8) & 0xff;
        const int cy1 =  c        & 0xff;
      
        if (IsClose(y1, cy1, luma_threshold  )) { y1accum += cy1; county1++; }
        if (IsClose(y2, cy2, luma_threshold  )) { y2accum += cy2; county2++; }
        if (IsClose(u , cu , chroma_threshold)) { uaccum  += cu ; countu++ ; }
        if (IsClose(v , cv , chroma_threshold)) { vaccum  += cv ; countv++ ; }
      }

      y1accum = (y1accum * scaletab[county1] + 16384) >> 15;
      y2accum = (y2accum * scaletab[county2] + 16384) >> 15;
      uaccum  = (uaccum  * scaletab[countu ] + 16384) >> 15;
      vaccum  = (vaccum  * scaletab[countv ] + 16384) >> 15;

      *pframe++ = (vaccum<<24) + (y2accum<<16) + (uaccum<<8) + y1accum;
        
    } while(--x);

    pframe += modulo >> 2;

  } while(--height);
}



void TemporalSoften::run_MMX(DWORD *pframe, int rowsize, int height, int modulo, int noffset, int coffset)
{
  noffset <<= 2;
  coffset <<= 2;
  const DWORD tresholds = (chroma_threshold << 16) | luma_threshold;
  DWORD* pbuf = accu;
  const int kern = kernel;
  __int64 counters = (__int64)kern * 0x0001000100010001i64;
  const int w = rowsize >> 2;
  const __int64* scaletab = scaletab_MMX;
  __declspec(align(8)) static const __int64 indexer = 0x1000010000100001i64;

  __asm {
    mov       esi, pframe
    mov       edi, pbuf
    movd      mm5, tresholds
    punpckldq mm5, mm5
    pxor      mm0, mm0
    mov       ebx, noffset

TS_yloop:
    mov       ecx, w
TS_xloop:
    mov       edx, [esi]          ; get new pixel
    mov       eax, coffset
    mov       [edi+ebx], edx      ; put into place
    pxor      mm6, mm6            ; clear accumulators
    movd      mm1, [edi+eax]      ; get center pixel
    punpcklbw mm1, mm0            ; 0 Vc 0 Y2c 0 Uc 0 Y1c
    add       esi, 4              ; pframe++
    mov       edx, kern           ; load kernel length
    movq      mm7, counters       ; initialize counters

TS_timeloop:
    movd      mm2, [edi]          ; *pbuf: V Y2 U Y1
    movq      mm3, mm1            ; center pixel
    punpcklbw mm2, mm0            ; 0 V 0 Y2 0 U 0 Y1
    movq      mm4, mm2
    psubusw   mm3, mm2
    psubusw   mm2, mm1
    por       mm3, mm2            ; |V-Vc| |Y2-Y2c| |U-Uc| |Y1-Y1c|
    pcmpgtw   mm3, mm5
    add       edi, 4
    paddw     mm7, mm3            ; -1 to counter if above treshold
    pandn     mm3, mm4            ; else add to accumulators
    paddw     mm6, mm3
    dec       edx
    jne       TS_timeloop         ; kernel times

    psllw     mm6, 1              ; accum *= 2
    paddw     mm6, mm7            ; accum += count  (for rounding)

    ; construct 16 bits index
    ; mm7 = count3 count2 count1 count0 (words, each count<=15)
    pmaddwd   mm7, indexer
    movq      mm2, mm7
    punpckhdq mm7, mm7
    mov       eax, scaletab
    paddd     mm2, mm7
    movd      edx, mm2

    ; index = edx = count0+16*(count1+16*(count2+16*count3))

    movq      mm7, [eax+edx*8]
    pmulhw    mm6, mm7
    packuswb  mm6, mm6
    dec       ecx
    movd      [esi-4], mm6        ; store output pixel
    jne       TS_xloop

    add       esi, modulo         ; skip to next scanline

    dec       height              ; yloop (height)
    jne       TS_yloop

    emms
  };
}



PVideoFrame TemporalSoften::LoadFrame(int n, int offset, IScriptEnvironment* env)
/**
  * Get a frame from child and put it at the specified offset in the interleaved buffer
 **/
{
  int m=min(max(n,0),vi.num_frames-1);

  const PVideoFrame frame = child->GetFrame(m, env);
  const BYTE* srcp = frame->GetReadPtr();
  const int pitch = frame->GetPitch();
  const int width = frame->GetRowSize() >> 2;
  const int height = frame->GetHeight();
  
  DWORD* accum = accu + offset;

  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++, accum += kernel)
      *accum = ((DWORD*)srcp)[x];
      srcp += pitch;
  }

  return frame;
}



void TemporalSoften::FillBuffer(int n, int offset, IScriptEnvironment* env)
/**
  * (re)initialize the interleaved buffer with necessary frames
 **/ 
{
  for (int i = 1; i < kernel; i++) {
    int numframe = n - (kernel / 2) - 1 + i;
    int pos = (i + offset) % kernel;
    LoadFrame(numframe, pos, env);
  }
}



AVSValue __cdecl TemporalSoften::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new TemporalSoften( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), 
                             args[3].AsInt(), env );
}







/****************************
 ****  Spatial Soften   *****
 ***************************/

SpatialSoften::SpatialSoften( PClip _child, int _radius, unsigned _luma_threshold, 
                              unsigned _chroma_threshold, IScriptEnvironment* env )
  : GenericVideoFilter(_child), diameter(_radius*2+1),
    luma_threshold(_luma_threshold), chroma_threshold(_chroma_threshold)
{
  if (!vi.IsYUY2())
    env->ThrowError("SpatialSoften: requires YUY2 input");
}


PVideoFrame SpatialSoften::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  int row_size = src->GetRowSize();

  for (int y=0; y<vi.height; ++y) 
  {
    const BYTE* line[65];    // better not make diameter bigger than this...
    for (int h=0; h<diameter; ++h)
      line[h] = &srcp[src_pitch * min(max(y+h-(diameter>>1), 0), vi.height-1)];
    int x;

    int edge = (diameter+1) & -4;
    for (x=0; x<edge; ++x)  // diameter-1 == (diameter>>1) * 2
      dstp[y*dst_pitch + x] = srcp[y*src_pitch + x];
    for (; x < row_size - edge; x+=2) 
    {
      int cnt=0, _y=0, _u=0, _v=0;
      int xx = x | 3;
      int Y = srcp[y*src_pitch + x], U = srcp[y*src_pitch + xx - 2], V = srcp[y*src_pitch + xx];
      for (int h=0; h<diameter; ++h) 
      {
        for (int w = -diameter+1; w < diameter; w += 2) 
        {
          int xw = (x+w) | 3;
          if (IsClose(line[h][x+w], Y, luma_threshold) && IsClose(line[h][xw-2], U,
                      chroma_threshold) && IsClose(line[h][xw], V, chroma_threshold)) 
          {
            ++cnt; _y += line[h][x+w]; _u += line[h][xw-2]; _v += line[h][xw];
          }
        }
      }
      dstp[y*dst_pitch + x] = (_y + (cnt>>1)) / cnt;
      if (!(x&3)) {
        dstp[y*dst_pitch + x+1] = (_u + (cnt>>1)) / cnt;
        dstp[y*dst_pitch + x+3] = (_v + (cnt>>1)) / cnt;
      }
    }
    for (; x<row_size; ++x)
      dstp[y*dst_pitch + x] = srcp[y*src_pitch + x];
  }

  return dst;
}


AVSValue __cdecl SpatialSoften::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new SpatialSoften( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), 
                            args[3].AsInt(), env );
}