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


#include "field.h"
#include "resample.h"




/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Field_filters[] = {
  { "ComplementParity", "c", ComplementParity::Create },
  { "AssumeTFF", "c", AssumeParity::Create, (void*)true },
  { "AssumeBFF", "c", AssumeParity::Create, (void*)false },
  { "AssumeFieldBased", "c", AssumeFieldBased::Create },
  { "AssumeFrameBased", "c", AssumeFrameBased::Create },
  { "SeparateFields", "c", SeparateFields::Create },
  { "Weave", "c", Create_Weave },
  { "DoubleWeave", "c", Create_DoubleWeave },
  { "Pulldown", "cii", Create_Pulldown },
  { "SelectEvery", "cii*", SelectEvery::Create },
  { "SelectEven", "c", SelectEvery::Create_SelectEven },
  { "SelectOdd", "c", SelectEvery::Create_SelectOdd },
  { "Interleave", "c+", Interleave::Create },
  { "SwapFields", "c", Create_SwapFields },
  { "Bob", "c[b]f[c]f[height]i", Create_Bob },
  { 0 }
};





/*********************************
 *******   SeparateFields   ******
 *********************************/

SeparateFields::SeparateFields(PClip _child, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  if (vi.height & 1)
    env->ThrowError("SeparateFields: height must be even");
  vi.height >>= 1;
  vi.fps_numerator *= 2;
  vi.num_frames *= 2;
  vi.field_based = true;
}


PVideoFrame SeparateFields::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame frame = child->GetFrame(n>>1, env);
  return frame->Subframe((GetParity(n) ^ vi.IsYUY2()) * frame->GetPitch(),
    frame->GetPitch()*2, frame->GetRowSize(), frame->GetHeight()>>1);
}


AVSValue __cdecl SeparateFields::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().field_based)
    return clip;
  else
    return new SeparateFields(clip, env);
}







/******************************
 *******   Interleave   *******
 ******************************/

Interleave::Interleave(int _num_children, const PClip* _child_array, IScriptEnvironment* env)
  : num_children(_num_children), child_array(_child_array)
{
  vi = child_array[0]->GetVideoInfo();
  vi.fps_numerator *= num_children;
  vi.num_frames = (vi.num_frames - 1) * num_children + 1;
  for (int i=1; i<num_children; ++i) 
  {
    const VideoInfo& vi2 = child_array[i]->GetVideoInfo();
    if (vi.width != vi2.width || vi.height != vi2.height || vi.pixel_type != vi2.pixel_type)
      env->ThrowError("Interleave: video attributes of all streams must be identical");
    vi.num_frames = max(vi.num_frames, (vi2.num_frames - 1) * num_children + i + 1);
  }
}

AVSValue __cdecl Interleave::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  args = args[0];
  const int num_args = args.ArraySize();
  if (num_args == 1)
    return args[0];
  PClip* child_array = new PClip[num_args];
  for (int i=0; i<num_args; ++i)
    child_array[i] = args[i].AsClip();
  return new Interleave(num_args, child_array, env);
}






/*********************************
 ********   SelectEvery    *******
 *********************************/


SelectEvery::SelectEvery(PClip _child, int _every, int _from)
 : GenericVideoFilter(_child), every(_every), from(_from)
{
  vi.fps_denominator *= every;
  vi.num_frames = (vi.num_frames-1-from) / every + 1;
}


AVSValue __cdecl SelectEvery::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  int num_vals = args[2].ArraySize();
  if (num_vals <= 1)
    return new SelectEvery(args[0].AsClip(), args[1].AsInt(), num_vals>0 ? args[2][0].AsInt() : 0);
  else {
    PClip* child_array = new PClip[num_vals];
    for (int i=0; i<num_vals; ++i)
      child_array[i] = new SelectEvery(args[0].AsClip(), args[1].AsInt(), args[2][i].AsInt());
    return new Interleave(num_vals, child_array, env);
  }
}








/**************************************
 ********   DoubleWeaveFrames   *******
 *************************************/

DoubleWeaveFields::DoubleWeaveFields(PClip _child)
  : GenericVideoFilter(_child) 
{
  vi.height *= 2;
  vi.field_based = false;
}


PVideoFrame DoubleWeaveFields::GetFrame(int n, IScriptEnvironment* env) 
{
  PVideoFrame a = child->GetFrame(n, env);
  PVideoFrame b = child->GetFrame(n+1, env);

  PVideoFrame result = env->NewVideoFrame(vi);

  bool parity = child->GetParity(n);

  CopyField(result, a, parity);
  CopyField(result, b, !parity);

  return result;
}


