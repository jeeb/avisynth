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

#ifndef __Transform_H__
#define __Transform_H__

#include "internal.h"


/********************************************************************
********************************************************************/


class FlipVertical : public GenericVideoFilter 
/**
  * Class to vertically flip a video
 **/
{
public:
  FlipVertical(PClip _child) : GenericVideoFilter(_child) {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};


class Crop : public GenericVideoFilter 
/**
  * Class to crop a video
 **/
{  
public:
  Crop(int _left, int _top, int _width, int _height, PClip _child, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  /*const*/ int left_bytes, top;
};



class AddBorders : public GenericVideoFilter 
/**
  * Class to add borders to a video
 **/
{  
public:
  AddBorders(int _left, int _top, int _right, int _bot, PClip _child);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  /*const*/ int left, top, right, bot;
  BYTE* mybuffer;
};








/**** Factory methods ****/

static AVSValue __cdecl Create_Letterbox(AVSValue args, void*, IScriptEnvironment* env);
static AVSValue __cdecl Create_CropBottom(AVSValue args, void*, IScriptEnvironment* env);



#endif  // __Transform_H__