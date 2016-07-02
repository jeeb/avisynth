
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

#include "conditional_functions.h"
#include "../../core/internal.h"
#include <avs/config.h>
#include <avs/minmax.h>
#include <avs/alignment.h>
#include <emmintrin.h>


extern const AVSFunction Conditional_funtions_filters[] = {
  {  "AverageLuma",    BUILTIN_FUNC_PREFIX, "c[offset]i", AveragePlane::Create_y },
  {  "AverageChromaU", BUILTIN_FUNC_PREFIX, "c[offset]i", AveragePlane::Create_u },
  {  "AverageChromaV", BUILTIN_FUNC_PREFIX, "c[offset]i", AveragePlane::Create_v },
//{  "AverageSat","c[offset]i", AverageSat::Create }, Sum(SatLookup[U,V])/N, SatLookup[U,V]=1.4087*sqrt((U-128)**2+(V-128)**2)
//{  "AverageHue","c[offset]i", AverageHue::Create }, Sum(HueLookup[U,V])/N, HueLookup[U,V]=40.5845*Atan2(U-128,V-128)

  {  "RGBDifference",     BUILTIN_FUNC_PREFIX, "cc", ComparePlane::Create_rgb },
  {  "LumaDifference",    BUILTIN_FUNC_PREFIX, "cc", ComparePlane::Create_y },
  {  "ChromaUDifference", BUILTIN_FUNC_PREFIX, "cc", ComparePlane::Create_u },
  {  "ChromaVDifference", BUILTIN_FUNC_PREFIX, "cc", ComparePlane::Create_v },
//{  "SatDifference","cc", CompareSat::Create }, Sum(Abs(SatLookup[U1,V1]-SatLookup[U2,V2]))/N
//{  "HueDifference","cc", CompareHue::Create }, Sum(Abs(HueLookup[U1,V1]-HueLookup[U2,V2]))/N

  {  "YDifferenceFromPrevious",   BUILTIN_FUNC_PREFIX, "c", ComparePlane::Create_prev_y },
  {  "UDifferenceFromPrevious",   BUILTIN_FUNC_PREFIX, "c", ComparePlane::Create_prev_u },
  {  "VDifferenceFromPrevious",   BUILTIN_FUNC_PREFIX, "c", ComparePlane::Create_prev_v },
  {  "RGBDifferenceFromPrevious", BUILTIN_FUNC_PREFIX, "c", ComparePlane::Create_prev_rgb },
//{  "SatDifferenceFromPrevious","c", CompareSat::Create_prev },
//{  "HueDifferenceFromPrevious","c", CompareHue::Create_prev },

  {  "YDifferenceToNext",   BUILTIN_FUNC_PREFIX, "c[offset]i", ComparePlane::Create_next_y },
  {  "UDifferenceToNext",   BUILTIN_FUNC_PREFIX, "c[offset]i", ComparePlane::Create_next_u },
  {  "VDifferenceToNext",   BUILTIN_FUNC_PREFIX, "c[offset]i", ComparePlane::Create_next_v },
  {  "RGBDifferenceToNext", BUILTIN_FUNC_PREFIX, "c[offset]i", ComparePlane::Create_next_rgb },
//{  "SatDifferenceFromNext","c[offset]i", CompareSat::Create_next },
//{  "HueDifferenceFromNext","c[offset]i", CompareHue::Create_next },
  {  "YPlaneMax",    BUILTIN_FUNC_PREFIX, "c[threshold]f[offset]i", MinMaxPlane::Create_max_y },
  {  "YPlaneMin",    BUILTIN_FUNC_PREFIX, "c[threshold]f[offset]i", MinMaxPlane::Create_min_y },
  {  "YPlaneMedian", BUILTIN_FUNC_PREFIX, "c[offset]i", MinMaxPlane::Create_median_y },
  {  "UPlaneMax",    BUILTIN_FUNC_PREFIX, "c[threshold]f[offset]i", MinMaxPlane::Create_max_u },
  {  "UPlaneMin",    BUILTIN_FUNC_PREFIX, "c[threshold]f[offset]i", MinMaxPlane::Create_min_u },
  {  "UPlaneMedian", BUILTIN_FUNC_PREFIX, "c[offset]i", MinMaxPlane::Create_median_u },
  {  "VPlaneMax",    BUILTIN_FUNC_PREFIX, "c[threshold]f", MinMaxPlane::Create_max_v },
  {  "VPlaneMin",    BUILTIN_FUNC_PREFIX, "c[threshold]f", MinMaxPlane::Create_min_v },
  {  "VPlaneMedian", BUILTIN_FUNC_PREFIX, "c[offset]i", MinMaxPlane::Create_median_v },
  {  "YPlaneMinMaxDifference", BUILTIN_FUNC_PREFIX, "c[threshold]f[offset]i", MinMaxPlane::Create_minmax_y },
  {  "UPlaneMinMaxDifference", BUILTIN_FUNC_PREFIX, "c[threshold]f", MinMaxPlane::Create_minmax_u },
  {  "VPlaneMinMaxDifference", BUILTIN_FUNC_PREFIX, "c[threshold]f", MinMaxPlane::Create_minmax_v },

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


AVSValue AveragePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_Y, args[1].AsInt(0), env);
}


