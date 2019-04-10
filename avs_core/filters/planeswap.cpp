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


// Avisynth filter:  Swap planes
// by Klaus Post
// adapted by Richard Berg (avisynth-dev@richardberg.net)
// iSSE code by Ian Brabham


#include "planeswap.h"
#include "../core/internal.h"
#include <tmmintrin.h>
#include <algorithm>
#include <avs/alignment.h>
#include "../convert/convert_planar.h"
#include "../convert/convert_rgb.h"
#include "../convert/convert.h"
#include "stdint.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Swap_filters[] = {
  {  "SwapUV", BUILTIN_FUNC_PREFIX, "c", SwapUV::CreateSwapUV },
  {  "UToY",   BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateUToY },
  {  "VToY",   BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateVToY },
  {  "UToY8",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateUToY8 },
  {  "VToY8",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateVToY8 },
  {  "ExtractY",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateYToY8 }, // differs, YUY2 checks inside
  {  "ExtractU",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateUToY8 }, // differs, YUY2 checks inside
  {  "ExtractV",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateVToY8 }, // differs, YUY2 checks inside
  {  "ExtractA",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateAnyToY8, (void *)SwapUVToY::AToY8 },
  {  "ExtractR",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateAnyToY8, (void *)SwapUVToY::RToY8 },
  {  "ExtractG",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateAnyToY8, (void *)SwapUVToY::GToY8 },
  {  "ExtractB",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateAnyToY8, (void *)SwapUVToY::BToY8 },
  {  "YToUV",  BUILTIN_FUNC_PREFIX, "cc", SwapYToUV::CreateYToUV },
  {  "YToUV",  BUILTIN_FUNC_PREFIX, "ccc", SwapYToUV::CreateYToYUV },
  {  "YToUV",  BUILTIN_FUNC_PREFIX, "cccc", SwapYToUV::CreateYToYUVA }, // avs+ alpha planes
  {  "PlaneToY",  BUILTIN_FUNC_PREFIX, "c[plane]s", SwapUVToY::CreatePlaneToY8 },
  {  "CombinePlanes",  BUILTIN_FUNC_PREFIX, "c[planes]s[source_planes]s[pixel_type]s[sample_clip]c", CombinePlanes::CreateCombinePlanes, (void *)1},
  {  "CombinePlanes",  BUILTIN_FUNC_PREFIX, "cc[planes]s[source_planes]s[pixel_type]s[sample_clip]c", CombinePlanes::CreateCombinePlanes, (void *)2},
  {  "CombinePlanes",  BUILTIN_FUNC_PREFIX, "ccc[planes]s[source_planes]s[pixel_type]s[sample_clip]c", CombinePlanes::CreateCombinePlanes, (void *)3},
  {  "CombinePlanes",  BUILTIN_FUNC_PREFIX, "cccc[planes]s[source_planes]s[pixel_type]s[sample_clip]c", CombinePlanes::CreateCombinePlanes, (void *)4},
  { 0 }
};


/**************************************
 *  Swap - swaps UV on planar maps
 **************************************/

static void yuy2_swap_sse2(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  const __m128i mask = _mm_set1_epi16(0x00FF);

  for (int y = 0; y < height; ++y ) {
    for (int x = 0; x < width; x += 16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));
      __m128i swapped = _mm_shufflelo_epi16(src, _MM_SHUFFLE(2, 3, 0, 1));
      swapped = _mm_shufflehi_epi16(swapped, _MM_SHUFFLE(2, 3, 0, 1));
      swapped = _mm_or_si128(_mm_and_si128(mask, src), _mm_andnot_si128(mask, swapped));
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), swapped);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

#if defined(GCC) || defined(CLANG)
__attribute__((__target__("ssse3")))
#endif
static void yuy2_swap_ssse3(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  const __m128i mask = _mm_set_epi8(13, 14, 15, 12, 9, 10, 11, 8, 5, 6, 7, 4, 1, 2, 3, 0);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x += 16) {
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));
      __m128i dst = _mm_shuffle_epi8(src, mask);
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), dst);
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
}

#ifdef X86_32
static void yuy2_swap_isse(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  __m64 mask = _mm_set1_pi16(0x00FF);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x+= 8) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+x);
      __m64 swapped = _mm_shuffle_pi16(src, _MM_SHUFFLE(2, 3, 0, 1));
      swapped = _mm_or_si64(_mm_and_si64(mask, src), _mm_andnot_si64(mask, swapped));
      *reinterpret_cast<__m64*>(dstp + x) = swapped;
    }

    dstp += dst_pitch;
    srcp += src_pitch;
  }
  _mm_empty();
}
#endif

static void yuy2_swap_c(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int width, int height)
{
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < width; x += 4) {
      dstp[x + 0] = srcp[x + 0];
      dstp[x + 3] = srcp[x + 1];
      dstp[x + 2] = srcp[x + 2];
      dstp[x + 1] = srcp[x + 3];
    }
    srcp += src_pitch;
    dstp += dst_pitch;
  }
}

AVSValue __cdecl SwapUV::CreateSwapUV(AVSValue args, void* , IScriptEnvironment* env)
{
  PClip p = args[0].AsClip();
  if (p->GetVideoInfo().NumComponents() == 1)
    return p;
  return new SwapUV(p, env);
}


SwapUV::SwapUV(PClip _child, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
  if (!vi.IsYUV() && !vi.IsYUVA())
    env->ThrowError("SwapUV: YUV or YUVA data only!");
}

