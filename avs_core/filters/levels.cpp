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


#include "levels.h"
#include "limiter.h"
#include <cstdio>
#include <cmath>
#include <avs/minmax.h>
#include <avs/alignment.h>
#include "../core/internal.h"
#include <xmmintrin.h>
#include <algorithm>

#define PI 3.141592653589793


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Levels_filters[] = {
  { "Levels",    BUILTIN_FUNC_PREFIX, "cifiii[coring]b[dither]b", Levels::Create },        // src_low, gamma, src_high, dst_low, dst_high
  { "RGBAdjust", BUILTIN_FUNC_PREFIX, "c[r]f[g]f[b]f[a]f[rb]f[gb]f[bb]f[ab]f[rg]f[gg]f[bg]f[ag]f[analyze]b[dither]b", RGBAdjust::Create },
  { "Tweak",     BUILTIN_FUNC_PREFIX, "c[hue]f[sat]f[bright]f[cont]f[coring]b[sse]b[startHue]f[endHue]f[maxSat]f[minSat]f[interp]f[dither]b[realcalc]b", Tweak::Create },
  { "MaskHS",    BUILTIN_FUNC_PREFIX, "c[startHue]f[endHue]f[maxSat]f[minSat]f[coring]b", MaskHS::Create },
  { "Limiter",   BUILTIN_FUNC_PREFIX, "c[min_luma]i[max_luma]i[min_chroma]i[max_chroma]i[show]s", Limiter::Create },
  { 0 }
};


