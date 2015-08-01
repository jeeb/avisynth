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


#include "stdafx.h"

#include "levels.h"
#include "limiter.h"


#define PI        3.141592653589793


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Levels_filters[] = {
  { "Levels", "cifiii[coring]b[dither]b", Levels::Create },        // src_low, gamma, src_high, dst_low, dst_high
  { "RGBAdjust", "c[r]f[g]f[b]f[a]f[rb]f[gb]f[bb]f[ab]f[rg]f[gg]f[bg]f[ag]f[analyze]b[dither]b", RGBAdjust::Create },
  { "Tweak", "c[hue]f[sat]f[bright]f[cont]f[coring]b[sse]b[startHue]f[endHue]f[maxSat]f[minSat]f[interp]f[dither]b", Tweak::Create },
  { "MaskHS", "c[startHue]f[endHue]f[maxSat]f[minSat]f[coring]b", MaskHS::Create },
  { "Limiter", "c[min_luma]i[max_luma]i[min_chroma]i[max_chroma]i[show]s", Limiter::Create },
  { 0 }
};



__declspec(align(64)) static const BYTE ditherMap[256] = {
#if 0
  // default 0231 recursed table
  0x00, 0x80, 0x20, 0xA0,  0x08, 0x88, 0x28, 0xA8,  0x02, 0x82, 0x22, 0xA2,  0x0A, 0x8A, 0x2A, 0xAA,
  0xC0, 0x40, 0xE0, 0x60,  0xC8, 0x48, 0xE8, 0x68,  0xC2, 0x42, 0xE2, 0x62,  0xCA, 0x4A, 0xEA, 0x6A,
  0x30, 0xB0, 0x10, 0x90,  0x38, 0xB8, 0x18, 0x98,  0x32, 0xB2, 0x12, 0x92,  0x3A, 0xBA, 0x1A, 0x9A,
  0xF0, 0x70, 0xD0, 0x50,  0xF8, 0x78, 0xD8, 0x58,  0xF2, 0x72, 0xD2, 0x52,  0xFA, 0x7A, 0xDA, 0x5A,

  0x0C, 0x8C, 0x2C, 0xAC,  0x04, 0x84, 0x24, 0xA4,  0x0E, 0x8E, 0x2E, 0xAE,  0x06, 0x86, 0x26, 0xA6,
  0xCC, 0x4C, 0xEC, 0x6C,  0xC4, 0x44, 0xE4, 0x64,  0xCE, 0x4E, 0xEE, 0x6E,  0xC6, 0x46, 0xE6, 0x66,
  0x3C, 0xBC, 0x1C, 0x9C,  0x34, 0xB4, 0x14, 0x94,  0x3E, 0xBE, 0x1E, 0x9E,  0x36, 0xB6, 0x16, 0x96,
  0xFC, 0x7C, 0xDC, 0x5C,  0xF4, 0x74, 0xD4, 0x54,  0xFE, 0x7E, 0xDE, 0x5E,  0xF6, 0x76, 0xD6, 0x56,

  0x03, 0x83, 0x23, 0xA3,  0x0B, 0x8B, 0x2B, 0xAB,  0x01, 0x81, 0x21, 0xA1,  0x09, 0x89, 0x29, 0xA9,
  0xC3, 0x43, 0xE3, 0x63,  0xCB, 0x4B, 0xEB, 0x6B,  0xC1, 0x41, 0xE1, 0x61,  0xC9, 0x49, 0xE9, 0x69,
  0x33, 0xB3, 0x13, 0x93,  0x3B, 0xBB, 0x1B, 0x9B,  0x31, 0xB1, 0x11, 0x91,  0x39, 0xB9, 0x19, 0x99,
  0xF3, 0x73, 0xD3, 0x53,  0xFB, 0x7B, 0xDB, 0x5B,  0xF1, 0x71, 0xD1, 0x51,  0xF9, 0x79, 0xD9, 0x59,

  0x0F, 0x8F, 0x2F, 0xAF,  0x07, 0x87, 0x27, 0xA7,  0x0D, 0x8D, 0x2D, 0xAD,  0x05, 0x85, 0x25, 0xA5,
  0xCF, 0x4F, 0xEF, 0x6F,  0xC7, 0x47, 0xE7, 0x67,  0xCD, 0x4D, 0xED, 0x6D,  0xC5, 0x45, 0xE5, 0x65,
  0x3F, 0xBF, 0x1F, 0x9F,  0x37, 0xB7, 0x17, 0x97,  0x3D, 0xBD, 0x1D, 0x9D,  0x35, 0xB5, 0x15, 0x95,
  0xFF, 0x7F, 0xDF, 0x5F,  0xF7, 0x77, 0xD7, 0x57,  0xFD, 0x7D, 0xDD, 0x5D,  0xF5, 0x75, 0xD5, 0x55,
#else
  // improved "equal sum" modified table
  0x00, 0xB0, 0x60, 0xD0,  0x0B, 0xBB, 0x6B, 0xDB,  0x06, 0xB6, 0x66, 0xD6,  0x0D, 0xBD, 0x6D, 0xDD,
  0xC0, 0x70, 0x90, 0x20,  0xCB, 0x7B, 0x9B, 0x2B,  0xC6, 0x76, 0x96, 0x26,  0xCD, 0x7D, 0x9D, 0x2D,
  0x30, 0x80, 0x50, 0xE0,  0x3B, 0x8B, 0x5B, 0xEB,  0x36, 0x86, 0x56, 0xE6,  0x3D, 0x8D, 0x5D, 0xED,
  0xF0, 0x40, 0xA0, 0x10,  0xFB, 0x4B, 0xAB, 0x1B,  0xF6, 0x46, 0xA6, 0x16,  0xFD, 0x4D, 0xAD, 0x1D,

  0x0C, 0xBC, 0x6C, 0xDC,  0x07, 0xB7, 0x67, 0xD7,  0x09, 0xB9, 0x69, 0xD9,  0x02, 0xB2, 0x62, 0xD2,
  0xCC, 0x7C, 0x9C, 0x2C,  0xC7, 0x77, 0x97, 0x27,  0xC9, 0x79, 0x99, 0x29,  0xC2, 0x72, 0x92, 0x22,
  0x3C, 0x8C, 0x5C, 0xEC,  0x37, 0x87, 0x57, 0xE7,  0x39, 0x89, 0x59, 0xE9,  0x32, 0x82, 0x52, 0xE2,
  0xFC, 0x4C, 0xAC, 0x1C,  0xF7, 0x47, 0xA7, 0x17,  0xF9, 0x49, 0xA9, 0x19,  0xF2, 0x42, 0xA2, 0x12,

  0x03, 0xB3, 0x63, 0xD3,  0x08, 0xB8, 0x68, 0xD8,  0x05, 0xB5, 0x65, 0xD5,  0x0E, 0xBE, 0x6E, 0xDE,
  0xC3, 0x73, 0x93, 0x23,  0xC8, 0x78, 0x98, 0x28,  0xC5, 0x75, 0x95, 0x25,  0xCE, 0x7E, 0x9E, 0x2E,
  0x33, 0x83, 0x53, 0xE3,  0x38, 0x88, 0x58, 0xE8,  0x35, 0x85, 0x55, 0xE5,  0x3E, 0x8E, 0x5E, 0xEE,
  0xF3, 0x43, 0xA3, 0x13,  0xF8, 0x48, 0xA8, 0x18,  0xF5, 0x45, 0xA5, 0x15,  0xFE, 0x4E, 0xAE, 0x1E,

  0x0F, 0xBF, 0x6F, 0xDF,  0x04, 0xB4, 0x64, 0xD4,  0x0A, 0xBA, 0x6A, 0xDA,  0x01, 0xB1, 0x61, 0xD1,
  0xCF, 0x7F, 0x9F, 0x2F,  0xC4, 0x74, 0x94, 0x24,  0xCA, 0x7A, 0x9A, 0x2A,  0xC1, 0x71, 0x91, 0x21,
  0x3F, 0x8F, 0x5F, 0xEF,  0x34, 0x84, 0x54, 0xE4,  0x3A, 0x8A, 0x5A, 0xEA,  0x31, 0x81, 0x51, 0xE1,
  0xFF, 0x4F, 0xAF, 0x1F,  0xF4, 0x44, 0xA4, 0x14,  0xFA, 0x4A, 0xAA, 0x1A,  0xF1, 0x41, 0xA1, 0x11,
#endif
};