AVSValue AveragePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_U, args[1].AsInt(0), env);
}


AVSValue AveragePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return AvgPlane(args[0], user_data, PLANAR_V, args[1].AsInt(0), env);
}

// Average plane
static size_t get_sum_of_pixels_c(const BYTE* srcp, size_t height, size_t width, size_t pitch) {
  unsigned int accum = 0;
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      accum += srcp[x];
    }
    srcp += pitch;
  }
  return accum;
}

static size_t get_sum_of_pixels_sse2(const BYTE* srcp, size_t height, size_t width, size_t pitch) {
  size_t mod16_width = width / 16 * 16;
  int result = 0;
  __m128i sum = _mm_setzero_si128();
  __m128i zero = _mm_setzero_si128();

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));
      __m128i sad = _mm_sad_epu8(src, zero);
      sum = _mm_add_epi32(sum, sad);
    }

    for (size_t x = mod16_width; x < width; ++x) {
      result += srcp[x];
    }

    srcp += pitch;
  }
  __m128i upper = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(sum)));
  sum = _mm_add_epi32(sum, upper);
  result += _mm_cvtsi128_si32(sum);
  return result;
}

#ifdef X86_32
static size_t get_sum_of_pixels_isse(const BYTE* srcp, size_t height, size_t width, size_t pitch) {
  size_t mod8_width = width / 8 * 8;
  int result = 0;
  __m64 sum = _mm_setzero_si64();
  __m64 zero = _mm_setzero_si64();

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp + x);
      __m64 sad = _mm_sad_pu8(src, zero);
      sum = _mm_add_pi32(sum, sad);
    }

    for (size_t x = mod8_width; x < width; ++x) {
      result += srcp[x];
    }

    srcp += pitch;
  }
  result += _mm_cvtsi64_si32(sum);
  _mm_empty();
  return result;
}
#endif



AVSValue AveragePlane::AvgPlane(AVSValue clip, void* user_data, int plane, int offset, IScriptEnvironment* env)
{
  if (!clip.IsClip())
    env->ThrowError("Average Plane: No clip supplied!");
  if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE))
    env->ThrowError("Average Plane: Requires Integer SSE capable CPU.");

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();

  if (!vi.IsPlanar())
    env->ThrowError("Average Plane: Only planar images (as YV12) supported!");

  AVSValue cn = env->GetVarDef("current_frame");
  if (!cn.IsInt())
    env->ThrowError("Average Plane: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = min(max(n+offset,0), vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  int height = src->GetHeight(plane);
  int width = src->GetRowSize(plane);
  int pitch = src->GetPitch(plane);

  if (width == 0 || height == 0)
    env->ThrowError("Average Plane: No chroma planes in Y8!");

  size_t sum = 0;
  
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && width >= 16) {
    sum = get_sum_of_pixels_sse2(srcp, height, width, pitch);
  } else 
#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
    sum = get_sum_of_pixels_isse(srcp, height, width, pitch);
  } else 
#endif
  {
    sum = get_sum_of_pixels_c(srcp, height, width, pitch);
  }

  float f = (float)((double)sum / (height * width));

  return (AVSValue)f;
}


