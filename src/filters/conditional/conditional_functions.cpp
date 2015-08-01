
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

#include "conditional_functions.h"


extern const AVSFunction Conditional_funtions_filters[] = {
  {  "AverageLuma","c[offset]i", AveragePlane::Create_y },
  {  "AverageChromaU","c[offset]i", AveragePlane::Create_u },
  {  "AverageChromaV","c[offset]i", AveragePlane::Create_v },
//{  "AverageSat","c[offset]i", AverageSat::Create }, Sum(SatLookup[U,V])/N, SatLookup[U,V]=1.4087*sqrt((U-128)**2+(V-128)**2)
//{  "AverageHue","c[offset]i", AverageHue::Create }, Sum(HueLookup[U,V])/N, HueLookup[U,V]=40.5845*Atan2(U-128,V-128)

  {  "RGBDifference","cc", ComparePlane::Create_rgb },
  {  "LumaDifference","cc", ComparePlane::Create_y },
  {  "ChromaUDifference","cc", ComparePlane::Create_u },
  {  "ChromaVDifference","cc", ComparePlane::Create_v },
//{  "SatDifference","cc", CompareSat::Create }, Sum(Abs(SatLookup[U1,V1]-SatLookup[U2,V2]))/N
//{  "HueDifference","cc", CompareHue::Create }, Sum(Abs(HueLookup[U1,V1]-HueLookup[U2,V2]))/N

  {  "YDifferenceFromPrevious","c", ComparePlane::Create_prev_y },
  {  "UDifferenceFromPrevious","c", ComparePlane::Create_prev_u },
  {  "VDifferenceFromPrevious","c", ComparePlane::Create_prev_v },
  {  "RGBDifferenceFromPrevious","c", ComparePlane::Create_prev_rgb },
//{  "SatDifferenceFromPrevious","c", CompareSat::Create_prev },
//{  "HueDifferenceFromPrevious","c", CompareHue::Create_prev },

  {  "YDifferenceToNext","c[offset]i", ComparePlane::Create_next_y },
  {  "UDifferenceToNext","c[offset]i", ComparePlane::Create_next_u },
  {  "VDifferenceToNext","c[offset]i", ComparePlane::Create_next_v },
  {  "RGBDifferenceToNext","c[offset]i", ComparePlane::Create_next_rgb },
//{  "SatDifferenceFromNext","c[offset]i", CompareSat::Create_next },
//{  "HueDifferenceFromNext","c[offset]i", CompareHue::Create_next },

  {  "YPlaneMax","c[threshold]f[offset]i", MinMaxPlane::Create_max_y },
  {  "YPlaneMin","c[threshold]f[offset]i", MinMaxPlane::Create_min_y },
  {  "YPlaneMedian","c[offset]i", MinMaxPlane::Create_median_y },
  {  "YPlaneMinMaxDifference","c[threshold]f[offset]i", MinMaxPlane::Create_minmax_y },
  {  "UPlaneMax","c[threshold]f[offset]i", MinMaxPlane::Create_max_u },
  {  "UPlaneMin","c[threshold]f[offset]i", MinMaxPlane::Create_min_u },
  {  "UPlaneMedian","c[offset]i", MinMaxPlane::Create_median_u },
  {  "UPlaneMinMaxDifference","c[threshold]f[offset]i", MinMaxPlane::Create_minmax_u },
  {  "VPlaneMax","c[threshold]f[offset]i", MinMaxPlane::Create_max_v },
  {  "VPlaneMin","c[threshold]f[offset]i", MinMaxPlane::Create_min_v },
  {  "VPlaneMedian","c[offset]i", MinMaxPlane::Create_median_v },
  {  "VPlaneMinMaxDifference","c[threshold]f[offset]i", MinMaxPlane::Create_minmax_v },

//{  "SatMax","c[threshold]f[offset]i", MinMaxPlane::Create_maxsat },  ++accum[SatLookup[U,V]]
//{  "SatMin","c[threshold]f[offset]i", MinMaxPlane::Create_minsat },
//{  "SatMedian","c[offset]i", MinMaxPlane::Create_mediansat },
//{  "SatMinMaxDifference","c[threshold]f[offset]i", MinMaxPlane::Create_minmaxsat },

//{  "HueMax","c[threshold]f[offset]i", MinMaxPlane::Create_maxhue },  ++accum[HueLookup[U,V]]
//{  "HueMin","c[threshold]f[offset]i", MinMaxPlane::Create_minhue },
//{  "HueMedian","c[offset]i", MinMaxPlane::Create_medianhue },
//{  "HueMinMaxDifference","c[threshold]f[offset]i", MinMaxPlane::Create_minmaxhue },

  { 0 }
};