__declspec(align(16)) static const BYTE ditherMap4[16] = {
  0x0, 0xB, 0x6, 0xD,
  0xC, 0x7, 0x9, 0x2,
  0x3, 0x8, 0x5, 0xE,
  0xF, 0x4, 0xA, 0x1,
};


/********************************
 *******   Levels Filter   ******
 ********************************/

Levels::Levels( PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, bool coring, bool _dither,
                IScriptEnvironment* env )
  : GenericVideoFilter(_child), map(0), mapchroma(0), dither(_dither)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  if (gamma <= 0.0)
    env->ThrowError("Levels: gamma must be positive");

  gamma = 1/gamma;

  int divisor = in_max - in_min + (in_max == in_min);
  int scale = 1;
  double bias  = 0.0;

  if (dither) {
    scale = 256;
    divisor *= 256;
    in_min  *= 256;
    bias = -127.5;
  }

  map = new BYTE[256*scale];

  if (vi.IsYUV()) {
    mapchroma = new BYTE[256*scale];

    for (int i=0; i<256*scale; ++i) {
      double p;

      if (coring)
        p = ((bias + i - 16*scale)*(255.0/219.0) - in_min) / divisor;
      else
        p = (bias + i - in_min) / divisor;

      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;

      if (coring)
        map[i] = min(max(BYTE(p*(219.0/255.0)+16.5),  16), 235);
      else
        map[i] = min(max(BYTE(p+0.5), 0), 255);

      BYTE q = BYTE(((bias + i - 128*scale) * (out_max-out_min)) / divisor + 128.5);

      if (coring)
        mapchroma[i] = min(max(q, 16), 240);
      else
        mapchroma[i] = min(max(q, 0), 255);
    }
  }
  else if (vi.IsRGB()) {
    for (int i=0; i<256*scale; ++i) {
      double p = (bias + i - in_min) / divisor;
      p = pow(min(max(p, 0.0), 1.0), gamma);
      p = p * (out_max - out_min) + out_min;
//    map[i] = PixelClip(int(p+0.5));
      map[i] = min(max(BYTE(p+0.5), 0), 255);
    }
  }

	}
	catch (...) { throw; }
}


Levels::~Levels() {
    if (map)       delete[] map;
    if (mapchroma) delete[] mapchroma;
}