void DoubleWeaveFields::CopyField(const PVideoFrame& dst, const PVideoFrame& src, bool parity) 
{
  int add_pitch = dst->GetPitch() * (parity ^ vi.IsYUY2());
  BitBlt( dst->GetWritePtr() + add_pitch, dst->GetPitch()*2,
          src->GetReadPtr(), src->GetPitch(), src->GetRowSize(), src->GetHeight() );
}








/**************************************
 ********   DoubleWeaveFrames   *******
 *************************************/

DoubleWeaveFrames::DoubleWeaveFrames(PClip _child) 
  : GenericVideoFilter(_child) 
{
  vi.num_frames *= 2;
  vi.fps_numerator *= 2;
}


PVideoFrame DoubleWeaveFrames::GetFrame(int n, IScriptEnvironment* env) 
{
  if (!(n&1)) 
  {
    return child->GetFrame(n>>1, env);
  } 
  else {
    PVideoFrame a = child->GetFrame(n>>1, env);
    PVideoFrame b = child->GetFrame((n+1)>>1, env);
    bool parity = this->GetParity(n);
    env->MakeWritable(&a);
    if (a->IsWritable()) 
    {
      CopyAlternateLines(a, b, !parity);
      return a;
    } 
    else {
      PVideoFrame result = env->NewVideoFrame(vi);
      CopyAlternateLines(result, a, parity);
      CopyAlternateLines(result, b, !parity);
      return result;
    }
  }
}


void DoubleWeaveFrames::CopyAlternateLines(const PVideoFrame& dst, const PVideoFrame& src, bool parity) 
{
  int src_add_pitch = src->GetPitch() * (parity ^ vi.IsYUY2());
  int dst_add_pitch = dst->GetPitch() * (parity ^ vi.IsYUY2());
  BitBlt( dst->GetWritePtr() + dst_add_pitch, dst->GetPitch()*2,
          src->GetReadPtr() + src_add_pitch, src->GetPitch()*2,
          src->GetRowSize(), src->GetHeight()>>1 );
}







/*******************************
 ********   Bob Filter   *******
 *******************************/

Fieldwise::Fieldwise(PClip _child1, PClip _child2) 
 : GenericVideoFilter(_child1), child2(_child2) {}


PVideoFrame __stdcall Fieldwise::GetFrame(int n, IScriptEnvironment* env) 
{
  return (child->GetParity(n) ? child2 : child)->GetFrame(n, env);
}







/************************************
 ********   Factory Methods   *******
 ***********************************/

AVSValue __cdecl Create_DoubleWeave(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().field_based)
    return new DoubleWeaveFields(clip);
  else
    return new DoubleWeaveFrames(clip);
}


AVSValue __cdecl Create_Weave(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new SelectEvery(Create_DoubleWeave(args, 0, env).AsClip(), 2, 0);
}


AVSValue __cdecl Create_Pulldown(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip clip = args[0].AsClip();
  PClip* child_array = new PClip[2];
  child_array[0] = new SelectEvery(clip, 5, args[1].AsInt() % 5);
  child_array[1] = new SelectEvery(clip, 5, args[2].AsInt() % 5);
  return new AssumeFrameBased(new Interleave(2, child_array, env));
}


AVSValue __cdecl Create_SwapFields(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new SelectEvery(new DoubleWeaveFields(new ComplementParity(
    new SeparateFields(args[0].AsClip(), env))), 2, 0);
}


AVSValue __cdecl Create_Bob(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (!clip->GetVideoInfo().field_based) 
    clip = new_SeparateFields(clip, env);
  
  const VideoInfo& vi = clip->GetVideoInfo();

  const double b = args[1].AsFloat(1./3.);
  const double c = args[2].AsFloat(1./3.);
  const int new_height = args[3].AsInt(vi.height*2);
  MitchellNetravaliFilter filter(b, c);
  return new_AssumeFrameBased(new Fieldwise(new FilteredResizeV(clip, -0.25, vi.height, 
                                                new_height, &filter, env),
	                                          new FilteredResizeV(clip, +0.25, vi.height, 
                                                new_height, &filter, env)));	  
}


PClip new_SeparateFields(PClip _child, IScriptEnvironment* env) 
{
  return new SeparateFields(_child, env);
}


PClip new_AssumeFrameBased(PClip _child) 
{
  return new AssumeFrameBased(_child);
}