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

#ifndef __Resample_H__
#define __Resample_H__

#include "internal.h"
#include "resample_functions.h"
#include "transform.h"



/********************************************************************
********************************************************************/

class FilteredResizeH : public GenericVideoFilter 
/**
  * Class to resize in the horizontal direction using a specified sampling filter
  * Helper for resample functions
 **/
{
public:
  FilteredResizeH( PClip _child, double subrange_left, double subrange_width, int target_width, 
                   ResamplingFunction* func, IScriptEnvironment* env );
  virtual ~FilteredResizeH(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

private:
  int* /*const*/ pattern_luma;
  int* /*const*/ pattern_chroma;
  int original_width;
  BYTE *tempY, *tempUV;
};

 

class FilteredResizeV : public GenericVideoFilter 
/**
  * Class to resize in the vertical direction using a specified sampling filter
  * Helper for resample functions
 **/
{
public:
  FilteredResizeV( PClip _child, double subrange_top, double subrange_height, int target_height,
                   ResamplingFunction* func, IScriptEnvironment* env );
  virtual ~FilteredResizeV(void);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);


private:
  int* /*const*/ resampling_pattern;
  int *yOfs;
};


/*** Resample factory methods ***/

static PClip CreateResizeH( PClip clip, double subrange_left, double subrange_width, int target_width, 
                            ResamplingFunction* func, IScriptEnvironment* env );

static PClip CreateResizeV( PClip clip, double subrange_top, double subrange_height, int target_height, 
                            ResamplingFunction* func, IScriptEnvironment* env );

static PClip CreateResize( PClip clip, int target_width, int target_height, const AVSValue* args, 
                           ResamplingFunction* f, IScriptEnvironment* env );

static AVSValue __cdecl Create_BilinearResize(AVSValue args, void*, IScriptEnvironment* env);

static AVSValue __cdecl Create_BicubicResize(AVSValue args, void*, IScriptEnvironment* env);


#endif // __Resample_H__