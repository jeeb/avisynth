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

class Image444 {
private:
  BYTE* Y_plane;
  BYTE* U_plane;
  BYTE* V_plane;
public:

  const int w;
  const int h;
  int pitch;
  Image444() : w(0), h(0) {}

  Image444(int _w, int _h) : w(_w), h(_h) {
    pitch = (w+15)&(~15);
    Y_plane = (BYTE*)_aligned_malloc(pitch*h, 64); 
    U_plane = (BYTE*)_aligned_malloc(pitch*h, 64); 
    V_plane = (BYTE*)_aligned_malloc(pitch*h, 64); 
  }

  Image444(BYTE* Y, BYTE* U, BYTE* V, int _w, int _h, int _pitch) : w(_w), h(_h) {
    if (!(w && h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    Y_plane = Y;
    U_plane = U;
    V_plane = V;
    pitch = _pitch;
  }

  void free() {
    if (!(w && h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    _aligned_free(Y_plane);
    _aligned_free(U_plane);
    _aligned_free(V_plane);
  }

  BYTE* GetPtr(int plane) {
    if (!(w && h)) {
      _RPT0(1,"Image444: Height or Width is 0");
    }
    switch (plane) {
      case PLANAR_Y:
        return Y_plane;
      case PLANAR_U:
        return U_plane;
      case PLANAR_V:
        return V_plane;
    }
    return Y_plane;
  }
};


#endif