// Helper function - exception protected wrapper

inline AVSValue GetVar(IScriptEnvironment* env, const char* name) {
  try {
    return env->GetVar(name);
  }
  catch (IScriptEnvironment::NotFound) {}

  return AVSValue();
}


AVSValue __cdecl AveragePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_Y, args[1].AsInt(0), env);
}


AVSValue __cdecl AveragePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_U, args[1].AsInt(0), env);
}


AVSValue __cdecl AveragePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_V, args[1].AsInt(0), env);
}

AVSValue AveragePlane::AvgPlane(AVSValue clip, void* user_data, int plane, int offset, IScriptEnvironment* env) {
  if (!clip.IsClip())
    env->ThrowError("Average Plane: No clip supplied!");
  if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE))
    env->ThrowError("Average Plane: Requires Integer SSE capable CPU.");

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();

  if (!vi.IsPlanar())
    env->ThrowError("Average Plane: Only planar images (as YV12) supported!");

  AVSValue cn = GetVar(env, "current_frame");
  if (!cn.IsInt())
    env->ThrowError("Average Plane: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = min(max(n+offset,0), vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  int h = src->GetHeight(plane);
  int w = src->GetRowSize(plane);
  int pitch = src->GetPitch(plane);

  if (w == 0 || h == 0)
    env->ThrowError("Average Plane: No chroma planes in Y8!");

  unsigned int b = 0;
  if (w & -16) b = isse_average_plane(srcp,           h, (w & -16), pitch);
  if (w &  15) b +=   C_average_plane(srcp+(w & -16), h, (w &  15), pitch);

  float f = (float)((double)b / (h * w));

  return (AVSValue)f;
}

// Average plane
unsigned int AveragePlane::C_average_plane(const BYTE* c_plane, int height, int width, int c_pitch) {
  unsigned int accum = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      accum += c_plane[x];
    }
    c_plane += c_pitch;
  }
  return accum;
}

unsigned int AveragePlane::isse_average_plane(const BYTE* c_plane, int height, int width, int c_pitch) {
  int hp=height;
  unsigned int returnvalue=0xbadbad00;
  __asm {
    xor ecx,ecx     // Height
    mov edx, c_pitch    //copy pitch
    pxor mm5,mm5  // Cleared
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    jmp yloopover
    align 16
yloop:
    inc ecx
    add esi,edx  // add pitch
yloopover:
    cmp ecx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[width]
    jge yloop

    movq mm0,[esi+eax]
    movq mm2,[esi+eax+8]
    psadbw mm0,mm5    // Sum of absolute difference (= sum of all pixels)
     psadbw mm2,mm5
    paddd mm6,mm0     // Add...
     paddd mm7,mm2

    add eax,16
    jmp xloop
endframe:
    paddd mm7,mm6
    movd returnvalue,mm7
    emms
  }
  return returnvalue;
}




AVSValue __cdecl ComparePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_Y, env);
}


AVSValue __cdecl ComparePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_U, env);
}


AVSValue __cdecl ComparePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, -1 , env);
}


AVSValue __cdecl ComparePlane::Create_prev_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_Y, env);
}

AVSValue __cdecl ComparePlane::Create_prev_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_U, env);
}

AVSValue __cdecl ComparePlane::Create_prev_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_prev_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, -1, env);
}


AVSValue __cdecl ComparePlane::Create_next_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_Y, env);
}

AVSValue __cdecl ComparePlane::Create_next_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_U, env);
}

AVSValue __cdecl ComparePlane::Create_next_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_next_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), -1, env);
}