avs_alignas(64) static const BYTE ditherMap[256] = {
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


avs_alignas(16) static const BYTE ditherMap4[16] = {
  0x0, 0xB, 0x6, 0xD,
  0xC, 0x7, 0x9, 0x2,
  0x3, 0x8, 0x5, 0xE,
  0xF, 0x4, 0xA, 0x1,
};

static void __cdecl free_buffer(void* buff, IScriptEnvironment* env)
{
    if (buff) {
        static_cast<IScriptEnvironment2*>(env)->Free(buff);
    }
}

/********************************
 *******   Levels Filter   ******
 ********************************/

Levels::Levels(PClip _child, int in_min, double gamma, int in_max, int out_min, int out_max, bool coring, bool _dither,
  IScriptEnvironment* env)
  : GenericVideoFilter(_child), dither(_dither)
{
  if (gamma <= 0.0)
    env->ThrowError("Levels: gamma must be positive");

  gamma = 1/gamma;

  int divisor;
  if (in_min == in_max)
    divisor = 1;
  else
    divisor = in_max - in_min;

  int scale = 1;
  double bias = 0.0;

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent(); // 8,10..16

  if (pixelsize == 4)
    env->ThrowError("Levels: cannot operate on float video formats");
  // No lookup for float. todo: slow on-the-fly realtime calculation

  int lookup_size = 1 << bits_per_pixel; // 256, 1024, 4096, 16384, 65536
  int real_lookup_size = (pixelsize == 1) ? 256 : 65536; // avoids lut overflow in case of non-standard content of a 10 bit clip
  int pixel_max = lookup_size - 1;

  use_lut = bits_per_pixel != 32; // for float: realtime (todo)

  if (!use_lut)
    dither = false;

  int tv_range_low   = 16 << (bits_per_pixel - 8); // 16
  int tv_range_hi_luma   = ((235+1) << (bits_per_pixel - 8)) - 1; // 16-235
  int range_luma = tv_range_hi_luma - tv_range_low; // 219

  int tv_range_hi_chroma = ((240+1) << (bits_per_pixel - 8)) - 1; // 16-240,64–963, 256–3855,... 4096-61695
  int range_chroma = tv_range_hi_chroma - tv_range_low; // 224

  int middle_chroma = 1 << (bits_per_pixel - 1); // 128

  if (dither) {
    // lut scale settings
    scale = 256; // lower 256 is dither value
    divisor *= 256;
    in_min *= 256;
    bias = -((1 << bits_per_pixel) - 1) / 2; // -127.5 for 8 bit, scaling because of dithershift
  }

  // one buffer for map and mapchroma
  map = nullptr;
  if(use_lut) {
    auto env2 = static_cast<IScriptEnvironment2*>(env);
    size_t number_of_maps = vi.IsYUV() || vi.IsYUVA() ? 2 : 1;
    int bufsize = pixelsize * real_lookup_size * scale * number_of_maps;
    map = static_cast<uint8_t*>(env2->Allocate(bufsize , 16, AVS_NORMAL_ALLOC));
    if (!map)
      env->ThrowError("Levels: Could not reserve memory.");
    env->AtExit(free_buffer, map);
    if(bits_per_pixel>=10 && bits_per_pixel<=14)
      std::fill_n(map, bufsize, 0); // 8 and 16 bit fully overwrites
  }

  if (vi.IsYUV() || vi.IsYUVA())
  {
    mapchroma = map + pixelsize * real_lookup_size * scale;

    for (int i = 0; i<lookup_size*scale; ++i) {
      double p;

      /* old 8 bit-only. Temporarily I left here for sample
      if (coring)
        p = ((bias + i - 16*scale)*(255.0/219.0) - in_min) / divisor;
      else
        p = (bias + i - in_min) / divisor;

      p = pow(clamp(p, 0.0, 1.0), gamma);
      p = p * (out_max - out_min) + out_min;

      if (coring)
        map[i] = (BYTE)clamp(int(p*(219.0/255.0)+16.5), 16, 235);
      else
        map[i] = (BYTE)clamp(int(p+0.5), 0, 255);

      BYTE q = BYTE(((bias + i - 128*scale) * (out_max-out_min)) / divisor + 128.5);

      if (coring)
        mapchroma[i] = (BYTE)clamp((int)q, 16, 240);
      else
        mapchroma[i] = (BYTE)clamp((int)q, 0, 255);
      */

      int ii;
      if(dither) {
        int i_base = dither ? (i & ~0xFF) : i;
        int i_dithershift = dither ? (i & 0xFF) << (bits_per_pixel - 8) : 0;
        ii = i_base + i_dithershift; // otherwise dither has no visible effect on 10..16 bit
      }
      else {
        ii = i;
      }

      if (coring)
        p = ((bias + ii - tv_range_low *scale)*((double)pixel_max/range_luma) - in_min) / divisor;
      else
        p = (bias + ii - in_min) / divisor;

      p = pow(clamp(p, 0.0, 1.0), gamma);
      p = p * (out_max - out_min) + out_min;

      int q = (int)(((bias + ii - middle_chroma*scale) * (out_max-out_min)) / divisor + middle_chroma + 0.5);

      int luma, chroma;
      if (coring) {
        luma = clamp(int(p*((double)range_luma/pixel_max)+tv_range_low + 0.5), tv_range_low, tv_range_hi_luma);
        chroma = clamp(q, tv_range_low, tv_range_hi_chroma);
      } else {
        luma = clamp(int(p+0.5), 0, pixel_max);
        chroma = clamp(q, 0, pixel_max);
      }

      if(pixelsize==1) {
        map[i] = (BYTE)luma;
        mapchroma[i] = (BYTE)chroma;
      }
      else { // pixelsize==2
        reinterpret_cast<uint16_t *>(map)[i] = (uint16_t)luma;
        reinterpret_cast<uint16_t *>(mapchroma)[i] = (uint16_t)chroma;
      }
    }
  }
  else if (vi.IsRGB())
  {
    // no coring option here
    // lookup for packed and planar RGBs
    for (int i = 0; i<lookup_size*scale; ++i) {
      int ii;
      if(dither) {
        int i_base = dither ? (i & ~0xFF) : i;
        int i_dithershift = dither ? (i & 0xFF) << (bits_per_pixel - 8) : 0;
        ii = i_base + i_dithershift; // otherwise dither has no visible effect on 10..16 bit
      }
      else {
        ii = i;
      }
      double p = (bias + ii - in_min) / divisor;
      p = pow(clamp(p, 0.0, 1.0), gamma);
      p = clamp((int)(p * (out_max - out_min) + out_min + 0.5), 0, pixel_max);
      if(pixelsize==1)
        map[i] = (BYTE)p; // 0..255
      else
        reinterpret_cast<uint16_t *>(map)[i] = (uint16_t)p;
    }
  }
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
      for (int y = 0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x = 0; x<vi.width; ++x) {
          p[x*2] = map[p[x*2]<<8 | ditherMap[(x&0x0f)|_y]];
        }
        for (int z = 0; z<UVwidth; ++z) {
          const int _dither = ditherMap[(z&0x0f)|_y];
          p[z*4+1] = mapchroma[p[z*4+1]<<8 | _dither];
          p[z*4+3] = mapchroma[p[z*4+3]<<8 | _dither];
        }
        p += pitch;
      }
    } else if (vi.IsPlanar()) {
      if(vi.IsYUV() || vi.IsYUVA()) {
        // planar YUV
        if(pixelsize==1) {
          for (int y = 0; y<vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<vi.width; ++x) {
              p[x] = map[p[x]<<8 | ditherMap[(x&0x0f)|_y]];
            }
            p += pitch;
          }
        }
        else { // pixelsize==2
          for (int y = 0; y<vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<vi.width; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x]<<8 | ditherMap[(x&0x0f)|_y]];
            }
            p += pitch;
          }
        }
        const int UVpitch = frame->GetPitch(PLANAR_U);
        const int w = frame->GetRowSize(PLANAR_U) / pixelsize;
        const int h = frame->GetHeight(PLANAR_U);
        p = frame->GetWritePtr(PLANAR_U);
        BYTE* q = frame->GetWritePtr(PLANAR_V);
        if(pixelsize==1) {
          for (int y = 0; y<h; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<w; ++x) {
              const int _dither = ditherMap[(x&0x0f)|_y];
              p[x] = mapchroma[p[x]<<8 | _dither];
              q[x] = mapchroma[q[x]<<8 | _dither];
            }
            p += UVpitch;
            q += UVpitch;
          }
        }
        else { // pixelsize==2
          for (int y = 0; y<h; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<w; ++x) {
              const int _dither = ditherMap[(x&0x0f)|_y];
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(mapchroma)[reinterpret_cast<uint16_t *>(p)[x]<<8 | _dither];
              reinterpret_cast<uint16_t *>(q)[x] = reinterpret_cast<uint16_t *>(mapchroma)[reinterpret_cast<uint16_t *>(q)[x]<<8 | _dither];
            }
            p += UVpitch;
            q += UVpitch;
          }
        }
      }
      else {
        // planar RGB
        BYTE* b = frame->GetWritePtr(PLANAR_B);
        BYTE* r = frame->GetWritePtr(PLANAR_R);
        const int pitch_b = frame->GetPitch(PLANAR_B);
        const int pitch_r = frame->GetPitch(PLANAR_R);
        if(pixelsize==1) {
          for (int y = 0; y<vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<vi.width; ++x) {
              p[x] = map[p[x]<<8 | ditherMap[(x&0x0f)|_y]];
              b[x] = map[b[x]<<8 | ditherMap[(x&0x0f)|_y]];
              r[x] = map[r[x]<<8 | ditherMap[(x&0x0f)|_y]];
            }
            p += pitch;
            b += pitch_b;
            r += pitch_r;
          }
        }
        else { // pixelsize==2
          for (int y = 0; y<vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x<vi.width; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x]<<8 | ditherMap[(x&0x0f)|_y]];
              reinterpret_cast<uint16_t *>(b)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(b)[x]<<8 | ditherMap[(x&0x0f)|_y]];
              reinterpret_cast<uint16_t *>(r)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(r)[x]<<8 | ditherMap[(x&0x0f)|_y]];
            }
            p += pitch;
            b += pitch_b;
            r += pitch_r;
          }
        }
      }
    } else if (vi.IsRGB32()) {
      for (int y = 0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x = 0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*4+0] = map[p[x*4+0]<<8 | _dither];
          p[x*4+1] = map[p[x*4+1]<<8 | _dither];
          p[x*4+2] = map[p[x*4+2]<<8 | _dither];
          p[x*4+3] = map[p[x*4+3]<<8 | _dither];
        }
        p += pitch;
      }
    } else if (vi.IsRGB24()) {
      for (int y = 0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x = 0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          p[x*3+0] = map[p[x*3+0]<<8 | _dither];
          p[x*3+1] = map[p[x*3+1]<<8 | _dither];
          p[x*3+2] = map[p[x*3+2]<<8 | _dither];
        }
        p += pitch;
      }
    } else if (vi.IsRGB64()) {
      for (int y = 0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x = 0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          reinterpret_cast<uint16_t *>(p)[x*4+0] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*4+0]<<8 | _dither];
          reinterpret_cast<uint16_t *>(p)[x*4+1] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*4+1]<<8 | _dither];
          reinterpret_cast<uint16_t *>(p)[x*4+2] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*4+2]<<8 | _dither];
          reinterpret_cast<uint16_t *>(p)[x*4+3] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*4+3]<<8 | _dither];
        }
        p += pitch;
      }
    } else if (vi.IsRGB48()) {
      for (int y = 0; y<vi.height; ++y) {
        const int _y = (y << 4) & 0xf0;
        for (int x = 0; x<vi.width; ++x) {
          const int _dither = ditherMap[(x&0x0f)|_y];
          reinterpret_cast<uint16_t *>(p)[x*3+0] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*3+0]<<8 | _dither];
          reinterpret_cast<uint16_t *>(p)[x*3+1] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*3+1]<<8 | _dither];
          reinterpret_cast<uint16_t *>(p)[x*3+2] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x*3+2]<<8 | _dither];
        }
        p += pitch;
      }
    }
  } else { // no dithering
    if (vi.IsYUY2()) {
      for (int y = 0; y<vi.height; ++y) {
        for (int x = 0; x<vi.width; ++x) {
          p[x*2+0] = map[p[x*2+0]];
          p[x*2+1] = mapchroma[p[x*2+1]];
        }
        p += pitch;
      }
    } else if (vi.IsPlanar()) {
      if(vi.IsYUV() || vi.IsYUVA()) {
        // planar YUV
        if(pixelsize==1) {
          for (int y = 0; y<vi.height; ++y) {
            for (int x = 0; x<vi.width; ++x) {
              p[x] = map[p[x]];
            }
            p += pitch;
          }
        } else { // pixelsize==2
          for (int y = 0; y<vi.height; ++y) {
            for (int x = 0; x<vi.width; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x]];
            }
            p += pitch;
          }
        }
        const int UVpitch = frame->GetPitch(PLANAR_U);
        p = frame->GetWritePtr(PLANAR_U);
        const int w = frame->GetRowSize(PLANAR_U) / pixelsize;
        const int h = frame->GetHeight(PLANAR_U);
        if(pixelsize==1) {
          for (int y = 0; y<h; ++y) {
            for (int x = 0; x<w; ++x) {
              p[x] = mapchroma[p[x]];
            }
            p += UVpitch;
          }
          p = frame->GetWritePtr(PLANAR_V);
          for (int y = 0; y<h; ++y) {
            for (int x = 0; x<w; ++x) {
              p[x] = mapchroma[p[x]];
            }
            p += UVpitch;
          }
        } else { // pixelsize==2
          for (int y = 0; y<h; ++y) {
            for (int x = 0; x<w; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(mapchroma)[reinterpret_cast<uint16_t *>(p)[x]];
            }
            p += UVpitch;
          }
          p = frame->GetWritePtr(PLANAR_V);
          for (int y = 0; y<h; ++y) {
            for (int x = 0; x<w; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(mapchroma)[reinterpret_cast<uint16_t *>(p)[x]];
            }
            p += UVpitch;
          }
        }
      }
      else {
        // Planar RGB
        BYTE* b = frame->GetWritePtr(PLANAR_B);
        BYTE* r = frame->GetWritePtr(PLANAR_R);
        const int pitch_b = frame->GetPitch(PLANAR_B);
        const int pitch_r = frame->GetPitch(PLANAR_R);
        if(pixelsize==1) {
          for (int y = 0; y<vi.height; ++y) {
            for (int x = 0; x<vi.width; ++x) {
              p[x] = map[p[x]];
              b[x] = map[b[x]];
              r[x] = map[r[x]];
            }
            p += pitch;
            b += pitch_b;
            r += pitch_r;
          }
        } else { // pixelsize==2
          for (int y = 0; y<vi.height; ++y) {
            for (int x = 0; x<vi.width; ++x) {
              reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x]];
              reinterpret_cast<uint16_t *>(b)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(b)[x]];
              reinterpret_cast<uint16_t *>(r)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(r)[x]];
            }
            p += pitch;
            b += pitch_b;
            r += pitch_r;
          }
        }
      }
    } else if (vi.IsRGB()) {
      // packed RGB
      const int work_width = frame->GetRowSize() / pixelsize;
      if(pixelsize==1) {
        for (int y = 0; y<vi.height; ++y) {
          for (int x = 0; x<work_width; ++x) {
            p[x] = map[p[x]];
          }
          p += pitch;
        }
      } else { // pixelsize==2
        for (int y = 0; y<vi.height; ++y) {
          for (int x = 0; x<work_width; ++x) {
            reinterpret_cast<uint16_t *>(p)[x] = reinterpret_cast<uint16_t *>(map)[reinterpret_cast<uint16_t *>(p)[x]];
          }
          p += pitch;
        }
      }
    }
  }
  return frame;
}

