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

#ifndef __Levels_H__
#define __Levels_H__

#include <avisynth.h>
#include <stdint.h>


/********************************************************************
********************************************************************/



class Levels : public GenericVideoFilter 
/**
  * Class for adjusting levels in a clip
 **/
{
public:
  Levels( PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, bool coring, bool _dither,
          IScriptEnvironment* env );
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  BYTE *map, *mapchroma;
  bool dither;
  // avs+
  int pixelsize;
  int bits_per_pixel; // 8,10..16
  bool use_lut;
};



class RGBAdjust : public GenericVideoFilter 
/**
  * Class for adjusting and analyzing colors in RGBA space
 **/
{
public:
  RGBAdjust(PClip _child, double r,  double g,  double b,  double a,
                          double rb, double gb, double bb, double ab,
                          double rg, double gg, double bg, double ag,
                          bool _analyze, bool _dither, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

private:
  bool analyze;
  bool dither;
  BYTE *mapR, *mapG, *mapB, *mapA;
};




class Tweak : public GenericVideoFilter
{
public:
  Tweak(PClip _child, double _hue, double _sat, double _bright, double _cont, bool _coring, bool _sse,
    double _startHue, double _endHue, double _maxSat, double _minSat, double _interp,
    bool _dither, bool _realcalc, IScriptEnvironment* env);

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
    int Sin, Cos;
    int Sat, Bright, Cont;
    bool coring, sse, dither;
    
    bool realcalc; // no lookup, realtime calculation, always for 16/32 bits
    double dhue, dsat, dbright, dcont, dstartHue, dendHue, dmaxSat, dminSat, dinterp;

    BYTE *map;
    uint16_t *mapUV;
};


class MaskHS : public GenericVideoFilter
{
public:
  MaskHS( PClip _child, double _startHue, double _endHue, double _maxSat, double _minSat, bool _coring, IScriptEnvironment* env );

  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void* user_data, IScriptEnvironment* env);

private:
    BYTE mapY[256*256];
};

#endif  // __Levels_H__

