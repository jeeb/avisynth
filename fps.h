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

#ifndef __FPS_H__
#define __FPS_H__

#include "internal.h"


/********************************************************************
********************************************************************/

class AssumeFPS : public GenericVideoFilter 
/**
  * Class to change the framerate without changing the frame count
 **/
{
public:
  AssumeFPS(PClip _child, int numerator, int denominator, bool sync_audio);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateFloat(AVSValue args, void*, IScriptEnvironment* env);
};


class ChangeFPS : public GenericVideoFilter 
/**
  * Class to change the framerate, deleting or adding frames as necessary
 **/
{
public:
  ChangeFPS(PClip _child, int new_numerator, int new_denominator);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateFloat(AVSValue args, void*, IScriptEnvironment* env);

private:
  __int64 a, b;
};



class ConvertFPS : public GenericVideoFilter 
/**
  * Class to change the framerate, attempting to smooth the transitions
 **/
{
public:
  ConvertFPS( PClip _child, int new_numerator, int new_denominator, int _zone, 
              int _vbi, IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  bool __stdcall GetParity(int n);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateFloat(AVSValue args, void*, IScriptEnvironment* env);

private:
  __int64 fa, fb;
  int zone;
//Variables used in switch mode only
  int vbi;    //Vertical Blanking Interval (lines)
  int lps;    //Lines per source frame
};


#endif  // __FPS_H_