PVideoFrame __stdcall Levels::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* p = frame->GetWritePtr();
  const int pitch = frame->GetPitch();
  if (dither) {
    if (vi.IsYUY2()) {
      const int UVwidth = vi.width/2;
      for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x<vi.width; ++x) {
          p[x*2]   = map[ p[x*2]<<8 | ditherMap[(x&0x0f)|_y] ];
        }
        for (int z=0; z<UVwidth; ++z) {
          const int _dither = ditherMap[(z&0x0f)|_y];
          p[z*4+1] = mapchroma[ p[z*4+1]<<8 | _dither ];
          p[z*4+3] = mapchroma[ p[z*4+3]<<8 | _dither ];
        }
        p += pitch;
      }
    }
    else if (vi.IsPlanar()) {
      {for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x<vi.width; ++x) {
          p[x] = map[ p[x]<<8 | ditherMap[(x&0x0f)|_y] ];
        }
        p += pitch;
      }}
      const int UVpitch = frame->GetPitch(PLANAR_U);
      const int w=frame->GetRowSize(PLANAR_U);
      const int h=frame->GetHeight(PLANAR_U);
      p = frame->GetWritePtr(PLANAR_U);
      BYTE* q = frame->GetWritePtr(PLANAR_V);
      {for (int y=0; y<h; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x<w; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x] = mapchroma[ p[x]<<8 | _dither ];
          q[x] = mapchroma[ q[x]<<8 | _dither ];
        }
        p += UVpitch;
        q += UVpitch;
      }}
    } else if (vi.IsRGB32()) {
      for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*4+0] = map[ p[x*4+0]<<8 | _dither ];
          p[x*4+1] = map[ p[x*4+1]<<8 | _dither ];
          p[x*4+2] = map[ p[x*4+2]<<8 | _dither ];
          p[x*4+3] = map[ p[x*4+3]<<8 | _dither ];
        }
        p += pitch;
      }
    } else if (vi.IsRGB24()) {
      for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*3+0] = map[ p[x*3+0]<<8 | _dither ];
          p[x*3+1] = map[ p[x*3+1]<<8 | _dither ];
          p[x*3+2] = map[ p[x*3+2]<<8 | _dither ];
        }
        p += pitch;
      }
    }
  }
  else {
    if (vi.IsYUY2()) {
      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<vi.width; ++x) {
          p[x*2+0] = map      [p[x*2+0]];
          p[x*2+1] = mapchroma[p[x*2+1]];
        }
        p += pitch;
      }
    }
    else if (vi.IsPlanar()) {
      {for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<vi.width; ++x) {
          p[x] = map[p[x]];
        }
        p += pitch;
      }}
      const int UVpitch = frame->GetPitch(PLANAR_U);
      p = frame->GetWritePtr(PLANAR_U);
      const int w=frame->GetRowSize(PLANAR_U);
      const int h=frame->GetHeight(PLANAR_U);
      {for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
          p[x] = mapchroma[p[x]];
        }
        p += UVpitch;
      }}
      p = frame->GetWritePtr(PLANAR_V);
      {for (int y=0; y<h; ++y) {
        for (int x=0; x<w; ++x) {
          p[x] = mapchroma[p[x]];
        }
        p += UVpitch;
      }}
    } else if (vi.IsRGB()) {
      const int row_size = frame->GetRowSize();
      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<row_size; ++x) {
          p[x] = map[p[x]];
        }
        p += pitch;
      }
    }
  }
  return frame;
}

AVSValue __cdecl Levels::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new Levels( args[0].AsClip(), args[1].AsInt(), args[2].AsFloat(), args[3].AsInt(),
                     args[4].AsInt(), args[5].AsInt(), args[6].AsBool(true), args[7].AsBool(false), env );
}








/********************************
 *******    RGBA Filter    ******
 ********************************/

RGBAdjust::RGBAdjust(PClip _child, double r,  double g,  double b,  double a,
                                   double rb, double gb, double bb, double ab,
                                   double rg, double gg, double bg, double ag,
                                   bool _analyze, bool _dither, IScriptEnvironment* env)
  : GenericVideoFilter(_child), analyze(_analyze), dither(_dither), mapR(0), mapG(0), mapB(0), mapA(0)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  if (!vi.IsRGB())
    env->ThrowError("RGBAdjust requires RGB input");

  if ((rg <= 0.0) || (gg <= 0.0) || (bg <= 0.0) || (ag <= 0.0))
    env->ThrowError("RGBAdjust: gammas must be positive");

  rg=1/rg; gg=1/gg; bg=1/bg; ag=1/ag;

  if (dither) {
    mapR = new BYTE[256*256];
    mapG = new BYTE[256*256];
    mapB = new BYTE[256*256];
    mapA = new BYTE[256*256];

    for (int i=0; i<256*256; ++i) {
      mapR[i] = BYTE(pow(min(max((rb*256 + i * r -127.5)/(255.0*256), 0.0), 1.0), rg) * 255.0 + 0.5);
      mapG[i] = BYTE(pow(min(max((gb*256 + i * g -127.5)/(255.0*256), 0.0), 1.0), gg) * 255.0 + 0.5);
      mapB[i] = BYTE(pow(min(max((bb*256 + i * b -127.5)/(255.0*256), 0.0), 1.0), bg) * 255.0 + 0.5);
      mapA[i] = BYTE(pow(min(max((ab*256 + i * a -127.5)/(255.0*256), 0.0), 1.0), ag) * 255.0 + 0.5);
    }
  }
  else {
    mapR = new BYTE[256];
    mapG = new BYTE[256];
    mapB = new BYTE[256];
    mapA = new BYTE[256];

    for (int i=0; i<256; ++i) {
      mapR[i] = BYTE(pow(min(max((rb + i * r)/255.0, 0.0), 1.0), rg) * 255.0 + 0.5);
      mapG[i] = BYTE(pow(min(max((gb + i * g)/255.0, 0.0), 1.0), gg) * 255.0 + 0.5);
      mapB[i] = BYTE(pow(min(max((bb + i * b)/255.0, 0.0), 1.0), bg) * 255.0 + 0.5);
      mapA[i] = BYTE(pow(min(max((ab + i * a)/255.0, 0.0), 1.0), ag) * 255.0 + 0.5);
    }
  }

  if (vi.IsRGB24()) {
    delete[] mapA;
    mapA = 0;
  }

	}
	catch (...) { throw; }
}