PVideoFrame __stdcall SwapUV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  
  if (vi.IsPlanar()) {
    // Abuse subframe to flip the UV plane pointers -- extremely fast but a bit naughty!
#ifdef SIZETMOD
    // !! be cautious when you subtract two unsigned size_t variables
    const size_t offs_v = src->GetOffset(PLANAR_V);
    const size_t offs_u = src->GetOffset(PLANAR_U);
    const int uvoffset = (offs_v > offs_u) ? (int)(offs_v - offs_u) : -(int)(offs_u - offs_v);
    // very naughty - don't do this at home!!
#else
    const int uvoffset = src->GetOffset(PLANAR_V) - src->GetOffset(PLANAR_U); // very naughty - don't do this at home!!
#endif
    if (vi.NumComponents() == 4) {
      IScriptEnvironment2* env2 = static_cast<IScriptEnvironment2*>(env);
      return env2->SubframePlanarA(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
        uvoffset, -uvoffset, src->GetPitch(PLANAR_V), 0);
    }
    else {
      return env->SubframePlanar(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
        uvoffset, -uvoffset, src->GetPitch(PLANAR_V));
    }
  }

  // YUY2
  PVideoFrame dst = env->NewVideoFrame(vi);
  const BYTE* srcp = src->GetReadPtr();
  BYTE* dstp = dst->GetWritePtr();
  int src_pitch = src->GetPitch();
  int dst_pitch = dst->GetPitch();
  int rowsize = src->GetRowSize();

  if ((env->GetCPUFlags() & CPUF_SSSE3) && IsPtrAligned(srcp, 16))
    yuy2_swap_ssse3(srcp, dstp, src_pitch, dst_pitch, rowsize, vi.height);
  else if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16))
    yuy2_swap_sse2(srcp, dstp, src_pitch, dst_pitch, rowsize, vi.height);
#ifdef X86_32
  else if (env->GetCPUFlags() & CPUF_INTEGER_SSE) // need pshufw
    yuy2_swap_isse(srcp, dstp, src_pitch, dst_pitch, rowsize, vi.height);
#endif
  else
    yuy2_swap_c(srcp, dstp, src_pitch, dst_pitch, rowsize, vi.height);
  return dst;
}


AVSValue __cdecl SwapUVToY::CreateUToY(AVSValue args, void* , IScriptEnvironment* env)
{
  return new SwapUVToY(args[0].AsClip(), UToY, env);
}

AVSValue __cdecl SwapUVToY::CreateUToY8(AVSValue args, void* , IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2UToY8 : UToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateYToY8(AVSValue args, void* , IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if(clip->GetVideoInfo().IsYUY2())
    return new ConvertToY8(clip, Rec601 /*n/a*/, env);
  else
    return new SwapUVToY(clip, YToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY(AVSValue args, void* , IScriptEnvironment* env)
{
  return new SwapUVToY(args[0].AsClip(), VToY, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY8(AVSValue args, void* , IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2VToY8 : VToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateAnyToY8(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  int mode = (int)(intptr_t)(user_data);
  PClip clip = args[0].AsClip();
  const VideoInfo& vi_input = clip->GetVideoInfo();

  // 161205: Packed RGB PlaneToY("R"),g,b,a or ExtractR,G,B,A
  // A generic way for using these PlaneToY() or Extract... functions for packed RGB types
  // We convert them to planar RGB (R,G,B plane reqest) or planar RGBA (only if A plane requested)
  if (vi_input.IsRGB() && !vi_input.IsPlanarRGB() && !vi_input.IsPlanarRGBA()) {
    if (mode == AToY8 || mode == RToY8 || mode == GToY8 || mode == BToY8) {
      clip = new PackedRGBtoPlanarRGB(clip, vi_input.IsRGB32() || vi_input.IsRGB64(), mode == AToY8);
    }
  }

  if(clip->GetVideoInfo().IsYUY2() && mode == YToY8)
    return new ConvertToY8(clip, Rec601 /*n/a*/, env);

  if (clip->GetVideoInfo().IsY() && mode == YToY8)
    return clip;

  return new SwapUVToY(clip, mode, env);
}

AVSValue __cdecl SwapUVToY::CreatePlaneToY8(AVSValue args, void* , IScriptEnvironment* env) {
    PClip clip = args[0].AsClip();

    const VideoInfo& vi_input = clip->GetVideoInfo();

    const char* plane = args[1].AsString("");
    int mode = 0;
    // enum {UToY=1, VToY, UToY8, VToY8, YUY2UToY8, YUY2VToY8, AToY8, RToY8, GToY8, BToY8, YToY8};
    if (!lstrcmpi(plane, "Y")) mode = YToY8;
    else if (!lstrcmpi(plane, "U")) mode = vi_input.IsYUY2() ? YUY2UToY8 : UToY8;
    else if (!lstrcmpi(plane, "V")) mode = vi_input.IsYUY2() ? YUY2VToY8 : VToY8;
    else if (!lstrcmpi(plane, "A")) mode = AToY8;
    else if (!lstrcmpi(plane, "R")) mode = RToY8;
    else if (!lstrcmpi(plane, "G")) mode = GToY8;
    else if (!lstrcmpi(plane, "B")) mode = BToY8;
    else env->ThrowError("PlaneToY: Invalid plane!");

    return CreateAnyToY8(args, (void* )(intptr_t)mode, env);
}


SwapUVToY::SwapUVToY(PClip _child, int _mode, IScriptEnvironment* env)
  : GenericVideoFilter(_child), mode(_mode)
{
  bool YUVmode = mode == YToY8 || mode == UToY8 || mode == VToY8 || mode == UToY || mode == VToY || mode == YUY2UToY8 || mode == YUY2VToY8;
  bool YUVAmode = mode == YToY8 || mode == UToY8 || mode == VToY8;
  bool RGBmode = mode == RToY8 || mode == GToY8 || mode == BToY8;
  bool Alphamode = mode == AToY8;

  if(!vi.IsYUVA() && !vi.IsPlanarRGBA() && Alphamode)
      env->ThrowError("PlaneToY: Clip has no Alpha channel!");

  if (!vi.IsYUV() && !vi.IsYUVA() && YUVmode )
    env->ThrowError("PlaneToY: clip is not YUV!");

  if (!vi.IsPlanarRGB() && !vi.IsPlanarRGBA() && RGBmode )
      env->ThrowError("PlaneToY: clip is not planar RGB!");

  if (vi.NumComponents() == 1 && mode != YToY8)
    env->ThrowError("PlaneToY: channel cannot be extracted from a greyscale clip!");

  if(YUVmode && (mode!=YToY8)){
    vi.height >>= vi.GetPlaneHeightSubsampling(PLANAR_U);
    vi.width  >>= vi.GetPlaneWidthSubsampling(PLANAR_U);
  }

  if (mode == YToY8 || mode == UToY8 || mode == VToY8 || mode == YUY2UToY8 || mode == YUY2VToY8 || RGBmode || Alphamode)
  {
    switch (vi.BitsPerComponent()) // although name is Y8, it means that greyscale stays in the same bitdepth
    {
    case 8: vi.pixel_type = VideoInfo::CS_Y8; break;
    case 10: vi.pixel_type = VideoInfo::CS_Y10; break;
    case 12: vi.pixel_type = VideoInfo::CS_Y12; break;
    case 14: vi.pixel_type = VideoInfo::CS_Y14; break;
    case 16: vi.pixel_type = VideoInfo::CS_Y16; break;
    case 32: vi.pixel_type = VideoInfo::CS_Y32; break;
    }
  }
}

static void yuy2_uvtoy_sse2(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int dst_width, int height, int pos)
{
  const __m128i chroma = _mm_set1_epi32(0x80008000);
  const __m128i mask = _mm_set1_epi32(0x000000FF);
  pos *= 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < dst_width; x += 16) {
      __m128i s0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp + 2 * x));
      __m128i s1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp + 2 * x + 16));
      s0 = _mm_and_si128(mask, _mm_srli_epi32(s0, pos));
      s1 = _mm_and_si128(mask, _mm_srli_epi32(s1, pos));
      s0 = _mm_packs_epi32(s0, s1);
      s0 = _mm_or_si128(s0, chroma);
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), s0);
    }
    srcp += src_pitch;
    dstp += dst_pitch;
  }
}

