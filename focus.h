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

#ifndef __Focus_H__
#define __Focus_H__

#include "internal.h"


_PixelClip PixelClip;



class AdjustFocusV : public GenericVideoFilter 
/**
  * Class to adjust focus in the vertical direction, helper for sharpen/blue
 **/
{
public:
  AdjustFocusV(double _amount, PClip _child);
  virtual ~AdjustFocusV(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  const int amount;
  BYTE* line;

};


class AdjustFocusH : public GenericVideoFilter 
/**
  * Class to adjust focus in the horizontal direction, helper for sharpen/blue
 **/
{
public:
  AdjustFocusH(double _amount, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  const int amount;

};


/*** Sharpen/Blur Factory methods ***/

static AVSValue __cdecl Create_Sharpen(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_Blur(AVSValue args, void*, IScriptEnvironment* env);




/*** Soften classes ***/

class TemporalSoften : public GenericVideoFilter 
/**
  * Class to soften the focus on the temporal axis
 **/
{
public:
  TemporalSoften( PClip _child, unsigned radius, unsigned luma_thresh, unsigned chroma_thresh,
                  IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  virtual ~TemporalSoften(void);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const unsigned luma_threshold, chroma_threshold;
  DWORD* accu;
  const int kernel;
  int nprev;

  static const short scaletab[];
  __int64* scaletab_MMX;

  enum { MAX_RADIUS=7 };

  PVideoFrame LoadFrame(int n,int offset, IScriptEnvironment* env);
  void FillBuffer(int n, int offset, IScriptEnvironment* env);

  typedef void run_func(DWORD *pframe, int rowsize, int height, int modulo, int noffset, int coffset);
  run_func run_C;
  run_func run_MMX;
};


class SpatialSoften : public GenericVideoFilter 
/**
  * Class to soften the focus on the spatial axis
 **/
{
public:
  SpatialSoften(PClip _child, int _radius, unsigned _luma_threshold, unsigned _chroma_threshold, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:  
  const unsigned luma_threshold, chroma_threshold;
  const int diameter;

};



#endif  // __Focus_H__