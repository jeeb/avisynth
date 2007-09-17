// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

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
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.

#ifndef __Convert_YUY2_H__
#define __Convert_YUY2_H__

#include "../internal.h"
#include "convert_yv12.h"


class ConvertToYUY2 : public GenericVideoFilter 
/**
  * Class for conversions to YUY2
 **/
{
public:
  ConvertToYUY2(PClip _child, bool _interlaced, const char *matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const int src_cs;  // Source colorspace
  const bool interlaced;

  int theMatrix;
  enum {Rec601=0, Rec709=1, PC_601=2, PC_709=3 };	// Note! convert_yuy2.cpp assumes these values
};

class ConvertBackToYUY2 : public GenericVideoFilter 
/**
  * Class for conversions to YUY2 (With Chroma copy)
 **/
{
public:
  ConvertBackToYUY2(PClip _child, const char *matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  const bool rgb32;

  int theMatrix;
  enum {Rec601=0, Rec709=1, PC_601=2, PC_709=3 };	// Note! convert_yuy2.cpp assumes these values
  
};


void mmx_ConvertRGB32toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix);
void mmx_ConvertRGB24toYUY2(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix);
void mmx_ConvertRGB32toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix);
void mmx_ConvertRGB24toYUY2_Dup(unsigned int *src,unsigned int *dst,int src_pitch, int dst_pitch,int w, int h, int matrix);

#endif // __Convert_YUY2_H__
