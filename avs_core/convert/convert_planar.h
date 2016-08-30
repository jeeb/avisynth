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

#include <avisynth.h>
#include <stdint.h>

// useful functions
template <typename pixel_t>
inline void fill_chroma(BYTE* dstp_u, BYTE* dstp_v, int height, int pitch, pixel_t val);

template <typename pixel_t>
inline void fill_plane(BYTE* dstp, int height, int pitch, pixel_t val);

struct ChannelConversionMatrix {
  int16_t r;    // for 15bit scaled integer arithmetic
  int16_t g;
  int16_t b;
  float r_f;    // for float operation
  float g_f;
  float b_f;
  int offset_y; // for 8 or 16 bit
  float offset_y_f; // for float
};

class ConvertToY8 : public GenericVideoFilter
{
public:
  ConvertToY8(PClip src, int matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  
private:
  bool blit_luma_only;
  bool yuy2_input;
  bool packed_rgb_input;
  bool planar_rgb_input;
  int pixel_step;
  int pixelsize;
  ChannelConversionMatrix matrix;
};

struct ConversionMatrix {
  int16_t y_r;
  int16_t y_g;
  int16_t y_b;
  int16_t u_r;
  int16_t u_g;
  int16_t u_b;
  int16_t v_r;
  int16_t v_g;
  int16_t v_b;

  float y_r_f;
  float y_g_f;
  float y_b_f;
  float u_r_f;
  float u_g_f;
  float u_b_f;
  float v_r_f;
  float v_g_f;
  float v_b_f;

  int offset_y;
};

class ConvertRGBToYV24 : public GenericVideoFilter
{
public:
  ConvertRGBToYV24(PClip src, int matrix, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
private:
  void BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift);
  ConversionMatrix matrix;
  int pixel_step;
  bool hasAlpha;
  bool isPlanarRGBfamily;
};

class ConvertYUY2ToYV16 : public GenericVideoFilter
{
public:
  ConvertYUY2ToYV16(PClip src, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};

// note for AVS16: renamed from ConvertYV24ToRGB (Convert444ToRGB is already used in Overlay)
class ConvertYUV444ToRGB : public GenericVideoFilter
{
public:
  ConvertYUV444ToRGB(PClip src, int matrix, int pixel_step, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

//  static AVSValue __cdecl Create24(AVSValue args, void*, IScriptEnvironment* env);
//  static AVSValue __cdecl Create32(AVSValue args, void*, IScriptEnvironment* env);
private:
  void BuildMatrix(double Kr, double Kb, int Sy, int Suv, int Oy, int shift);
  ConversionMatrix matrix;
  int pixel_step;
};

class ConvertYV16ToYUY2 : public GenericVideoFilter
{
public:
  ConvertYV16ToYUY2(PClip src, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
};

class ConvertToPlanarGeneric : public GenericVideoFilter
{
public:
  ConvertToPlanarGeneric(PClip src, int dst_space, bool interlaced,
                         const AVSValue& InPlacement, const AVSValue& ChromaResampler,
                         const AVSValue& OutPlacement, IScriptEnvironment* env);
  ~ConvertToPlanarGeneric() {}
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl CreateYV411(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateYUV420(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateYUV422(AVSValue args, void*, IScriptEnvironment* env);
  static AVSValue __cdecl CreateYUV444(AVSValue args, void*, IScriptEnvironment* env);

private:
  static AVSValue Create(AVSValue& args, const char* filter, IScriptEnvironment* env);
  bool Yinput;
  int pixelsize;
  PClip Usource;
  PClip Vsource;
};


//--------------- planar bit depth conversions
// todo: separate file?
typedef void (*BitDepthConvFuncPtr)(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, float float_range);

class ConvertTo8bit : public GenericVideoFilter
{
public:
  ConvertTo8bit(PClip _child, const float _float_range, const int _dither_mode, const int _source_bitdepth, const int _truerange, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  
private:
  BitDepthConvFuncPtr conv_function;
  float float_range;
  int dither_mode;
  int pixelsize;
  int source_bitdepth;
  int truerange;
};

class ConvertTo16bit : public GenericVideoFilter
{
public:
  ConvertTo16bit(PClip _child, const float _float_range, const int _dither_mode, const int _source_bitdepth, const int _target_bitdepth, bool _truerange, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  
private:
  BitDepthConvFuncPtr conv_function;
  float float_range;
  int dither_mode;
  int pixelsize;
  int source_bitdepth; // effective 10/12/14/16 bits within the 2 byte container
  int target_bitdepth; // effective 10/12/14/16 bits within the 2 byte container
  bool truerange; // if 16->10 range reducing or e.g. 14->16 bit range expansion needed
  bool change_only_format; // if 16->10 bit affects only pixel_type
};

class ConvertToFloat : public GenericVideoFilter
{
public:
  ConvertToFloat(PClip _child, const float _float_range, const int _source_bitdepth, bool _truerange, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);  
private:
  BitDepthConvFuncPtr conv_function;
  float float_range;
  int source_bitdepth; // effective 10/12/14/16 bits within the 2 byte container
  bool truerange; // if 16->10 range reducing or e.g. 14->16 bit range expansion needed
  int pixelsize;
};



#endif