static void yuy2_uvtoy8_sse2(const BYTE* srcp, BYTE* dstp, int src_pitch, int dst_pitch, int dst_width, int height, int pos)
{
  const __m128i mask = _mm_set1_epi32(0x000000FF);
  pos *= 8;

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < dst_width; x += 8) {
      __m128i s0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp + 4 * x));
      __m128i s1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp + 4 * x + 16));
      s0 = _mm_and_si128(mask, _mm_srli_epi32(s0, pos));
      s1 = _mm_and_si128(mask, _mm_srli_epi32(s1, pos));
      s0 = _mm_packs_epi32(s0, s1);
      s0 = _mm_packus_epi16(s0, s0);
      _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp + x), s0);
    }
    srcp += src_pitch;
    dstp += dst_pitch;
  }
}

template <typename T>
static void fill_plane(BYTE* dstp, int rowsize, int height, int pitch, T val)
{
  rowsize /= sizeof(T);
  for (int y = 0; y < height; ++y) {
    std::fill_n(reinterpret_cast<T*>(dstp), rowsize, val);
    dstp += pitch;
  }
}

PVideoFrame __stdcall SwapUVToY::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  bool NonYUY2toY8 = true;
  int target_plane, source_plane;
  switch (mode) {
  case YToY8: source_plane = PLANAR_Y; target_plane = PLANAR_Y; break;
  case UToY8: source_plane = PLANAR_U; target_plane = PLANAR_Y; break;
  case VToY8: source_plane = PLANAR_V; target_plane = PLANAR_Y; break;
  case RToY8: source_plane = PLANAR_R; target_plane = PLANAR_G; break; // Planar RGB: GBR!
  case GToY8: source_plane = PLANAR_G; target_plane = PLANAR_G; break;
  case BToY8: source_plane = PLANAR_B; target_plane = PLANAR_G; break;
  case AToY8: source_plane = PLANAR_A; target_plane = vi.IsYUVA() ? PLANAR_Y : PLANAR_G; break; // Planar RGB: GBR!
  default: NonYUY2toY8 = false;
  }
  if (NonYUY2toY8) {
#ifdef SIZETMOD
    // !! be cautious when you subtract two unsigned size_t variables
    const size_t offs_src = src->GetOffset(source_plane);
    const size_t offs_tgt = src->GetOffset(target_plane);
    const int offset = (offs_src > offs_tgt) ? (int)(offs_src - offs_tgt) : -(int)(offs_tgt - offs_src);
#else
    const int offset = src->GetOffset(source_plane) - src->GetOffset(target_plane); // very naughty - don't do this at home!!
                                                                                    // Abuse Subframe to snatch the U/V/R/G/B/A plane
#endif
    return env->Subframe(src, offset, src->GetPitch(source_plane), src->GetRowSize(source_plane), src->GetHeight(source_plane));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (mode == YUY2UToY8 || mode == YUY2VToY8 || vi.IsYUY2()) {
    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = dst->GetWritePtr();
    int src_pitch = src->GetPitch();
    int dst_pitch = dst->GetPitch();
    int pos = (mode == YUY2UToY8 || mode == UToY) ? 1 : 3; // YUYV U=offset#1 V=offset#3

    if (vi.IsYUY2()) {  // YUY2 To YUY2
      int rowsize = dst->GetRowSize();
      if (env->GetCPUFlags() & CPUF_SSE2) {
        yuy2_uvtoy_sse2(srcp, dstp, src_pitch, dst_pitch, rowsize, vi.height, pos);
        return dst;
      }

      srcp += pos;
      for (int y = 0; y < vi.height; ++y) {
        for (int x = 0; x < rowsize; x += 2) {
          dstp[x + 0] = srcp[2 * x];
          dstp[x + 1] = 0x80;
        }
        srcp += src_pitch;
        dstp += dst_pitch;
      }
      return dst;
    }

    // YUY2 to Y8
    if (env->GetCPUFlags() & CPUF_SSE2) {
      yuy2_uvtoy8_sse2(srcp, dstp, src_pitch, dst_pitch, vi.width, vi.height, pos);
      return dst;
    }

    srcp += pos;
    for (int y = 0; y < vi.height; ++y) {
      for (int x = 0; x < vi.width; ++x) {
        dstp[x] = srcp[x * 4];
      }
      srcp += src_pitch;
      dstp += dst_pitch;
    }
    return dst;
  }

  // Planar to Planar
  const int plane = mode == UToY ? PLANAR_U : PLANAR_V;
  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y), src->GetReadPtr(plane),
    src->GetPitch(plane), src->GetRowSize(plane), src->GetHeight(plane));

  // Clear chroma
  int pitch = dst->GetPitch(PLANAR_U);
  int height = dst->GetHeight(PLANAR_U);
  int rowsize = dst->GetRowSize(PLANAR_U);
  BYTE* dstp_u = dst->GetWritePtr(PLANAR_U);
  BYTE* dstp_v = dst->GetWritePtr(PLANAR_V);

  if (vi.ComponentSize() == 1) {  // 8bit
    fill_plane<BYTE>(dstp_u, rowsize, height, pitch, 0x80);
    fill_plane<BYTE>(dstp_v, rowsize, height, pitch, 0x80);
  }
  else if (vi.ComponentSize() == 2) {  // 16bit
    uint16_t grey_val = 1 << (vi.BitsPerComponent() - 1); // 0x8000 for 16 bit
    fill_plane<uint16_t>(dstp_u, rowsize, height, pitch, grey_val);
    fill_plane<uint16_t>(dstp_v, rowsize, height, pitch, grey_val);
  }
  else {  // 32bit(float)
    float grey_val = uv8tof(128);
    fill_plane<float>(dstp_u, rowsize, height, pitch, grey_val);
    fill_plane<float>(dstp_v, rowsize, height, pitch, grey_val);
  }

  return dst;
}