AVSValue __cdecl Levels::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  enum { CHILD, IN_MIN, GAMMA, IN_MAX, OUT_MIN, OUT_MAX, CORING, DITHER };
  return new Levels( args[CHILD].AsClip(), args[IN_MIN].AsInt(), args[GAMMA].AsFloat(), args[IN_MAX].AsInt(),
                     args[OUT_MIN].AsInt(), args[OUT_MAX].AsInt(), args[CORING].AsBool(true), args[DITHER].AsBool(false), env );
}








/********************************
 *******    RGBA Filter    ******
 ********************************/

RGBAdjust::RGBAdjust(PClip _child, double r, double g, double b, double a,
    double rb, double gb, double bb, double ab,
    double rg, double gg, double bg, double ag,
    bool _analyze, bool _dither, IScriptEnvironment* env)
    : GenericVideoFilter(_child), analyze(_analyze), dither(_dither)
{
    if (!vi.IsRGB())
        env->ThrowError("RGBAdjust requires RGB input");

    if ((rg <= 0.0) || (gg <= 0.0) || (bg <= 0.0) || (ag <= 0.0))
        env->ThrowError("RGBAdjust: gammas must be positive");

    rg = 1 / rg; gg = 1 / gg; bg = 1 / bg; ag = 1 / ag;

    pixelsize = vi.ComponentSize();
    bits_per_pixel = vi.BitsPerComponent(); // 8,10..16

    if (pixelsize == 4)
      env->ThrowError("RGBAdjust: cannot operate on float video formats");
    // No lookup for float. todo: slow on-the-fly realtime calculation

    int lookup_size = 1 << bits_per_pixel; // 256, 1024, 4096, 16384, 65536
    int real_lookup_size = (pixelsize == 1) ? 256 : 65536; // avoids lut overflow in case of non-standard content of a 10 bit clip
    int pixel_max = lookup_size - 1;

    use_lut = bits_per_pixel != 32; // for float: realtime (todo)

    if (!use_lut)
      dither = false;

    // one buffer for all maps
    mapR = nullptr;

    if(use_lut) {
      auto env2 = static_cast<IScriptEnvironment2*>(env);
      size_t number_of_maps = (vi.IsRGB24() || vi.IsRGB48() || vi.IsPlanarRGB()) ? 3 : 4;
      int one_bufsize = pixelsize * real_lookup_size;
      if (dither) one_bufsize *= 256;

      mapR = static_cast<uint8_t*>(env2->Allocate(one_bufsize * number_of_maps, 16, AVS_NORMAL_ALLOC));
      if (!mapR)
          env->ThrowError("RGBAdjust: Could not reserve memory.");
      env->AtExit(free_buffer, mapR);
      if(bits_per_pixel>=10 && bits_per_pixel<=14)
        std::fill_n(mapR, one_bufsize * number_of_maps, 0); // 8 and 16 bit fully overwrites
      mapG = mapR + one_bufsize;
      mapB = mapG + one_bufsize;
      mapA = number_of_maps == 4 ? mapB + one_bufsize : nullptr;

      void(*set_map)(BYTE*, int, int, const double, const double, const double);
      if (dither) {
          set_map = [](BYTE* map, int lookup_size, int bits_per_pixel, const double c0, const double c1, const double c2) {
              double bias = -((1 << bits_per_pixel) - 1) / 2; // -127.5 for 8 bit, scaling because of dithershift
              double pixel_max = (1 << bits_per_pixel) - 1;
              if(bits_per_pixel == 8) {
                for (int i = 0; i < lookup_size * 256; ++i) {
                  int i_base = i & ~0xFF;
                  int i_dithershift = (i & 0xFF) << (bits_per_pixel - 8);
                  int ii = ii = i_base + i_dithershift; // otherwise dither has no visible effect on 10..16 bit
                  map[i] = BYTE(pow(clamp((c0 * 256 + ii * c1 - bias) / (double(pixel_max) * 256), 0.0, 1.0), c2) * (double)pixel_max + 0.5);
                }
              }
              else {
                for (int i = 0; i < lookup_size * 256; ++i) {
                  int i_base = i & ~0xFF;
                  int i_dithershift = (i & 0xFF) << (bits_per_pixel - 8);
                  int ii = ii = i_base + i_dithershift; // otherwise dither has no visible effect on 10..16 bit
                  reinterpret_cast<uint16_t *>(map)[i] = uint16_t(pow(clamp((c0 * 256 + ii * c1 - bias) / (double(pixel_max) * 256), 0.0, 1.0), c2) * (double)pixel_max + 0.5);
                }
              }
          };
      } else {
          set_map = [](BYTE* map, int lookup_size, int bits_per_pixel, const double c0, const double c1, const double c2) {
            double pixel_max = (1 << bits_per_pixel) - 1;
            if(bits_per_pixel==8) {
              for (int i = 0; i < lookup_size; ++i) { // fix of bug introduced in an earlier refactor was: i < 256 * 256
                    map[i] = BYTE(pow(clamp((c0 + i * c1) / (double)pixel_max, 0.0, 1.0), c2) * double(pixel_max) + 0.5);
                }
            }
            else {
              for (int i = 0; i < lookup_size; ++i) { // fix of bug introduced in an earlier refactor was: i < 256 * 256
                reinterpret_cast<uint16_t *>(map)[i] = uint16_t(pow(clamp((c0 + i * c1) / (double)pixel_max, 0.0, 1.0), c2) * double(pixel_max) + 0.5);
              }
            }
          };
      }

      set_map(mapR, lookup_size, bits_per_pixel, rb, r, rg);
      set_map(mapG, lookup_size, bits_per_pixel, gb, g, gg);
      set_map(mapB, lookup_size, bits_per_pixel, bb, b, bg);
      if (number_of_maps == 4)
          set_map(mapA, lookup_size, bits_per_pixel, ab, a, ag);
    }
}


