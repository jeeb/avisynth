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

#ifndef __Misc_H__
#define __Misc_H__


#include "internal.h"


/********************************************************************
********************************************************************/


class FixLuminance : public GenericVideoFilter 
/**
  * Class to progressively darken the top of a YUY2 clip to compensate for bad VCRs
 **/
{
public:
  FixLuminance(PClip _child, int _vertex, int _slope, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int vertex, slope;
};


class FixBrokenChromaUpsampling : public GenericVideoFilter 
/**
  * Class to correct for the incorrect chroma upsampling done by the MS DV codec
 **/
{
public:
  FixBrokenChromaUpsampling(PClip _clip, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};


class PeculiarBlend : public GenericVideoFilter 
/**
  * Class to remove nasty telecining effect (see docs)
 **/
{
public:
  PeculiarBlend(PClip _child, int _cutoff, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  

private:
  const int cutoff;
};


#endif  // __Misc_H__