RGBAdjust::~RGBAdjust() {
    if (mapR) delete[] mapR;
    if (mapG) delete[] mapG;
    if (mapB) delete[] mapB;
    if (mapA) delete[] mapA;
}


PVideoFrame __stdcall RGBAdjust::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  BYTE* p = frame->GetWritePtr();
  const int pitch = frame->GetPitch();

  if (dither) {
    if (vi.IsRGB32()) {
      for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x < vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*4+0] = mapB[ p[x*4+0]<<8 | _dither ];
          p[x*4+1] = mapG[ p[x*4+1]<<8 | _dither ];
          p[x*4+2] = mapR[ p[x*4+2]<<8 | _dither ];
          p[x*4+3] = mapA[ p[x*4+3]<<8 | _dither ];
        }
        p += pitch;
      }
    }
    else if (vi.IsRGB24()) {
      for (int y=0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x=0; x < vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*3+0] = mapB[ p[x*3+0]<<8 | _dither ];
          p[x*3+1] = mapG[ p[x*3+1]<<8 | _dither ];
          p[x*3+2] = mapR[ p[x*3+2]<<8 | _dither ];
        }
        p += pitch;
      }
    }
  }
  else {
    if (vi.IsRGB32()) {
      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x < vi.width; ++x) {
          p[x*4]   = mapB[p[x*4]];
          p[x*4+1] = mapG[p[x*4+1]];
          p[x*4+2] = mapR[p[x*4+2]];
          p[x*4+3] = mapA[p[x*4+3]];
        }
        p += pitch;
      }
    }
    else if (vi.IsRGB24()) {
      const int row_size = frame->GetRowSize();
      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<row_size; x+=3) {
           p[x]   = mapB[p[x]];
           p[x+1] = mapG[p[x+1]];
           p[x+2] = mapR[p[x+2]];
        }
        p += pitch;
      }
    }
  }

	if (analyze) {
		const int w = frame->GetRowSize();
		const int h = frame->GetHeight();
		const int t= vi.IsRGB24() ? 3 : 4;
		unsigned int accum_r[256], accum_g[256], accum_b[256];
		
		p = frame->GetWritePtr();

		{for (int i=0;i<256;i++) {
			accum_r[i]=0;
			accum_g[i]=0;
			accum_b[i]=0;
		}}

		for (int y=0;y<h;y++) {
			for (int x=0;x<w;x+=t) {
				accum_r[p[x+2]]++;
				accum_g[p[x+1]]++;
				accum_b[p[x]]++;
			}
			p += pitch;
		}
	
		int pixels = vi.width*vi.height;
		float avg_r=0, avg_g=0, avg_b=0;
		float st_r=0, st_g=0, st_b=0;
		int min_r=0, min_g=0, min_b=0;
		int max_r=0, max_g=0, max_b=0;
		bool hit_r=false, hit_g=false, hit_b=false;
		int Amin_r=0, Amin_g=0, Amin_b=0;
		int Amax_r=0, Amax_g=0, Amax_b=0;
		bool Ahit_minr=false,Ahit_ming=false,Ahit_minb=false;
		bool Ahit_maxr=false,Ahit_maxg=false,Ahit_maxb=false;
		int At_256=(pixels+128)/256; // When 1/256th of all pixels have been reached, trigger "Loose min/max"

		
		{for (int i=0;i<256;i++) {
			avg_r+=(float)accum_r[i]*(float)i;
			avg_g+=(float)accum_g[i]*(float)i;
			avg_b+=(float)accum_b[i]*(float)i;

			if (accum_r[i]!=0) {max_r=i;hit_r=true;} else {if (!hit_r) min_r=i+1;}
			if (accum_g[i]!=0) {max_g=i;hit_g=true;} else {if (!hit_g) min_g=i+1;}
			if (accum_b[i]!=0) {max_b=i;hit_b=true;} else {if (!hit_b) min_b=i+1;}

			if (!Ahit_minr) {Amin_r+=accum_r[i]; if (Amin_r>At_256){Ahit_minr=true; Amin_r=i;} }
			if (!Ahit_ming) {Amin_g+=accum_g[i]; if (Amin_g>At_256){Ahit_ming=true; Amin_g=i;} }
			if (!Ahit_minb) {Amin_b+=accum_b[i]; if (Amin_b>At_256){Ahit_minb=true; Amin_b=i;} }

			if (!Ahit_maxr) {Amax_r+=accum_r[255-i]; if (Amax_r>At_256){Ahit_maxr=true; Amax_r=255-i;} }
			if (!Ahit_maxg) {Amax_g+=accum_g[255-i]; if (Amax_g>At_256){Ahit_maxg=true; Amax_g=255-i;} }
			if (!Ahit_maxb) {Amax_b+=accum_b[255-i]; if (Amax_b>At_256){Ahit_maxb=true; Amax_b=255-i;} }
		}}
		
		float Favg_r=avg_r/pixels;
		float Favg_g=avg_g/pixels;
		float Favg_b=avg_b/pixels;

		{for (int i=0;i<256;i++) {
			st_r+=(float)accum_r[i]*(float (i-Favg_r)*(i-Favg_r));
			st_g+=(float)accum_g[i]*(float (i-Favg_g)*(i-Favg_g));
			st_b+=(float)accum_b[i]*(float (i-Favg_b)*(i-Favg_b));
		}}
		
		float Fst_r=sqrt(st_r/pixels);
		float Fst_g=sqrt(st_g/pixels);
		float Fst_b=sqrt(st_b/pixels);

		char text[512];
		sprintf(text,
			"             Frame: %-8u (  Red  / Green / Blue  )\n"
			"           Average:      ( %7.2f / %7.2f / %7.2f )\n"
			"Standard Deviation:      ( %7.2f / %7.2f / %7.2f )\n"
			"           Minimum:      ( %3d    / %3d    / %3d    )\n"
			"           Maximum:      ( %3d    / %3d    / %3d    )\n"
			"     Loose Minimum:      ( %3d    / %3d    / %3d    )\n"
			"     Loose Maximum:      ( %3d    / %3d    / %3d    )\n"
			,
			n,
			Favg_r,Favg_g,Favg_b,
			Fst_r,Fst_g,Fst_b,
			min_r,min_g,min_b,
			max_r,max_g,max_b,
			Amin_r,Amin_g,Amin_b,
			Amax_r,Amax_g,Amax_b
			);
		env->ApplyMessage(&frame, vi, text, vi.width/4, 0xa0a0a0, 0, 0);
	}
  return frame;
}


