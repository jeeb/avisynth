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


#include "histogram.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Histogram_filters[] = {
  { "Histogram", "c", Histogram::Create },   // src clip
  { 0 }
};




/***********************************
 *******   Histogram Filter   ******
 **********************************/

Histogram::Histogram(PClip _child, IScriptEnvironment* env) 
  : GenericVideoFilter(_child)
{
  if (!vi.IsYUY2())
    env->ThrowError("Histogram: YUY2 data only");
  vi.width += 256;
}


PVideoFrame __stdcall Histogram::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);
  BitBlt(p, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  for (int y=0; y<src->GetHeight(); ++y) {
    int hist[256] = {0};
    int x;
    for (x=0; x<vi.width-256; ++x) {
      hist[p[x*2]]++;
    }
    for (x=0; x<256; ++x) {
      p[x*2+vi.width*2-512] = min(255, hist[x]*3);
      p[x*2+vi.width*2-511] = 128;
    }
    p += dst->GetPitch();
  }
  return dst;
}


AVSValue __cdecl Histogram::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Histogram(env->Invoke("ConvertToYUY2", args[0]).AsClip(), env);
}
