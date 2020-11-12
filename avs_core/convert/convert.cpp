// Avisynth v2.5.  Copyright 2002-2009 Ben Rudiak-Gould et al.
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


#include "convert.h"
#include "convert_planar.h"
#include "convert_rgb.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include <avs/alignment.h>
#include <avs/minmax.h>
#include <avs/config.h>
#include <tuple>
#include <map>
#include <algorithm>

#ifdef AVS_WINDOWS
#include <avs/win.h>
#else
#include <avs/posix.h>
#endif

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Convert_filters[] = {       // matrix can be "rec601", "rec709", "PC.601" or "PC.709" or "rec2020"
  { "ConvertToRGB",   BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)0 },
  { "ConvertToRGB24", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)24 },
  { "ConvertToRGB32", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)32 },
  { "ConvertToRGB48", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)48 },
  { "ConvertToRGB64", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)64 },
  { "ConvertToPlanarRGB",  BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-1 },
  { "ConvertToPlanarRGBA", BUILTIN_FUNC_PREFIX, "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create, (void *)-2 },
  { "ConvertToY8",    BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY::Create, (void*)0 }, // user_data == 0 -> only 8 bit sources
  { "ConvertToYV12",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToYV12::Create, (void*)0 },
  { "ConvertToYV24",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444, (void*)0},
  { "ConvertToYV16",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV422, (void*)0},
  { "ConvertToYV411", BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411, (void*)0},
  { "ConvertToYUY2",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToYUY2::Create },
  { "ConvertBackToYUY2", BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertBackToYUY2::Create },
  { "ConvertToY",       BUILTIN_FUNC_PREFIX, "c[matrix]s", ConvertToY::Create, (void*)1 }, // user_data == 1 -> any bit depth sources
  { "ConvertToYUV411", BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411, (void*)1}, // alias for ConvertToYV411, 8 bit check later
  { "ConvertToYUV420",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToPlanarGeneric::CreateYUV420, (void*)1},
  { "ConvertToYUV422",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV422, (void*)1},
  { "ConvertToYUV444",  BUILTIN_FUNC_PREFIX, "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYUV444, (void*)1},
  { "ConvertTo8bit",  BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)8 },
  { "ConvertTo16bit", BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)16 },
  { "ConvertToFloat", BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)32 },
  { "ConvertBits",    BUILTIN_FUNC_PREFIX, "c[bits]i[truerange]b[dither]f[dither_bits]i[fulls]b[fulld]b", ConvertBits::Create, (void *)0 },
  { "AddAlphaPlane",  BUILTIN_FUNC_PREFIX, "c[mask].", AddAlphaPlane::Create},
  { "RemoveAlphaPlane",  BUILTIN_FUNC_PREFIX, "c", RemoveAlphaPlane::Create},
  { 0 }
};

// for YUY2
static const int crv_rec601 = int(1.596*65536+0.5);
static const int cgv_rec601 = int(0.813*65536+0.5);
static const int cgu_rec601 = int(0.391*65536+0.5);
static const int cbu_rec601 = int(2.018*65536+0.5);

static const int crv_rec709 = int(1.793*65536+0.5);
static const int cgv_rec709 = int(0.533*65536+0.5);
static const int cgu_rec709 = int(0.213*65536+0.5);
static const int cbu_rec709 = int(2.112*65536+0.5);

static const int crv_pc601 = int(1.407*65536+0.5);
static const int cgv_pc601 = int(0.717*65536+0.5);
static const int cgu_pc601 = int(0.345*65536+0.5);
static const int cbu_pc601 = int(1.779*65536+0.5);

static const int crv_pc709 = int(1.581*65536+0.5);
static const int cgv_pc709 = int(0.470*65536+0.5);
static const int cgu_pc709 = int(0.188*65536+0.5);
static const int cbu_pc709 = int(1.863*65536+0.5);

static const int cy_rec = int((255.0/219.0)*65536+0.5);
static const int cy_pc = 65536;

// still YUY2 only
static const int crv_values[4] = { crv_rec601, crv_rec709, crv_pc601, crv_pc709 };
static const int cgv_values[4] = { cgv_rec601, cgv_rec709, cgv_pc601, cgv_pc709 };
static const int cgu_values[4] = { cgu_rec601, cgu_rec709, cgu_pc601, cgu_pc709 };
static const int cbu_values[4] = { cbu_rec601, cbu_rec709, cbu_pc601, cbu_pc709 };
static const int cy_values[4]  = { cy_rec,     cy_rec,     cy_pc,     cy_pc};


int getMatrix( const char* matrix, IScriptEnvironment* env) {
  if (matrix) {
    if (!lstrcmpi(matrix, "rec601"))
      return Rec601;
    if (!lstrcmpi(matrix, "rec709"))
      return Rec709;
    if (!lstrcmpi(matrix, "PC.601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC.709"))
      return PC_709;
    if (!lstrcmpi(matrix, "PC601"))
      return PC_601;
    if (!lstrcmpi(matrix, "PC709"))
      return PC_709;
    if (!lstrcmpi(matrix, "AVERAGE"))
      return AVERAGE;
    if (!lstrcmpi(matrix, "rec2020"))
      return Rec2020;
    env->ThrowError("Convert: Unknown colormatrix");
  }
  return Rec601; // Default colorspace conversion for AviSynth
}


/****************************************
*******   Convert to RGB / RGBA   ******
***************************************/

// YUY2 only
ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix,
                           IScriptEnvironment* env )
                           : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
  // no rec2020 here
  if (matrix) {
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "PC709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("ConvertToRGB: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"PC.601\" or \"PC.709\")");
  }
  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;
}

template<int rgb_size>
static void convert_yuy2_to_rgb_c(const BYTE *srcp, BYTE* dstp, int src_pitch, int dst_pitch, int height, int width, int crv, int cgv, int cgu, int cbu, int cy, int tv_scale) {
  srcp += height * src_pitch;
  for (int y = height; y > 0; --y) {
    srcp -= src_pitch;
    int x;
    for (x = 0; x < width-2; x+=2) {
      int scaled_y0 = (srcp[x*2+0] - tv_scale) * cy;
      int u0 = srcp[x*2+1]-128;
      int v0 = srcp[x*2+3]-128;
      int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
      int u1 = srcp[x*2+5]-128;
      int v1 = srcp[x*2+7]-128;

      dstp[x*rgb_size + 0] = ScaledPixelClip(scaled_y0 + u0 * cbu);                 // blue
      dstp[x*rgb_size + 1] = ScaledPixelClip(scaled_y0 - u0 * cgu - v0 * cgv); // green
      dstp[x*rgb_size + 2] = ScaledPixelClip(scaled_y0            + v0 * crv); // red

      dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + (u0+u1) * (cbu / 2));                     // blue
      dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - (u0+u1) * (cgu / 2) - (v0+v1) * (cgv/2)); // green
      dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1                       + (v0+v1) * (crv/2)); // red

      if constexpr(rgb_size == 4) {
        dstp[x*4+3] = 255;
        dstp[x*4+7] = 255;
      }
    }

    int scaled_y0 = (srcp[x*2+0] - tv_scale) * cy;
    int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
    int u = srcp[x*2+1]-128;
    int v = srcp[x*2+3]-128;

    dstp[x*rgb_size + 0]     = ScaledPixelClip(scaled_y0 + u * cbu);                 // blue
    dstp[x*rgb_size + 1]     = ScaledPixelClip(scaled_y0 - u * cgu - v * cgv); // green
    dstp[x*rgb_size + 2]     = ScaledPixelClip(scaled_y0           + v * crv); // red

    dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + u * cbu);                 // blue
    dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - u * cgu - v * cgv); // green
    dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1           + v * crv); // red

    if constexpr(rgb_size == 4) {
      dstp[x*4+3] = 255;
      dstp[x*4+7] = 255;
    }
    dstp += dst_pitch;
  }
}

// YUY2 only
PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  const int dst_pitch = dst->GetPitch();
  BYTE* dstp = dst->GetWritePtr();
  int tv_scale = theMatrix == Rec601 || theMatrix == Rec709 ? 16 : 0;


  {
    if (vi.IsRGB32()) {
      convert_yuy2_to_rgb_c<4>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    } else {
      convert_yuy2_to_rgb_c<3>(srcp, dstp, src_pitch, dst_pitch, vi.height, vi.width,
        crv_values[theMatrix], cgv_values[theMatrix], cgu_values[theMatrix], cbu_values[theMatrix], cy_values[theMatrix], tv_scale);
    }
  }
  return dst;
}