AVSValue ComparePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_Y, env);
}


AVSValue ComparePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_U, env);
}


AVSValue ComparePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, PLANAR_V, env);
}

AVSValue ComparePlane::Create_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlane(args[0],args[1], user_data, -1 , env);
}


AVSValue ComparePlane::Create_prev_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_Y, env);
}

AVSValue ComparePlane::Create_prev_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_U, env);
}

AVSValue ComparePlane::Create_prev_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, PLANAR_V, env);
}

AVSValue ComparePlane::Create_prev_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, -1, -1, env);
}


AVSValue ComparePlane::Create_next_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_Y, env);
}

AVSValue ComparePlane::Create_next_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_U, env);
}

AVSValue ComparePlane::Create_next_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), PLANAR_V, env);
}

AVSValue ComparePlane::Create_next_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return CmpPlaneSame(args[0], user_data, args[1].AsInt(1), -1, env);
}



static size_t get_sad_c(const BYTE* c_plane, const BYTE* tplane, size_t height, size_t width, size_t c_pitch, size_t t_pitch) {
  size_t accum = 0;
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x++) {
      accum += abs(tplane[x] - c_plane[x]);
    }
    c_plane += c_pitch;
    tplane += t_pitch;
  }
  return accum;

}

static size_t get_sad_rgb_c(const BYTE* c_plane, const BYTE* tplane, size_t height, size_t width, size_t c_pitch, size_t t_pitch) {
  size_t accum = 0;
  for (size_t y = 0; y < height; y++) {
    for (size_t x = 0; x < width; x+=4) {
      accum += abs(tplane[x] - c_plane[x]);
      accum += abs(tplane[x+1] - c_plane[x+1]);
      accum += abs(tplane[x+2] - c_plane[x+2]);
    }
    c_plane += c_pitch;
    tplane += t_pitch;
  }
  return accum;

}

static size_t get_sad_sse2(const BYTE* src_ptr, const BYTE* other_ptr, size_t height, size_t width, size_t src_pitch, size_t other_pitch) {
  size_t mod16_width = width / 16 * 16;
  size_t result = 0;
  __m128i sum = _mm_setzero_si128();
  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(src_ptr + x));
      __m128i other = _mm_load_si128(reinterpret_cast<const __m128i*>(other_ptr + x));
      __m128i sad = _mm_sad_epu8(src, other);
      sum = _mm_add_epi32(sum, sad);
    }

    for (size_t x = mod16_width; x < width; ++x) {
      result += std::abs(src_ptr[x] - other_ptr[x]);
    }

    src_ptr += src_pitch;
    other_ptr += other_pitch;
  }
  __m128i upper = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(sum)));
  sum = _mm_add_epi32(sum, upper);
  result += _mm_cvtsi128_si32(sum);
  return result;
}

static size_t get_sad_rgb_sse2(const BYTE* src_ptr, const BYTE* other_ptr, size_t height, size_t width, size_t src_pitch, size_t other_pitch) {
  size_t mod16_width = width / 16 * 16;
  size_t result = 0;
  __m128i sum = _mm_setzero_si128();
  __m128i rgb_mask = _mm_set1_epi32(0x00FFFFFF);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod16_width; x+=16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(src_ptr + x));
      __m128i other = _mm_load_si128(reinterpret_cast<const __m128i*>(other_ptr + x));
      src = _mm_and_si128(src, rgb_mask);
      other = _mm_and_si128(other, rgb_mask);
      __m128i sad = _mm_sad_epu8(src, other);
      sum = _mm_add_epi32(sum, sad);
    }

    for (size_t x = mod16_width; x < width; ++x) {
      result += std::abs(src_ptr[x] - other_ptr[x]);
    }

    src_ptr += src_pitch;
    other_ptr += other_pitch;
  }
  __m128i upper = _mm_castps_si128(_mm_movehl_ps(_mm_setzero_ps(), _mm_castsi128_ps(sum)));
  sum = _mm_add_epi32(sum, upper);
  result += _mm_cvtsi128_si32(sum);
  return result;
}

#ifdef X86_32

