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

#ifndef __Field_H__
#define __Field_H__

#include "internal.h"


/********************************************************************
********************************************************************/


class ComplementParity : public GenericVideoFilter 
/**
  * Class to switch field precedence
 **/
{
public:
  ComplementParity(PClip _child) : GenericVideoFilter(_child) {}
  inline bool __stdcall GetParity(int n) 
    { return !child->GetParity(n); }

  inline static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env)
    { return new ComplementParity(args[0].AsClip()); }
};


class AssumeParity : public GenericVideoFilter
/**
  * Class to assume field precedence
 **/
{
public:
  AssumeParity(PClip _child, bool _parity) : GenericVideoFilter(_child), parity(_parity) {}
  inline bool __stdcall GetParity(int n)
    { return parity ^ (vi.field_based && (n & 1)); }

	inline static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env)
		{ return new AssumeParity(args[0].AsClip(), user_data!=0); }

private:
	bool parity;
};

class AssumeFieldBased : public GenericVideoFilter 
/**
  * Class to assume field-based video
 **/
{
public:
  AssumeFieldBased(PClip _child) : GenericVideoFilter(_child) 
    { vi.field_based = true; }
  inline bool __stdcall GetParity(int n) 
    { return n&1; }

  inline static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env)
    { return new AssumeFieldBased(args[0].AsClip()); }
};


class AssumeFrameBased : public GenericVideoFilter 
/**
  * Class to assume frame-based video
 **/
{
public:
  AssumeFrameBased(PClip _child) : GenericVideoFilter(_child) 
    { vi.field_based = false; }
  inline bool __stdcall GetParity(int n) 
    { return false; }

  inline static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env)
    { return new AssumeFrameBased(args[0].AsClip()); }
};


class SeparateFields : public GenericVideoFilter 
/**
  * Class to separate fields of interlaced video
 **/
{
public:
  SeparateFields(PClip _child, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  inline bool __stdcall GetParity(int n)
    { return child->GetParity(n>>1) ^ (n&1); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env); 
};


class DoubleWeaveFields : public GenericVideoFilter 
/**
  * Class to weave fields into an equal number of frames
 **/
{
public:
  DoubleWeaveFields(PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  // bool GetParity(int n);

private:
  void CopyField(const PVideoFrame& dst, const PVideoFrame& src, bool parity);
};


class DoubleWeaveFrames : public GenericVideoFilter 
/**
  * Class to double-weave frames
 **/
{
public:
  DoubleWeaveFrames(PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  inline bool __stdcall GetParity(int n)
    { return child->GetParity(n>>1) ^ (n&1); }

private:
  void CopyAlternateLines(const PVideoFrame& dst, const PVideoFrame& src, bool parity);
};


class Interleave : public IClip 
/**
  * Class to interleave several clips frame-by-frame
 **/
{
public:
  Interleave(int _num_children, const PClip* _child_array, IScriptEnvironment* env);
  
  inline const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    { return child_array[n % num_children]->GetFrame(n / num_children, env); }
  inline void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child_array[0]->GetAudio(buf, start, count, env);  }
  inline bool __stdcall GetParity(int n) 
    { return child_array[n % num_children]->GetParity(n / num_children); }
  virtual ~Interleave() 
    { delete[] child_array; }
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int num_children;
  const PClip* child_array;
  VideoInfo vi;
};


class SelectEvery : public GenericVideoFilter 
/**
  * Class to perform generalized pulldown (patterned frame removal)
 **/
{
public:
  SelectEvery(PClip _child, int _every, int _from);
  inline PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    { return child->GetFrame(n*every+from, env); }
  inline bool __stdcall GetParity(int n)
    { return child->GetParity(n*every+from); }
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  
  inline static AVSValue __cdecl Create_SelectEven(AVSValue args, void*, IScriptEnvironment* env)
    { return new SelectEvery(args[0].AsClip(), 2, 0); }
  inline static AVSValue __cdecl Create_SelectOdd(AVSValue args, void*, IScriptEnvironment* env)
    { return new SelectEvery(args[0].AsClip(), 2, 1); }

private:
  const int every, from;
};



class Fieldwise : public GenericVideoFilter 
/**
  * Helper class for Bob filter
 **/
{  
public:
  Fieldwise(PClip _child1, PClip _child2);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  PClip child2;
};



/**** Factory methods ****/

static AVSValue __cdecl Create_DoubleWeave(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_Weave(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_Pulldown(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_SwapFields(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_Bob(AVSValue args, void*, IScriptEnvironment* env);
static PClip new_SeparateFields(PClip _child, IScriptEnvironment* env);
static PClip new_AssumeFrameBased(PClip _child);

#endif  // __Field_H__