AVSValue __cdecl SwapYToUV::CreateYToUV(AVSValue args, void* , IScriptEnvironment* env)
{
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), NULL , NULL, env);
}

AVSValue __cdecl SwapYToUV::CreateYToYUV(AVSValue args, void* , IScriptEnvironment* env)
{
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), NULL, env);
}

AVSValue __cdecl SwapYToUV::CreateYToYUVA(AVSValue args, void* , IScriptEnvironment* env)
{
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), args[3].AsClip(), env);
}

SwapYToUV::SwapYToUV(PClip _child, PClip _clip, PClip _clipY, PClip _clipA, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), clipY(_clipY), clipA(_clipA)
{
  if(!(vi.IsYUVA() || vi.IsY()) && clipA)
    env->ThrowError("YToUV: Only Y or YUVA data accepted when alpha clip is provided"); // Y, YUV and YUY2
  if (!vi.IsYUV() && !vi.IsYUVA())
  {
    env->ThrowError("YToUV: Only YUV or YUVA data accepted"); // Y, YUV and YUY2
  }

  const VideoInfo& vi2 = clip->GetVideoInfo();
  if (vi.height != vi2.height)
    env->ThrowError("YToUV: Clips do not have the same height (U & V mismatch) !");
  if (vi.width != vi2.width)
    env->ThrowError("YToUV: Clips do not have the same width (U & V mismatch) !");
  if (vi.IsYUY2() != vi2.IsYUY2()) 
    env->ThrowError("YToUV: YUY2 Clips must have same colorspace (U & V mismatch) !");

  // no third parameter: no Y clip
  if (!clipY) {
    if (vi.IsYUY2())
      vi.width *= 2;
    else if (vi.IsY()) {
      switch(vi.BitsPerComponent()) {
      case 8: vi.pixel_type = VideoInfo::CS_YV24; break;
      case 10: vi.pixel_type = VideoInfo::CS_YUV444P10; break;
      case 12: vi.pixel_type = VideoInfo::CS_YUV444P12; break;
      case 14: vi.pixel_type = VideoInfo::CS_YUV444P14; break;
      case 16: vi.pixel_type = VideoInfo::CS_YUV444P16; break;
      case 32: vi.pixel_type = VideoInfo::CS_YUV444PS; break;
      }
    }
    else {
      vi.height <<= vi.GetPlaneHeightSubsampling(PLANAR_U);
      vi.width <<= vi.GetPlaneWidthSubsampling(PLANAR_U);
    }
    return;
  }

  // Y clip parameter exists, Y channel will be copied from that
  const VideoInfo& vi3 = clipY->GetVideoInfo();
  if (vi.IsYUY2() != vi3.IsYUY2()) 
    env->ThrowError("YToUV: YUY2 Clips must have same colorspace (UV & Y mismatch) !");

  if (vi.IsYUY2()) {
    if (vi3.height != vi.height)
      env->ThrowError("YToUV: Y clip does not have the same height of the UV clips! (YUY2 mode)");
    vi.width *= 2;
    if (vi3.width != vi.width)
      env->ThrowError("YToUV: Y clip does not have the double width of the UV clips!");
    return;
  }

  if (clipA) {
    if(vi.IsYUY2())
      env->ThrowError("YToUV: YUY2 not supported with alpha clip");
    const VideoInfo& vi4 = clipA->GetVideoInfo();
    if (vi4.width != vi3.width || vi4.height != vi3.height) // Y width == A width
      env->ThrowError("YToUV: different Y and A clip dimensions");
    if(vi4.BitsPerComponent() != vi3.BitsPerComponent())
      env->ThrowError("YToUV: different Y and A clip bit depth");
  }

  // Autogenerate destination colorformat
  switch (vi.BitsPerComponent())
  { // CS_Sub_Width_2 and CS_Sub_Height_2 are 0, vi bitfield can or'd if change needed
  case  8: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420 : vi.pixel_type = VideoInfo::CS_YV12; break;
  case 10: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420P10 : vi.pixel_type = VideoInfo::CS_YUV420P10; break;
  case 12: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420P12 : vi.pixel_type = VideoInfo::CS_YUV420P12; break;
  case 14: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420P14 : VideoInfo::CS_YUV420P14; break;
  case 16: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420P16 : VideoInfo::CS_YUV420P16; break;
  case 32: vi.pixel_type = clipA ? VideoInfo::CS_YUVA420PS : VideoInfo::CS_YUV420PS; break;
  }

  if (vi3.width == vi.width) // Y width == U width -> subsampling 1:1
    vi.pixel_type |= VideoInfo::CS_Sub_Width_1;
  else if (vi3.width == vi.width * 2) // Y width == U width*2 -> horiz. subsampling 2
    vi.width *= 2; // YV12 subsampling CS_Sub_Width_2 is o.k.
  else if (vi3.width == vi.width * 4) { // Y width == U width*4 -> horiz. subsampling 4
    vi.pixel_type |= VideoInfo::CS_Sub_Width_4;
    vi.width *= 4; // final clip width is 3x of the U channel width
  }
  else
    env->ThrowError("YToUV: Video width ratio does not match any internal colorspace.");

  if (vi3.height == vi.height)
    vi.pixel_type |= VideoInfo::CS_Sub_Height_1;
  else if (vi3.height == vi.height * 2)
    vi.height *= 2;
  else if (vi3.height == vi.height * 4) {
    vi.pixel_type |= VideoInfo::CS_Sub_Height_4;
    vi.height *= 4;
  }
  else
    env->ThrowError("YToUV: Video height ratio does not match any internal colorspace.");
}

