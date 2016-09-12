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

#ifndef __Overlay_helpers_h
#define __Overlay_helpers_h

#include <avisynth.h>
#include <avs/minmax.h>

#define USE_ORIG_FRAME

class Image444 {
private:
  IScriptEnvironment2 * Env;

  PVideoFrame &frame;

  BYTE* Y_plane;
  BYTE* U_plane;
  BYTE* V_plane;
  BYTE* A_plane;

  BYTE* fake_Y_plane;
  BYTE* fake_U_plane;
  BYTE* fake_V_plane;
  BYTE* fake_A_plane;

  int fake_w;
  int fake_h;
  const int _w;
  const int _h;
  const int _bits_per_pixel;
  const bool hasAlpha;

  bool return_original;


public:
  int pitch;
  int pitchUV;
  int pitchA;

  //Image444(IScriptEnvironment* env) : Env(static_cast<IScriptEnvironment2*>(env)), _w(0), _h(0), _bits_per_pixel(8), hasAlpha(false) {}

  /*Image444(Image444* img, IScriptEnvironment* env) : Env(static_cast<IScriptEnvironment2*>(env)),
    _w(img->w()), _h(img->h()), pitch(img->pitch), pitchUV(img->pitchUV), pitchA(img->pitchA), _bits_per_pixel(img->_bits_per_pixel), hasAlpha(img->hasAlpha) {
    Y_plane = img->GetPtr(PLANAR_Y);
    U_plane = img->GetPtr(PLANAR_U);
    V_plane = img->GetPtr(PLANAR_V);
    A_plane = img->GetPtr(PLANAR_A);
    ResetFake();
  }
  */

  Image444(
#ifdef USE_ORIG_FRAME
    PVideoFrame &_frame,
#endif
    int _inw, int _inh, int _in_bits_per_pixel, bool _hasAlpha, IScriptEnvironment* env) :
    Env(static_cast<IScriptEnvironment2*>(env)),
#ifdef USE_ORIG_FRAME
    frame(_frame),
#endif
    _w(_inw), _h(_inh), _bits_per_pixel(_in_bits_per_pixel), hasAlpha(_hasAlpha) {

    int pixelsize;
    if (_bits_per_pixel == 8) pixelsize = 1;
    else if (_bits_per_pixel <= 16) pixelsize = 2;
    else pixelsize = 4;


#ifdef USE_ORIG_FRAME
    pitch = frame->GetPitch(PLANAR_Y);
    pitchUV = frame->GetPitch(PLANAR_U);
    pitchA = frame->GetPitch(PLANAR_A);

    Y_plane = (BYTE*) frame->GetReadPtr(PLANAR_Y);
    U_plane = (BYTE*) frame->GetReadPtr(PLANAR_U);
    V_plane = (BYTE*) frame->GetReadPtr(PLANAR_V);
    A_plane = (BYTE*) frame->GetReadPtr(PLANAR_A);
#else
    const int INTERNAL_ALIGN = 16;
    pitch = (_w * pixelsize +(INTERNAL_ALIGN-1))&(~(INTERNAL_ALIGN-1));
    pitchA = hasAlpha ? pitch : 0;

    Y_plane = (BYTE*) Env->Allocate(pitch*_h, 64, AVS_POOLED_ALLOC);
    U_plane = (BYTE*) Env->Allocate(pitch*_h, 64, AVS_POOLED_ALLOC);
    V_plane = (BYTE*) Env->Allocate(pitch*_h, 64, AVS_POOLED_ALLOC);
    A_plane = hasAlpha ? (BYTE*) Env->Allocate(pitch*_h, 64, AVS_POOLED_ALLOC) : nullptr;
    if (!Y_plane || !U_plane || !V_plane || (hasAlpha && !A_plane)) {
	  	Env->Free(Y_plane);
	  	Env->Free(U_plane);
	  	Env->Free(V_plane);
      Env->Free(A_plane);
      Env->ThrowError("Image444: Could not reserve memory.");
	  }
#endif

    ResetFake();
  }