static size_t get_sad_isse(const BYTE* src_ptr, const BYTE* other_ptr, size_t height, size_t width, size_t src_pitch, size_t other_pitch) {
  size_t mod8_width = width / 8 * 8;
  size_t result = 0;
  __m64 sum = _mm_setzero_si64();

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(src_ptr + x);
      __m64 other = *reinterpret_cast<const __m64*>(other_ptr + x);
      __m64 sad = _mm_sad_pu8(src, other);
      sum = _mm_add_pi32(sum, sad);
    }

    for (size_t x = mod8_width; x < width; ++x) {
      result += abs(src_ptr[x] - other_ptr[x]);
    }

    src_ptr += src_pitch;
    other_ptr += other_pitch;
  }
  result += _mm_cvtsi64_si32(sum);
  _mm_empty();
  return result;
}

static size_t get_sad_rgb_isse(const BYTE* src_ptr, const BYTE* other_ptr, size_t height, size_t width, size_t src_pitch, size_t other_pitch) {
  size_t mod8_width = width / 8 * 8;
  size_t result = 0;
  __m64 rgb_mask = _mm_set1_pi32(0x00FFFFFF);
  __m64 sum = _mm_setzero_si64();

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(src_ptr + x);
      __m64 other = *reinterpret_cast<const __m64*>(other_ptr + x);
      src = _mm_and_si64(src, rgb_mask);
      other = _mm_and_si64(other, rgb_mask);
      __m64 sad = _mm_sad_pu8(src, other);
      sum = _mm_add_pi32(sum, sad);
    }

    for (size_t x = mod8_width; x < width; ++x) {
      result += abs(src_ptr[x] - other_ptr[x]);
    }

    src_ptr += src_pitch;
    other_ptr += other_pitch;
  }
  result += _mm_cvtsi64_si32(sum);
  _mm_empty();
  return result;
}

#endif



AVSValue ComparePlane::CmpPlane(AVSValue clip, AVSValue clip2, void* user_data, int plane, IScriptEnvironment* env)
{
  if (!clip.IsClip())
    env->ThrowError("Plane Difference: No clip supplied!");
  if (!clip2.IsClip())
    env->ThrowError("Plane Difference: Second parameter is not a clip!");

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


  AVSValue cn = env->GetVarDef("current_frame");
  if (!cn.IsInt())
    env->ThrowError("Plane Difference: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = clamp(n,0,vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);
  PVideoFrame src2 = child2->GetFrame(n,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  const BYTE* srcp2 = src2->GetReadPtr(plane);
  const int height = src->GetHeight(plane);
  const int width = src->GetRowSize(plane);
  const int pitch = src->GetPitch(plane);
  const int height2 = src2->GetHeight(plane);
  const int width2 = src2->GetRowSize(plane);
  const int pitch2 = src2->GetPitch(plane);

  if (width == 0 || height == 0)
    env->ThrowError("Plane Difference: No chroma planes in Y8!");

  if (height != height2 || width != width2)
    env->ThrowError("Plane Difference: Images are not the same size!");

  size_t sad = 0;
  if (vi.IsRGB32()) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(srcp2, 16) && width >= 16) {
      sad = get_sad_rgb_sse2(srcp, srcp2, height, width, pitch, pitch2);
    } else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
        sad = get_sad_rgb_isse(srcp, srcp2, height, width, pitch, pitch2);
      } else 
#endif
      {
        sad = get_sad_rgb_c(srcp, srcp2, height, width, pitch, pitch2);
      }
  } else {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(srcp2, 16) && width >= 16) {
      sad = get_sad_sse2(srcp, srcp2, height, width, pitch, pitch2);
    } else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
        sad = get_sad_isse(srcp, srcp2, height, width, pitch, pitch2);
      } else 
#endif
      {
        sad = get_sad_c(srcp, srcp2, height, width, pitch, pitch2);
      }
  }

  float f;

  if (vi.IsRGB32())
    f = (float)((double)(sad * 4) / (height * width * 3));
  else
    f = (float)((double)sad / (height * width));

  return (AVSValue)f;
}


