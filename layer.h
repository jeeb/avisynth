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

#ifndef __Layer_H__
#define __Layer_H__

#include "internal.h"


/********************************************************************
********************************************************************/

class Mask : public IClip 
/**
  * Class for overlaying a mask clip on a video clip
 **/
{ 
public:
  Mask(PClip _child1, PClip _child2, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline virtual void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env)
    { child1->GetAudio(buf, start, count, env); }
  inline virtual const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline virtual bool __stdcall GetParity(int n) 
    { return child1->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child1, child2;
  VideoInfo vi;
  int mask_frames;
  bool YUYmask;

};



class ColorKeyMask : public GenericVideoFilter
/**
  * Class for setting a mask on a video clip based on a color key
**/
{
public:
  ColorKeyMask(PClip _child, int _color, int _tolerance, IScriptEnvironment *env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  int color, tol;
};



class Layer: public IClip 
/**
  * Class for layering two clips on each other, combined by various functions
 **/ 
{ 
public:
  Layer( PClip _child1, PClip _child2, const char _op[], int _lev, int _x, int _y, 
         int _t, bool _chroma, IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline virtual void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child1->GetAudio(buf, start, count, env); }
  inline virtual const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline virtual bool __stdcall GetParity(int n) 
    { return child1->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child1, child2;
  const int levelA, levelB,T;
  const bool chroma;
  int map1[256], map2[256], ydest, xdest, ysrc, xsrc, ofsX, ofsY, ycount, xcount, overlay_frames;
  const  char* Op;
  VideoInfo vi;

};



class Subtract : public IClip 
/**
  * Class for subtracting one clip from another
 **/
{
public:
  Subtract(PClip _child1, PClip _child2, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  inline virtual void __stdcall GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
    { child1->GetAudio(buf, start, count, env);  }
  inline virtual const VideoInfo& __stdcall GetVideoInfo() 
    { return vi; }
  inline virtual bool __stdcall GetParity(int n) 
    { return child1->GetParity(n); }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const PClip child1, child2;
  VideoInfo vi;

};



#endif  // __Layer_H__