AVSValue __cdecl RGBAdjust::Create(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  return new RGBAdjust(args[ 0].AsClip(),
                       args[ 1].AsDblDef(1.0), args[ 2].AsDblDef(1.0), args[ 3].AsDblDef(1.0), args[ 4].AsDblDef(1.0),
                       args[ 5].AsDblDef(0.0), args[ 6].AsDblDef(0.0), args[ 7].AsDblDef(0.0), args[ 8].AsDblDef(0.0),
                       args[ 9].AsDblDef(1.0), args[10].AsDblDef(1.0), args[11].AsDblDef(1.0), args[12].AsDblDef(1.0),
                       args[13].AsBool(false), args[14].AsBool(false), env );
	}
	catch (...) { throw; }
}



/* helper function for Tweak and MaskHS filters */
bool ProcessPixel(double X, double Y, double startHue, double endHue,
                  double maxSat, double minSat, double p, int &iSat)
{
	// a hue analog
	double T = atan2(X, Y) * 180.0 / PI;
	if ( T < 0.0) T += 360.0;

	// startHue <= hue <= endHue
	if (startHue < endHue) {
		if (T>endHue || T<startHue) return false;
	} else {
		if (T<startHue && T>endHue) return false;
	}

	const double W = X*X + Y*Y;

	// In Range, full adjust but no need to interpolate
	if (minSat*minSat <= W && W <= maxSat*maxSat) return true;

	// p == 0 (no interpolation) needed for MaskHS
	if (p == 0.0) return false;

	// Interpolation range is +/-p for p>0
	const double max = min(maxSat+p, 180.0);
	const double min = max(minSat-p,   0.0);

	// Outside of [min-p, max+p] no adjustment
	// minSat-p <= (U^2 + V^2) <= maxSat+p
	if (W <= min*min || max*max <= W) return false; // don't adjust

	// Interpolate saturation value
	const double holdSat = W < 180.0*180.0 ? sqrt(W) : 180.0;

	if (holdSat < minSat) { // within p of lower range
		iSat += (int)((512 - iSat) * (minSat - holdSat) / p);
	} else { // within p of upper range
		iSat += (int)((512 - iSat) * (holdSat - maxSat) / p);
	}
	
	return true;
}


/**********************
******   Tweak    *****
**********************/

