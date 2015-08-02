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


#include "stdafx.h"

#include "convert.h"
#include "convert_rgb.h"
#include "convert_yv12.h"
#include "convert_yuy2.h"
#include "convert_planar.h"



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





/****************************************
 *******   Convert to RGB / RGBA   ******
 ***************************************/

ConvertToRGB::ConvertToRGB( PClip _child, bool rgb24, const char* matrix,
                            IScriptEnvironment* env )
  : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
  is_yv12=false;
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
  use_mmx = (env->GetCPUFlags() & CPUF_MMX) != 0;

  if ((theMatrix != Rec601) && ((vi.width & 3) != 0) || !use_mmx)
    env->ThrowError("ConvertToRGB: Rec.709 and PC Levels support require MMX and horizontal width a multiple of 4");
  vi.pixel_type = rgb24 ? VideoInfo::CS_BGR24 : VideoInfo::CS_BGR32;

  Generate(vi.IsRGB32(), theMatrix, env);

}


PVideoFrame __stdcall ConvertToRGB::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  const int src_pitch = src->GetPitch();
  const BYTE* srcp = src->GetReadPtr();

  int src_rowsize = __min(src_pitch, (src->GetRowSize()+7) & -8);
  // assumption: is_yuy2
  if (use_mmx && ((src_rowsize & 7) == 0) && (src_rowsize >= 16)) {
    VideoInfo vi2 = vi;
    vi2.width=src_rowsize / 2;
    PVideoFrame dst = env->NewVideoFrame(vi2,-2); // force pitch == rowsize
    BYTE* dstp = dst->GetWritePtr();

    assembly.Call(srcp, dstp, srcp + vi.height * src_pitch, src_pitch, src_rowsize);

    if (vi.width & 3) {  // Did we extend off the right edge of picture?
      const int dst_pitch = dst->GetPitch();
      const int x2 = (vi.width-2) * 2;
      const int xE = (vi.width-1) * (vi2.BitsPerPixel()>>3);
      srcp += vi.height * src_pitch;
      for (int y=vi.height; y>0; --y) {
        srcp -= src_pitch;
        YUV2RGB(srcp[x2+2], srcp[x2+1], srcp[x2+3], &dstp[xE]);
        dstp += dst_pitch;
      }
    }
    return env->Subframe(dst,0, dst->GetPitch(), vi2.BytesFromPixels(vi.width), vi.height);
  }
  else {
    PVideoFrame dst = env->NewVideoFrame(vi);
    const int dst_pitch = dst->GetPitch();
    BYTE* dstp = dst->GetWritePtr();

    if (vi.IsRGB32()) {
      srcp += vi.height * src_pitch;
      for (int y=vi.height; y>0; --y) {
        srcp -= src_pitch;
        int x;
        for (x=0; x<vi.width-2; x+=2) {
          YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*4]);
          YUV2RGB2(srcp[x*2+2], srcp[x*2+1], srcp[x*2+5], srcp[x*2+3], srcp[x*2+7], &dstp[x*4+4]);
          dstp[x*4+3] = 255;
          dstp[x*4+7] = 255;
        }
        YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*4]);
        YUV2RGB(srcp[x*2+2], srcp[x*2+1], srcp[x*2+3], &dstp[x*4+4]);
        dstp[x*4+3] = 255;
        dstp[x*4+7] = 255;
        dstp += dst_pitch;
      }
    }
    else if (vi.IsRGB24()) {
      srcp += vi.height * src_pitch;
      for (int y=vi.height; y>0; --y) {
        srcp -= src_pitch;
        int x;
        for (x=0; x<vi.width-2; x+=2) {
          YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*3]);
          YUV2RGB2(srcp[x*2+2], srcp[x*2+1], srcp[x*2+5], srcp[x*2+3], srcp[x*2+7], &dstp[x*3+3]);
        }
        YUV2RGB(srcp[x*2+0], srcp[x*2+1], srcp[x*2+3], &dstp[x*3]);
        YUV2RGB(srcp[x*2+2], srcp[x*2+1], srcp[x*2+3], &dstp[x*3+3]);
        dstp += dst_pitch;
      }
    }
    return dst;
  }
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
    if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
      isse_yuy2_i_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
                          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
                          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    } else {
      mmx_yuy2_i_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
                          dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
                          dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    }
  } else {
    if ((env->GetCPUFlags() & CPUF_INTEGER_SSE)) {
      isse_yuy2_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
                        dst->GetWritePtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V),
                        dst->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U), src->GetHeight());
    } else {
      mmx_yuy2_to_yv12(src->GetReadPtr(), src->GetRowSize(), src->GetPitch(),
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