PVideoFrame __stdcall RGBAdjust::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame frame = child->GetFrame(n, env);
    env->MakeWritable(&frame);
    BYTE* p = frame->GetWritePtr();
    const int pitch = frame->GetPitch();

    if (dither) {
        if (vi.IsRGB32()) {
            for (int y = 0; y < vi.height; ++y) {
                const int _y = (y << 4) & 0xf0;
                for (int x = 0; x < vi.width; ++x) {
                    const int _dither = ditherMap[(x & 0x0f) | _y];
                    p[x * 4 + 0] = mapB[p[x * 4 + 0] << 8 | _dither];
                    p[x * 4 + 1] = mapG[p[x * 4 + 1] << 8 | _dither];
                    p[x * 4 + 2] = mapR[p[x * 4 + 2] << 8 | _dither];
                    p[x * 4 + 3] = mapA[p[x * 4 + 3] << 8 | _dither];
                }
                p += pitch;
            }
        }
        else if (vi.IsRGB24()) {
            for (int y = 0; y < vi.height; ++y) {
                const int _y = (y << 4) & 0xf0;
                for (int x = 0; x < vi.width; ++x) {
                    const int _dither = ditherMap[(x & 0x0f) | _y];
                    p[x * 3 + 0] = mapB[p[x * 3 + 0] << 8 | _dither];
                    p[x * 3 + 1] = mapG[p[x * 3 + 1] << 8 | _dither];
                    p[x * 3 + 2] = mapR[p[x * 3 + 2] << 8 | _dither];
                }
                p += pitch;
            }
        }
        else if (vi.IsRGB64()) {
          for (int y = 0; y < vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x < vi.width; ++x) {
              const int _dither = ditherMap[(x & 0x0f) | _y];
              uint16_t *p16 = reinterpret_cast<uint16_t *>(p);
              p16[x * 4 + 0] = reinterpret_cast<uint16_t *>(mapB)[p16[x * 4 + 0] << 8 | _dither];
              p16[x * 4 + 1] = reinterpret_cast<uint16_t *>(mapG)[p16[x * 4 + 1] << 8 | _dither];
              p16[x * 4 + 2] = reinterpret_cast<uint16_t *>(mapR)[p16[x * 4 + 2] << 8 | _dither];
              p16[x * 4 + 3] = reinterpret_cast<uint16_t *>(mapA)[p16[x * 4 + 3] << 8 | _dither];
            }
            p += pitch;
          }
        }
        else if (vi.IsRGB48()) {
          for (int y = 0; y < vi.height; ++y) {
            const int _y = (y << 4) & 0xf0;
            for (int x = 0; x < vi.width; ++x) {
              const int _dither = ditherMap[(x & 0x0f) | _y];
              uint16_t *p16 = reinterpret_cast<uint16_t *>(p);
              p16[x * 3 + 0] = reinterpret_cast<uint16_t *>(mapB)[p16[x * 3 + 0] << 8 | _dither];
              p16[x * 3 + 1] = reinterpret_cast<uint16_t *>(mapG)[p16[x * 3 + 1] << 8 | _dither];
              p16[x * 3 + 2] = reinterpret_cast<uint16_t *>(mapR)[p16[x * 3 + 2] << 8 | _dither];
            }
            p += pitch;
          }
        }
        else {
          // Planar RGB
          bool hasAlpha = vi.IsPlanarRGBA();
          BYTE *srcpG = p;
          BYTE *srcpB = frame->GetWritePtr(PLANAR_B);
          BYTE *srcpR = frame->GetWritePtr(PLANAR_R);
          BYTE *srcpA = frame->GetWritePtr(PLANAR_A);
          const int pitchG = pitch;
          const int pitchB = frame->GetPitch(PLANAR_B);
          const int pitchR = frame->GetPitch(PLANAR_R);
          const int pitchA = frame->GetPitch(PLANAR_A);
          // no float support
          if(pixelsize==1) {
            for (int y=0; y<vi.height; y++) {
              const int _y = (y << 4) & 0xf0;
              for (int x=0; x<vi.width; x++) {
                const int _dither = ditherMap[(x & 0x0f) | _y];
                srcpG[x] = mapG[srcpG[x] << 8 | _dither];
                srcpB[x] = mapB[srcpB[x] << 8 | _dither];
                srcpR[x] = mapR[srcpR[x] << 8 | _dither];
                if(hasAlpha)
                  srcpA[x] = mapA[srcpA[x]];
              }
              srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
              srcpA += pitchA;
            }
          } else if(pixelsize==2) {
            for (int y=0; y<vi.height; y++) {
              const int _y = (y << 4) & 0xf0;
              for (int x=0; x<vi.width; x++) {
                const int _dither = ditherMap[(x & 0x0f) | _y];
                reinterpret_cast<uint16_t *>(srcpG)[x] = reinterpret_cast<uint16_t *>(mapG)[reinterpret_cast<uint16_t *>(srcpG)[x] << 8 | _dither];
                reinterpret_cast<uint16_t *>(srcpB)[x] = reinterpret_cast<uint16_t *>(mapB)[reinterpret_cast<uint16_t *>(srcpB)[x] << 8 | _dither];
                reinterpret_cast<uint16_t *>(srcpR)[x] = reinterpret_cast<uint16_t *>(mapR)[reinterpret_cast<uint16_t *>(srcpR)[x] << 8 | _dither];
                if(hasAlpha)
                  reinterpret_cast<uint16_t *>(srcpA)[x] = reinterpret_cast<uint16_t *>(mapA)[reinterpret_cast<uint16_t *>(srcpA)[x] << 8 | _dither];
              }
              srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
              srcpA += pitchA;
            }
          }
        }
    }
    else {
      // no dither
        if (vi.IsRGB32()) {
            for (int y = 0; y < vi.height; ++y) {
                for (int x = 0; x < vi.width; ++x) {
                    p[x * 4 + 0] = mapB[p[x * 4]];
                    p[x * 4 + 1] = mapG[p[x * 4 + 1]];
                    p[x * 4 + 2] = mapR[p[x * 4 + 2]];
                    p[x * 4 + 3] = mapA[p[x * 4 + 3]];
                }
                p += pitch;
            }
        }
        else if (vi.IsRGB24()) {
            for (int y = 0; y < vi.height; ++y) {
                for (int x = 0; x < vi.width; x += 3) {
                    p[x * 3 + 0] = mapB[p[x]];
                    p[x * 3 + 1] = mapG[p[x + 1]];
                    p[x * 3 + 2] = mapR[p[x + 2]];
                }
                p += pitch;
            }
        } else if (vi.IsRGB64()) {
          for (int y = 0; y < vi.height; ++y) {
            for (int x = 0; x < vi.width; ++x) {
              uint16_t *p16 = reinterpret_cast<uint16_t *>(p);
              p16[x * 4 + 0] = reinterpret_cast<uint16_t *>(mapB)[p16[x * 4]];
              p16[x * 4 + 1] = reinterpret_cast<uint16_t *>(mapG)[p16[x * 4 + 1]];
              p16[x * 4 + 2] = reinterpret_cast<uint16_t *>(mapR)[p16[x * 4 + 2]];
              p16[x * 4 + 3] = reinterpret_cast<uint16_t *>(mapA)[p16[x * 4 + 3]];
            }
            p += pitch;
          }
        } else if (vi.IsRGB48()) {
          for (int y = 0; y < vi.height; ++y) {
            for (int x = 0; x < vi.width; ++x) {
              uint16_t *p16 = reinterpret_cast<uint16_t *>(p);
              p16[x * 3 + 0] = reinterpret_cast<uint16_t *>(mapB)[p16[x * 3]];
              p16[x * 3 + 1] = reinterpret_cast<uint16_t *>(mapG)[p16[x * 3 + 1]];
              p16[x * 3 + 2] = reinterpret_cast<uint16_t *>(mapR)[p16[x * 3 + 2]];
            }
            p += pitch;
          }
        }
        else {
          // Planar RGB
          bool hasAlpha = vi.IsPlanarRGBA();
          BYTE *srcpG = p;
          BYTE *srcpB = frame->GetWritePtr(PLANAR_B);
          BYTE *srcpR = frame->GetWritePtr(PLANAR_R);
          BYTE *srcpA = frame->GetWritePtr(PLANAR_A);
          const int pitchG = pitch;
          const int pitchB = frame->GetPitch(PLANAR_B);
          const int pitchR = frame->GetPitch(PLANAR_R);
          const int pitchA = frame->GetPitch(PLANAR_A);
          // no float support
          if(pixelsize==1) {
            for (int y=0; y<vi.height; y++) {
              for (int x=0; x<vi.width; x++) {
                srcpG[x] = mapG[srcpG[x]];
                srcpB[x] = mapB[srcpB[x]];
                srcpR[x] = mapR[srcpR[x]];
                if(hasAlpha)
                  srcpA[x] = mapA[srcpA[x]];
              }
              srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
              srcpA += pitchA;
            }
          } else if(pixelsize==2) {
            for (int y=0; y<vi.height; y++) {
              for (int x=0; x<vi.width; x++) {
                reinterpret_cast<uint16_t *>(srcpG)[x] = reinterpret_cast<uint16_t *>(mapG)[reinterpret_cast<uint16_t *>(srcpG)[x]];
                reinterpret_cast<uint16_t *>(srcpB)[x] = reinterpret_cast<uint16_t *>(mapB)[reinterpret_cast<uint16_t *>(srcpB)[x]];
                reinterpret_cast<uint16_t *>(srcpR)[x] = reinterpret_cast<uint16_t *>(mapR)[reinterpret_cast<uint16_t *>(srcpR)[x]];
                if(hasAlpha)
                  reinterpret_cast<uint16_t *>(srcpA)[x] = reinterpret_cast<uint16_t *>(mapA)[reinterpret_cast<uint16_t *>(srcpA)[x]];
              }
              srcpG += pitchG; srcpB += pitchB; srcpR += pitchR;
              srcpA += pitchA;
            }
          }
        }
    }

    if (analyze) {
        const int w = frame->GetRowSize() / pixelsize;
        const int h = frame->GetHeight();

        int lookup_size = 1 << bits_per_pixel; // 256, 1024, 4096, 16384, 65536
        int real_lookup_size = (pixelsize == 1) ? 256 : 65536; // avoids lut overflow in case of non-standard content of a 10 bit clip
        int pixel_max = lookup_size - 1;

        // worst case
        unsigned int accum_r[65536], accum_g[65536], accum_b[65536];

        for (int i = 0; i < lookup_size; i++) {
          accum_r[i] = 0;
          accum_g[i] = 0;
          accum_b[i] = 0;
        }

        p = frame->GetWritePtr();
        if(vi.IsPlanarRGB() || vi.IsPlanarRGBA())
        {
          const BYTE *p_g = p;
          const BYTE *p_b = frame->GetReadPtr(PLANAR_B);
          const BYTE *p_r = frame->GetReadPtr(PLANAR_R);
          const int pitchG = pitch;
          const int pitchB= frame->GetPitch(PLANAR_B);
          const int pitchR= frame->GetPitch(PLANAR_R);
          if(pixelsize==1) {
            for (int y = 0; y < h; y++) {
              for (int x = 0; x < w; x++) {
                accum_r[p_r[x]]++;
                accum_g[p_g[x]]++;
                accum_b[p_b[x]]++;
              }
              p_g += pitchG;
              p_b += pitchB;
              p_r += pitchR;
            }
          }
          else {
            // pixelsize == 2
            for (int y = 0; y < h; y++) {
              for (int x = 0; x < w; x++) {
                accum_r[reinterpret_cast<const uint16_t *>(p_r)[x]]++;
                accum_g[reinterpret_cast<const uint16_t *>(p_g)[x]]++;
                accum_b[reinterpret_cast<const uint16_t *>(p_b)[x]]++;
              }
              p_g += pitchG;
              p_b += pitchB;
              p_r += pitchR;
            }
          }
        } else {
          // packed RGB
          const int pixel_step = vi.IsRGB24() || vi.IsRGB48() ? 3 : 4;
          if(pixelsize==1) {
            for (int y = 0; y < h; y++) {
              for (int x = 0; x < w; x += pixel_step) {
                accum_r[p[x + 2]]++;
                accum_g[p[x + 1]]++;
                accum_b[p[x]]++;
              }
              p += pitch;
            }
          }
          else { // pixelsize==2
            for (int y = 0; y < h; y++) {
              for (int x = 0; x < w; x += pixel_step) {
                accum_r[reinterpret_cast<uint16_t *>(p)[x + 2]]++;
                accum_g[reinterpret_cast<uint16_t *>(p)[x + 1]]++;
                accum_b[reinterpret_cast<uint16_t *>(p)[x]]++;
              }
              p += pitch;
            }
          }
        }


        int pixels = vi.width*vi.height;
        float avg_r = 0, avg_g = 0, avg_b = 0;
        float st_r = 0, st_g = 0, st_b = 0;
        int min_r = 0, min_g = 0, min_b = 0;
        int max_r = 0, max_g = 0, max_b = 0;
        bool hit_r = false, hit_g = false, hit_b = false;
        int Amin_r = 0, Amin_g = 0, Amin_b = 0;
        int Amax_r = 0, Amax_g = 0, Amax_b = 0;
        bool Ahit_minr = false, Ahit_ming = false, Ahit_minb = false;
        bool Ahit_maxr = false, Ahit_maxg = false, Ahit_maxb = false;
        int At_256 = (pixels + 128) / 256; // When 1/256th of all pixels have been reached, trigger "Loose min/max"


        {for (int i = 0; i < lookup_size; i++) {
            avg_r += (float)accum_r[i] * (float)i;
            avg_g += (float)accum_g[i] * (float)i;
            avg_b += (float)accum_b[i] * (float)i;

            if (accum_r[i] != 0) { max_r = i; hit_r = true; }
            else { if (!hit_r) min_r = i + 1; }
            if (accum_g[i] != 0) { max_g = i; hit_g = true; }
            else { if (!hit_g) min_g = i + 1; }
            if (accum_b[i] != 0) { max_b = i; hit_b = true; }
            else { if (!hit_b) min_b = i + 1; }

            if (!Ahit_minr) { Amin_r += accum_r[i]; if (Amin_r > At_256) { Ahit_minr = true; Amin_r = i; } }
            if (!Ahit_ming) { Amin_g += accum_g[i]; if (Amin_g > At_256) { Ahit_ming = true; Amin_g = i; } }
            if (!Ahit_minb) { Amin_b += accum_b[i]; if (Amin_b > At_256) { Ahit_minb = true; Amin_b = i; } }

            if (!Ahit_maxr) { Amax_r += accum_r[pixel_max - i]; if (Amax_r > At_256) { Ahit_maxr = true; Amax_r = pixel_max - i; } }
            if (!Ahit_maxg) { Amax_g += accum_g[pixel_max - i]; if (Amax_g > At_256) { Ahit_maxg = true; Amax_g = pixel_max - i; } }
            if (!Ahit_maxb) { Amax_b += accum_b[pixel_max - i]; if (Amax_b > At_256) { Ahit_maxb = true; Amax_b = pixel_max - i; } }
        }}

        float Favg_r = avg_r / pixels;
        float Favg_g = avg_g / pixels;
        float Favg_b = avg_b / pixels;

        {for (int i = 0; i < lookup_size; i++) {
            st_r += (float)accum_r[i] * (float(i - Favg_r)*(i - Favg_r));
            st_g += (float)accum_g[i] * (float(i - Favg_g)*(i - Favg_g));
            st_b += (float)accum_b[i] * (float(i - Favg_b)*(i - Favg_b));
        }}

        float Fst_r = sqrt(st_r / pixels);
        float Fst_g = sqrt(st_g / pixels);
        float Fst_b = sqrt(st_b / pixels);

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
            (unsigned int)n,
            Favg_r, Favg_g, Favg_b,
            Fst_r, Fst_g, Fst_b,
            min_r, min_g, min_b,
            max_r, max_g, max_b,
            Amin_r, Amin_g, Amin_b,
            Amax_r, Amax_g, Amax_b
        );
        env->ApplyMessage(&frame, vi, text, vi.width / 4, 0xa0a0a0, 0, 0);
    }
    return frame;
}