Tweak::Tweak( PClip _child, double _hue, double _sat, double _bright, double _cont, bool _coring, bool _sse,
                            double startHue, double endHue, double _maxSat, double _minSat, double p,
                            bool _dither, IScriptEnvironment* env )
  : GenericVideoFilter(_child), coring(_coring), sse(_sse), dither(_dither), map(0), mapUV(0)
{
  try {  // HIDE DAMN SEH COMPILER BUG!!!
    if (vi.IsRGB())
          env->ThrowError("Tweak: YUV data only (no RGB)");

    // Flag to skip special processing if doing all pixels
    // If defaults, don't check for ranges, just do all
    const bool allPixels = (startHue == 0.0 && endHue == 360.0 && _maxSat == 150.0 && _minSat == 0.0);

// The new "mapping" C code is faster than the iSSE code on my 3GHz P4HT - Make it optional
    if (sse && (!allPixels || coring || dither || !vi.IsYUY2()))
        env->ThrowError("Tweak: SSE option only available for YUY2 with coring=false and no options.");
    if (sse && !(env->GetCPUFlags() & CPUF_INTEGER_SSE))
        env->ThrowError("Tweak: SSE option needs an iSSE capable processor");

    if (vi.IsY8()) {
        if (!(_hue == 0.0 && _sat == 1.0 && allPixels))
        env->ThrowError("Tweak: bright and cont are the only options available for Y8.");
    }

    if (startHue < 0.0 || startHue >= 360.0)
          env->ThrowError("Tweak: startHue must be greater than or equal to 0.0 and less than 360.0");

    if (endHue <= 0.0 || endHue > 360.0)
          env->ThrowError("Tweak: endHue must be greater than 0.0 and less than or equal to 360.0");

    if (_minSat >= _maxSat)
          env->ThrowError("Tweak: MinSat must be less than MaxSat");

    if (_minSat < 0.0 || _minSat >= 150.0)
          env->ThrowError("Tweak: minSat must be greater than or equal to 0 and less than 150.");

    if (_maxSat <= 0.0 || _maxSat > 150.0)
          env->ThrowError("Tweak: maxSat must be greater than 0 and less than or equal to 150.");

    if (p>=150.0 || p<0.0)
          env->ThrowError("Tweak: Interp must be greater than or equal to 0 and less than 150.");

    Sat = (int) (_sat * 512);
    Cont = (int) (_cont * 512);
    Bright = (int) _bright;

    const double Hue = (_hue * PI) / 180.0;
    const double SIN = sin(Hue);
    const double COS = cos(Hue);

    Sin = (int) (SIN * 4096 + 0.5);
    Cos = (int) (COS * 4096 + 0.5);

    if (dither) {
      map = new BYTE[256*256];

      if (coring) {
        for (int i = 0; i < 256*256; i++) {
          /* brightness and contrast */
          BYTE y = BYTE(((i - 16*256)*_cont + _bright*256 - 127.5)/256 + 16.5);
          map[i] = min(max(y, 16), 235);
        }
      }
      else {
        for (int i = 0; i < 256*256; i++) {
          /* brightness and contrast */
          BYTE y = BYTE((i*_cont + _bright*256 - 127.5)/256 + 0.5);
          map[i] = min(max(y, 0), 255);
        }
      }
    }
    else {
      map = new BYTE[256];

      if (coring) {
        for (int i = 0; i < 256; i++) {
          /* brightness and contrast */
          BYTE y = BYTE((i - 16)*_cont + _bright + 16.5);
          map[i] = min(max(y, 16), 235);
        }
      }
      else {
        for (int i = 0; i < 256; i++) {
          /* brightness and contrast */
          BYTE y = BYTE(i*_cont + _bright + 0.5);
          map[i] = min(max(y, 0), 255);
        }
      }
    }

    // 100% equals sat=119 (= maximal saturation of valid RGB (R=255,G=B=0)
    // 150% (=180) - 100% (=119) overshoot
    const double minSat = 1.19 * _minSat;
    const double maxSat = 1.19 * _maxSat;

    p *= 1.19; // Same units as minSat/maxSat

    const int maxUV = coring ? 240 : 255;
    const int minUV = coring ? 16 : 0;

    if (dither) {
      mapUV = new unsigned short[256*256*16];

      for (int d = 0; d < 16; d++) {
        for (int u = 0; u < 256; u++) {
          const double destu = ((u<<4|d) - 7.5)/16.0-128.0;
          for (int v = 0; v < 256; v++) {
            const double destv = ((v<<4|d) - 7.5)/16.0-128.0;
            int iSat = Sat;
            if (allPixels || ProcessPixel(destv, destu, startHue, endHue, maxSat, minSat, p, iSat)) {
              int du = int ( (destu*COS + destv*SIN) * iSat + 0x100) >> 9;
              int dv = int ( (destv*COS - destu*SIN) * iSat + 0x100) >> 9;
              du = min(max(du+128,minUV),maxUV);
              dv = min(max(dv+128,minUV),maxUV);
              mapUV[(u<<12)|(v<<4)|d]  = (unsigned short)(du | (dv<<8));
            } else {
              mapUV[(u<<12)|(v<<4)|d]  = (unsigned short)(min(max(u,minUV),maxUV) | ((min(max(v,minUV),maxUV))<<8));
            }
          }
        }
      }
    }
    else {
      mapUV = new unsigned short[256*256];

      for (int u = 0; u < 256; u++) {
        const double destu = u-128;
        for (int v = 0; v < 256; v++) {
          const double destv = v-128;
          int iSat = Sat;
          if (allPixels || ProcessPixel(destv, destu, startHue, endHue, maxSat, minSat, p, iSat)) {
            int du = int ( (destu*COS + destv*SIN) * iSat ) >> 9;
            int dv = int ( (destv*COS - destu*SIN) * iSat ) >> 9;
            du = min(max(du+128,minUV),maxUV);
            dv = min(max(dv+128,minUV),maxUV);
            mapUV[(u<<8)|v]  = (unsigned short)(du | (dv<<8));
          } else {
            mapUV[(u<<8)|v]  = (unsigned short)(min(max(u,minUV),maxUV) | ((min(max(v,minUV),maxUV))<<8));
          }
        }
      }
    }

  }
  catch (...) { throw; }
}


Tweak::~Tweak() {
  if (map) delete[] map;
  if (mapUV) delete[] mapUV;
}


