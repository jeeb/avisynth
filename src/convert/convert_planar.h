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

// ConvertPlanar (c) 2005 by Klaus Post

#ifndef __Convert_PLANAR_H__
#define __Convert_PLANAR_H__

#include "../internal.h"
#include "convert_matrix.h"
#include "convert_rgbtoy8.h"
#include "../filters/resample.h"

enum {Rec601=0, Rec709=1, PC_601=2, PC_709=3, AVERAGE=4 };

int getMatrix( const char* matrix, IScriptEnvironment* env);

enum   {PLACEMENT_MPEG2, PLACEMENT_MPEG1, PLACEMENT_DV } ;

static int getPlacement( const AVSValue& _placement, IScriptEnvironment* env);

static ResamplingFunction* getResampler( const char* resampler, IScriptEnvironment* env);


class ConvertToY8 : public GenericVideoFilter, public RGBtoY8Generator
{
public:
  ConvertToY8(PClip src, int matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  
  ~ConvertToY8();
private:
  void convYUV422toY8(const unsigned char *src, unsigned char *py, int pitch1, int pitch2y, int width, int height);
  bool blit_luma_only;
  bool yuy2_input;
  bool rgb_input;
  int pixel_step;
  signed short* matrix;
  int offset_y;
};


class ConvertRGBToYV24 : public GenericVideoFilter, public MatrixGenerator3x3
{
public:
  ConvertRGBToYV24(PClip src, int matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
  ~ConvertRGBToYV24();
private:
  void BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift);
  signed short* matrix;
  BYTE* unpckbuf;
  int offset_y;
  int mul_out;
  int pixel_step;
};

class ConvertYUY2ToYV16 : public GenericVideoFilter
{
public:
  ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
private:
  void convYUV422to422(const unsigned char *src, unsigned char *py, unsigned char *pu, unsigned char *pv,
       int pitch1, int pitch2y, int pitch2uv, int width, int height);

};

class ConvertYV24ToRGB : public GenericVideoFilter, public MatrixGenerator3x3
{
public:
  ConvertYV24ToRGB(PClip src, int matrix, int pixel_step, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create24(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl Create32(AVSValue args, void*, IScriptEnvironment* env);
  ~ConvertYV24ToRGB();
private:
  void BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift);
  signed short* matrix;
  BYTE* packbuf;
  int offset_y;
  int pixel_step;
};

class ConvertYV16ToYUY2 : public GenericVideoFilter
{
public:
  ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
private:
  void conv422toYUV422(const unsigned char *py, const unsigned char *pu,
					   const unsigned char *pv, unsigned char *dst,
					   int pitch1Y, int pitch1UV, int pitch2, int width, int height);
};

class ConvertToPlanarGeneric : public GenericVideoFilter
{
public:
  ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                         const AVSValue& InPlacement, const AVSValue& ChromaResampler,
                         const AVSValue& OutPlacement, IScriptEnvironment* env);
  ~ConvertToPlanarGeneric() {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);
  static AVSValue __cdecl CreateYV12(AVSValue args, void*, IScriptEnvironment* env);   
  static AVSValue __cdecl CreateYV16(AVSValue args, void*, IScriptEnvironment* env);   
  static AVSValue __cdecl CreateYV24(AVSValue args, void*, IScriptEnvironment* env);   
  static AVSValue __cdecl CreateYV411(AVSValue args, void*, IScriptEnvironment* env);   
private:
  bool Y8input;
  PClip Usource;
  PClip Vsource;
};


#endif