AVSValue ComparePlane::CmpPlane(AVSValue clip, AVSValue clip2, void* user_data, int plane, IScriptEnvironment* env) {
  if (!clip.IsClip())
    env->ThrowError("Plane Difference: No clip supplied!");
  if (!clip2.IsClip())
    env->ThrowError("Plane Difference: Second parameter is not a clip!");
  bool ISSE = !!(env->GetCPUFlags() & CPUF_INTEGER_SSE);

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();
  PClip child2 = clip2.AsClip();
  VideoInfo vi2 = child2->GetVideoInfo();
  if (plane !=-1 ) {
    if (!vi.IsPlanar())
      env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
    if (!vi2.IsPlanar())
      env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
  } else {
    if (!vi.IsRGB())
      env->ThrowError("RGB Difference: RGB difference can only be tested on RGB images! (clip 1)");
    if (!vi2.IsRGB())
      env->ThrowError("RGB Difference: RGB difference can only be tested on RGB images! (clip 2)");
    plane = 0;
  }


  AVSValue cn = GetVar(env, "current_frame");
  if (!cn.IsInt())
    env->ThrowError("Plane Difference: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = min(max(n,0),vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);
  PVideoFrame src2 = child2->GetFrame(n,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  const BYTE* srcp2 = src2->GetReadPtr(plane);
  const int h = src->GetHeight(plane);
  const int w = src->GetRowSize(plane);
  const int pitch = src->GetPitch(plane);
  const int h2 = src2->GetHeight(plane);
  const int w2 = src2->GetRowSize(plane);
  const int pitch2 = src2->GetPitch(plane);

  if (w == 0 || h == 0)
    env->ThrowError("Plane Difference: No chroma planes in Y8!");

  if (h != h2 || w != w2)
    env->ThrowError("Plane Difference: Images are not the same size!");

  unsigned int b = 0;
  if (vi.IsRGB32()) {
    if (ISSE) {
      if (w & -16) b = isse_scenechange_rgb_16(srcp,           srcp2,           h, (w & -16), pitch, pitch2);
      if (w &  15) b +=   C_scenechange_rgb_16(srcp+(w & -16), srcp2+(w & -16), h, (w &  15), pitch, pitch2);
    }
    else
      b = C_scenechange_rgb_16(srcp, srcp2, h, w, pitch, pitch2);
  } else {
    if (ISSE) {
      if (w & -16) b = isse_scenechange_16(srcp,           srcp2,           h, (w & -16), pitch, pitch2);
      if (w &  15) b +=   C_scenechange_16(srcp+(w & -16), srcp2+(w & -16), h, (w &  15), pitch, pitch2);
    }
    else
      b = C_scenechange_16(srcp, srcp2, h, w, pitch, pitch2);
  }

  float f;

  if (vi.IsRGB32())
    f = (float)((double)(b * 4) / (h * w * 3));
  else
    f = (float)((double)b / (h * w));

  return (AVSValue)f;
}


AVSValue ComparePlane::CmpPlaneSame(AVSValue clip, void* user_data, int offset, int plane, IScriptEnvironment* env) {
  if (!clip.IsClip())
    env->ThrowError("Plane Difference: No clip supplied!");

  bool ISSE = !!(env->GetCPUFlags() & CPUF_INTEGER_SSE);

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();
  if (plane ==-1 ) {
    if (!vi.IsRGB())
      env->ThrowError("RGB Difference: RGB difference can only be calculated on RGB images");
    plane = 0;
  } else {
    if (!vi.IsPlanar())
      env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
  }

  AVSValue cn = GetVar(env, "current_frame");
  if (!cn.IsInt())
    env->ThrowError("Plane Difference: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = min(max(n,0),vi.num_frames-1);
  int n2 = min(max(n+offset,0),vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);
  PVideoFrame src2 = child->GetFrame(n2,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  const BYTE* srcp2 = src2->GetReadPtr(plane);
  int h = src->GetHeight(plane);
  int w = src->GetRowSize(plane);
  int pitch = src->GetPitch(plane);
  int pitch2 = src2->GetPitch(plane);

  if (w == 0 || h == 0)
    env->ThrowError("Plane Difference: No chroma planes in Y8!");

  unsigned int b = 0;
  if (vi.IsRGB32()) {
    if (ISSE) {
      if (w & -16) b = isse_scenechange_rgb_16(srcp,           srcp2,           h, (w & -16), pitch, pitch2);
      if (w &  15) b +=   C_scenechange_rgb_16(srcp+(w & -16), srcp2+(w & -16), h, (w &  15), pitch, pitch2);
    }
    else
     b = C_scenechange_rgb_16(srcp, srcp2, h, w, pitch, pitch2);
  } else {
    if (ISSE) {
      if (w & -16) b = isse_scenechange_16(srcp,           srcp2,           h, (w & -16), pitch, pitch2);
      if (w &  15) b +=   C_scenechange_16(srcp+(w & -16), srcp2+(w & -16), h, (w &  15), pitch, pitch2);
    }
    else
      b = C_scenechange_16(srcp, srcp2, h, w, pitch, pitch2);
  }

  float f;

  if (vi.IsRGB32())
    f = (float)((double)(b * 4) / (h * w * 3));
  else
    f = (float)((double)b / (h * w));

  return (AVSValue)f;
}


// Y Planes functions

AVSValue __cdecl MinMaxPlane::Create_max_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_Y, MAX, env);
}

AVSValue __cdecl MinMaxPlane::Create_min_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_Y, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_median_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0f, args[1].AsInt(0), PLANAR_Y, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_minmax_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_Y, MINMAX_DIFFERENCE, env);
}