AVSValue __cdecl RGBAdjust::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new RGBAdjust(args[ 0].AsClip(),
                       args[ 1].AsDblDef(1.0), args[ 2].AsDblDef(1.0), args[ 3].AsDblDef(1.0), args[ 4].AsDblDef(1.0),
                       args[ 5].AsDblDef(0.0), args[ 6].AsDblDef(0.0), args[ 7].AsDblDef(0.0), args[ 8].AsDblDef(0.0),
                       args[ 9].AsDblDef(1.0), args[10].AsDblDef(1.0), args[11].AsDblDef(1.0), args[12].AsDblDef(1.0),
                       args[13].AsBool(false), args[14].AsBool(false), env );
}



/* helper function for Tweak and MaskHS filters */
static bool ProcessPixel(double X, double Y, double startHue, double endHue,
    double maxSat, double minSat, double p, int &iSat)
{
    // a hue analog
    double T = atan2(X, Y) * 180.0 / PI;
    if (T < 0.0) T += 360.0;

    // startHue <= hue <= endHue
    if (startHue < endHue) {
        if (T > endHue || T < startHue) return false;
    }
    else {
        if (T<startHue && T>endHue) return false;
    }

    const double W = X*X + Y*Y;

    // In Range, full adjust but no need to interpolate
    if (minSat*minSat <= W && W <= maxSat*maxSat) return true;

    // p == 0 (no interpolation) needed for MaskHS
    if (p == 0.0) return false;

    // Interpolation range is +/-p for p>0
    const double max = min(maxSat + p, 180.0);
    const double min = ::max(minSat - p, 0.0);

    // Outside of [min-p, max+p] no adjustment
    // minSat-p <= (U^2 + V^2) <= maxSat+p
    if (W <= min*min || max*max <= W) return false; // don't adjust

    // Interpolate saturation value
    const double holdSat = W < 180.0*180.0 ? sqrt(W) : 180.0;

    if (holdSat < minSat) { // within p of lower range
        iSat += (int)((512 - iSat) * (minSat - holdSat) / p);
    }
    else { // within p of upper range
        iSat += (int)((512 - iSat) * (holdSat - maxSat) / p);
    }

    return true;
}

