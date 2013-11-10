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
#include "convert_rgb.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include "convert_planar.h"
#include "avs/alignment.h"
#include <cstdlib>
#include <emmintrin.h>



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Convert_filters[] = {       // matrix can be "rec601", rec709", "PC.601" or "PC.709"
  { "ConvertToRGB",   "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create },
  { "ConvertToRGB24", "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create24 },
  { "ConvertToRGB32", "c[matrix]s[interlaced]b[ChromaInPlacement]s[chromaresample]s", ConvertToRGB::Create32 },
  { "ConvertToY8",    "c[matrix]s", ConvertToY8::Create },
  { "ConvertToYV12",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s[ChromaOutPlacement]s", ConvertToYV12::Create },
  { "ConvertToYV24",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV24},
  { "ConvertToYV16",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV16},
  { "ConvertToYV411", "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToPlanarGeneric::CreateYV411},
  { "ConvertToYUY2",  "c[interlaced]b[matrix]s[ChromaInPlacement]s[chromaresample]s", ConvertToYUY2::Create },
  { "ConvertBackToYUY2", "c[matrix]s", ConvertBackToYUY2::Create },
  { 0 }
};

const int crv_rec601 = int(1.596*65536+0.5);
const int cgv_rec601 = int(0.813*65536+0.5);
const int cgu_rec601 = int(0.391*65536+0.5);
const int cbu_rec601 = int(2.018*65536+0.5);

const int crv_rec709 = int(1.793*65536+0.5); 
const int cgv_rec709 = int(0.533*65536+0.5);
const int cgu_rec709 = int(0.213*65536+0.5);
const int cbu_rec709 = int(2.112*65536+0.5);

const int crv_pc601 = int(1.407*65536+0.5);
const int cgv_pc601 = int(0.717*65536+0.5);
const int cgu_pc601 = int(0.345*65536+0.5); 
const int cbu_pc601 = int(1.779*65536+0.5);

const int crv_pc709 = int(1.581*65536+0.5);
const int cgv_pc709 = int(0.470*65536+0.5);
const int cgu_pc709 = int(0.188*65536+0.5);
const int cbu_pc709 = int(1.863*65536+0.5);

const int cy_rec = int((255.0/219.0)*65536+0.5);
const int cy_pc = 65536;

const int crv_values[4] = { crv_rec601, crv_rec709, crv_pc601, crv_pc709 };
const int cgv_values[4] = { cgv_rec601, cgv_rec709, cgv_pc601, cgv_pc709 };
const int cgu_values[4] = { cgu_rec601, cgu_rec709, cgu_pc601, cgu_pc709 };
const int cbu_values[4] = { cbu_rec601, cbu_rec709, cbu_pc601, cbu_pc709 };
const int cy_values[4]  = { cy_rec,     cy_rec,     cy_pc,     cy_pc};


/****************************************
 *******   Convert to RGB / RGBA   ******
 ***************************************/

ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix,
                            IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
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
      int u0 = srcp[x*2+1];
      int v0 = srcp[x*2+3];
      int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
      int u1 = srcp[x*2+5];
      int v1 = srcp[x*2+7];

      dstp[x*rgb_size + 0] = ScaledPixelClip(scaled_y0 + (u0-128) * cbu);                 // blue
      dstp[x*rgb_size + 1] = ScaledPixelClip(scaled_y0 - (u0-128) * cgu - (v0-128) * cgv); // green
      dstp[x*rgb_size + 2] = ScaledPixelClip(scaled_y0                  + (v0-128) * crv); // red

      dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + (u0+u1-256) * (cbu / 2));                     // blue
      dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - (u0+u1-256) * (cgu / 2) - (v0+v1-256) * (cgv/2)); // green
      dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1                     + (v0+v1-256) * (crv/2)); // red

      if (rgb_size == 4) {
        dstp[x*4+3] = 255;
        dstp[x*4+7] = 255;
      }
    }

    int scaled_y0 = (srcp[x*2+0] - tv_scale) * cy;
    int scaled_y1 = (srcp[x*2+2] - tv_scale) * cy;
    int u = srcp[x*2+1];
    int v = srcp[x*2+3];

    dstp[x*rgb_size + 0]     = ScaledPixelClip(scaled_y0 + (u-128) * cbu);                 // blue
    dstp[x*rgb_size + 1]     = ScaledPixelClip(scaled_y0 - (u-128) * cgu - (v-128) * cgv); // green
    dstp[x*rgb_size + 2]     = ScaledPixelClip(scaled_y0                 + (v-128) * crv); // red

    dstp[(x+1)*rgb_size + 0] = ScaledPixelClip(scaled_y1 + (u-128) * cbu);                 // blue
    dstp[(x+1)*rgb_size + 1] = ScaledPixelClip(scaled_y1 - (u-128) * cgu - (v-128) * cgv); // green
    dstp[(x+1)*rgb_size + 2] = ScaledPixelClip(scaled_y1                 + (v-128) * crv); // red

    if (rgb_size == 4) {
      dstp[x*4+3] = 255;
      dstp[x*4+7] = 255;
    }
    dstp += dst_pitch;
  }
}


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();

  PVideoFrame dst = env->NewVideoFrame(vi);
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


