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


VerticalReduceBy2::VerticalReduceBy2(PClip _child)
 : GenericVideoFilter(_child)
{
  original_height = vi.height;
  vi.height >>= 1;
}


PVideoFrame VerticalReduceBy2::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  const int src_pitch = src->GetPitch();
  const int dst_pitch = dst->GetPitch();
  const int row_size = src->GetRowSize();

  BYTE* dstp = dst->GetWritePtr();

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

  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();

  if (vi.IsYUY2()) {
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





/**************************************
 *****  ReduceBy2 Factory Method  *****
 *************************************/
 

AVSValue __cdecl Create_ReduceBy2(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new VerticalReduceBy2(new HorizontalReduceBy2(args[0].AsClip(), env));
}