// for float
static bool ProcessPixelUnscaled(double X, double Y, double startHue, double endHue,
    double maxSat, double minSat, double p, double &dSat)
{
    // a hue analog
    double T = atan2(X, Y) * 180.0 / PI;
    if (T < 0.0) T += 360.0;

    // startHue <= hue <= endHue
    if (startHue < endHue) {
        if (T > endHue || T < startHue) return false;
    }
    else {
        if (T<startHue && T>endHue) return false;
    }

    const double W = X*X + Y*Y;

    // In Range, full adjust but no need to interpolate
    if (minSat*minSat <= W && W <= maxSat*maxSat) return true;

    // p == 0 (no interpolation) needed for MaskHS
    if (p == 0.0) return false;

    // Interpolation range is +/-p for p>0
    const double max = min(maxSat + p, 180.0);
    const double min = ::max(minSat - p, 0.0);

    // Outside of [min-p, max+p] no adjustment
    // minSat-p <= (U^2 + V^2) <= maxSat+p
    if (W <= min*min || max*max <= W) return false; // don't adjust

    // Interpolate saturation value
    const double holdSat = W < 180.0*180.0 ? sqrt(W) : 180.0;

    if (holdSat < minSat) { // within p of lower range
        dSat += ((1 - dSat) * (minSat - holdSat) / p);
    }
    else { // within p of upper range
        dSat += ((1 - dSat) * (holdSat - maxSat) / p);
    }

    return true;
}


/**********************
******   Tweak    *****
**********************/