PVideoFrame __stdcall Tweak::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	env->MakeWritable(&src);

	BYTE* srcp = src->GetWritePtr();

	int src_pitch = src->GetPitch();
	int height = src->GetHeight();
	int row_size = src->GetRowSize();

	if (vi.IsYUY2()) {
		if (sse && !coring && !dither && (env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
			const __int64 hue64 = (in64 Cos<<48) + (in64 (-Sin)<<32) + (in64 Sin<<16) + in64 Cos;
			const __int64 satcont64 = (in64 Sat<<48) + (in64 Cont<<32) + (in64 Sat<<16) + in64 Cont;
			const __int64 bright64 = (in64 Bright<<32) + in64 Bright;

			asm_tweak_ISSE_YUY2(srcp, row_size>>2, height, src_pitch-row_size, hue64, satcont64, bright64);
			return src;
		}

		if (dither) {
			const int UVwidth = vi.width/2;
			for (int y = 0; y < height; y++) {
				{const int _y = (y << 4) & 0xf0;
				for (int x = 0; x < vi.width; ++x) {
					/* brightness and contrast */
					srcp[x*2] = map[ srcp[x*2]<<8 | ditherMap[(x&0x0f)|_y] ];
				}}
				{const int _y = (y << 2) & 0xC;
				for (int x = 0; x < UVwidth; ++x) {
					const int _dither = ditherMap4[(x&0x3)|_y];
					/* hue and saturation */
					const int u = srcp[x*4+1];
					const int v = srcp[x*4+3];
					const int mapped = mapUV[(u<<12) | (v<<4) | _dither];
					srcp[x*4+1] = (BYTE)(mapped&0xff);
					srcp[x*4+3] = (BYTE)(mapped>>8);
				}}
				srcp += src_pitch;
			}
		}
		else {
			for (int y = 0; y < height; y++)
			{
				for (int x = 0; x < row_size; x+=4)
				{
					/* brightness and contrast */
					srcp[x] = map[srcp[x]];
					srcp[x+2] = map[srcp[x+2]];

					/* hue and saturation */
					const int u = srcp[x+1];
					const int v = srcp[x+3];
					const int mapped = mapUV[(u<<8) | v];
					srcp[x+1] = (BYTE)(mapped&0xff);
					srcp[x+3] = (BYTE)(mapped>>8);
				}
				srcp += src_pitch;
			}
		}
	} else if (vi.IsPlanar()) {
		if (dither) {
			for (int y=0; y<height; ++y) {
				const int _y = (y << 4) & 0xf0;
				for (int x=0; x<row_size; ++x) {
					/* brightness and contrast */
					srcp[x] = map[ srcp[x]<<8 | ditherMap[(x&0x0f)|_y] ];
				}
				srcp += src_pitch;
			}
		}
		else {
			for (int y=0; y<height; ++y) {
				for (int x=0; x<row_size; ++x) {
					/* brightness and contrast */
					srcp[x] = map[srcp[x]];
				}
				srcp += src_pitch;
			}
		}

		src_pitch = src->GetPitch(PLANAR_U);
		BYTE * srcpu = src->GetWritePtr(PLANAR_U);
		BYTE * srcpv = src->GetWritePtr(PLANAR_V);
		row_size = src->GetRowSize(PLANAR_U);
		height = src->GetHeight(PLANAR_U);

		if (dither) {
			for (int y=0; y<height; ++y) {
				const int _y = (y << 2) & 0xC;
				for (int x=0; x<row_size; ++x) {
					const int _dither = ditherMap4[(x&0x3)|_y];
					/* hue and saturation */
					const int u = srcpu[x];
					const int v = srcpv[x];
					const int mapped = mapUV[(u<<12) | (v<<4) | _dither];
					srcpu[x] = (BYTE)(mapped&0xff);
					srcpv[x] = (BYTE)(mapped>>8);
				}
				srcpu += src_pitch;
				srcpv += src_pitch;
			}
		}
		else {
			for (int y=0; y<height; ++y) {
				for (int x=0; x<row_size; ++x) {
					/* hue and saturation */
					const int u = srcpu[x];
					const int v = srcpv[x];
					const int mapped = mapUV[(u<<8) | v];
					srcpu[x] = (BYTE)(mapped&0xff);
					srcpv[x] = (BYTE)(mapped>>8);
				}
				srcpu += src_pitch;
				srcpv += src_pitch;
			}
		}
	}

	return src;
}

AVSValue __cdecl Tweak::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
    return new Tweak(args[0].AsClip(),
					 args[1].AsDblDef(0.0),		// hue
					 args[2].AsDblDef(1.0),		// sat
					 args[3].AsDblDef(0.0),		// bright
					 args[4].AsDblDef(1.0),		// cont
					 args[5].AsBool(true),      // coring
					 args[6].AsBool(false),     // sse
					 args[7].AsDblDef(0.0),     // startHue
					 args[8].AsDblDef(360.0),   // endHue
					 args[9].AsDblDef(150.0),   // maxSat
					 args[10].AsDblDef(0.0),    // minSat
					 args[11].AsDblDef(16.0/1.19),// interp
					 args[12].AsBool(false),    // dither
					 env);
	}
	catch (...) { throw; }
}



// Integer SSE optimization by "Dividee".
void __declspec(naked) asm_tweak_ISSE_YUY2( BYTE *srcp, int w, int h, int modulo, __int64 hue,
                                       __int64 satcont, __int64 bright )
{
	static const __int64 norm = 0x0080001000800010i64;

	__asm {
		push		ebp
		push		edi
		push		esi
		push		ebx

		pxor		mm0, mm0
		movq		mm1, norm				// 128 16 128 16
		movq		mm2, [esp+16+20]		// Cos -Sin Sin Cos (fix12)
		movq		mm3, [esp+16+28]		// Sat Cont Sat Cont (fix9)
		movq		mm4, mm1
		paddw		mm4, [esp+16+36]		// 128 16+Bright 128 16+Bright

		mov			esi, [esp+16+4]			// srcp
		mov			edx, [esp+16+12]		// height
y_loop:
		mov			ecx, [esp+16+8]			// width
x_loop:
		movd		mm7, [esi]   			// 0000VYUY
		punpcklbw	mm7, mm0
		psubw		mm7, mm1				//  V Y U Y
		pshufw		mm6, mm7, 0xDD			//  V U V U
		pmaddwd		mm6, mm2				// V*Cos-U*Sin V*Sin+U*Cos (fix12)
		psrad		mm6, 12					// ? V' ? U'
		movq		mm5, mm7
		punpcklwd	mm7, mm6				// ? ? U' Y
		punpckhwd	mm5, mm6				// ? ? V' Y
		punpckldq	mm7, mm5				// V' Y U' Y
		psllw		mm7, 7					// (fix7)
		pmulhw		mm7, mm3	            // V'*Sat Y*Cont U'*Sat Y*Cont
		paddw		mm7, mm4				// V" Y" U" Y"
		packuswb	mm7, mm0				// 0000V"Y"U"Y"
		movd		[esi], mm7

		add			esi, 4
		dec			ecx
		jnz			x_loop

		add			esi, [esp+16+16]		// skip to next scanline
		dec			edx
		jnz			y_loop

		pop			ebx
		pop			esi
		pop			edi
		pop			ebp
		emms
		ret
	};
}


/**********************
******   MaskHS   *****
**********************/

