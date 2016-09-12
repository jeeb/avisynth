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

// Overlay (c) 2003, 2004 by Klaus Post

#ifndef __444Convert_h
#define __444Convert_h

#include <avisynth.h>
#include "imghelpers.h"

void Convert444FromYV12(PVideoFrame &src, PVideoFrame &dst, int pixelsize, int bits_per_pixel, IScriptEnvironment* env);
void Convert444FromYUY2(PVideoFrame &src, PVideoFrame &dst, int pixelsize, int bits_per_pixel, IScriptEnvironment* env);
void Convert444ToYV12(PVideoFrame &src, PVideoFrame &dst, int pixelsize, int bits_per_pixel, IScriptEnvironment* env);
void Convert444ToYUY2(PVideoFrame &src, PVideoFrame &dst, int pixelsize, int bits_per_pixel, IScriptEnvironment* env);

#if 0
class ConvertTo444 {
  private:
    VideoInfo* inputVi;
    int pixelsize;
    int bits_per_pixel;

  public:
    ConvertTo444() { inputVi = 0; }
    virtual void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env) {
      env->ThrowError("Overlay: Unable to convert input image.");
    }
    virtual void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env) {
      env->ThrowError("Overlay: Unable to convert input image.");
    }
    void SetVideoInfo(VideoInfo* in_vi) {
      inputVi = in_vi;
      pixelsize = inputVi->ComponentSize();
      bits_per_pixel = inputVi->BitsPerComponent();
    }
};
#endif

#if 0
class ConvertFrom444 {
  public:
    ConvertFrom444() { }
    virtual PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, int pixelsize, int bits_per_pixel, IScriptEnvironment* env) {
      env->ThrowError("Overlay: Unable to convert output image.");
      return 0;
    }
};

// avs+ we are using this
// simple Blts
void CopyToImage444(PVideoFrame src, Image444* dst, IScriptEnvironment* env);
void CopyToImage444LumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env);
// Simple BLTs for YUV444 and Y, or else invoke Avisynth's converters
PVideoFrame Convert444ToOriginal(VideoInfo *vi, Image444 *src, PVideoFrame dst, bool rgb_full_range, IScriptEnvironment *env);

#if 0
class Convert444FromYV24 : public ConvertTo444 {
public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
  void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};
#endif

#if 0
class Convert444FromY8 : public ConvertTo444 {
public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
  void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};
#endif

#if 0
class Convert444FromYV12 : public ConvertTo444 {
public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
  void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};
#endif

#if 0
class Convert444FromYUY2 : public ConvertTo444 {
public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
  void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};
#endif

#if 0
class Convert444FromRGB : public ConvertTo444 {
private:

public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
  void ConvertImageLumaOnly(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};

// Uses LumaOnly from "Convert444FromRGB"
class Convert444NonCCIRFromRGB : public Convert444FromRGB {
private:
public:
  void ConvertImage(PVideoFrame src_frame, Image444* dst_frame, IScriptEnvironment* env);
};
#endif

#if 0
class Convert444ToYV24 : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};

class Convert444ToYV12 : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};

class Convert444ToY8 : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};

class Convert444ToYUY2 : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};

class Convert444ToRGB : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};

class Convert444NonCCIRToRGB : public ConvertFrom444 {
public:
  PVideoFrame ConvertImage(Image444* src_frame, PVideoFrame dst_frame, IScriptEnvironment* env);
};
#endif
#endif

#endif //444Convert