template <bool has_clipY>
static void yuy2_ytouv_sse2(const BYTE* srcp_y, const BYTE* srcp_u, const BYTE* srcp_v, BYTE* dstp, int pitch_y, int pitch_u, int pitch_v, int dst_pitch, int dst_rowsize, int height)
{
  const __m128i mask = _mm_set1_epi16(0x00FF);
  const __m128i zero = _mm_setzero_si128();
  const __m128i fill = _mm_set1_epi16(0x007e);

  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < dst_rowsize; x += 32) {
      __m128i u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp_u + x / 2));
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp_v + x / 2));
      __m128i uv = _mm_or_si128(_mm_and_si128(u, mask), _mm_slli_epi16(v, 8));
      __m128i uv_lo = _mm_unpacklo_epi8(zero, uv);
      __m128i uv_hi = _mm_unpackhi_epi8(zero, uv);
      if (has_clipY) {
        __m128i y_lo = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp_y + x));
        __m128i y_hi = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp_y + x + 16));
        uv_lo = _mm_or_si128(uv_lo, _mm_and_si128(y_lo, mask));
        uv_hi = _mm_or_si128(uv_hi, _mm_and_si128(y_hi, mask));
      }
      else {
        uv_lo = _mm_or_si128(uv_lo, fill);
        uv_hi = _mm_or_si128(uv_hi, fill);
      }
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), uv_lo);
      _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x + 16), uv_hi);
    }
    srcp_y += pitch_y;
    srcp_u += pitch_u;
    srcp_v += pitch_v;
    dstp += dst_pitch;
  }
}

template <bool has_clipY>
static void yuy2_ytouv_c(const BYTE* src_y, const BYTE* src_u, const BYTE* src_v, BYTE* dstp, int pitch_y, int pitch_u, int pitch_v, int dst_pitch, int dst_rowsize, int height)
{
  for (int y = 0; y < height; ++y) {
    for (int x = 0; x < dst_rowsize; x += 4) {
      dstp[x + 0] = has_clipY ? src_y[x] : 0x7e;
      dstp[x + 1] = src_u[x / 2];
      dstp[x + 2] = has_clipY ? src_y[x + 2] : 0x7e;
      dstp[x + 3] = src_v[x / 2];
    }
    src_y += pitch_y;
    src_u += pitch_u;
    src_v += pitch_v;
    dstp += dst_pitch;
  }
}