// U Planes functions

AVSValue __cdecl MinMaxPlane::Create_max_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_U, MAX, env);
}

AVSValue __cdecl MinMaxPlane::Create_min_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_U, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_median_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0f, args[1].AsInt(0), PLANAR_U, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_minmax_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_U, MINMAX_DIFFERENCE, env);
}
// V Planes functions

AVSValue __cdecl MinMaxPlane::Create_max_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_V, MAX, env);
}

AVSValue __cdecl MinMaxPlane::Create_min_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_V, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_median_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0f, args[1].AsInt(0), PLANAR_V, MIN, env);
}

AVSValue __cdecl MinMaxPlane::Create_minmax_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsFloat(0.0f), args[2].AsInt(0), PLANAR_V, MINMAX_DIFFERENCE, env);
}


AVSValue MinMaxPlane::MinMax(AVSValue clip, void* user_data, float threshold, int offset, int plane, int mode, IScriptEnvironment* env) {
  unsigned int accum[256];

  if (!clip.IsClip())
    env->ThrowError("MinMax: No clip supplied!");

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();

  if (!vi.IsPlanar())
    env->ThrowError("MinMax: Image must be planar");

  // Get current frame number
  AVSValue cn = GetVar(env, "current_frame");
  if (!cn.IsInt())
    env->ThrowError("MinMax: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = min(max(n+offset,0),vi.num_frames-1);

  // Prepare the source
  PVideoFrame src = child->GetFrame(n, env);

  const BYTE* srcp = src->GetReadPtr(plane);
  int pitch = src->GetPitch(plane);
  int w = src->GetRowSize(plane);
  int h = src->GetHeight(plane);

  if (w == 0 || h == 0)
    env->ThrowError("MinMax: No chroma planes in Y8!");

  // Reset accumulators
  for (int i=0;i<256;i++) {
    accum[i]=0;
  }

  // Count each component.
  for (int y=0;y<h;y++) {
    for (int x=0;x<w;x++) {
       accum[srcp[x]]++;
    }
    srcp+=pitch;
  }

  int pixels = w*h;
  threshold /=100.0f;  // Thresh now 0-1
  threshold = max(0.0f,min(threshold,1.0f));

  unsigned int tpixels = (unsigned int)((float)pixels*threshold);

  // Find the value we need.
  if (mode == MIN) {
    unsigned int counted=0;
    for (int i = 0; i< 256;i++) {
      counted += accum[i];
      if (counted>tpixels)
        return AVSValue(i);
    }
    return AVSValue(255);
  }

  if (mode == MAX) {
    unsigned int counted=0;
    for (int i = 255; i>=0;i--) {
      counted += accum[i];
      if (counted>tpixels)
        return AVSValue(i);
    }
    return AVSValue(0);
  }

  if (mode == MINMAX_DIFFERENCE) {
    unsigned int counted=0;
    int i, t_min = 0;
    // Find min
    for (i = 0; i < 256;i++) {
      counted += accum[i];
      if (counted>tpixels) {
        t_min=i;
        break;
      }
    }

    // Find max
    counted=0;
    int t_max = 255;
    for (i = 255; i>=0;i--) {
      counted += accum[i];
      if (counted>tpixels) {
        t_max=i;
        break;
      }
    }

    return AVSValue(t_max-t_min);  // results <0 will be returned if threshold > 50
  }
  return AVSValue(-1);
}

unsigned int ComparePlane::C_scenechange_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
  unsigned int accum = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x++) {
      accum += abs(tplane[x] - c_plane[x]);
    }
    c_plane += c_pitch;
    tplane += t_pitch;
  }
  return accum;

}