Tweak::Tweak(PClip _child, double _hue, double _sat, double _bright, double _cont, bool _coring, bool _sse,
            double _startHue, double _endHue, double _maxSat, double _minSat, double p,
            bool _dither, bool _realcalc, IScriptEnvironment* env)
  : GenericVideoFilter(_child), coring(_coring), sse(_sse), dither(_dither), realcalc(_realcalc),
  dhue(_hue), dsat(_sat), dbright(_bright), dcont(_cont), dstartHue(_startHue), dendHue(_endHue),
  dmaxSat(_maxSat), dminSat(_minSat), dinterp(p)
{
  if (vi.IsRGB())
        env->ThrowError("Tweak: YUV data only (no RGB)");

  // Flag to skip special processing if doing all pixels
  // If defaults, don't check for ranges, just do all
  const bool allPixels = (_startHue == 0.0 && _endHue == 360.0 && _maxSat == 150.0 && _minSat == 0.0);

// The new "mapping" C code is faster than the iSSE code on my 3GHz P4HT - Make it optional
  if (sse && (!allPixels || coring || dither || !vi.IsYUY2()))
      env->ThrowError("Tweak: SSE option only available for YUY2 with coring=false and no options.");
  if (sse && !(env->GetCPUFlags() & CPUF_INTEGER_SSE))
      env->ThrowError("Tweak: SSE option needs an iSSE capable processor");

  if (vi.NumComponents() == 1) {
      if (!(_hue == 0.0 && _sat == 1.0 && allPixels))
      env->ThrowError("Tweak: bright and cont are the only options available for greyscale.");
  }

  if (_startHue < 0.0 || _startHue >= 360.0)
        env->ThrowError("Tweak: startHue must be greater than or equal to 0.0 and less than 360.0");

  if (_endHue <= 0.0 || _endHue > 360.0)
        env->ThrowError("Tweak: endHue must be greater than 0.0 and less than or equal to 360.0");

  if (_minSat >= _maxSat)
        env->ThrowError("Tweak: MinSat must be less than MaxSat");

  if (_minSat < 0.0 || _minSat >= 150.0)
        env->ThrowError("Tweak: minSat must be greater than or equal to 0 and less than 150.");

  if (_maxSat <= 0.0 || _maxSat > 150.0)
        env->ThrowError("Tweak: maxSat must be greater than 0 and less than or equal to 150.");

  if (p>=150.0 || p<0.0)
        env->ThrowError("Tweak: Interp must be greater than or equal to 0 and less than 150.");

  Sat = (int) (_sat * 512);    // 9 bits extra precision
  Cont = (int) (_cont * 512);
  Bright = (int) _bright;

  const double Hue = (_hue * PI) / 180.0;
  const double SIN = sin(Hue);
  const double COS = cos(Hue);

  Sin = (int) (SIN * 4096 + 0.5);
  Cos = (int) (COS * 4096 + 0.5);

  int bits_per_pixel = vi.BitsPerComponent(); // 8,10..16,32

  if (vi.IsPlanar() && (bits_per_pixel > 8))
    realcalc = true; // 8bit: lut OK. 10+ bits: no lookup tables.
  // todo: 10 bit lut is still OK

  auto env2 = static_cast<IScriptEnvironment2*>(env);

  if(!(realcalc && vi.IsPlanar()))
  { // fill brightness/constrast lookup tables
    size_t map_size = dither ? 256 * 256 : 256;
    map = static_cast<uint8_t*>(env2->Allocate(map_size, 8, AVS_NORMAL_ALLOC));
    if (!map)
      env->ThrowError("Tweak: Could not reserve memory.");
    env->AtExit(free_buffer, map);

    if (dither) {
      if (coring) {
        for (int i = 0; i < 256 * 256; i++) {
          /* brightness and contrast */
          int y = int(((i - 16 * 256)*_cont + _bright * 256 - 127.5) / 256 + 16.5);
          map[i] = (BYTE)clamp(y, 16, 235);

        }
      }
      else {
        for (int i = 0; i < 256 * 256; i++) {
          /* brightness and contrast */
          int y = int((i*_cont + _bright * 256 - 127.5) / 256 + 0.5);
          map[i] = (BYTE)clamp(y, 0, 255);
        }
      }
    }
    else {
      if (coring) {
        for (int i = 0; i < 256; i++) {
          /* brightness and contrast */
          int y = int((i - 16)*_cont + _bright + 16.5);
          map[i] = (BYTE)clamp(y, 16, 235);

        }
      }
      else {
        for (int i = 0; i < 256; i++) {
          /* brightness and contrast */
          int y = int(i*_cont + _bright + 0.5);
          map[i] = (BYTE)clamp(y, 0, 255);
        }
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

  if (!(realcalc && vi.IsPlanar()))
  { // fill lookup tables for UV
    size_t map_size = 256 * 256 * sizeof(uint16_t) * (dither ? 16 : 1);
    mapUV = static_cast<uint16_t*>(env2->Allocate(map_size, 8, AVS_NORMAL_ALLOC));
    if (!mapUV)
      env->ThrowError("Tweak: Could not reserve memory.");
    env->AtExit(free_buffer, mapUV);

    if (dither) {
      for (int d = 0; d < 16; d++) {
        for (int u = 0; u < 256; u++) {
          const double destu = ((u << 4 | d) - 7.5) / 16.0 - 128.0;
          for (int v = 0; v < 256; v++) {
            const double destv = ((v << 4 | d) - 7.5) / 16.0 - 128.0;
            int iSat = Sat;
            if (allPixels || ProcessPixel(destv, destu, _startHue, _endHue, maxSat, minSat, p, iSat)) {
              int du = int((destu*COS + destv*SIN) * iSat + 0x100) >> 9; // back from the extra 9 bits Sat precision
              int dv = int((destv*COS - destu*SIN) * iSat + 0x100) >> 9;
              du = clamp(du + 128, minUV, maxUV);
              dv = clamp(dv + 128, minUV, maxUV);
              mapUV[(u << 12) | (v << 4) | d] = (uint16_t)(du | (dv << 8));
            }
            else {
              mapUV[(u << 12) | (v << 4) | d] = (uint16_t)(clamp(u, minUV, maxUV) | (clamp(v, minUV, maxUV) << 8));
            }
          }
        }
      }
    }
    else {
      for (int u = 0; u < 256; u++) {
        const double destu = u - 128;
        for (int v = 0; v < 256; v++) {
          const double destv = v - 128;
          int iSat = Sat;
          if (allPixels || ProcessPixel(destv, destu, _startHue, _endHue, maxSat, minSat, p, iSat)) {
            int du = int((destu*COS + destv*SIN) * iSat) >> 9; // back from the extra 9 bits Sat precision
            int dv = int((destv*COS - destu*SIN) * iSat) >> 9;
            du = clamp(du + 128, minUV, maxUV);
            dv = clamp(dv + 128, minUV, maxUV);
            mapUV[(u << 8) | v] = (uint16_t)(du | (dv << 8));
          }
          else {
            mapUV[(u << 8) | v] = (uint16_t)(clamp(u, minUV, maxUV) | (clamp(v, minUV, maxUV) << 8));
          }
        }
      }
    }
  }
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

        if (dither) {
            const int UVwidth = vi.width / 2;
            for (int y = 0; y < height; y++) {
                {const int _y = (y << 4) & 0xf0;
                for (int x = 0; x < vi.width; ++x) {
                    /* brightness and contrast */
                    srcp[x * 2] = map[srcp[x * 2] << 8 | ditherMap[(x & 0x0f) | _y]];
                }}
                {const int _y = (y << 2) & 0xC;
                for (int x = 0; x < UVwidth; ++x) {
                    const int _dither = ditherMap4[(x & 0x3) | _y];
                    /* hue and saturation */
                    const int u = srcp[x * 4 + 1];
                    const int v = srcp[x * 4 + 3];
                    const int mapped = mapUV[(u << 12) | (v << 4) | _dither];
                    srcp[x * 4 + 1] = (BYTE)(mapped & 0xff);
                    srcp[x * 4 + 3] = (BYTE)(mapped >> 8);
                }}
                srcp += src_pitch;
            }
        }
        else {
            for (int y = 0; y < height; y++)
            {
                for (int x = 0; x < row_size; x += 4)
                {
                    /* brightness and contrast */
                    srcp[x] = map[srcp[x]];
                    srcp[x + 2] = map[srcp[x + 2]];

                    /* hue and saturation */
                    const int u = srcp[x + 1];
                    const int v = srcp[x + 3];
                    const int mapped = mapUV[(u << 8) | v];
                    srcp[x + 1] = (BYTE)(mapped & 0xff);
                    srcp[x + 3] = (BYTE)(mapped >> 8);
                }
                srcp += src_pitch;
            }
        }
    }
    else if (vi.IsPlanar()) {
        // brightness and contrast
        if (realcalc) // no lookup! alway true for 16/32 bit, optional for 8 bit
        {
            int pixelsize = vi.ComponentSize();
            int width = row_size / pixelsize;
            float maxY;
            float minY;
            float ditherval = 0.0f;
            // unique for each bit-depth, difference in the innermost loop (speed)
            if (pixelsize == 1) { // uint8_t
                maxY = coring ? 235.0f : 255.0f;
                minY = coring ? 16.0f : 0;
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 4) & 0xf0;
                    for (int x = 0; x < width; ++x) {
                        if (dither)
                            ditherval = (ditherMap[(x & 0x0f) | _y] - 127.5f) / 256.0f; // 0x00..0xFF -> -0.5 .. + 0.5 (+/- maxrange/512)
                        float y0 = minY + ((srcp[x] - minY) + ditherval)*(float)dcont + (float)dbright; // dbright parameter always 0..255
                        srcp[x] = (BYTE)clamp(y0, minY, maxY);
                    }
                    srcp += src_pitch;
                }

            } else if (pixelsize == 2) { // uint16_t
                maxY = coring ? 235.0f * 256 : 65535.0f;
                minY = coring ? 16.0f * 256 : 0;
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 4) & 0xf0;
                    for (int x = 0; x < width; ++x) {
                        if (dither) ditherval = (ditherMap[(x & 0x0f) | _y] - 127.5f); // 0x00..0xFF -> -0.7F .. + 0.7F (+/- maxrange/512)
                        float y0 = (reinterpret_cast<uint16_t *>(srcp)[x] - minY);
                        y0 = minY + ( (y0 + ditherval)*(float)dcont + 256.0f*(float)dbright); // dbright parameter always 0..255
                        reinterpret_cast<uint16_t *>(srcp)[x] = (uint16_t)clamp(y0, minY, maxY);
                    }
                    srcp += src_pitch;
                }
            } else { // pixelsize 4: float
                maxY = coring ? 235.0f / 256 : 1.0f; // scale into 0..1 range
                minY = coring ? 16.0f / 256 : 0;
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 4) & 0xf0;
                    for (int x = 0; x < width; ++x) {
                        if (dither) ditherval = (ditherMap[(x & 0x0f) | _y] - 127.5f) / 65536.0f; // 0x00..0xFF -> -0.5 .. + 0.5 (+/- maxrange/512)
                        float y0 = (reinterpret_cast<float *>(srcp)[x] - minY);
                        y0 = minY + (y0 + ditherval)*(float)dcont + (float)dbright / 256.0f; // dbright parameter always 0..255, scale it to 0..1
                        reinterpret_cast<float *>(srcp)[x] = (float)clamp(y0, minY, maxY);
                    }
                    srcp += src_pitch;
                }
            }

        }
        else {
            // use lookup for 8 bit
            if (dither) {
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 4) & 0xf0;
                    for (int x = 0; x < row_size; ++x) {
                        /* brightness and contrast */
                        srcp[x] = map[srcp[x] << 8 | ditherMap[(x & 0x0f) | _y]];
                    }
                    srcp += src_pitch;
                }
            }
            else {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < row_size; ++x) {
                        /* brightness and contrast */
                        srcp[x] = map[srcp[x]];
                    }
                    srcp += src_pitch;
                }
            }
        }
        // Y: brightness and contrast done

        // UV: hue and saturation start
        src_pitch = src->GetPitch(PLANAR_U);
        BYTE * srcpu = src->GetWritePtr(PLANAR_U);
        BYTE * srcpv = src->GetWritePtr(PLANAR_V);
        row_size = src->GetRowSize(PLANAR_U);
        height = src->GetHeight(PLANAR_U);

        if (realcalc) {
            // no lookup, alway true for 16/32 bit, optional for 8 bit
            const double Hue = (dhue * PI) / 180.0;
            // 100% equals sat=119 (= maximal saturation of valid RGB (R=255,G=B=0)
            // 150% (=180) - 100% (=119) overshoot
            const double minSat = 1.19 * dminSat;
            const double maxSat = 1.19 * dmaxSat;

            double p = dinterp * 1.19; // Same units as minSat/maxSat

            int pixelsize = vi.ComponentSize();
            int width = row_size / pixelsize;
            double maxUV = coring ? 240.0f : 255.0f;
            double minUV = coring ? 16.0f : 0;
            double ditherval = 0.0;
            double u, v;
            double cosHue = cos(Hue);
            double sinHue = sin(Hue);

            // unique for each bit-depth, difference in the innermost loop (speed)
            if (pixelsize==1)
            {
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 2) & 0xC;
                    for (int x = 0; x < width; ++x) {
                        if (dither)
                            ditherval = ((double(ditherMap4[(x & 0x3) | _y]) - 7.5) / 16) / 256;
                        u = srcpu[x] / 256.0;
                        v = srcpv[x] / 256.0;

                        u = u + ditherval - 0.5; // going from 0..1 to +/-0.5
                        v = v + ditherval - 0.5;
                        double dWorkSat = dsat; // init from original param
                        ProcessPixelUnscaled(v, u, dstartHue, dendHue, dmaxSat, dminSat, p, dWorkSat);
                        double du = (u*cosHue + v*sinHue) * dWorkSat + 0.5f; // back to 0..1
                        double dv = (v*cosHue - u*sinHue) * dWorkSat + 0.5f;
                        srcpu[x] = (BYTE)clamp(du * 256.0, minUV, maxUV);
                        srcpv[x] = (BYTE)clamp(dv * 256.0, minUV, maxUV);
                    }
                    srcpu += src_pitch;
                    srcpv += src_pitch;
                }
            } else if (pixelsize==2) {
                maxUV *= 256;
                minUV *= 256;
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 2) & 0xC;
                    for (int x = 0; x < width; ++x) {

                        if (dither)
                            ditherval = ((double(ditherMap4[(x & 0x3) | _y]) - 7.5) / 16) / 256;
                        u = reinterpret_cast<uint16_t *>(srcpu)[x] / 65536.0f;
                        v = reinterpret_cast<uint16_t *>(srcpv)[x] / 65536.0f;

                        u = u + ditherval - 0.5; // going from 0..1 to +/-0.5
                        v = v + ditherval - 0.5;
                        double dWorkSat = dsat; // init from original param
                        ProcessPixelUnscaled(v, u, dstartHue, dendHue, dmaxSat, dminSat, p, dWorkSat);
                        double du = ((u*cosHue + v*sinHue) * dWorkSat) + 0.5; // back to 0..1
                        double dv = ((v*cosHue - u*sinHue) * dWorkSat) + 0.5;
                        reinterpret_cast<uint16_t *>(srcpu)[x] = (uint16_t)clamp(du * 65536.0, minUV, maxUV);
                        reinterpret_cast<uint16_t *>(srcpv)[x] = (uint16_t)clamp(dv * 65536.0, minUV, maxUV);
                    }
                    srcpu += src_pitch;
                    srcpv += src_pitch;
                }
            } else { // pixelsize==4 float
                maxUV /= 256;
                minUV /= 256;
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 2) & 0xC;
                    for (int x = 0; x < width; ++x) {
                        if (dither)
                            ditherval = ((double(ditherMap4[(x & 0x3) | _y]) - 7.5) / 16) / 256;
                        u = reinterpret_cast<float *>(srcpu)[x];
                        v = reinterpret_cast<float *>(srcpv)[x];

                        u = u + ditherval - 0.5; // going from 0..1 to +/-0.5
                        v = v + ditherval - 0.5;
                        double dWorkSat = dsat; // init from original param
                        ProcessPixelUnscaled(v, u, dstartHue, dendHue, dmaxSat, dminSat, p, dWorkSat);
                        double du = ((u*cosHue + v*sinHue) * dWorkSat) + 0.5; // back to 0..1
                        double dv = ((v*cosHue - u*sinHue) * dWorkSat) + 0.5;
                        reinterpret_cast<float *>(srcpu)[x] = (float)clamp(du/* * factor*/, minUV, maxUV);
                        reinterpret_cast<float *>(srcpv)[x] = (float)clamp(dv/* * factor*/, minUV, maxUV);
                    }
                    srcpu += src_pitch;
                    srcpv += src_pitch;
                }
            }
        }
        else {
            if (dither) {
                for (int y = 0; y < height; ++y) {
                    const int _y = (y << 2) & 0xC;
                    for (int x = 0; x < row_size; ++x) {
                        const int _dither = ditherMap4[(x & 0x3) | _y];
                        /* hue and saturation */
                        const int u = srcpu[x];
                        const int v = srcpv[x];
                        const int mapped = mapUV[(u << 12) | (v << 4) | _dither];
                        srcpu[x] = (BYTE)(mapped & 0xff);
                        srcpv[x] = (BYTE)(mapped >> 8);
                    }
                    srcpu += src_pitch;
                    srcpv += src_pitch;
                }
            }
            else {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < row_size; ++x) {
                        /* hue and saturation */
                        const int u = srcpu[x];
                        const int v = srcpv[x];
                        const int mapped = mapUV[(u << 8) | v];
                        srcpu[x] = (BYTE)(mapped & 0xff);
                        srcpv[x] = (BYTE)(mapped >> 8);
                    }
                    srcpu += src_pitch;
                    srcpv += src_pitch;
                }
            }
        }
    }

    return src;
}