AVSValue __cdecl ConvertToRGB::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 4 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);

  return clip;
}


AVSValue __cdecl ConvertToRGB::Create32(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 4 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB32: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, false, matrix, env);

  if (vi.IsRGB24())
    return new RGB24to32(clip);

  return clip;
}


AVSValue __cdecl ConvertToRGB::Create24(AVSValue args, void*, IScriptEnvironment* env)
{
  const bool haveOpts = args[3].Defined() || args[4].Defined();
  PClip clip = args[0].AsClip();
  const char* const matrix = args[1].AsString(0);
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsPlanar()) {
    AVSValue new_args[5] = { clip, args[2], args[1], args[3], args[4] };
    clip = ConvertToPlanarGeneric::CreateYV24(AVSValue(new_args, 5), NULL, env).AsClip();
    return new ConvertYV24ToRGB(clip, getMatrix(matrix, env), 3 , env);
  }

  if (haveOpts)
    env->ThrowError("ConvertToRGB24: ChromaPlacement and ChromaResample options are not supported.");

  if (vi.IsYUV())
    return new ConvertToRGB(clip, true, matrix, env);

  if (vi.IsRGB32())
    return new RGB32to24(clip);

  return clip;
}

/**********************************
 *******   Convert to YV12   ******
 *********************************/


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

  if ((env->GetCPUFlags() & CPUF_MMX) == 0)
    env->ThrowError("ConvertToYV12: YV12 support require a MMX capable processor.");
}

PVideoFrame __stdcall ConvertToYV12::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);

  if (interlaced) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src->GetReadPtr(), 16)) 
    {
      convert_yuy2_to_yv12_interlaced_sse2(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#ifdef X86_32
    if ((env->GetCPUFlags() & CPUF_INTEGER_SSE))
    {
      convert_yuy2_to_yv12_interlaced_isse(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#endif
    {
      convert_yuy2_to_yv12_interlaced_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
  } 
  else
  {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src->GetReadPtr(), 16)) 
    {
      convert_yuy2_to_yv12_progressive_sse2(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#ifdef X86_32
    if ((env->GetCPUFlags() & CPUF_INTEGER_SSE))
    {
      convert_yuy2_to_yv12_progressive_isse(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
                        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
                        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
    else
#endif
    {
      convert_yuy2_to_yv12_progressive_c(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
  }

  return dst;

}

AVSValue __cdecl ConvertToYV12::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsYUY2() && !args[3].Defined() && !args[4].Defined() && !args[5].Defined())  // User has not requested options, do it fast!
    return new ConvertToYV12(clip,args[1].AsBool(false),env);

  return ConvertToPlanarGeneric::CreateYV12(args,0,env);
}





