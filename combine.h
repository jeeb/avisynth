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

#ifndef __Combine_H__
#define __Combine_H__

#include "internal.h"


/********************************************************************
********************************************************************/


class StackVertical : public IClip 
/**
  * Class to stack clips vertically
 **/
{
public:
  StackVertical(PClip _child1, PClip _child2, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  
  inline void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child1->GetAudio(buf, start, count, env); }
  inline const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline bool __stdcall GetParity(int n) 
    { return child1->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  /*const*/ PClip child1, child2;
  VideoInfo vi;

};



class StackHorizontal : public IClip 
/**
  * Class to stack clips vertically
 **/
{  
public:
  StackHorizontal(PClip _child1, PClip _child2, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child1->GetAudio(buf, start, count, env); }
  inline const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline bool __stdcall GetParity(int n) 
    { return child1->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child1, child2;
  VideoInfo vi;

};




class ShowFiveVersions : public IClip 
/**
  * Class to show every pulldown combination
 **/
{  
public:
  ShowFiveVersions(PClip* children, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child[0]->GetAudio(buf, start, count, env); }
  inline const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline bool __stdcall GetParity(int n) 
    { return child[0]->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  PClip child[5];
  VideoInfo vi;

};



class Animate : public IClip 
/**
  * Class to allow recursive animation of multiple clips (see docs)  *
 **/
{
  enum { cache_size = 3 };
  PClip cache[cache_size];
  int cache_stage[cache_size];
  const int first, last;
  AVSValue *args_before, *args_after, *args_now;
  int num_args;
  const char* name;
public:
  Animate( PClip context, int _first, int _last, const char* _name, const AVSValue* _args_before, 
           const AVSValue* _args_after, int _num_args, IScriptEnvironment* env );
  virtual ~Animate() 
    { delete[] args_before; }
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { cache[0]->GetAudio(buf, start, count, env); } // this could be implemented better...
  inline const VideoInfo& __stdcall GetVideoInfo() 
    { return cache[0]->GetVideoInfo(); }
  inline bool __stdcall GetParity(int n) 
    { return cache[0]->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};


#endif  // __Combine_H__