  /*
  Image444(BYTE* Y, BYTE* U, BYTE* V, BYTE *A, int _inw, int _inh, int _pitch, int _pitchUV, int _pitchA, int _in_bits_per_pixel, bool _hasAlpha, IScriptEnvironment* env) :
    Env(static_cast<IScriptEnvironment2*>(env)), _w(_inw), _h(_inh), _bits_per_pixel(_in_bits_per_pixel), hasAlpha(_hasAlpha) {
    if (!(_w && _h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    Y_plane = Y;
    U_plane = U;
    V_plane = V;
    A_plane = A;
    pitch = _pitch;
    pitchUV = _pitchUV;
    pitchA = _pitchA;
    ResetFake();
  }
  */

  void free_chroma() {
#ifndef USE_ORIG_FRAME
    Env->Free(U_plane);
    Env->Free(V_plane);
#endif
  }

  void free_luma() {
#ifndef USE_ORIG_FRAME
    Env->Free(Y_plane);
    Env->Free(A_plane);
#endif
  }

  void free_all() {
#ifndef USE_ORIG_FRAME
    if (!(_w && _h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    free_luma();
    free_chroma();
#endif
  }

  __inline int w() { return (return_original) ? _w : fake_w; }
  __inline int h() { return (return_original) ? _h : fake_h; }

  BYTE* GetPtr(int plane) {
    if (!(_w && _h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    switch (plane) {
      case PLANAR_Y:
        return (return_original) ? Y_plane : fake_Y_plane;
      case PLANAR_U:
        return (return_original) ? U_plane : fake_U_plane;
      case PLANAR_V:
        return (return_original) ? V_plane : fake_V_plane;
      case PLANAR_A:
        return (return_original) ? A_plane : fake_A_plane;
    }
    return Y_plane;
  }

  void SetPtr(BYTE* ptr, int plane) {
    if (!(_w && _h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    switch (plane) {
      case PLANAR_Y:
        fake_Y_plane = Y_plane = ptr;
        break;
      case PLANAR_U:
        fake_Y_plane = U_plane = ptr;
        break;
      case PLANAR_V:
        fake_Y_plane = V_plane = ptr;
        break;
      case PLANAR_A:
        fake_A_plane = A_plane = ptr;
        break;
    }
  }

  int GetPitch(int plane) {
    if (!(_w && _h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    switch (plane) {
    case PLANAR_Y:
      return pitch;
    case PLANAR_U:
    case PLANAR_V:
      return pitchUV;
    case PLANAR_A:
      return pitchA;
    }
    return pitch;
  }

  void SubFrame(int x, int y, int new_w, int new_h) {
    new_w = min(new_w, w()-x);
    new_h = min(new_h, h()-y);

    int pixelsize;
    switch(_bits_per_pixel) {
    case 8: pixelsize = 1; break;
    case 32: pixelsize = 4; break;
    default: pixelsize = 2;
    }

    fake_Y_plane = GetPtr(PLANAR_Y) + x*pixelsize + (y*pitch);
    fake_U_plane = GetPtr(PLANAR_U) + x*pixelsize + (y*pitchUV);
    fake_V_plane = GetPtr(PLANAR_V) + x*pixelsize + (y*pitchUV);
    fake_A_plane = pitchA > 0 ? GetPtr(PLANAR_A) + x*pixelsize + (y*pitchA) : nullptr;

    fake_w = new_w;
    fake_h = new_h;
  }

  bool IsSizeZero() {
    if (w()<=0) return true;
    if (h()<=0) return true;
    if (!(pitch && Y_plane && V_plane && U_plane)) return true;
    return false;
  }

  void ReturnOriginal(bool shouldI) {
    return_original = shouldI;
  }

  void ResetFake() {
    return_original = true;
    fake_Y_plane = Y_plane;
    fake_U_plane = U_plane;
    fake_V_plane = V_plane;
    fake_A_plane = A_plane;
    fake_w = _w;
    fake_h = _h;
  }

};


#endif