PVideoFrame __stdcall SwapYToUV::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame dst = env->NewVideoFrame(vi);
  
  if (vi.IsYUY2()) {
    const BYTE* srcp_u = src->GetReadPtr();
    const int pitch_u = src->GetPitch();

    PVideoFrame srcv = clip->GetFrame(n, env);
    const BYTE* srcp_v = srcv->GetReadPtr();
    const int pitch_v = srcv->GetPitch();

    BYTE* dstp = dst->GetWritePtr();
    const int rowsize = dst->GetRowSize();
    const int dst_pitch = dst->GetPitch();

    if (clipY) {
      PVideoFrame srcy = clipY->GetFrame(n, env);
      const BYTE* srcp_y = srcy->GetReadPtr();
      const int pitch_y = srcy->GetPitch();

      if (env->GetCPUFlags() & CPUF_SSE2)
        yuy2_ytouv_sse2<true>(srcp_y, srcp_u, srcp_v, dstp, pitch_y, pitch_u, pitch_v, dst_pitch, rowsize, vi.height);
      else
        yuy2_ytouv_c<true>(srcp_y, srcp_u, srcp_v, dstp, pitch_y, pitch_u, pitch_v, dst_pitch, rowsize, vi.height);
    }
    else if (env->GetCPUFlags() & CPUF_SSE2)
      yuy2_ytouv_sse2<false>(nullptr, srcp_u, srcp_v, dstp, 0, pitch_u, pitch_v, dst_pitch, rowsize, vi.height);
    else
      yuy2_ytouv_c<false>(nullptr, srcp_u, srcp_v, dstp, 0, pitch_u, pitch_v, dst_pitch, rowsize, vi.height);

    return dst;
  }

  // Planar:
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
              src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  
  src = clip->GetFrame(n, env);
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
              src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));

  if (clipA) {
    int source_plane = (clipA->GetVideoInfo().IsPlanarRGBA() ||
      clipA->GetVideoInfo().IsYUVA()) ? PLANAR_A : PLANAR_Y;
    src = clipA->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_A), dst->GetPitch(PLANAR_A),
      src->GetReadPtr(source_plane), src->GetPitch(source_plane), src->GetRowSize(source_plane), src->GetHeight(source_plane));
  }

  if (clipY) {
    src = clipY->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
    return dst;
  }

  // if no Y script was given, fill Y plane with neutral value
  // Luma = 126 (0x7e)
  BYTE* dstp = dst->GetWritePtr(PLANAR_Y);
  int rowsize = dst->GetRowSize(PLANAR_Y);
  int pitch = dst->GetPitch(PLANAR_Y);

  if (vi.ComponentSize() == 1)  // 8bit
    fill_plane<BYTE>(dstp, rowsize, vi.height, pitch, 0x7e);
  else if (vi.ComponentSize() == 2) { // 16bit
    uint16_t luma_val = 0x7e << (vi.BitsPerComponent() - 8);
    fill_plane<uint16_t>(dstp, rowsize, vi.height, pitch, luma_val);
  } else { // 32bit(float)
    fill_plane<float>(dstp, rowsize, vi.height, pitch, 126.0f / 256);
  }

  return dst;
}

// AVS+: Combine planes free-style for all planar formats
AVSValue __cdecl CombinePlanes::CreateCombinePlanes(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  int mode = (int)(intptr_t)(user_data);
  int target_planes_param = 0 + mode;
  int source_planes_param = 1 + mode;
  int pixel_type_param = 2 + mode;
  int sample_clip_param = 3 + mode;

  bool hasSampleClip = args[sample_clip_param].Defined();

  return new CombinePlanes(args[0].AsClip(),
    mode >= 2 ? args[1].AsClip() : nullptr,
    mode >= 3 ? args[2].AsClip() : nullptr,
    mode >= 4 ? args[3].AsClip() : nullptr,
    hasSampleClip ? args[sample_clip_param].AsClip() : nullptr,
    args[target_planes_param].AsString(""),
    args[source_planes_param].AsString(""),
    args[pixel_type_param].AsString(""),
    env);
}

