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

TemporalSoften::TemporalSoften( PClip _child, int _radius, unsigned _luma_threshold, 
                                unsigned _chroma_threshold, IScriptEnvironment* env )
 : GenericVideoFilter(_child), radius(_radius), luma_threshold(_luma_threshold), 
   chroma_threshold(_chroma_threshold), cur_frame(-32768)
{
  if (!vi.IsYUY2())
    env->ThrowError("TemporalSoften: requires YUY2 input");

  src = new PVideoFrame[radius*2+1] + radius;
  srcp = new const BYTE*[radius*2+1] + radius;
  pitch = new int[radius*2+1] + radius;
}


TemporalSoften::~TemporalSoften(void) 
{
  delete[] (src - radius);
  delete[] (srcp - radius);
  delete[] (pitch - radius);
}


void TemporalSoften::GoToFrame(int frame, IScriptEnvironment* env) 
{
  _RPT2(0, "GoToFrame: frame=%d  cur_frame=%d\n", frame, cur_frame);
  const int offset = frame - cur_frame;
  if (offset >= 0) {
    int i;
    for (i = -radius; i <= radius-offset; ++i)
      src[i] = src[i+offset];
    for (; i <= radius; ++i)
      src[i] = 0;
  } 
  else {
    int i;
    for (i = radius; i >= -radius-offset; --i)
      src[i] = src[i+offset];
    for (; i >= -radius; --i)
      src[i] = 0;
  }
  for (int j = -radius; j <= radius; ++j) 
  {
    if (!src[j]) 
    {
      _RPT1(0, "           reading frame %d\n", j+frame);
      src[j] = child->GetFrame(j+frame, env);
    }
  }
  cur_frame = frame;
}


PVideoFrame TemporalSoften::GetFrame(int n, IScriptEnvironment* env) 
{
  GoToFrame(n, env);
  for (int j = -radius; j <= radius; ++j) 
  {
    srcp[j] = src[j]->GetReadPtr();
    pitch[j] = src[j]->GetPitch();
  }
  PVideoFrame dst = env->NewVideoFrame(vi);
  int dst_pitch = dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  for (int y = 0; y < vi.height; ++y) 
  {
    for (int x = 0; x < vi.width*2; x += 2) 
    {
      int cnt1=0, cnt2=0, yy=0, uv=0;
      int YY=srcp[0][x], UV=srcp[0][x+1];
      for (int a = -radius; a <= radius; ++a) 
      {
        if (IsClose(srcp[a][x], YY, luma_threshold)) 
        {
          ++cnt1; yy += srcp[a][x];
        }
        if (IsClose(srcp[a][x+1], UV, chroma_threshold)) 
        {
          ++cnt2; uv += srcp[a][x+1];
        }
      }
      dstp[x+0] = (yy+(cnt1>>1)) / cnt1;
      dstp[x+1] = (uv+(cnt2>>1)) / cnt2;
    }
    for (int a = -radius; a <= radius; ++a)
      srcp[a] += pitch[a];
    dstp += dst_pitch;
  }
  return dst;
}


AVSValue __cdecl TemporalSoften::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new TemporalSoften( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(), 
                             args[3].AsInt(), env );
}







/****************************
 ****  Spacial Soften   *****
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