AVSValue __cdecl Tweak::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new Tweak(args[0].AsClip(),
        args[1].AsDblDef(0.0),     // hue
        args[2].AsDblDef(1.0),     // sat
        args[3].AsDblDef(0.0),     // bright
        args[4].AsDblDef(1.0),     // cont
        args[5].AsBool(true),      // coring
        args[6].AsBool(false),     // sse
        args[7].AsDblDef(0.0),     // startHue
        args[8].AsDblDef(360.0),   // endHue
        args[9].AsDblDef(150.0),   // maxSat
        args[10].AsDblDef(0.0),    // minSat
        args[11].AsDblDef(16.0 / 1.19),// interp
        args[12].AsBool(false),    // dither
        args[13].AsBool(false),    // realcalc: force no-lookup (pure double calc/pixel) for 8 bit
        env);
}

/**********************
******   MaskHS   *****
**********************/

MaskHS::MaskHS(PClip _child, double startHue, double endHue, double _maxSat, double _minSat, bool coring,
    IScriptEnvironment* env)
    : GenericVideoFilter(_child)
{
    if (vi.IsRGB())
        env->ThrowError("MaskHS: YUV data only (no RGB)");

    if (vi.NumComponents() == 1) {
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
        const double destu = u - 128;
        for (int v = 0; v < 256; v++) {
            const double destv = v - 128;
            int iSat = 0; // won't be used in MaskHS; interpolation is skipped since p==0:
            if (ProcessPixel(destv, destu, startHue, endHue, maxSat, minSat, 0.0, iSat)) {
                mapY[(u << 8) | v] = maxY;
            }
            else {
                mapY[(u << 8) | v] = minY;
            }
        }
    }
    // #define MaskPointResizing
#ifndef MaskPointResizing
    vi.width >>= vi.GetPlaneWidthSubsampling(PLANAR_U);
    vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
#endif
    vi.pixel_type = VideoInfo::CS_Y8;
}



PVideoFrame __stdcall MaskHS::GetFrame(int n, IScriptEnvironment* env)
{
    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

    uint8_t* dstp = dst->GetWritePtr();
    int dst_pitch = dst->GetPitch();

    // show mask
    if (child->GetVideoInfo().IsYUY2()) {
        const uint8_t* srcp = src->GetReadPtr();
        const int src_pitch = src->GetPitch();
        const int height = src->GetHeight();

#ifndef MaskPointResizing
        const int row_size = src->GetRowSize() >> 2;

        for (int y = 0; y < height; y++) {
            for (int x = 0; x < row_size; x++) {
                dstp[x] = mapY[((srcp[x * 4 + 1]) << 8) | srcp[x * 4 + 3]];
            }
            srcp += src_pitch;
            dstp += dst_pitch;
        }
#else
        const int row_size = src->GetRowSize();

        for (int y = 0; y < height; y++) {
            for (int xs = 0, xd = 0; xs < row_size; xs += 4, xd += 2) {
                const BYTE mapped = mapY[((srcp[xs + 1]) << 8) | srcp[xs + 3]];
                dstp[xd] = mapped;
                dstp[xd + 1] = mapped;
            }
            srcp += src_pitch;
            dstp += dst_pitch;
        }
#endif
    }
    else if (child->GetVideoInfo().IsPlanar()) {
        const int srcu_pitch = src->GetPitch(PLANAR_U);
        const uint8_t* srcpu = src->GetReadPtr(PLANAR_U);
        const uint8_t* srcpv = src->GetReadPtr(PLANAR_V);
        const int row_sizeu = src->GetRowSize(PLANAR_U);
        const int heightu = src->GetHeight(PLANAR_U);

#ifndef MaskPointResizing
        for (int y = 0; y < heightu; ++y) {
            for (int x = 0; x < row_sizeu; ++x) {
                dstp[x] = mapY[((srcpu[x]) << 8) | srcpv[x]];
            }
            dstp += dst_pitch;
            srcpu += srcu_pitch;
            srcpv += srcu_pitch;
        }
#else
        const int swidth = child->GetVideoInfo().GetPlaneWidthSubsampling(PLANAR_U);
        const int sheight = child->GetVideoInfo().GetPlaneHeightSubsampling(PLANAR_U);
        const int sw = 1 << swidth;
        const int sh = 1 << sheight;

        const int dpitch = dst_pitch << sheight;
        for (int y = 0; y < heightu; ++y) {
            for (int x = 0; x < row_sizeu; ++x) {
                const BYTE mapped = mapY[((srcpu[x]) << 8) | srcpv[x]];
                const int sx = x << swidth;

                for (int lumv = 0; lumv < sh; ++lumv) {
                    const int sy = lumv*dst_pitch + sx;

                    for (int lumh = 0; lumh < sw; ++lumh) {
                        dstp[sy + lumh] = mapped;
                    }
                }
            }
            dstp += dpitch;
            srcpu += srcu_pitch;
            srcpv += srcu_pitch;
        }
#endif
    }
    return dst;
}



AVSValue __cdecl MaskHS::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
    return new MaskHS(args[0].AsClip(),
        args[1].AsDblDef(0.0),    // startHue
        args[2].AsDblDef(360.0),    // endHue
        args[3].AsDblDef(150.0),    // maxSat
        args[4].AsDblDef(0.0),    // minSat
        args[5].AsBool(false),      // coring
        env);
}