CombinePlanes::CombinePlanes(PClip _child, PClip _clip2, PClip _clip3, PClip _clip4, PClip _sample,
  const char *_target_planes_str, const char *_source_planes_str, const char *_pixel_type, IScriptEnvironment* env)
  : GenericVideoFilter(_child)
{
  clips[0] = _child;
  clips[1] = _clip2;
  clips[2] = _clip3;
  clips[3] = _clip4;
  // planes(_planes), pixel_type(pixel_type)
  // getting target video format
  VideoInfo vi_default;
  memset(&vi_default, 0, sizeof(VideoInfo));

  bool videoFormatOverridden = false;

  if (_sample) {
    vi_default = _sample->GetVideoInfo();
    videoFormatOverridden = true;
  }
  else { // no sample video: format from first clip
    vi_default = child->GetVideoInfo();
  }
  // 1.) sample clip 2.) first clip 3.) pixel_type override
  // 4.) when input clips are greyscale, automatically use YUV(A)/RGB(A) depending on "planes" string
  if (*_pixel_type) {
    int i_pixel_type = GetPixelTypeFromName(_pixel_type);
    if (i_pixel_type == VideoInfo::CS_UNKNOWN)
      env->ThrowError("CombinePlanes: unknown pixel_type %s", _pixel_type);
    vi_default.pixel_type = i_pixel_type;
    videoFormatOverridden = true;
  }

  if (!vi_default.IsPlanar())
    env->ThrowError("CombinePlanes: output clip video format is not planar!");

  // autoconvert packed RGB or YUY2 inputs, in order to able to extract planes
  for (int i = 0; i < 4; i++) {
    if (!clips[i]) continue;
    const VideoInfo &vi_test = clips[i]->GetVideoInfo();
    if (vi_test.IsRGB() && !vi_test.IsPlanar()) {
      bool hasAlpha = vi_test.NumComponents() == 4;
      clips[i] = new PackedRGBtoPlanarRGB(clips[i], hasAlpha, hasAlpha);
    }
    else if (vi_test.IsYUY2()) {
      AVSValue emptyValue;
      clips[i] = new ConvertToPlanarGeneric(clips[i], VideoInfo::CS_YV16, false, emptyValue, emptyValue, emptyValue, env);
    }
  }

  int source_plane_count = (int)strlen(_source_planes_str); // no check here, can be 0
  int target_plane_count = (int)strlen(_target_planes_str);
  if (target_plane_count == 0)
    env->ThrowError("CombinePlanes: no target planes given!");
  int clip_count = clips[3] ? 4 : clips[2] ? 3 : clips[1] ? 2 : 1;
  if (target_plane_count < clip_count)
    env->ThrowError("CombinePlanes: more clips specified than target planes");

  // If no video format was forced and no input planes were given
  // and all the source clips are Y, then
  // we give it a try of easy greyscale->RGB(A) or YUV(A) conversion
  // depending on the _target_planes_str
  bool allIsGrey = true;
  for (int i = 0; i < clip_count; i++) {
    if (!clips[i]->GetVideoInfo().IsY()) {
      allIsGrey = false;
      break;
    }
  }

  if (!videoFormatOverridden && source_plane_count == 0) {
    if (allIsGrey) {
      // special case. Figure out RGB(A) or YUV(A) or Y
      bool allIsYUV = true;
      bool allIsRGB = true;
      for (int i = 0; i < target_plane_count; i++) {
        char ch = toupper(_target_planes_str[i]);
        if (ch == 'R' || ch == 'G' || ch == 'B') allIsYUV = false;
        if (ch == 'Y' || ch == 'U' || ch == 'V') allIsRGB = false;
      }
      if (allIsYUV || allIsRGB) {
        int new_pixel_type;
        if (allIsRGB)
          new_pixel_type = target_plane_count == 4 ? VideoInfo::CS_GENERIC_RGBAP : VideoInfo::CS_GENERIC_RGBP;
        else // if (allIsYUV)
          new_pixel_type = target_plane_count == 4 ? VideoInfo::CS_GENERIC_YUVA444 : VideoInfo::CS_GENERIC_YUV444;
        int bits_mask = clips[0]->GetVideoInfo().pixel_type & VideoInfo::CS_Sample_Bits_Mask;
        new_pixel_type |= bits_mask; // copy bit-depth from the first clip
        vi_default.pixel_type = new_pixel_type;
      }
    }
  }

  vi = vi_default;

  if(!vi_default.IsPlanar())
    env->ThrowError("CombinePlanes: target format must be planar!");

  if(target_plane_count > vi_default.NumComponents())
    env->ThrowError("CombinePlanes: too many target planes (%d)! Target video plane count is %d!", target_plane_count, vi_default.NumComponents());

  if(source_plane_count != 0 && source_plane_count != target_plane_count)
    env->ThrowError("CombinePlanes: source plane count must match with target plane count if provided!");

  // useful for later check
  bool targetIsYUV = vi_default.IsYUV() || vi_default.IsYUVA();
  bool targetHasAlpha = vi_default.IsYUVA() || vi_default.IsPlanarRGBA();
  bool targetIsY = vi_default.IsY();

  // class variables
  bits_per_pixel = vi_default.BitsPerComponent();
  pixelsize = vi_default.ComponentSize();
  planecount = target_plane_count;

  // if source plane is given, use it otherwise assume these
  const char * rgb_source_planes_str_def = "RGBA";
  const char * yuv_source_planes_str_def = allIsGrey ? "YYYY" : "YUVA";

  int last_clip_index = 0;
  for (int i = 0; i < target_plane_count; i++) {
    char ch = toupper(_target_planes_str[i]);
    bool isRGB = ch == 'R' || ch == 'G' || ch == 'B';
    bool isYUV = ch == 'Y' || ch == 'U' || ch == 'V';
    bool isAlpha = ch == 'A';
    if(!isRGB && !isYUV && !isAlpha)
      env->ThrowError("CombinePlanes: invalid plane definifion :%s", planes);
    if((targetIsYUV && isRGB) || (!targetIsYUV && isYUV) || (!targetHasAlpha && isAlpha) || (targetIsY && ch!='Y'))
      env->ThrowError("CombinePlanes: target has no such plane %c", ch);

    int current_target_plane;
    switch (ch) {
    case 'R': current_target_plane = PLANAR_R; break;
    case 'G': current_target_plane = PLANAR_G; break;
    case 'B': current_target_plane = PLANAR_B; break;
    case 'A': current_target_plane = PLANAR_A; break;
    case 'Y': current_target_plane = PLANAR_Y; break;
    case 'U': current_target_plane = PLANAR_U; break;
    case 'V': current_target_plane = PLANAR_V; break;
    }
    target_planes[i] = current_target_plane;
    int target_plane_width = vi_default.width >> vi_default.GetPlaneWidthSubsampling(current_target_plane);
    int target_plane_height = vi_default.height >> vi_default.GetPlaneHeightSubsampling(current_target_plane);

    if (clips[i]) // source clip count can be less than target planes count
      last_clip_index = i; // last defined clip is used for the others

    // check source clips and optinally their plane order
    VideoInfo src_vi = clips[last_clip_index]->GetVideoInfo();

    if(src_vi.BitsPerComponent() != bits_per_pixel)
      env->ThrowError("CombinePlanes: source bit depth is different from %d", bits_per_pixel);

    bool sourceIsYUV = src_vi.IsYUV() || src_vi.IsYUVA();
    bool sourceHasAlpha = src_vi.IsYUVA() || src_vi.IsPlanarRGBA();
    bool sourceIsY = src_vi.IsY();
    // check source
    // source_plane_count is either 0 or == target_plane_count
    {
      char ch;
      if (source_plane_count > 0) // optinal! defaults are filled
        ch = toupper(_source_planes_str[i]);
      else if (sourceIsYUV)
        ch = toupper(yuv_source_planes_str_def[i]);
      else // rgb
        ch = toupper(rgb_source_planes_str_def[i]);
      bool isRGB = ch == 'R' || ch == 'G' || ch == 'B';
      bool isYUV = ch == 'Y' || ch == 'U' || ch == 'V';
      bool isAlpha = ch == 'A';
      if(!isRGB && !isYUV && !isAlpha)
        env->ThrowError("CombinePlanes: invalid source plane definifion :%s", planes);
      if((sourceIsYUV && isRGB) || (!sourceIsYUV && isYUV) || (!sourceHasAlpha && isAlpha) || (sourceIsY && ch!='Y'))
        env->ThrowError("CombinePlanes: source has no such plane %c", ch);
      // todo lambda
      int current_source_plane;
      switch (ch) {
      case 'R': current_source_plane = PLANAR_R; break;
      case 'G': current_source_plane = PLANAR_G; break;
      case 'B': current_source_plane = PLANAR_B; break;
      case 'A': current_source_plane = PLANAR_A; break;
      case 'Y': current_source_plane = PLANAR_Y; break;
      case 'U': current_source_plane = PLANAR_U; break;
      case 'V': current_source_plane = PLANAR_V; break;
      }
      source_planes[i] = current_source_plane;
      // check dimensions
      int source_plane_width = src_vi.width >> src_vi.GetPlaneWidthSubsampling(current_source_plane);
      int source_plane_height = src_vi.height >> src_vi.GetPlaneHeightSubsampling(current_source_plane);
      if(source_plane_width != target_plane_width || source_plane_height != target_plane_height)
        env->ThrowError("CombinePlanes: source and target plane dimensions are different");
    }
  }
}


