// Avisynth v2.5.  Copyright 2009 Ben Rudiak-Gould et al.
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

#ifndef __Convert_Audio_H__
#define __Convert_Audio_H__


class ConvertAudio : public GenericVideoFilter
/**
  * Helper class to convert audio to any format
 **/
{
public:
  ConvertAudio(PClip _clip, int prefered_format);
  void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env);
  int __stdcall SetCacheHints(int cachehints,int frame_range);  // We do pass cache requests upwards, to the cache!

  static PClip Create(PClip clip, int sample_type, int prefered_type);
  static AVSValue __cdecl Create_float(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_32bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_24bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_16bit(AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_8bit (AVSValue args, void*, IScriptEnvironment*);
  static AVSValue __cdecl Create_Any  (AVSValue args, void*, IScriptEnvironment*);
  virtual ~ConvertAudio();

private:
  void convertToFloat(char* inbuf, float* outbuf, int sample_type, int count);
  void convertToFloat_3DN(char* inbuf, float* outbuf, int sample_type, int count);
  void convertToFloat_SSE(char* inbuf, float* outbuf, int sample_type, int count);
  void convertToFloat_SSE2(char* inbuf, float* outbuf, int sample_type, int count);
  void convertFromFloat(float* inbuf, void* outbuf, int sample_type, int count);
  void convertFromFloat_3DN(float* inbuf, void* outbuf, int sample_type, int count);
  void convertFromFloat_SSE(float* inbuf, void* outbuf, int sample_type, int count);
  void convertFromFloat_SSE2(float* inbuf, void* outbuf, int sample_type, int count);

  __inline int Saturate_int8(float n);
  __inline short Saturate_int16(float n);
  __inline int Saturate_int24(float n);
  __inline int Saturate_int32(float n);

  int src_format;
  int dst_format;
  int src_bps;
  int tempbuffer_size;
  char *tempbuffer;
  int floatbuffer_size;
  SFLOAT *floatbuffer;
};

#endif //__Convert_Audio_H__
