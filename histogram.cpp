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
  if (!vi.IsYUV())
    env->ThrowError("Histogram: YUV data only");
  vi.width += 256;
}


PVideoFrame __stdcall Histogram::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* p = dst->GetWritePtr();
  PVideoFrame src = child->GetFrame(n, env);
  env->BitBlt(p, dst->GetPitch(), src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight());
  if (vi.IsPlanar()) {
    env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
    env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
    BYTE* p2 = dst->GetWritePtr(PLANAR_U);
    BYTE* p3 = dst->GetWritePtr(PLANAR_V);
    for (int y=0; y<src->GetHeight(PLANAR_Y); ++y) {
      int hist[256] = {0};
      int x;
      for (x=0; x<vi.width-256; ++x) {
        hist[p[x]]++;
      }
      if (y&1) {
        for (x=0; x<128; ++x) {
          p[x*2+vi.width-256] = min(255, hist[x*2]*4);
          p[x*2+vi.width-256+1] = min(255, hist[x*2+1]*4);
          if (x<8) {
            p2[x+(vi.width>>1)-128]  = 0;
            p3[x+(vi.width>>1)-128]  = 160; 
          } else if (x>118) {
            p2[x+(vi.width>>1)-128]  = 0;
            p3[x+(vi.width>>1)-128]  = 160;
          } else if (x==62) {
            p2[x+(vi.width>>1)-128]  = 160;
            p3[x+(vi.width>>1)-128]  = 0;
          } else {
            p2[x+(vi.width>>1)-128]  = 128;
            p3[x+(vi.width>>1)-128]  = 128;
          }
        }
        p2+= dst->GetPitch(PLANAR_U);
        p3+= dst->GetPitch(PLANAR_U);
      } else {
        for (x=0; x<256; ++x) {
          p[x+vi.width-256] = min(255, hist[x]*4);
        }
      }
      p += dst->GetPitch();
    }
    return dst;
  }
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
  return new Histogram(args[0].AsClip(), env);
}