PVideoFrame __stdcall CombinePlanes::GetFrame(int n, IScriptEnvironment* env) {

  VideoInfo vi_src = clips[0]->GetVideoInfo();

  // check if fast Subframe magic can replace BitBlt
  if (!clips[1] && vi.NumComponents() <= vi_src.NumComponents()) // YUV<->RGB, YUVA<->RGBA YUV->Y
  {
    // we have only one clip, plane shuffle is valid if target has less plane that defined in source
    PVideoFrame src = clips[0]->GetFrame(n, env);

    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *planes = (vi_src.IsYUV() || vi_src.IsYUVA()) ? planes_y : planes_r;

#ifdef SIZETMOD
    size_t Offsets[4];
#else
    int Offsets[4];
#endif
    int Pitches[4], NewPitches[4];
    int RowSizes[4], NewRowSizes[4];

    int RelOffsets[4];

    for (int i = 0; i < vi_src.NumComponents(); i++) {
      Offsets[i] = src->GetOffset(planes[i]);
      Pitches[i] = NewPitches[i] = src->GetPitch(planes[i]);
      RowSizes[i] = NewRowSizes[i] = src->GetRowSize(planes[i]);
      RelOffsets[i] = 0;
    }

    for (int i = 0; i < planecount; i++) {
      int target_plane = target_planes[i];
      int source_plane = source_planes[i];
      int target_index, source_index;
      switch (target_plane) {
      case PLANAR_Y: case PLANAR_G: target_index = 0; break;
      case PLANAR_U: case PLANAR_B: target_index = 1; break;
      case PLANAR_V: case PLANAR_R: target_index = 2; break;
      case PLANAR_A: target_index = 3; break;
      }
      switch (source_plane) {
      case PLANAR_Y: case PLANAR_G: source_index = 0; break;
      case PLANAR_U: case PLANAR_B: source_index = 1; break;
      case PLANAR_V: case PLANAR_R: source_index = 2; break;
      case PLANAR_A: source_index = 3; break;
      }
#ifdef SIZETMOD
      // !! be cautious when you subtract two unsigned size_t variables
      const size_t offs_src = src->GetOffset(source_plane);
      const size_t offs_tgt = src->GetOffset(target_plane);
      RelOffsets[target_index] = (offs_src > offs_tgt) ? (int)(offs_src - offs_tgt) : -(int)(offs_tgt - offs_src);
#else
      RelOffsets[target_index] = Offsets[source_index] - Offsets[target_index];
#endif
      NewPitches[target_index] = Pitches[source_index];
      NewRowSizes[target_index] = RowSizes[source_index];
      // Y            U           V          A
      // 10         1010        2010       3010     offsets
      // src: AUVY target: YVUA
      // 3010-10   2010-1010  1010-2010    10-3010
      //  3000     =+1000      =-1000      =-3000   reloffsets
      //  3010       2010        1010       10      new offsets inside
    }

    IScriptEnvironment2* env2 = static_cast<IScriptEnvironment2*>(env);
    if (vi.NumComponents() == 4) {
      return env2->SubframePlanarA(src, RelOffsets[0], NewPitches[0], NewRowSizes[0], src->GetHeight(),
        RelOffsets[1], RelOffsets[2], NewPitches[1], RelOffsets[3]);
    }
    else if (vi.NumComponents() == 3) {
      return env->SubframePlanar(src, RelOffsets[0], NewPitches[0], NewRowSizes[0], src->GetHeight(),
        RelOffsets[1], RelOffsets[2], NewPitches[1]);
    }
    else {
      return env->Subframe(src, RelOffsets[0], NewPitches[0], NewRowSizes[0], src->GetHeight());
    }
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  PVideoFrame src;
  for (int i = 0; i < planecount; i++) {
    if (clips[i]) { // source clips can be less than defined planes
      src = clips[i]->GetFrame(n, env); // last defined clip is used for the others
    }
    int target_plane = target_planes[i];
    int source_plane = source_planes[i];
    env->BitBlt(dst->GetWritePtr(target_plane), dst->GetPitch(target_plane),
      src->GetReadPtr(source_plane), src->GetPitch(source_plane), src->GetRowSize(source_plane), src->GetHeight(source_plane));
  }
  return dst;
}