unsigned int ComparePlane::C_scenechange_rgb_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
  unsigned int accum = 0;
  for (int y = 0; y < height; y++) {
    for (int x = 0; x < width; x+=4) {
      accum += abs(tplane[x] - c_plane[x]);
      accum += abs(tplane[x+1] - c_plane[x+1]);
      accum += abs(tplane[x+2] - c_plane[x+2]);
    }
    c_plane += c_pitch;
    tplane += t_pitch;
  }
  return accum;

}

/*********************
 * YV12 Scenechange detection.
 *
 * (c) 2003, Klaus Post
 *
 * ISSE, MOD 16 version.
 *
 * Returns an int of the accumulated absolute difference
 * between two planes.
 *
 * The absolute difference between two planes are returned as an int.
 * This version is optimized for mod16 widths. Others widths are allowed,
 *  but the remaining pixels are simply skipped.
 *********************/


unsigned int ComparePlane::isse_scenechange_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
  int wp=width;
  int hp=height;
  unsigned int returnvalue=0xbadbad00;
  __asm {
    xor ecx,ecx    // Height
    pxor mm5,mm5   // Maximum difference
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    mov edi, tplane
    jmp yloopover
    align 16
yloop:
    inc ecx
    add edi, t_pitch     // add pitch to both planes
    add esi, c_pitch
yloopover:
    cmp ecx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[wp]
    jge yloop

    movq mm0,[esi+eax]
     movq mm2,[esi+eax+8]
    movq mm1,[edi+eax]
     movq mm3,[edi+eax+8]
    psadbw mm0,mm1    // Sum of absolute difference
     psadbw mm2,mm3
    paddd mm6,mm0     // Add...
     paddd mm7,mm2

    add eax,16
    jmp xloop
endframe:
    paddd mm7,mm6
    movd returnvalue,mm7
    emms
  }
  return returnvalue;
}



/*********************
 * RGB32 Scenechange detection.
 *
 * (c) 2003, Klaus Post
 *
 * ISSE, MOD 16 version.
 *
 * Returns an int of the accumulated absolute difference
 * between two planes.
 *
 * The absolute difference between two planes are returned as an int.
 * This version is optimized for mod16 widths. Others widths are allowed,
 *  but the remaining pixels are simply skipped.
 *********************/


unsigned int ComparePlane::isse_scenechange_rgb_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
   __declspec(align(8)) static const __int64 Mask1 =  0x00ffffff00ffffff;
  int wp=width;
  int hp=height;
  unsigned int returnvalue=0xbadbad00;
  __asm {
    xor ecx,ecx     // Height
    movq mm5,[Mask1]  // Mask for RGB32
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    mov edi, tplane
    jmp yloopover
    align 16
yloop:
    inc ecx
    add edi, t_pitch     // add pitch to both planes
    add esi, c_pitch
yloopover:
    cmp ecx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[wp]
    jge yloop

    movq mm0,[esi+eax]
     movq mm2,[esi+eax+8]
    pand mm0,mm5
     pand mm2,mm5
    movq mm1,[edi+eax]
     movq mm3,[edi+eax+8]
    pand mm1,mm5
     pand mm3,mm5
    psadbw mm0,mm1    // Sum of absolute difference
     psadbw mm2,mm3
    paddd mm6,mm0     // Add...
     paddd mm7,mm2

    add eax,16
    jmp xloop
endframe:
    paddd mm7,mm6
    movd returnvalue,mm7
    emms
  }
  return returnvalue;
}