// general for all colorspaces
// however class is constructed only for YUY2 input
AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  VideoInfo vi = clip->GetVideoInfo();

  // common Create for all CreateRGB24/32/48/64/Planar(RGBP:-1, RGPAP:-2) using user_data
  int target_rgbtype = (int)reinterpret_cast<intptr_t>(user_data);
  // -1,-2: Planar RGB(A)
  //  0: not specified (leave if input is packed RGB, convert to rgb32/64 input colorspace dependent)
  // 24,32,48,64: RGB24/32/48/64

  // planar YUV-like
  if (vi.IsPlanar() && (vi.IsYUV() || vi.IsYUVA())) {
    bool needConvertFinalBitdepth = false;
    int finalBitdepth = -1;

    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    // conversion to planar or packed RGB is always from 444
    clip = ConvertToPlanarGeneric::CreateYUV444(AVSValue(new_args, 5), (void *)1, env).AsClip(); // (void *)1: not restricted to 8 bits
    if ((target_rgbtype == 24 || target_rgbtype == 32)) {
      if (vi.BitsPerComponent() != 8) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 8;
        target_rgbtype = (target_rgbtype == 24) ? -1 : -2; // planar rgb intermediate
      }
    }
    else if ((target_rgbtype == 48 || target_rgbtype == 64)) {
      if (vi.BitsPerComponent() != 16) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 16;
        target_rgbtype = (target_rgbtype == 48) ? -1 : -2; // planar rgb intermediate
      }
    }
    else if(target_rgbtype==0 && vi.ComponentSize()==4)
        env->ThrowError("ConvertToRGB: conversion is allowed only from 8 or 16 bit colorspaces");
    int rgbtype_param;
    bool reallyConvert = true;
    switch (target_rgbtype)
    {
    case -1: case -2:
        rgbtype_param = target_rgbtype; break; // planar RGB(A)
    case 0:
        rgbtype_param = vi.ComponentSize() == 1 ? 4 : 8; break; // input bitdepth adaptive
    case 24:
        rgbtype_param = 3; break; // RGB24
    case 32:
        rgbtype_param = 4; break; // RGB32
    case 48: {
            // instead of C code of YUV444P16->RGB48
            // we convert to PlanarRGB then to RGB48 (both is fast)
          AVSValue new_args2[5] = { clip, args[1], args[2], args[3], args[4] };
          clip = ConvertToRGB::Create(AVSValue(new_args2, 5), (void *)-1, env).AsClip();
          vi = clip->GetVideoInfo();
          reallyConvert = false;
          rgbtype_param = 6; // old option RGB48 target, slow C
        }
        break; // RGB48
    case 64: {
        // instead of C code of YUV(A)444P16->RGB64
        // we convert to PlanarRGB(A) then to RGB64 (both is fast)
        AVSValue new_args2[5] = { clip, args[1], args[2], args[3], args[4] };
        clip = ConvertToRGB::Create(AVSValue(new_args2, 5), vi.IsYUVA() ? (void *)-2 : (void *)-1, env).AsClip();
        vi = clip->GetVideoInfo();
        reallyConvert = false;
        rgbtype_param = 8; // old option RGB64 target, slow C
      }
      break; // RGB64
    }
    if (reallyConvert) {
      clip = new ConvertYUV444ToRGB(clip, getMatrix(matrix, env), rgbtype_param, env);

      if (needConvertFinalBitdepth) {
        // from any planar rgb(a) -> rgb24/32/48/64
        clip = new ConvertBits(clip, -1 /*dither_type*/, finalBitdepth /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
        vi = clip->GetVideoInfo();

        // source here is always a 8/16bit planar RGB(A), finally it has to be converted to RGB24/32/48/64
        const bool isRGBA = target_rgbtype == -2;
        clip = new PlanarRGBtoPackedRGB(clip, isRGBA);
        vi = clip->GetVideoInfo();
      }
      return clip;
    }
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB: ChromaPlacement and ChromaResample options are not supported.");

  // planar RGB-like source
  if (vi.IsPlanarRGB() || vi.IsPlanarRGBA())
  {
    if (target_rgbtype < 0) // planar to planar
    {
      if (vi.IsPlanarRGB()) {
        if (target_rgbtype == -1)
          return clip;
        // prgb->prgba create with default alpha
        return new AddAlphaPlane(clip, nullptr, 0.0f, false, env);
      }
      // planar rgba source
      if (target_rgbtype == -2)
        return clip;
      return new RemoveAlphaPlane(clip, env);
    }

    // planar to packed 24/32/48/64
    bool needConvertFinalBitdepth = false;
    int finalBitdepth = -1;

    if (target_rgbtype == 24 || target_rgbtype == 32) {
      if (vi.BitsPerComponent() != 8) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 8;
      }
    }
    else if (target_rgbtype == 48 || target_rgbtype == 64) {
      if (vi.BitsPerComponent() != 16) {
        needConvertFinalBitdepth = true;
        finalBitdepth = 16;
      }
    }

    if (needConvertFinalBitdepth) {
      // from any bitdepth planar rgb(a) -> 8/16 bits
      clip = new ConvertBits(clip, -1 /*dither_type*/, finalBitdepth /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo();
    }

    return new PlanarRGBtoPackedRGB(clip, (target_rgbtype == 32 || target_rgbtype == 64));
  }

  // YUY2
  if (vi.IsYUV()) // at this point IsYUV means YUY2 (non-planar)
  {
    if (target_rgbtype == 48 || target_rgbtype == 64)
      env->ThrowError("ConvertToRGB: conversion from YUY2 is allowed only to 8 bits");
    if (target_rgbtype < 0) {
      // rgb32 intermediate is faster
      clip = new ConvertToRGB(clip, false, matrix, env); // YUY2->RGB32
      return new PackedRGBtoPlanarRGB(clip, true, target_rgbtype == -2);
    }
    else
      return new ConvertToRGB(clip, target_rgbtype == 24, matrix, env);
  }

  // conversions from packed RGB

  if (target_rgbtype == 24 || target_rgbtype == 32) {
    if (vi.ComponentSize() != 1) {
      // 64->32, 48->24
      clip = new ConvertBits(clip, -1 /*dither_type*/, 8 /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo(); // new format
    }
  }
  else if (target_rgbtype == 48 || target_rgbtype == 64) {
    if (vi.ComponentSize() != 2) {
      // 32->64, 24->48
      clip = new ConvertBits(clip, -1 /*dither_type*/, 16 /*target_bitdepth*/, true /*assume_truerange*/, true /*fulls*/, true /*fulld*/, 8 /*n/a dither_bitdepth*/, env);
      vi = clip->GetVideoInfo(); // new format
    }
  }

  if(target_rgbtype==32 || target_rgbtype==64)
      if (vi.IsRGB24() || vi.IsRGB48())
          return new RGBtoRGBA(clip); // 24->32 or 48->64

  if(target_rgbtype==24 || target_rgbtype==48)
      if (vi.IsRGB32() || vi.IsRGB64())
          return new RGBAtoRGB(clip); // 32->24 or 64->48

  // <0: target is planar RGB(A)
  if (target_rgbtype < 0) {
    // RGB24/32/48/64 ->
    const bool isSrcRGBA = vi.IsRGB32() || vi.IsRGB64();
    const bool isTargetRGBA = target_rgbtype == -2;
    return new PackedRGBtoPlanarRGB(clip, isSrcRGBA, isTargetRGBA);
  }

  return clip;
}


/**********************************
*******   Convert to YV12   ******
*********************************/

// for YUY2->YV12 only
// all other sources use ConvertToPlanarGeneric
ConvertToYV12::ConvertToYV12(PClip _child, bool _interlaced, IScriptEnvironment* env)
  : GenericVideoFilter(_child),
  interlaced(_interlaced)
{
  if (vi.width & 1)
    env->ThrowError("ConvertToYV12: Image width must be multiple of 2");

  if (interlaced && (vi.height & 3))
    env->ThrowError("ConvertToYV12: Interlaced image height must be multiple of 4");

  if ((!interlaced) && (vi.height & 1))
    env->ThrowError("ConvertToYV12: Image height must be multiple of 2");

  if (!vi.IsYUY2())
    env->ThrowError("ConvertToYV12: Source must be YUY2.");

  vi.pixel_type = VideoInfo::CS_YV12;

#ifdef INTEL_INTRINSICS
  if ((env->GetCPUFlags() & CPUF_MMX) == 0)
    env->ThrowError("ConvertToYV12: YV12 support require a MMX capable processor.");
#endif
}

PVideoFrame __stdcall ConvertToYV12::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);

  if (interlaced) {
      {
        convert_yuy2_to_yv12_interlaced_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
  }
  else
  {
      {
        convert_yuy2_to_yv12_progressive_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
      }
  }

  return dst;

}

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();
  bool only_8bit = reinterpret_cast<intptr_t>(user_data) == 0;
  if (only_8bit && vi.BitsPerComponent() != 8)
    env->ThrowError("ConvertToYV12: only 8 bit sources allowed");

  if (vi.IsYUY2() && !args[3].Defined() && !args[4].Defined() && !args[5].Defined())  // User has not requested options, do it fast!
    return new ConvertToYV12(clip,args[1].AsBool(false),env);

  return ConvertToPlanarGeneric::CreateYUV420(args, NULL,env);
}

/**********************************
******  Bitdepth conversions  *****
**********************************/
// 10->8
// repeated 4x for sse size 16
static const struct dither2x2_t
{
  const BYTE data[4] = {
    0, 2,
    3, 1,
  };
  // cycle: 2
  alignas(16) const BYTE data_sse2[2 * 16] = {
    0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
    3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1
  };
  dither2x2_t() {};
} dither2x2;


// 12->8
static const struct dither4x4_t
{
  const BYTE data[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
  };
  // cycle: 4
  alignas(16) const BYTE data_sse2[4 * 16] = {
     0,  8,  2, 10,  0,  8,  2, 10,  0,  8,  2, 10,  0,  8,  2, 10,
    12,  4, 14,  6, 12,  4, 14,  6, 12,  4, 14,  6, 12,  4, 14,  6,
     3, 11,  1,  9,  3, 11,  1,  9,  3, 11,  1,  9,  3, 11,  1,  9,
    15,  7, 13,  5, 15,  7, 13,  5, 15,  7, 13,  5, 15,  7, 13,  5
  };
  dither4x4_t() {};
} dither4x4;

// 14->8
static const struct dither8x8_t
{
  const BYTE data[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42}, /* 8x8 Bayer ordered dithering */
    {48, 16, 56, 24, 50, 18, 58, 26}, /* pattern. Each input pixel */
    {12, 44,  4, 36, 14, 46,  6, 38}, /* is scaled to the 0..63 range */
    {60, 28, 52, 20, 62, 30, 54, 22}, /* before looking in this table */
    { 3, 35, 11, 43,  1, 33,  9, 41}, /* to determine the action. */
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
  };
  // cycle: 8
  alignas(16) const BYTE data_sse2[8][16] = {
    {  0, 32,  8, 40,  2, 34, 10, 42,  0, 32,  8, 40,  2, 34, 10, 42 }, /* 8x8 Bayer ordered dithering */
    { 48, 16, 56, 24, 50, 18, 58, 26, 48, 16, 56, 24, 50, 18, 58, 26 }, /* pattern. Each input pixel */
    { 12, 44,  4, 36, 14, 46,  6, 38, 12, 44,  4, 36, 14, 46,  6, 38 }, /* is scaled to the 0..63 range */
    { 60, 28, 52, 20, 62, 30, 54, 22, 60, 28, 52, 20, 62, 30, 54, 22 }, /* before looking in this table */
    {  3, 35, 11, 43,  1, 33,  9, 41,  3, 35, 11, 43,  1, 33,  9, 41 }, /* to determine the action. */
    { 51, 19, 59, 27, 49, 17, 57, 25, 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47,  7, 39, 13, 45,  5, 37, 15, 47,  7, 39, 13, 45,  5, 37 },
    { 63, 31, 55, 23, 61, 29, 53, 21, 63, 31, 55, 23, 61, 29, 53, 21 }
  };
  dither8x8_t() {};
} dither8x8;