MaskHS::MaskHS( PClip _child, double startHue, double endHue, double _maxSat, double _minSat, bool coring,
                IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  try {  // HIDE DAMN SEH COMPILER BUG!!!
    if (vi.IsRGB())
          env->ThrowError("MaskHS: YUV data only (no RGB)");

    if (vi.IsY8()) {
        env->ThrowError("MaskHS: clip must contain chroma.");
    }

    if (startHue < 0.0 || startHue >= 360.0)
          env->ThrowError("MaskHS: startHue must be greater than or equal to 0.0 and less than 360.0");

    if (endHue <= 0.0 || endHue > 360.0)
          env->ThrowError("MaskHS: endHue must be greater than 0.0 and less than or equal to 360.0");

    if (_minSat >= _maxSat)
          env->ThrowError("MaskHS: MinSat must be less than MaxSat");

    if (_minSat < 0.0 || _minSat >= 150.0)
          env->ThrowError("MaskHS: minSat must be greater than or equal to 0 and less than 150.");

    if (_maxSat <= 0.0 || _maxSat > 150.0)
          env->ThrowError("MaskHS: maxSat must be greater than 0 and less than or equal to 150.");


    const BYTE maxY = coring ? 235 : 255;
    const BYTE minY = coring ? 16 : 0;

    // 100% equals sat=119 (= maximal saturation of valid RGB (R=255,G=B=0)
    // 150% (=180) - 100% (=119) overshoot
    const double minSat = 1.19 * _minSat;
    const double maxSat = 1.19 * _maxSat;

    // apply mask
    for (int u = 0; u < 256; u++) {
      const double destu = u-128;
      for (int v = 0; v < 256; v++) {
        const double destv = v-128;
        int iSat = 0; // won't be used in MaskHS; interpolation is skipped since p==0:
        if (ProcessPixel(destv, destu, startHue, endHue, maxSat, minSat, 0.0, iSat)) {
          mapY[(u<<8)|v] = maxY;
        } else {
          mapY[(u<<8)|v] = minY;
        }
      }
    }
// #define MaskPointResizing
#ifndef MaskPointResizing
    vi.width  >>= vi.GetPlaneWidthSubsampling(PLANAR_U);
    vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
#endif
    vi.pixel_type = VideoInfo::CS_Y8;
  }
  catch (...) { throw; }
}



PVideoFrame __stdcall MaskHS::GetFrame(int n, IScriptEnvironment* env)
{
	PVideoFrame src = child->GetFrame(n, env);
	PVideoFrame dst = env->NewVideoFrame(vi);

	unsigned char* dstp = dst->GetWritePtr();
	int dst_pitch = dst->GetPitch();

	// show mask
	if (child->GetVideoInfo().IsYUY2()) {
		const unsigned char* srcp = src->GetReadPtr();
		const int src_pitch = src->GetPitch();
		const int height = src->GetHeight();

#ifndef MaskPointResizing
		const int row_size = src->GetRowSize() >> 2;

		for (int y = 0; y < height; y++) {
			for (int x=0; x < row_size; x++) {
				dstp[x] = mapY[( (srcp[x*4+1])<<8 ) | srcp[x*4+3]];
			}
			srcp += src_pitch;
			dstp += dst_pitch;
		}
#else
		const int row_size = src->GetRowSize();

		for (int y = 0; y < height; y++) {
			for (int xs=0, xd=0; xs < row_size; xs+=4, xd+=2) {
				const BYTE mapped = mapY[( (srcp[xs+1])<<8 ) | srcp[xs+3]];
				dstp[xd]   = mapped;
				dstp[xd+1] = mapped;
			}
			srcp += src_pitch;
			dstp += dst_pitch;
		}
#endif
	} else if (child->GetVideoInfo().IsPlanar()) {
		const int srcu_pitch = src->GetPitch(PLANAR_U);
		const unsigned char* srcpu = src->GetReadPtr(PLANAR_U);
		const unsigned char* srcpv = src->GetReadPtr(PLANAR_V);
		const int row_sizeu = src->GetRowSize(PLANAR_U);
		const int heightu = src->GetHeight(PLANAR_U);

#ifndef MaskPointResizing
		for (int y=0; y<heightu; ++y) {
			for (int x=0; x<row_sizeu; ++x) {
				dstp[x] = mapY[( (srcpu[x])<<8 ) | srcpv[x]];
			}
			dstp  += dst_pitch;
			srcpu += srcu_pitch;
			srcpv += srcu_pitch;
		}
#else
		const int swidth = child->GetVideoInfo().GetPlaneWidthSubsampling(PLANAR_U);
		const int sheight = child->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_U);
		const int sw = 1<<swidth;
		const int sh = 1<<sheight;

		const int dpitch = dst_pitch << sheight;
		for (int y=0; y<heightu; ++y) {
			for (int x=0; x<row_sizeu; ++x) {
				const BYTE mapped = mapY[( (srcpu[x])<<8 ) | srcpv[x]];
				const int sx = x<<swidth;

				for (int lumv=0; lumv<sh; ++lumv) {
					const int sy = lumv*dst_pitch+sx;

					for (int lumh=0; lumh<sw; ++lumh) {
						dstp[sy+lumh] = mapped;
					}
				}
			}
			dstp  += dpitch;
			srcpu += srcu_pitch;
			srcpv += srcu_pitch;
		}
#endif
	}
	return dst;
}



AVSValue __cdecl MaskHS::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
	return new MaskHS(args[0].AsClip(),
					  args[1].AsDblDef(  0.0),    // startHue
					  args[2].AsDblDef(360.0),    // endHue
					  args[3].AsDblDef(150.0),    // maxSat
					  args[4].AsDblDef(  0.0),    // minSat
					  args[5].AsBool(false),      // coring
					  env);
	}
	catch (...) { throw; }
}