AVSValue ComparePlane::CmpPlaneSame(AVSValue clip, void* user_data, int offset, int plane, IScriptEnvironment* env)
{
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

  AVSValue cn = env->GetVarDef("current_frame");
  if (!cn.IsInt())
    env->ThrowError("Plane Difference: This filter can only be used within run-time filters");

  int n = cn.AsInt();
  n = clamp(n,0,vi.num_frames-1);
  int n2 = clamp(n+offset,0,vi.num_frames-1);

  PVideoFrame src = child->GetFrame(n,env);
  PVideoFrame src2 = child->GetFrame(n2,env);

  const BYTE* srcp = src->GetReadPtr(plane);
  const BYTE* srcp2 = src2->GetReadPtr(plane);
  int height = src->GetHeight(plane);
  int width = src->GetRowSize(plane);
  int pitch = src->GetPitch(plane);
  int pitch2 = src2->GetPitch(plane);

  if (width == 0 || height == 0)
    env->ThrowError("Plane Difference: No chroma planes in Y8!");

  size_t sad = 0;
  if (vi.IsRGB32()) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(srcp2, 16) && width >= 16) {
      sad = get_sad_rgb_sse2(srcp, srcp2, height, width, pitch, pitch2);
    } else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
        sad = get_sad_rgb_isse(srcp, srcp2, height, width, pitch, pitch2);
      } else 
#endif
      {
        sad = get_sad_rgb_c(srcp, srcp2, height, width, pitch, pitch2);
      }
  } else {
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) && IsPtrAligned(srcp2, 16) && width >= 16) {
      sad = get_sad_sse2(srcp, srcp2, height, width, pitch, pitch2);
    } else
#ifdef X86_32
      if ((env->GetCPUFlags() & CPUF_INTEGER_SSE) && width >= 8) {
        sad = get_sad_isse(srcp, srcp2, height, width, pitch, pitch2);
      } else 
#endif
      {
        sad = get_sad_c(srcp, srcp2, height, width, pitch, pitch2);
      }
  }

  float f;

  if (vi.IsRGB32())
    f = (float)((double)(sad * 4) / (height * width * 3));
  else
    f = (float)((double)sad / (height * width));

  return (AVSValue)f;
}


// Y Planes functions

AVSValue MinMaxPlane::Create_max_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_Y, MAX, env);
}

AVSValue MinMaxPlane::Create_min_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_Y, MIN, env);
}

AVSValue MinMaxPlane::Create_median_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0, args[1].AsInt(0), PLANAR_Y, MIN, env);
}

AVSValue MinMaxPlane::Create_minmax_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_Y, MINMAX_DIFFERENCE, env);
}

// U Planes functions

AVSValue MinMaxPlane::Create_max_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_U, MAX, env);
}

AVSValue MinMaxPlane::Create_min_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_U, MIN, env);
}

AVSValue MinMaxPlane::Create_median_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0, args[1].AsInt(0), PLANAR_U, MIN, env);
}

AVSValue MinMaxPlane::Create_minmax_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_U, MINMAX_DIFFERENCE, env);
}
// V Planes functions

AVSValue MinMaxPlane::Create_max_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_V, MAX, env);
}

AVSValue MinMaxPlane::Create_min_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_V, MIN, env);
}

AVSValue MinMaxPlane::Create_median_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, 50.0, args[1].AsInt(0), PLANAR_V, MIN, env);
}

AVSValue MinMaxPlane::Create_minmax_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
  return MinMax(args[0], user_data, args[1].AsDblDef(0.0), args[2].AsInt(0), PLANAR_V, MINMAX_DIFFERENCE, env);
}


AVSValue MinMaxPlane::MinMax(AVSValue clip, void* user_data, double threshold, int offset, int plane, int mode, IScriptEnvironment* env) {
  unsigned int accum[256];

  if (!clip.IsClip())
    env->ThrowError("MinMax: No clip supplied!");

  PClip child = clip.AsClip();
  VideoInfo vi = child->GetVideoInfo();

  if (!vi.IsPlanar())
    env->ThrowError("MinMax: Image must be planar");

  // Get current frame number
  AVSValue cn = env->GetVarDef("current_frame");
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
  threshold /=100.0;  // Thresh now 0-1
  threshold = clamp(threshold, 0.0, 1.0);

  unsigned int tpixels = (unsigned int)(pixels*threshold);

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