// 16->8
static const struct dither16x16_t
{
  // cycle: 16x
  alignas(16) const BYTE data[16][16] = {
    {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
    { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
    {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
    { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
    {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
    { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
    {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
    { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
    {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
    { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
    {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
    { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
    {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
    { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
    {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
    { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
  };
  dither16x16_t() {};
} dither16x16;

template<uint8_t sourcebits, int dither_mode, int TARGET_DITHER_BITDEPTH, int rgb_step>
static void convert_rgb_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = 8; // here is constant (uint8_t target)

  // for test, make it 2,4,6,8. sourcebits-TARGET_DITHER_BITDEPTH cannot exceed 8 bit
  // const int TARGET_DITHER_BITDEPTH = 2;

  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits-TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for(int y=0; y<src_height; y++)
  {
    if constexpr(dither_mode == 0)
      _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      if constexpr(dither_mode < 0) // -1: no dither
      {
        const float mulfactor = sourcebits == 16 ? (1.0f / 257.0f) :
          sourcebits == 14 ? (255.0f / 16383.0f) :
          sourcebits == 12 ? (255.0f / 4095.0f) :
          (255.0f / 1023.0f); // 10 bits

        dstp[x] = (uint8_t)(srcp0[x] * mulfactor + 0.5f);
        // C cast truncates, use +0.5f rounder, which uses cvttss2si

        // old method: no rounding but fast
        // no integer division (fast tricky algorithm by compiler), rounding problems, pic gets darker
        // dstp[x] = srcp0[x] / 257; // RGB: full range 0..255 <-> 0..65535 (*255 / 65535)
        // dstp[x] = srcp0[x] * 255 / 16383; // RGB: full range 0..255 <-> 0..16384-1
        // dstp[x] = srcp0[x] * 255 / 4095; // RGB: full range 0..255 <-> 0..4096-1
        // dstp[x] = srcp0[x] * 255 / 1023; // RGB: full range 0..255 <-> 0..1024-1
      }
      else { // dither_mode == 0 -> ordered dither
        const int corr = matrix[_y | ((x / rgb_step) & MASK)];
        // vvv for the non-fullscale version: int new_pixel = ((srcp0[x] + corr) >> DITHER_BIT_DIFF);
        int new_pixel;

        const float mulfactor =
          DITHER_BIT_DIFF == 8 ? (1.0f / 257.0f) :
          DITHER_BIT_DIFF == 6 ? (255.0f / 16383.0f) :
          DITHER_BIT_DIFF == 4 ? (255.0f / 4095.0f) :
          DITHER_BIT_DIFF == 2 ? (255.0f / 1023.0f) : // 10 bits
          1.0f;

        if constexpr(TARGET_DITHER_BITDEPTH <= 4)
          new_pixel = (uint16_t)((srcp0[x] + corr) * mulfactor); // rounding here makes brightness shift
        else if constexpr(DITHER_BIT_DIFF > 0)
          new_pixel = (uint16_t)((srcp0[x] + corr) * mulfactor + 0.5f);
        else
          new_pixel = (uint16_t)(srcp0[x] + corr);

        new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper

        // scale back to the required bit depth
        // for generality. Now target == 8 bit, and dither_target is also 8 bit
        // for test: source:10 bit, target=8 bit, dither_target=4 bit
        const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
        if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
          new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // if implemented non-8bit dither target, this should be fullscale
        dstp[x] = (BYTE)new_pixel;
      }
    } // x
    dstp += dst_pitch;
    srcp0 += src_pitch;
  }
}

// idea borrowed from fmtConv
#define FS_OPTIMIZED_SERPENTINE_COEF

template<int direction>
static AVS_FORCEINLINE void diffuse_floyd(int err, int &nextError, int *error_ptr)
{
#if defined (FS_OPTIMIZED_SERPENTINE_COEF)
  const int      e1 = 0;
  const int      e3 = (err * 4 + 8) >> 4;
#else
  const int      e1 = (err + 8) >> 4;
  const int      e3 = (err * 3 + 8) >> 4;
#endif
  const int      e5 = (err * 5 + 8) >> 4;
  const int      e7 = err - e1 - e3 - e5;

  nextError = error_ptr[direction];
  error_ptr[-direction] += e3;
  error_ptr[0] += e5;
  error_ptr[direction] = e1;
  nextError += e7;
}

template<int direction>
static void diffuse_floyd_f(float err, float &nextError, float *error_ptr)
{
#if defined (FS_OPTIMIZED_SERPENTINE_COEF)
  const float    e1 = 0;
  const float    e3 = err * (4.0f / 16);
#else
  const float    e1 = err * (1.0f / 16);
  const float    e3 = err * (3.0f / 16);
#endif
  const float    e5 = err * (5.0f / 16);
  const float    e7 = err * (7.0f / 16);

  nextError = error_ptr[direction];
  error_ptr[-direction] += e3;
  error_ptr[0] += e5;
  error_ptr[direction] = e1;
  nextError += e7;
}

template<typename source_pixel_t, typename target_pixel_t, uint8_t sourcebits, uint8_t TARGET_BITDEPTH, int TARGET_DITHER_BITDEPTH>
static void convert_uint_floyd_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const source_pixel_t *srcp = reinterpret_cast<const source_pixel_t *>(srcp8);
  src_pitch = src_pitch / sizeof(source_pixel_t);
  int src_width = src_rowsize / sizeof(source_pixel_t);

  target_pixel_t *dstp = reinterpret_cast<target_pixel_t *>(dstp8);
  dst_pitch = dst_pitch / sizeof(target_pixel_t);

  const int max_pixel_value = (1 << TARGET_BITDEPTH) - 1;
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);

  int *error_ptr_safe = new int[1 + src_width + 1]; // accumulated errors
  std::fill_n(error_ptr_safe, src_width + 2, 0);

  int *error_ptr = error_ptr_safe + 1;

  const int INTERNAL_BITS = DITHER_BIT_DIFF < 6 ? sourcebits+8 : sourcebits; // keep accuracy
  const int SHIFTBITS_TO_INTERNAL = INTERNAL_BITS - sourcebits;
  const int SHIFTBITS_FROM_INTERNAL = INTERNAL_BITS - TARGET_DITHER_BITDEPTH;
  const int ROUNDER = 1 << (SHIFTBITS_FROM_INTERNAL - 1); // rounding

  for (int y = 0; y < src_height; y++)
  {
    int nextError = error_ptr[0];
    // serpentine forward
    if ((y & 1) == 0)
    {
      for (int x = 0; x < src_width; x++)
      {
        int err = nextError;
        int new_pixel = srcp[x] << SHIFTBITS_TO_INTERNAL; // if necessary
        int sum = new_pixel + err;
        int quantized = (sum + ROUNDER) >> (SHIFTBITS_FROM_INTERNAL);
        err = sum - (quantized << SHIFTBITS_FROM_INTERNAL);
        quantized <<= BITDIFF_BETWEEN_DITHER_AND_TARGET;
        int pix = max(min(max_pixel_value, quantized), 0); // clamp to target bit
        dstp[x] = (target_pixel_t)pix;
        diffuse_floyd<1>(err, nextError, error_ptr+x);
      }
    }
    else {
      // serpentine backward
      for (int x = src_width - 1; x >= 0; --x)
      {
        int err = nextError;
        int new_pixel = srcp[x] << SHIFTBITS_TO_INTERNAL; // if necessary
        int sum = new_pixel + err;
        int quantized = (sum + ROUNDER) >> (SHIFTBITS_FROM_INTERNAL);
        err = sum - (quantized << SHIFTBITS_FROM_INTERNAL);
        quantized <<= BITDIFF_BETWEEN_DITHER_AND_TARGET;
        int pix = max(min(max_pixel_value, quantized), 0); // clamp to target bit
        dstp[x] = (target_pixel_t)pix;
        diffuse_floyd<-1>(err, nextError, error_ptr + x);
      }
    }
    error_ptr[0] = nextError;
    dstp += dst_pitch;
    srcp += src_pitch;
  }

  delete[] error_ptr_safe;
}


// YUV conversions (bit shifts)
// BitDepthConvFuncPtr
// Conversion from 16-14-12-10 to 8 bits (bitshift: 8-6-4-2)
// both dither and non-dither
template<uint8_t sourcebits, int dither_mode, int TARGET_DITHER_BITDEPTH>
static void convert_uint16_to_8_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
  src_pitch = src_pitch / sizeof(uint16_t);
  int src_width = src_rowsize / sizeof(uint16_t);

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = 8; // here is constant (uint8_t target)
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits-TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for(int y=0; y<src_height; y++)
  {
    if constexpr(dither_mode == 0) _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      if constexpr (dither_mode < 0) { // -1: no dither
        constexpr auto round = 1 << (sourcebits - TARGET_BITDEPTH - 1);
        dstp[x] = (srcp0[x] + round) >> (sourcebits - TARGET_BITDEPTH); // no dithering, no range conversion, simply shift
      }
      else { // dither_mode == 0 -> ordered dither
        int corr = matrix[_y | (x & MASK)];
        //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
        int new_pixel = ((srcp0[x] + corr) >> DITHER_BIT_DIFF);
        new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper
        // scale back to the required bit depth
        // for generality. Now target == 8 bit, and dither_target is also 8 bit
        // for test: source:10 bit, target=8 bit, dither_target=4 bit
        const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
        if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
          new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // closest in palette: simple shift with
        dstp[x] = (BYTE)new_pixel;
      }
    }
    dstp += dst_pitch;
    srcp0 += src_pitch;
  }
}

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4305 4309)
#endif

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// float to 8 bit, float to 10/12/14/16 bit
template<typename pixel_t, uint8_t targetbits, bool chroma, bool fulls, bool fulld>
static void convert_32_to_uintN_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const float *srcp0 = reinterpret_cast<const float *>(srcp);
  pixel_t *dstp0 = reinterpret_cast<pixel_t *>(dstp);

  src_pitch = src_pitch / sizeof(float);
  dst_pitch = dst_pitch / sizeof(pixel_t);

  int src_width = src_rowsize / sizeof(float);

  constexpr float max_dst_pixelvalue = (float)((1<<targetbits) - 1); // 255, 1023, 4095, 16383, 65535.0
  constexpr float half = (float)(1 << (targetbits - 1));

  constexpr int limit_lo_d = (fulld ? 0 : 16) << (targetbits - 8);
  constexpr int limit_hi_d = fulld ? ((1 << targetbits) - 1) : ((chroma ? 240 : 235) << (targetbits - 8));
  constexpr float range_diff_d = (float)limit_hi_d - limit_lo_d;

  constexpr int limit_lo_s = fulls ? 0 : 16;
  constexpr float limit_lo_s_ps = limit_lo_s / 255.0f;
  constexpr int limit_hi_s = fulls ? 255 : (chroma ? 240 : 235);
  constexpr float range_diff_s = (limit_hi_s - limit_lo_s) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240       Y = Y * ((235-16) << (bpp-8)) + 16, Chroma= Chroma * ((240-16) << (bpp-8)) + 16
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255
  constexpr float factor = range_diff_d / range_diff_s;

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel;
      if constexpr(chroma) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
        // shift 0.5 before, shift back half_int after. 0.5->exact half of 128/512/...
        pixel = (srcp0[x] - 0.5f);
#else
        pixel = srcp0[x];
#endif
        pixel = pixel * factor + half + 0.5f;
      }
      else {
        if constexpr(!fulls)
          pixel = (srcp0[x] - limit_lo_s_ps) * factor + 0.5f + limit_lo_d;
        else
          pixel = srcp0[x] * factor + 0.5f + limit_lo_d;
      }
      dstp0[x] = pixel_t(clamp(pixel, 0.0f, max_dst_pixelvalue)); // we clamp here!
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
}

// rgb/alpha: full scale. No bit shift, scale full ranges
template<uint8_t targetbits>
static void convert_rgb_8_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
    const uint8_t *srcp0 = reinterpret_cast<const uint8_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint8_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    int src_width = src_rowsize / sizeof(uint8_t);

    constexpr int MUL = (targetbits == 16) ? 257 : ((1 << targetbits) - 1);
    constexpr int DIV = (targetbits == 16) ? 1 : 255;

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
          if constexpr (targetbits == 16) {
            dstp0[x] = (uint16_t)(srcp0[x] * MUL / DIV); // MUL is 257, DIV is 1
          }
          else {
            constexpr float mul_factor = (float)MUL / DIV;
            dstp0[x] = (uint16_t)(srcp0[x] * mul_factor + 0.5f); // RGB: full range 0..255 <-> 0..16384-1
          }
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

// YUV: bit shift 8 to 10-12-14-16 bits
template<uint8_t targetbits>
static void convert_8_to_uint16_c(const BYTE *srcp, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  dst_pitch = dst_pitch / sizeof(uint16_t);

  int src_width = src_rowsize / sizeof(uint8_t); // intentional

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
        dstp[x] = srcp[x] << (targetbits-8);
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

// RGB full range: 10-12-14-16 <=> 10-12-14-16 bits
template<uint8_t sourcebits, uint8_t targetbits>
static void convert_rgb_uint16_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint16_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    const int src_width = src_rowsize / sizeof(uint16_t);

    constexpr int source_max = (1 << sourcebits) - 1;
    constexpr int target_max = (1 << targetbits) - 1;

    constexpr float factor1 = (float)target_max / source_max;

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
          dstp0[x] = (uint16_t)std::min((int)((float)srcp0[x] * factor1 + 0.5f), target_max);
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

template<uint8_t sourcebits, uint8_t targetbits, int TARGET_DITHER_BITDEPTH>
static void convert_rgb_uint16_to_uint16_dither_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const uint16_t *srcp = reinterpret_cast<const uint16_t *>(srcp8);
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  src_pitch = src_pitch / sizeof(uint16_t);
  dst_pitch = dst_pitch / sizeof(uint16_t);

  const int src_width = src_rowsize / sizeof(uint16_t);

  const int source_max = (1 << sourcebits) - 1;

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = targetbits;
  const int max_pixel_value = (1 << TARGET_BITDEPTH) - 1;
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits - TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for (int y = 0; y < src_height; y++)
  {
    _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      int corr = matrix[_y | (x & MASK)];
      //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
      //int new_pixel = ((srcp[x] + corr) >> DITHER_BIT_DIFF);
      int64_t new_pixel = (int64_t)(srcp[x] + corr) * max_pixel_value_dithered / source_max;

      // new_pixel = min(new_pixel, max_pixel_value_dithered_i); // clamp upper
      // scale back to the required bit depth
      const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
      if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0) {
        new_pixel = new_pixel * max_pixel_value / max_pixel_value_dithered;
      }
      dstp[x] = (uint16_t)(min((int)new_pixel, max_pixel_value));
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }

}

// YUV: bit shift 10-12-14-16 <=> 10-12-14-16 bits
// shift right or left, depending on expandrange template param
template<bool expandrange, uint8_t shiftbits>
static void convert_uint16_to_uint16_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
    const uint16_t *srcp0 = reinterpret_cast<const uint16_t *>(srcp);
    uint16_t *dstp0 = reinterpret_cast<uint16_t *>(dstp);

    src_pitch = src_pitch / sizeof(uint16_t);
    dst_pitch = dst_pitch / sizeof(uint16_t);

    const int src_width = src_rowsize / sizeof(uint16_t);

    for(int y=0; y<src_height; y++)
    {
        for (int x = 0; x < src_width; x++)
        {
            if(expandrange)
                dstp0[x] = srcp0[x] << shiftbits;  // expand range. No clamp before, source is assumed to have valid range
            else {
              constexpr auto round = 1 << (shiftbits - 1);
              dstp0[x] = (srcp0[x] + round) >> shiftbits;  // reduce range
            }
        }
        dstp0 += dst_pitch;
        srcp0 += src_pitch;
    }
}

// YUV: bit shift 10-12-14-16 <=> 10-12-14-16 bits
// shift right or left, depending on expandrange template param
template<uint8_t sourcebits, uint8_t targetbits, int TARGET_DITHER_BITDEPTH>
static void convert_uint16_to_uint16_dither_c(const BYTE *srcp8, BYTE *dstp8, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const uint16_t *srcp = reinterpret_cast<const uint16_t *>(srcp8);
  uint16_t *dstp = reinterpret_cast<uint16_t *>(dstp8);

  src_pitch = src_pitch / sizeof(uint16_t);
  dst_pitch = dst_pitch / sizeof(uint16_t);

  const int src_width = src_rowsize / sizeof(uint16_t);

  int _y = 0; // for ordered dither

  const int TARGET_BITDEPTH = targetbits;
  const int max_pixel_value_dithered = (1 << TARGET_DITHER_BITDEPTH) - 1;
  // precheck ensures:
  // TARGET_BITDEPTH >= TARGET_DITHER_BITDEPTH
  // sourcebits - TARGET_DITHER_BITDEPTH <= 8
  // sourcebits - TARGET_DITHER_BITDEPTH is even (later we can use PRESHIFT)
  const int DITHER_BIT_DIFF = (sourcebits - TARGET_DITHER_BITDEPTH); // 2, 4, 6, 8
  const int PRESHIFT = DITHER_BIT_DIFF & 1;  // 0 or 1: correction for odd bit differences (not used here but generality)
  const int DITHER_ORDER = (DITHER_BIT_DIFF + PRESHIFT) / 2;
  const int DITHER_SIZE = 1 << DITHER_ORDER; // 9,10=2  11,12=4  13,14=8  15,16=16
  const int MASK = DITHER_SIZE - 1;
  // 10->8: 0x01 (2x2)
  // 11->8: 0x03 (4x4)
  // 12->8: 0x03 (4x4)
  // 14->8: 0x07 (8x8)
  // 16->8: 0x0F (16x16)
  const BYTE *matrix;
  switch (sourcebits - TARGET_DITHER_BITDEPTH) {
  case 2: matrix = reinterpret_cast<const BYTE *>(dither2x2.data); break;
  case 4: matrix = reinterpret_cast<const BYTE *>(dither4x4.data); break;
  case 6: matrix = reinterpret_cast<const BYTE *>(dither8x8.data); break;
  case 8: matrix = reinterpret_cast<const BYTE *>(dither16x16.data); break;
  default: return; // n/a
  }

  for (int y = 0; y < src_height; y++)
  {
    _y = (y & MASK) << DITHER_ORDER; // ordered dither
    for (int x = 0; x < src_width; x++)
    {
      int corr = matrix[_y | (x & MASK)];
      //BYTE new_pixel = (((srcp0[x] << PRESHIFT) >> (sourcebits - 8)) + corr) >> PRESHIFT; // >> (sourcebits - 8);
      int new_pixel = ((srcp[x] + corr) >> DITHER_BIT_DIFF);
      new_pixel = min(new_pixel, max_pixel_value_dithered); // clamp upper
      // scale back to the required bit depth
      // for generality. Now target == 8 bit, and dither_target is also 8 bit
      // for test: source:10 bit, target=8 bit, dither_target=4 bit
      const int BITDIFF_BETWEEN_DITHER_AND_TARGET = DITHER_BIT_DIFF - (sourcebits - TARGET_BITDEPTH);
      if constexpr(BITDIFF_BETWEEN_DITHER_AND_TARGET != 0)  // dither to 8, target to 8
        new_pixel = new_pixel << BITDIFF_BETWEEN_DITHER_AND_TARGET; // closest in palette: simple shift with
      dstp[x] = (uint16_t)new_pixel;
    }
    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

// 8 bit to float, 16/14/12/10 bits to float
template<typename pixel_t, uint8_t sourcebits, bool chroma, bool fulls, bool fulld>
static void convert_uintN_to_float_c(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch)
{
  const pixel_t *srcp0 = reinterpret_cast<const pixel_t *>(srcp);
  float *dstp0 = reinterpret_cast<float *>(dstp);

  src_pitch = src_pitch / sizeof(pixel_t);
  dst_pitch = dst_pitch / sizeof(float);

  int src_width = src_rowsize / sizeof(pixel_t);

  const int limit_lo_s = (fulls ? 0 : 16) << (sourcebits - 8);
  const int limit_hi_s = fulls ? ((1 << sourcebits) - 1) : ((chroma ? 240 : 235) << (sourcebits - 8));
  const float range_diff_s = (float)limit_hi_s - limit_lo_s;

  const int limit_lo_d = fulld ? 0 : 16;
  const int limit_hi_d = fulld ? 255 : (chroma ? 240 : 235);
  const float range_diff_d = (limit_hi_d - limit_lo_d) / 255.0f;

  // fulls fulld luma             luma_new   chroma                          chroma_new
  // true  false 0..1              16-235     -0.5..0.5                      16-240       Y = Y * ((235-16) << (bpp-8)) + 16, Chroma= Chroma * ((240-16) << (bpp-8)) + 16
  // true  true  0..1               0-255     -0.5..0.5                      0-128-255
  // false false 16/255..235/255   16-235     (16-128)/255..(240-128)/255    16-240
  // false true  16/255..235/255    0..1      (16-128)/255..(240-128)/255    0-128-255
  const float factor = range_diff_d / range_diff_s;

  const int half = 1 << (sourcebits - 1);

  // 0..255,65535 -> 0..1.0 (or -0.5..+0.5) or less if !full

  for(int y=0; y<src_height; y++)
  {
    for (int x = 0; x < src_width; x++)
    {
      float pixel;
      if (chroma) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
        if (fulls)
          pixel = srcp0[x] * factor; // 0..255->0..1
        else
          pixel = (srcp0[x] - half) * factor + 0.5f; // back to 0..1.0 (0.5 centered)
#else
        if (fulls)
          pixel = srcp0[x] * factor - 0.5f; // 0..1->-0.5..0.5
        else
          pixel = (srcp0[x] - half) * factor; // -0.5..0.5 when fulld
#endif
      }
      else {
        pixel = (srcp0[x] - limit_lo_s) * factor + limit_lo_d / 255.0f;
      }
      dstp0[x] = pixel;
    }
    dstp0 += dst_pitch;
    srcp0 += src_pitch;
  }
  // seems we better stuck with C in the future on such a simple loops
  // if we could put it in a separate file
  // VS2015 AVX2 code for this:
  // takes (8 uint16_t -> 8*float(256 bit) at a time) * unroll_by_2
  // then makes singles with unrolled_by_4 until it can, then do the rest.
  /*
  AVX2 by VS2015: (8*uint16->8*float)xUnrollBy2
  $LL7@convert_ui:
    vpmovzxwd ymm0, XMMWORD PTR [esi+ecx*2]
    vcvtdq2ps ymm0, ymm0
    vmulps	ymm0, ymm0, ymm2
    vmovups	YMMWORD PTR [edi+ecx*4], ymm0
    vpmovzxwd ymm0, XMMWORD PTR [esi+ecx*2+16]
    vcvtdq2ps ymm0, ymm0
    vmulps	ymm0, ymm0, ymm2
    vmovups	YMMWORD PTR [edi+ecx*4+32], ymm0
    add	ecx, 16					; 00000010H
    cmp	ecx, ebx
    jl	SHORT $LL7@convert_ui

  SSE2 by VS2015 (4*uint16->4*float)xUnrollBy2
    $LL7@convert_ui:
    movq	xmm1, QWORD PTR [ebp+ecx*2]
    xorps	xmm0, xmm0
    punpcklwd xmm1, xmm0
    cvtdq2ps xmm0, xmm1
    mulps	xmm0, xmm3
    movups	XMMWORD PTR [ebx+ecx*4], xmm0
    movq	xmm1, QWORD PTR [ebp+ecx*2+8]
    xorps	xmm0, xmm0
    punpcklwd xmm1, xmm0
    cvtdq2ps xmm0, xmm1
    mulps	xmm0, xmm3
    movups	XMMWORD PTR [ebx+ecx*4+16], xmm0
    add	ecx, 8
    cmp	ecx, esi
    jl	SHORT $LL7@convert_ui
  */
}

BitDepthConvFuncPtr get_convert_to_8_function(bool full_scale, int source_bitdepth, int dither_mode, int dither_bitdepth, int rgb_step, int cpu)
{
  std::map<std::tuple<bool, int, int, int, int, int>, BitDepthConvFuncPtr> func_copy;
  using std::make_tuple;
  /*
  conv_function_full_scale = (sse2 && dither_mode<0) ? convert_rgb_uint16_to_8_c<10, -1, 8, 1> : (dither_mode>=0 ? convert_rgb_uint16_to_8_c<10, 0, 8, 1> : convert_rgb_uint16_to_8_c<10, -1, 8, 1>);
  conv_function_full_scale_no_dither = sse2 ? convert_rgb_uint16_to_8_c<10, -1, 8, 1> : convert_rgb_uint16_to_8_c<10, -1, 8, 1>;
  conv_function_shifted_scale = (sse2 && dither_mode<0) ? convert_uint16_to_8_sse2<10> : (dither_mode>=0 ? convert_uint16_to_8_c<10, 0, 8> : convert_uint16_to_8_c<10, -1, 8>);
  */
  const int DITHER_TARGET_BITDEPTH_8 = 8;
  const int DITHER_TARGET_BITDEPTH_7 = 7;
  const int DITHER_TARGET_BITDEPTH_6 = 6;
  const int DITHER_TARGET_BITDEPTH_5 = 5;
  const int DITHER_TARGET_BITDEPTH_4 = 4;
  const int DITHER_TARGET_BITDEPTH_3 = 3;
  const int DITHER_TARGET_BITDEPTH_2 = 2;
  const int DITHER_TARGET_BITDEPTH_1 = 1;
  const int DITHER_TARGET_BITDEPTH_0 = 0;

  if (dither_mode < 0)
    dither_bitdepth = 8; // default entry in the tables below
  if (dither_mode == 1) // no special version for fullscale dithering down.
  {
    // floyd
    rgb_step = 1; // rgb_step n/a for packed rgb formats.
    // packed rgb floyd result is not the same as because it treats the rgb pixels
    // as a consecutive bgrbgrbgr pixel flow, packed rgb is converted to and from planar rgb instead.
    full_scale = false;
  }

  // full scale

  // no dither, C
  func_copy[make_tuple(true, 10, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<10, -1, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 12, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<12, -1, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 14, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<14, -1, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 16, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<16, -1, DITHER_TARGET_BITDEPTH_8, 1>;
  // for RGB48 and RGB64 source
  func_copy[make_tuple(true, 16, -1, DITHER_TARGET_BITDEPTH_8, 3, 0)] = convert_rgb_uint16_to_8_c<16, -1, DITHER_TARGET_BITDEPTH_8, 1>; // dither rgb_step param is n/a
  func_copy[make_tuple(true, 16, -1, DITHER_TARGET_BITDEPTH_8, 4, 0)] = convert_rgb_uint16_to_8_c<16, -1, DITHER_TARGET_BITDEPTH_8, 1>; // dither rgb_step param is n/a

  //-----------
  // full scale, dither, C
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_4, 1>;
  func_copy[make_tuple(true, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_rgb_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_2, 1>;

  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_6, 1>;
  func_copy[make_tuple(true, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_4, 1>;

  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  func_copy[make_tuple(true, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_6, 1>;

  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 1>;
  // for RGB48 and RGB64 source
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 3, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 3>; // dither rgb_step param is filled
  func_copy[make_tuple(true, 16, 0, DITHER_TARGET_BITDEPTH_8, 4, 0)] = convert_rgb_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8, 4>; // dither rgb_step param is filled

  //-----------
  // Floyd dither, C, dither to 8 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_8>;
  // Floyd dither, C, dither to 7 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_7>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_7>;
  // Floyd dither, C, dither to 6 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_6>;
  // Floyd dither, C, dither to 5 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_5>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_5>;
  // Floyd dither, C, dither to 4 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_4>;
  // Floyd dither, C, dither to 3 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_3>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_3>;
  // Floyd dither, C, dither to 2 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_2>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_2>;
  // Floyd dither, C, dither to 1 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_1>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_1>;
  // Floyd dither, C, dither to 0 bits
  func_copy[make_tuple(false, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 10, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 12, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 14, 8, DITHER_TARGET_BITDEPTH_0>;
  func_copy[make_tuple(false, 16, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint8_t, 16, 8, DITHER_TARGET_BITDEPTH_0>;

  // shifted scale (YUV)

  // no dither, C
  func_copy[make_tuple(false, 10, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<10, -1, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<12, -1, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<14, -1, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, -1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<16, -1, DITHER_TARGET_BITDEPTH_8>;

  // dither, C, dither to 8 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_8>;
  func_copy[make_tuple(false, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_8_c<16, 0, DITHER_TARGET_BITDEPTH_8>;

  // dither, C, dither to 6 bits, max diff 8, allowed from 10-14 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_6>;
  func_copy[make_tuple(false, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_8_c<14, 0, DITHER_TARGET_BITDEPTH_6>;

  // dither, C, dither to 4 bits, max diff 8, allowed from 10-12 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_4>;
  func_copy[make_tuple(false, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_8_c<12, 0, DITHER_TARGET_BITDEPTH_4>;

  // dither, C, dither to 2 bits, max diff 8, allowed from 10 bits
  func_copy[make_tuple(false, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint16_to_8_c<10, 0, DITHER_TARGET_BITDEPTH_2>;

  BitDepthConvFuncPtr result = func_copy[make_tuple(full_scale, source_bitdepth, dither_mode, dither_bitdepth, rgb_step, cpu)];
  if (result == nullptr)
    result = func_copy[make_tuple(full_scale, source_bitdepth, dither_mode, dither_bitdepth, rgb_step, 0)]; // fallback to C
  return result;
}

BitDepthConvFuncPtr get_convert_to_16_16_down_dither_function(bool full_scale, int source_bitdepth, int target_bitdepth, int dither_mode, int dither_bitdepth, int rgb_step, int cpu)
{
  std::map<std::tuple<bool, int /*src*/, int /*target*/, int /*dithermode*/, int /*ditherbits*/, int /*rgbstep*/, int /*cpu*/>, BitDepthConvFuncPtr> func_copy;
  using std::make_tuple;
  /*
  conv_function_full_scale = (sse2 && dither_mode<0) ? convert_rgb_uint16_to_8_c<10, -1, 8, 1> : (dither_mode>=0 ? convert_rgb_uint16_to_8_c<10, 0, 8, 1> : convert_rgb_uint16_to_8_c<10, -1, 8, 1>);
  conv_function_full_scale_no_dither = sse2 ? convert_rgb_uint16_to_8_c<10, -1, 8, 1> : convert_rgb_uint16_to_8_c<10, -1, 8, 1>;
  conv_function_shifted_scale = (sse2 && dither_mode<0) ? convert_uint16_to_8_sse2<10> : (dither_mode>=0 ? convert_uint16_to_8_c<10, 0, 8> : convert_uint16_to_8_c<10, -1, 8>);
  */
  const int DITHER_TARGET_BITDEPTH_14 = 14;
  const int DITHER_TARGET_BITDEPTH_12 = 12;
  const int DITHER_TARGET_BITDEPTH_10 = 10;
  const int DITHER_TARGET_BITDEPTH_8 = 8;
  const int DITHER_TARGET_BITDEPTH_7 = 7;
  const int DITHER_TARGET_BITDEPTH_6 = 6;
  const int DITHER_TARGET_BITDEPTH_5 = 5;
  const int DITHER_TARGET_BITDEPTH_4 = 4;
  const int DITHER_TARGET_BITDEPTH_3 = 3;
  const int DITHER_TARGET_BITDEPTH_2 = 2; // only for 10->10 bits, but dithering_bits==2
  const int DITHER_TARGET_BITDEPTH_1 = 1; // FloydSteinberg allows any difference in the implementation
  const int DITHER_TARGET_BITDEPTH_0 = 0; // FloydSteinberg allows any difference in the implementation

  if (dither_mode == 1) // no special version for fullscale dithering down.
    full_scale = false;

  if (full_scale) {
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 16, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 16, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_8>;

    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 14, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_6>;

    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 12, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_4>;

    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 12, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_4>;

    // 10->10
    // dither, C, dither to N bits
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(true, 10, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_rgb_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_2>;
  }
  else {

    // floyd 16->
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 10, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 12, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 14, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 16, 16, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 16, 16, DITHER_TARGET_BITDEPTH_0>;
    // floyd 14->
    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 10, DITHER_TARGET_BITDEPTH_0>;

    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 12, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 14, 14, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 14, 14, DITHER_TARGET_BITDEPTH_0>;
    // floyd 12->
    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 12, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 10, DITHER_TARGET_BITDEPTH_0>;
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 12, 12, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 12, 12, DITHER_TARGET_BITDEPTH_0>;
    // floyd 12->
    // 10->10
    // dither, C, dither to N bits
    // keeping bit depth but dither down
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_7, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_7>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_5, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_5>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_3, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_3>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_2>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_1, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_1>;
    func_copy[make_tuple(false, 10, 10, 1, DITHER_TARGET_BITDEPTH_0, 1, 0)] = convert_uint_floyd_c<uint16_t, uint16_t, 10, 10, DITHER_TARGET_BITDEPTH_0>;

    // end of floyd

    // shifted scale
    // 16->10,12,14
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 16, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 10, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 14, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 12, DITHER_TARGET_BITDEPTH_8>;

    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_14, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_14>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 16, 16, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<16, 16, DITHER_TARGET_BITDEPTH_8>;

    // 14->10,12
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 10, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 12, DITHER_TARGET_BITDEPTH_6>;

    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_12, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_12>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 14, 14, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<14, 14, DITHER_TARGET_BITDEPTH_6>;

    // 12->10
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 10, DITHER_TARGET_BITDEPTH_4>;

    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_10, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_10>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 12, 12, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<12, 12, DITHER_TARGET_BITDEPTH_4>;

    // 10->10 only dither down
    // dither, C, dither to N bits
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_8, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_8>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_6, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_6>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_4, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_4>;
    func_copy[make_tuple(false, 10, 10, 0, DITHER_TARGET_BITDEPTH_2, 1, 0)] = convert_uint16_to_uint16_dither_c<10, 10, DITHER_TARGET_BITDEPTH_2>;

  }
  BitDepthConvFuncPtr result = func_copy[make_tuple(full_scale, source_bitdepth, target_bitdepth, dither_mode, dither_bitdepth, rgb_step, cpu)];
  if (result == nullptr)
    result = func_copy[make_tuple(full_scale, source_bitdepth, target_bitdepth, dither_mode, dither_bitdepth, rgb_step, 0)]; // fallback to C
  return result;
}


ConvertBits::ConvertBits(PClip _child, const int _dither_mode, const int _target_bitdepth, bool _truerange, bool _fulls, bool _fulld, int _dither_bitdepth, IScriptEnvironment* env) :
  GenericVideoFilter(_child), dither_mode(_dither_mode), target_bitdepth(_target_bitdepth),
  dither_bitdepth(_dither_bitdepth), truerange(_truerange), fulls(_fulls), fulld(_fulld)
{

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();
  format_change_only = false;

#ifdef INTEL_INTRINSICS
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool sse4 = !!(env->GetCPUFlags() & CPUF_SSE4_1);
  bool avx =  !!(env->GetCPUFlags() & CPUF_AVX);
  bool avx2 =  !!(env->GetCPUFlags() & CPUF_AVX2);
#endif

  BitDepthConvFuncPtr conv_function_full_scale;
  BitDepthConvFuncPtr conv_function_full_scale_no_dither;
  BitDepthConvFuncPtr conv_function_shifted_scale;

  conv_function_chroma = nullptr; // used only for 32bit float

  if (bits_per_pixel < 32 && target_bitdepth < 32) {
    // 32 bit source: fulls, fulld handled properly
    if (fulls != fulld)
      env->ThrowError("ConvertBits: fulls and fulld should be the same for non-32bit float formats");
  }

  // 8-16bit->32bits support fulls fulld, alpha is always full-full
#define convert_uintN_to_float_functions(uint_X_t, source_bits) \
      conv_function_a = convert_uintN_to_float_c<uint_X_t, source_bits, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, source_bits, false, true, true>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, source_bits, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, source_bits, false, true, false>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, source_bits, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, source_bits, false, false, true>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, source_bits, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = convert_uintN_to_float_c<uint_X_t, source_bits, false, false, false>; \
        conv_function_chroma = convert_uintN_to_float_c<uint_X_t, source_bits, true, false, false>; \
      }

  // ConvertToFloat
  if (target_bitdepth == 32) {
    if (pixelsize == 1) // 8->32 bit
    {
      convert_uintN_to_float_functions(uint8_t, 8)
    }
    else if (pixelsize == 2) // 16->32 bit
    {
      if (vi.IsPlanar() && truerange)
      {
        switch (bits_per_pixel)
        {
        case 10:
          convert_uintN_to_float_functions(uint16_t, 10);
          break;
        case 12:
          convert_uintN_to_float_functions(uint16_t, 12);
          break;
        case 14:
          convert_uintN_to_float_functions(uint16_t, 14);
          break;
        case 16:
          convert_uintN_to_float_functions(uint16_t, 16);
          break;
        default: env->ThrowError("ConvertToFloat: unsupported bit depth");
        }
      }
      else {
        convert_uintN_to_float_functions(uint16_t, 16);
      }
    }
    else
      env->ThrowError("ConvertToFloat: internal error 32->32 is not valid here");

    conv_function_a = conv_function;

    if (vi.NumComponents() == 1)
      vi.pixel_type = VideoInfo::CS_Y32;
    else if (vi.Is420())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420PS : VideoInfo::CS_YUV420PS;
    else if (vi.Is422())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422PS : VideoInfo::CS_YUV422PS;
    else if (vi.Is444())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444PS : VideoInfo::CS_YUV444PS;
    else if (vi.IsPlanarRGB())
      vi.pixel_type = VideoInfo::CS_RGBPS;
    else if (vi.IsPlanarRGBA())
      vi.pixel_type = VideoInfo::CS_RGBAPS;
    else
      env->ThrowError("ConvertToFloat: unsupported color space");

    return;
  }
  // ConvertToFloat end

// 32bit->8-16bits support fulls fulld
#define convert_32_to_uintN_functions(uint_X_t, target_bits) \
      conv_function_a = convert_32_to_uintN_c<uint_X_t, target_bits, false, true, true>; /* full-full */ \
      if (fulls && fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, target_bits, false, true, true>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, target_bits, true, true, true>; \
      } \
      else if (fulls && !fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, target_bits, false, true, false>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, target_bits, true, true, false>; \
      } \
      else if (!fulls && fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, target_bits, false, false, true>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, target_bits, true, false, true>; \
      } \
      else if (!fulls && !fulld) { \
        conv_function = convert_32_to_uintN_c<uint_X_t, target_bits, false, false, false>; \
        conv_function_chroma = convert_32_to_uintN_c<uint_X_t, target_bits, true, false, false>; \
      }

  // ConvertTo16bit() (10, 12, 14, 16)
  // Conversion to uint16_t targets
  // planar YUV(A) and RGB(A):
  //   from 8 bit -> 10/12/14/16 with strict range expansion or expansion to 16
  //   from 10/12/14 -> 16 bit with strict source range (expansion from 10/12/14 to 16 bit) or just casting pixel_type
  //   from 16 bit -> 10/12/14 bit with strict target range (reducing range from 16 bit to 10/12/14 bits) or just casting pixel_type
  //   from float -> 10/12/14/16 with strict range expansion or expansion to 16
  // packed RGB:
  //   RGB24->RGB48, RGB32->RGB64
  if (target_bitdepth > 8 && target_bitdepth <= 16) {
    // 8,10-16,32 -> 16 bit
    if (pixelsize == 1) // 8->10-12-14-16 bit
    {
      if (truerange)
      {
        switch (target_bitdepth)
        {
        case 10:
          conv_function_full_scale = convert_rgb_8_to_uint16_c<10>;
          conv_function_shifted_scale = convert_8_to_uint16_c<10>;
          break;
        case 12:
          conv_function_full_scale = convert_rgb_8_to_uint16_c<12>;
          conv_function_shifted_scale = convert_8_to_uint16_c<12>;
          break;
        case 14:
          conv_function_full_scale = convert_rgb_8_to_uint16_c<14>;
          conv_function_shifted_scale = convert_8_to_uint16_c<14>;
          break;
        case 16:
          conv_function_full_scale = convert_rgb_8_to_uint16_c<16>;
          conv_function_shifted_scale = convert_8_to_uint16_c<16>;
          break;
        default: env->ThrowError("ConvertBits: unsupported bit depth");
        }
      }
      else {
        conv_function_full_scale = convert_rgb_8_to_uint16_c<16>;
        conv_function_shifted_scale = convert_8_to_uint16_c<16>;
      }

      if (fulls)
        conv_function = conv_function_full_scale; // rgb default, RGB scaling is not shift by 2/4/6/8 as in YUV but like 0..255->0..65535
      else
        conv_function = conv_function_shifted_scale; // yuv default

      conv_function_a = conv_function_full_scale; // alpha copy is the same full scale
    }
    else if (pixelsize == 2)
    {
      // 10-16->10-16
      if (truerange)
      {

        // full_scale is used for alpha plane always (keep max opacity 255, 1023, 4095, 16383, 65535)

        // fill conv_function_full_scale and conv_function_shifted_scale
        // first get full_scale converter functions, normal and optional dithered
        if (bits_per_pixel >= target_bitdepth) // reduce range or dither down keeping bit-depth format
        {
          conv_function_full_scale = nullptr; // BitBlt in GetFrame

          if (bits_per_pixel == 16) { // 16->10/12/14 keep full range
            switch (target_bitdepth)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<16, 10>;
              break;
            case 12: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<16, 12>;
              break;
            case 14: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<16, 14>;
              break;
            }
          }
          else if (bits_per_pixel == 14) { // 14->10/12 keep full range
            switch (target_bitdepth)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<14, 10>;
              break;
            case 12: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<14, 12>;
              break;
            }
          }
          else if (bits_per_pixel == 12) { // 12->10 keep full range
            switch (target_bitdepth)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<12, 10>;
              break;
            }
          }

          conv_function_full_scale_no_dither = conv_function_full_scale; // save ditherless, used for possible alpha

          if (dither_mode >= 0) {
            conv_function_full_scale = get_convert_to_16_16_down_dither_function(true /*full scale*/, bits_per_pixel, target_bitdepth, dither_mode, dither_bitdepth, 1/*rgb_step n/a*/, 0 /*cpu none*/);
          }
        }
        else {// expand
          // no dither here
          if (target_bitdepth == 16) { // 10/12/14->16 keep full range
            switch (bits_per_pixel)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<10, 16>;
              break;
            case 12: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<12, 16>;
              break;
            case 14: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<14, 16>;
              break;
            }
          }
          else if (target_bitdepth == 14) { // 10/12->14 keep full range
            switch (bits_per_pixel)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<10, 14>;
              break;
            case 12: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<12, 14>;
              break;
            }
          }
          else if (target_bitdepth == 12) { // 10->12 keep full range
            switch (bits_per_pixel)
            {
            case 10: conv_function_full_scale = convert_rgb_uint16_to_uint16_c<10, 12>;
              break;
            }
          } else {
            env->ThrowError("ConvertBits: unsupported bit depth");
          }

          conv_function_full_scale_no_dither = conv_function_full_scale; // save ditherless, used for possible alpha
        }

        // fill shift_range converter functions
        if (bits_per_pixel >= target_bitdepth) { // reduce range 16->14/12/10 14->12/10 12->10. template: bitshift
          if (dither_mode < 0) {
            // convert down, no dither
            switch (bits_per_pixel - target_bitdepth)
            {
            case 2:
              conv_function_shifted_scale = convert_uint16_to_uint16_c<false, 2>;
              break;
            case 4:
              conv_function_shifted_scale = convert_uint16_to_uint16_c<false, 4>;
              break;
            case 6:
              conv_function_shifted_scale = convert_uint16_to_uint16_c<false, 6>;
              break;
            }
          }
          else {
            // convert down, dither
            conv_function_shifted_scale = get_convert_to_16_16_down_dither_function(false /*not full scale*/, bits_per_pixel, target_bitdepth, dither_mode, dither_bitdepth, 1/*rgb_step n/a*/, 0 /*cpu none*/);
          }
        }
        else { // expand range
          // convert up
          switch (target_bitdepth - bits_per_pixel)
          {
          case 2: conv_function_shifted_scale = convert_uint16_to_uint16_c<true, 2>; break;
          case 4: conv_function_shifted_scale = convert_uint16_to_uint16_c<true, 4>; break;
          case 6: conv_function_shifted_scale = convert_uint16_to_uint16_c<true, 6>; break;
          }
        }
      }
      else {
        // no conversion for truerange == false
      }

      // 10/12/14 -> 16 bit or 16 bit -> 10/12/14 bit
      // range reducing or expansion (truerange=true), or just overriding the pixel_type, keeping scale at 16 bits
      // 10-16 -> 10->16 truerange == false already handled
      if (truerange) {
        if (fulls)
          conv_function = conv_function_full_scale; // rgb default, RGB scaling is not shift by 2/4/6/8 as in YUV but like 0..255->0..65535
        else
          conv_function = conv_function_shifted_scale; // yuv default

        conv_function_a = conv_function_full_scale_no_dither; // alpha copy is always full scale w/o dithering
      }
      else { // truerange==false
             // 10->12 .. 16->12 etc
             // only vi bit_depth format override
        format_change_only = true;
      }
    }
    else if (pixelsize == 4) // 32->10-16 bit
    {
      if (truerange) {
        switch (target_bitdepth)
        {
        case 10:
          convert_32_to_uintN_functions(uint16_t, 10); // all variations of fulls fulld
          break;
        case 12:
          convert_32_to_uintN_functions(uint16_t, 12);
          break;
        case 14:
          convert_32_to_uintN_functions(uint16_t, 14);
          break;
        case 16:
          convert_32_to_uintN_functions(uint16_t, 16);
          break;
        }
      }
      else {
        convert_32_to_uintN_functions(uint16_t, 16);
      }
    }
    else {
      env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }

    // set output vi format
    if (vi.IsRGB24()) {
      if (target_bitdepth == 16)
        vi.pixel_type = VideoInfo::CS_BGR48;
      else
        env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }
    else if (vi.IsRGB32()) {
      if (target_bitdepth == 16)
        vi.pixel_type = VideoInfo::CS_BGR64;
      else
        env->ThrowError("ConvertTo16bit: unsupported bit depth");
    }
    else {
      // Y or YUV(A) or PlanarRGB(A)
      if (vi.IsYV12()) // YV12 can have an exotic compatibility constant
        vi.pixel_type = VideoInfo::CS_YV12;
      int new_bitdepth_bits;
      switch (target_bitdepth) {
      case 8: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_8; break;
      case 10: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_10; break;
      case 12: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_12; break;
      case 14: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_14; break;
      case 16: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_16; break;
      case 32: new_bitdepth_bits = VideoInfo::CS_Sample_Bits_32; break;
      }
      vi.pixel_type = (vi.pixel_type & ~VideoInfo::CS_Sample_Bits_Mask) | new_bitdepth_bits;
    }

    return;
  }

  // ConvertTo8bit()
  if (target_bitdepth == 8) {
    if (pixelsize == 2) // 16(,14,12,10)->8 bit
    {
      // it gets complicated, so we better using tuples for function lookup
      // parameters for full scale: source bitdepth, dither_type (-1:none, 0:ordered), target_dither_bitdepth(default 8, 2,4,6), rgb_step(3 for RGB48, 4 for RGB64, 1 for all planars)
      // rgb_step can differ from 1 only when source bits_per_pixel==16 and packed RGB type
      // target_dither_bitdepth==8 (RFU for dithering down from e.g. 10->2 bit)

      if(dither_mode==0 && (dither_bitdepth !=2 && dither_bitdepth != 4 && dither_bitdepth != 6 && dither_bitdepth != 8))
        env->ThrowError("ConvertBits: invalid dither target bitdepth %d", dither_bitdepth);

      // fill conv_function_full_scale and conv_function_shifted_scale
      // conv_function_full_scale_no_dither: for alpha plane
      if (truerange) {
        conv_function_full_scale = get_convert_to_8_function(true, bits_per_pixel, dither_mode, dither_bitdepth, 1, CPUF_SSE2);
        conv_function_full_scale_no_dither = get_convert_to_8_function(true, bits_per_pixel, -1, dither_bitdepth /* n/a */, 1, CPUF_SSE2); // force dither_mode==-1
        conv_function_shifted_scale = get_convert_to_8_function(false, bits_per_pixel, dither_mode, dither_bitdepth, 1, CPUF_SSE2);
      }
      else {
        conv_function_full_scale = get_convert_to_8_function(true, 16, dither_mode, dither_bitdepth, 1, CPUF_SSE2);
        conv_function_full_scale_no_dither = get_convert_to_8_function(true, 16, -1, dither_bitdepth /* n/a */, 1, CPUF_SSE2);
        conv_function_shifted_scale = get_convert_to_8_function(false, 16, dither_mode, dither_bitdepth, 1, CPUF_SSE2);
      }

      // override for RGB48 and 64 (internal rgb_step may differ when dithering is used
      if(vi.IsRGB48()) { // packed RGB: specify rgb_step 3 or 4 for dither table access
        conv_function_full_scale = get_convert_to_8_function(true, 16, dither_mode, dither_bitdepth, 3, CPUF_SSE2);
      } else if(vi.IsRGB64()) {
        conv_function_full_scale = get_convert_to_8_function(true, 16, dither_mode, dither_bitdepth, 4, CPUF_SSE2);
      }

      // packed RGB scaling is full_scale 0..65535->0..255
      if (fulls)
        conv_function = conv_function_full_scale; // rgb default, RGB scaling is not shift by 2/4/6/8 as in YUV but like 0..255->0..65535
      else
        conv_function = conv_function_shifted_scale; // yuv default

      conv_function_a = conv_function_full_scale_no_dither; // alpha copy is the same full scale, w/o dithering

    }
    else if (vi.ComponentSize() == 4) // 32->8 bit, no dithering option atm
    {
      convert_32_to_uintN_functions(uint8_t, 8); // all combinations of fulls, fulld
    }
    else
      env->ThrowError("ConvertTo8bit: unsupported bit depth");

    if (vi.NumComponents() == 1)
      vi.pixel_type = VideoInfo::CS_Y8;
    else if (vi.Is420())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA420 : VideoInfo::CS_YV12;
    else if (vi.Is422())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA422 : VideoInfo::CS_YV16;
    else if (vi.Is444())
      vi.pixel_type = vi.IsYUVA() ? VideoInfo::CS_YUVA444 : VideoInfo::CS_YV24;
    else if (vi.IsRGB48())
      vi.pixel_type = VideoInfo::CS_BGR24;
    else if (vi.IsRGB64())
      vi.pixel_type = VideoInfo::CS_BGR32;
    else if (vi.IsPlanarRGB())
      vi.pixel_type = VideoInfo::CS_RGBP;
    else if (vi.IsPlanarRGBA())
      vi.pixel_type = VideoInfo::CS_RGBAP;
    else
      env->ThrowError("ConvertTo8bit: unsupported color space");

    return;
  }

  env->ThrowError("ConvertBits: unsupported target bit-depth (%d)", target_bitdepth);

}

AVSValue __cdecl ConvertBits::Create(AVSValue args, void* user_data, IScriptEnvironment* env) {
  PClip clip = args[0].AsClip();
  //0   1        2        3         4         5           6
  //c[bits]i[truerange]b[dither]i[dither_bits]i[fulls]b[fulld]b

  const VideoInfo &vi = clip->GetVideoInfo();

  int create_param = (int)reinterpret_cast<intptr_t>(user_data);

  // bits parameter is compulsory
  if (!args[1].Defined() && create_param == 0) {
    env->ThrowError("ConvertBits: missing bits parameter");
  }

  // when converting from/true 10-16 bit formats, truerange=false indicates bitdepth of 16 bits regardless of the 10-12-14 bit format
  bool assume_truerange = args[2].AsBool(true); // n/a for non planar formats
                                                // bits parameter

  int target_bitdepth = args[1].AsInt(create_param); // default comes by calling from old To8,To16,ToFloat functions
  int source_bitdepth = vi.BitsPerComponent();
  int dither_bitdepth = args[4].AsInt(target_bitdepth);

  if(target_bitdepth!=8 && target_bitdepth!=10 && target_bitdepth!=12 && target_bitdepth!=14 && target_bitdepth!=16 && target_bitdepth!=32)
    env->ThrowError("ConvertBits: invalid bit depth: %d", target_bitdepth);

  if(create_param == 8 && target_bitdepth !=8)
    env->ThrowError("ConvertTo8Bit: invalid bit depth: %d", target_bitdepth);
  if(create_param == 32 && target_bitdepth !=32)
    env->ThrowError("ConvertToFloat: invalid bit depth: %d", target_bitdepth);
  if(create_param == 16 && (target_bitdepth == 8 || target_bitdepth ==32))
    env->ThrowError("ConvertTo16bit: invalid bit depth: %d", target_bitdepth);

  if (args[2].Defined()) {
    if (!vi.IsPlanar())
      env->ThrowError("ConvertBits: truerange specified for non-planar source");
  }

  // override defaults, e.g. set full range for greyscale clip conversion that is RGB
  // Post 2664: can be set. Full range is default also for float (and cannot be set to false)
  bool fulls = args[5].AsBool(vi.IsRGB()/* || ((target_bitdepth == 32 || source_bitdepth == 32))*/);
  bool fulld = args[6].AsBool(fulls);

  int dither_type = args[3].AsInt(-1);
  bool dither_defined = args[3].Defined();
  if(dither_defined && dither_type != 1 && dither_type != 0 && dither_type != -1)
    env->ThrowError("ConvertBits: invalid dither type parameter. Only -1 (disabled), 0 (ordered dither) or 1 (Floyd-S) is allowed");

  if (dither_type >= 0) {
    if (source_bitdepth < target_bitdepth)
      env->ThrowError("ConvertBits: dithering is allowed only for scale down");
    if (dither_bitdepth > target_bitdepth)
      env->ThrowError("ConvertBits: dither_bits must be <= target bitdepth");
    if (target_bitdepth == 32)
      env->ThrowError("ConvertBits: dithering is not allowed only for 32 bit targets");
  }

  if (source_bitdepth == 8 && dither_bitdepth == 8)
    dither_type = -1; // ignore dithering from 8 to 8 bit

  if(dither_type == 0) {

    if (source_bitdepth == 32)
      env->ThrowError("ConvertBits: dithering is not allowed for 32 bit sources");

    if (dither_bitdepth < 2 || dither_bitdepth > 16)
      env->ThrowError("ConvertBits: invalid dither_bits specified");

    if (dither_bitdepth % 2)
      env->ThrowError("ConvertBits: dither_bits must be even");

    if(source_bitdepth - dither_bitdepth > 8)
      env->ThrowError("ConvertBits: dither_bits cannot differ with more than 8 bits from source");

    if (source_bitdepth == 8)
      env->ThrowError("ConvertBits: dithering down to less than 8 bits is not supported for 8 bit sources");
  }

  // floyd
  if (dither_type == 1) {

    if (source_bitdepth == 8 || source_bitdepth == 32)
      env->ThrowError("ConvertBits: Floyd-S: dithering is allowed only for 10-16 bit sources");

    if (dither_bitdepth < 0 || dither_bitdepth > 16)
      env->ThrowError("ConvertBits: Floyd-S: invalid dither_bits specified");

    if ((dither_bitdepth > 8 && (dither_bitdepth%2) != 0)) // must be even above 8 bits. 0 is ok, means real b/w
      env->ThrowError("ConvertBits: Floyd-S: dither_bits must be 0..8, 10, 12, 14, 16");
  }

  // no change -> return unmodified if no dithering required, or dither bitdepth is the same as target
  if (source_bitdepth == target_bitdepth) { // 10->10 .. 16->16
    if(dither_type < 0 || dither_bitdepth == target_bitdepth)
      return clip;
    if(vi.IsRGB() && !vi.IsPlanar())
      env->ThrowError("ConvertBits: dithering_bits should be the as target bitdepth for packed RGB formats");
    // here: we allow e.g. a 16->16 bit conversion with dithering bitdepth of 8
  }

  // YUY2 conversion is limited
  if (vi.IsYUY2()) {
    env->ThrowError("ConvertBits: YUY2 source is 8-bit only");
  }

  if (vi.IsYV411()) {
    env->ThrowError("ConvertBits: YV411 source cannot be converted");
  }

  // packed RGB conversion is limited
  if (vi.IsRGB24() || vi.IsRGB32()) {
    if (target_bitdepth != 16)
      env->ThrowError("ConvertBits: invalid bit-depth specified for packed RGB");
  }

  if (vi.IsRGB48() || vi.IsRGB64()) {
    if (target_bitdepth != 8)
      env->ThrowError("ConvertBits: invalid bit-depth specified for packed RGB");
  }

    // remark
    // source_10_bit.ConvertTo16bit(truerange=true)  : upscale range
    // source_10_bit.ConvertTo16bit(truerange=false) : leaves data, only format conversion
    // source_10_bit.ConvertTo16bit(bits=12,truerange=true)  : upscale range from 10 to 12
    // source_10_bit.ConvertTo16bit(bits=12,truerange=false) : leaves data, only format conversion
    // source_16_bit.ConvertTo16bit(bits=10, truerange=true)  : downscale range
    // source_16_bit.ConvertTo16bit(bits=10, truerange=false) : leaves data, only format conversion

  if (fulls != fulld && target_bitdepth != 32 && source_bitdepth != 32)
    env->ThrowError("ConvertBits: fulls must be the same as fulld for non 32bit target and source");

  // for floyd, planar rgb conversion happens
  bool need_convert_48 = vi.IsRGB48() && dither_type == 1 && target_bitdepth == 8;
  bool need_convert_64 = vi.IsRGB64() && dither_type == 1 && target_bitdepth == 8;

  // convert to planar on the fly
  if (need_convert_48) {
    AVSValue new_args[1] = { clip };
    clip = env->Invoke("ConvertToPlanarRGB", AVSValue(new_args, 1)).AsClip();
  } else if (need_convert_64) {
    AVSValue new_args[1] = { clip };
    clip = env->Invoke("ConvertToPlanarRGBA", AVSValue(new_args, 1)).AsClip();
  }

  AVSValue result = new ConvertBits(clip, dither_type, target_bitdepth, assume_truerange, fulls, fulld, dither_bitdepth, env);

  // convert back to packed rgb on the fly
  if (need_convert_48) {
    AVSValue new_args[1] = { result };
    result = env->Invoke("ConvertToRGB24", AVSValue(new_args, 1)).AsClip();
  } else if (need_convert_64) {
    AVSValue new_args[1] = { result };
    result = env->Invoke("ConvertToRGB32", AVSValue(new_args, 1)).AsClip();
  }

  return result;
}


PVideoFrame __stdcall ConvertBits::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);

  if (format_change_only)
  {
    // for 10-16 bit: simple format override in constructor
    return src;
  }

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);

  if(vi.IsPlanar())
  {
    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    for (int p = 0; p < vi.NumComponents(); ++p) {
      const int plane = planes[p];
      if (plane == PLANAR_A) {
        if (conv_function_a == nullptr)
          env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
        else
          conv_function_a(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane));
      }
      else if (conv_function == nullptr)
        env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane), src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
      else {
        const bool chroma = (plane == PLANAR_U || plane == PLANAR_V);
        if (chroma && conv_function_chroma != nullptr)
          // 32bit float needs separate conversion (possible chroma -0.5 .. 0.5 option)
          // until then the conv_function_ch behaves the same as conv_function
          // see #ifdef FLOAT_CHROMA_IS_HALF_CENTERED
          conv_function_chroma(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane));
        else
          conv_function(src->GetReadPtr(plane), dst->GetWritePtr(plane),
            src->GetRowSize(plane), src->GetHeight(plane),
            src->GetPitch(plane), dst->GetPitch(plane));
      }
    }
  }
  else {
    // packed RGBs
    conv_function(src->GetReadPtr(), dst->GetWritePtr(),
      src->GetRowSize(), src->GetHeight(),
      src->GetPitch(), dst->GetPitch());
  }
  return dst;
}

AVSValue AddAlphaPlane::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  bool isMaskDefined = args[1].Defined();
  bool maskIsClip = false;
  // if mask is not defined and videoformat has Alpha then we return
  if(isMaskDefined && !args[1].IsClip() && !args[1].IsFloat())
    env->ThrowError("AddAlphaPlane: mask parameter should be clip or number");
  const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
  if (!isMaskDefined && (vi.IsPlanarRGBA() || vi.IsYUVA() || vi.IsRGB32() || vi.IsRGB64()))
    return args[0].AsClip();
  PClip alphaClip = nullptr;
  if (isMaskDefined && args[1].IsClip()) {
    const VideoInfo& viAlphaClip = args[1].AsClip()->GetVideoInfo();
    maskIsClip = true;
    if(viAlphaClip.BitsPerComponent() != vi.BitsPerComponent())
      env->ThrowError("AddAlphaPlane: alpha clip is of different bit depth");
    if(viAlphaClip.width != vi.width || viAlphaClip.height != vi.height )
      env->ThrowError("AddAlphaPlane: alpha clip is of different size");
    if (viAlphaClip.IsY())
      alphaClip = args[1].AsClip();
    else if (viAlphaClip.NumComponents() == 4) {
      AVSValue new_args[1] = { args[1].AsClip() };
      alphaClip = env->Invoke("ExtractA", AVSValue(new_args, 1)).AsClip();
    } else {
      env->ThrowError("AddAlphaPlane: alpha clip should be greyscale or should have alpha plane");
    }
    // alphaClip is always greyscale here
  }
  float maskAsFloat = -1.0f;
  if (!maskIsClip)
    maskAsFloat = (float)args[1].AsFloat(-1.0f);
  if (vi.IsRGB24()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    PClip child = env->Invoke("ConvertToRGB32", AVSValue(new_args, 1)).AsClip();
    return new AddAlphaPlane(child, alphaClip, maskAsFloat, isMaskDefined, env);
  } else if(vi.IsRGB48()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    PClip child = env->Invoke("ConvertToRGB64", AVSValue(new_args, 1)).AsClip();
    return new AddAlphaPlane(child, alphaClip, maskAsFloat, isMaskDefined, env);
  }
  return new AddAlphaPlane(args[0].AsClip(), alphaClip, maskAsFloat, isMaskDefined, env);
}

AddAlphaPlane::AddAlphaPlane(PClip _child, PClip _alphaClip, float _mask_f, bool isMaskDefined, IScriptEnvironment* env)
  : GenericVideoFilter(_child), alphaClip(_alphaClip)
{
  if(vi.IsYUY2())
    env->ThrowError("AddAlphaPlane: YUY2 is not allowed");
  if(vi.IsY())
    env->ThrowError("AddAlphaPlane: greyscale source is not allowed");
  if(vi.IsYUV() && !vi.Is420() && !vi.Is422() && !vi.Is444()) // e.g. 410
    env->ThrowError("AddAlphaPlane: YUV format not supported, must be 420, 422 or 444");
  if(!vi.IsYUV() && !vi.IsYUVA() && !vi.IsRGB())
    env->ThrowError("AddAlphaPlane: format not supported");

  pixelsize = vi.ComponentSize();
  bits_per_pixel = vi.BitsPerComponent();

  if (vi.IsYUV()) {
    int pixel_type = vi.pixel_type;
    if (vi.IsYV12())
      pixel_type = VideoInfo::CS_YV12;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_YUV) | VideoInfo::CS_YUVA;
    vi.pixel_type = new_pixel_type;
  } else if(vi.IsPlanarRGB()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_RGB_TYPE) | VideoInfo::CS_RGBA_TYPE;
    vi.pixel_type = new_pixel_type;
  }
  // RGB24 and RGB48 already converted to 32/64
  // RGB32, RGB64, YUVA and RGBA: no change

  // mask parameter. If none->max transparency

  if (!alphaClip) {
    int max_pixel_value = (1 << bits_per_pixel) - 1; // n/a for float
    if (!isMaskDefined) {
      mask_f = 1.0f;
      mask = max_pixel_value;
    }
    else {
      mask_f = _mask_f;
      mask = (mask_f < 0) ? 0 : (mask_f > max_pixel_value) ? max_pixel_value : (int)mask_f;
      mask = clamp(mask, 0, max_pixel_value);
      // no clamp for float
    }
  }
}

PVideoFrame AddAlphaPlane::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  if(vi.IsPlanar())
  {
    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
    // copy existing 3 planes
    for (int p = 0; p < 3; ++p) {
      const int plane = planes[p];
      env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane),
           src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
    }
  } else {
    // Packed RGB, already converted to RGB32 or RGB64
    env->BitBlt(dst->GetWritePtr(), dst->GetPitch(), src->GetReadPtr(),
      src->GetPitch(), src->GetRowSize(), src->GetHeight());
  }

  if (vi.IsPlanarRGBA() || vi.IsYUVA()) {
    if (alphaClip) {
      PVideoFrame srcAlpha = alphaClip->GetFrame(n, env);
      env->BitBlt(dst->GetWritePtr(PLANAR_A), dst->GetPitch(PLANAR_A), srcAlpha->GetReadPtr(PLANAR_Y),
        srcAlpha->GetPitch(PLANAR_Y), srcAlpha->GetRowSize(PLANAR_Y), srcAlpha->GetHeight(PLANAR_Y));
    }
    else {
      // default constant
      const int dst_pitchA = dst->GetPitch(PLANAR_A);
      BYTE* dstp_a = dst->GetWritePtr(PLANAR_A);
      const int heightA = dst->GetHeight(PLANAR_A);

      switch (vi.ComponentSize())
      {
      case 1:
        fill_plane<BYTE>(dstp_a, heightA, dst_pitchA, mask);
        break;
      case 2:
        fill_plane<uint16_t>(dstp_a, heightA, dst_pitchA, mask);
        break;
      case 4:
        fill_plane<float>(dstp_a, heightA, dst_pitchA, mask_f);
        break;
      }
    }
    return dst;
  }
  // RGB32 and RGB64

  BYTE* pf = dst->GetWritePtr();
  int pitch = dst->GetPitch();
  int rowsize = dst->GetRowSize();
  int height = dst->GetHeight();
  int width = vi.width;

  if (alphaClip) {
    // fill by alpha clip already converted to grey-only
    PVideoFrame srcAlpha = alphaClip->GetFrame(n, env);
    const BYTE* srcp_a = srcAlpha->GetReadPtr(PLANAR_Y);
    size_t pitch_a = srcAlpha->GetPitch(PLANAR_Y);

    pf += pitch * (vi.height - 1); // start from bottom: packed RGB is upside down

    if (vi.IsRGB32()) {
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x ++) {
          pf[x*4+3] = srcp_a[x];
        }
        pf -= pitch; // packed RGB is upside down
        srcp_a += pitch_a;
      }
    }
    else if (vi.IsRGB64()) {
      rowsize /= sizeof(uint16_t);
      for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x ++) {
          reinterpret_cast<uint16_t *>(pf)[x*4+3] = reinterpret_cast<const uint16_t *>(srcp_a)[x];
        }
        pf -= pitch; // packed RGB is upside down
        srcp_a += pitch_a;
      }
    }
  }
  else {
    // fill with constant
    if (vi.IsRGB32()) {
      for (int y = 0; y < height; y++) {
        for (int x = 3; x < rowsize; x += 4) {
          pf[x] = mask;
        }
        pf += pitch;
      }
    }
    else if (vi.IsRGB64()) {
      rowsize /= sizeof(uint16_t);
      for (int y = 0; y < height; y++) {
        for (int x = 3; x < rowsize; x += 4) {
          reinterpret_cast<uint16_t *>(pf)[x] = mask;
        }
        pf += pitch;
      }
    }
  }

  return dst;
}

AVSValue RemoveAlphaPlane::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  // if videoformat has no Alpha then we return
  const VideoInfo& vi = args[0].AsClip()->GetVideoInfo();
  if(vi.IsPlanar() && (vi.IsYUV() || vi.IsPlanarRGB())) // planar and no alpha
    return args[0].AsClip();
  if(vi.IsRGB24() || vi.IsRGB48()) // packed RGB and no alpha
    return args[0].AsClip();
  if (vi.IsRGB32()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    return env->Invoke("ConvertToRGB24", AVSValue(new_args, 1)).AsClip();
  }
  if (vi.IsRGB64()) {
    AVSValue new_args[1] = { args[0].AsClip() };
    return env->Invoke("ConvertToRGB48", AVSValue(new_args, 1)).AsClip();
  }
  return new RemoveAlphaPlane(args[0].AsClip(), env);
}

RemoveAlphaPlane::RemoveAlphaPlane(PClip _child, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  if(vi.IsYUY2())
    env->ThrowError("RemoveAlphaPlane: YUY2 is not allowed");
  if(vi.IsY())
    env->ThrowError("RemoveAlphaPlane: greyscale source is not allowed");

  if (vi.IsYUVA()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_YUVA) | VideoInfo::CS_YUV;
    vi.pixel_type = new_pixel_type;
  } else if(vi.IsPlanarRGBA()) {
    int pixel_type = vi.pixel_type;
    int new_pixel_type = (pixel_type & ~VideoInfo::CS_RGBA_TYPE) | VideoInfo::CS_RGB_TYPE;
    vi.pixel_type = new_pixel_type;
  }
}

PVideoFrame RemoveAlphaPlane::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  // Packed RGB: already handled in ::Create through Invoke 32->24 or 64->48 conversion
  // only planar here
  int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
  int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
  int *planes = (vi.IsYUV() || vi.IsYUVA()) ? planes_y : planes_r;
  // Abuse Subframe to snatch the YUV/GBR planes
  return env->SubframePlanar(src, 0, src->GetPitch(planes[0]), src->GetRowSize(planes[0]), src->GetHeight(planes[0]),0,0,src->GetPitch(planes[1]));

#if 0
  // BitBlt version. Kept for reference
  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  // copy 3 planes w/o alpha
  for (int p = 0; p < 3; ++p) {
    const int plane = planes[p];
    env->BitBlt(dst->GetWritePtr(plane), dst->GetPitch(plane), src->GetReadPtr(plane),
      src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));
  }
return dst;
#endif
}

