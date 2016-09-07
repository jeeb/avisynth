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


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Swap_filters[] = {
  {  "SwapUV", BUILTIN_FUNC_PREFIX, "c", SwapUV::CreateSwapUV },
  {  "UToY",   BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateUToY },
  {  "VToY",   BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateVToY },
  {  "UToY8",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateUToY8 },
  {  "VToY8",  BUILTIN_FUNC_PREFIX, "c", SwapUVToY::CreateVToY8 },
  {  "YToUV",  BUILTIN_FUNC_PREFIX, "cc", SwapYToUV::CreateYToUV },
  {  "YToUV",  BUILTIN_FUNC_PREFIX, "ccc", SwapYToUV::CreateYToYUV },
  {  "PlaneToY",  BUILTIN_FUNC_PREFIX, "c[plane]s", SwapUVToY::CreatePlaneToY8 },
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

AVSValue __cdecl SwapUV::CreateSwapUV(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  PClip p = args[0].AsClip();
  if (p->GetVideoInfo().NumComponents() == 1)
    return p;
  return new SwapUV(p, env);
}


SwapUV::SwapUV(PClip _child, IScriptEnvironment* env) : GenericVideoFilter(_child)
{
  if (!vi.IsYUV())
    env->ThrowError("SwapUV: YUV data only!");
}

PVideoFrame __stdcall SwapUV::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);
  
  if (vi.IsPlanar()) {
    // Abuse subframe to flip the UV plane pointers -- extremely fast but a bit naughty!
    const int uvoffset = src->GetOffset(PLANAR_V) - src->GetOffset(PLANAR_U); // very naughty - don't do this at home!!
        // todo: check for YUVA??? env-> has no SubFramePlanar with alpha option!
    return env->SubframePlanar(src, 0, src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y),
                         uvoffset, -uvoffset, src->GetPitch(PLANAR_V));
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


AVSValue __cdecl SwapUVToY::CreateUToY(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new SwapUVToY(args[0].AsClip(), UToY, env);
}

AVSValue __cdecl SwapUVToY::CreateUToY8(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2UToY8 : UToY8, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new SwapUVToY(args[0].AsClip(), VToY, env);
}

AVSValue __cdecl SwapUVToY::CreateVToY8(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  return new SwapUVToY(clip, (clip->GetVideoInfo().IsYUY2()) ? YUY2VToY8 : VToY8, env);
}

AVSValue __cdecl SwapUVToY::CreatePlaneToY8(AVSValue args, void* user_data, IScriptEnvironment* env) {
    PClip clip = args[0].AsClip();
    const char* plane = args[1].AsString("");
    int mode = 0;
    // enum {UToY=1, VToY, UToY8, VToY8, YUY2UToY8, YUY2VToY8, AToY8, RToY8, GToY8, BToY8, YToY8};
    if (!lstrcmpi(plane, "Y")) mode = YToY8;
    else if (!lstrcmpi(plane, "U")) mode = clip->GetVideoInfo().IsYUY2() ? YUY2UToY8 : UToY8;
    else if (!lstrcmpi(plane, "V")) mode = clip->GetVideoInfo().IsYUY2() ? YUY2VToY8 : VToY8;
    else if (!lstrcmpi(plane, "A")) mode = AToY8;
    else if (!lstrcmpi(plane, "R")) mode = RToY8;
    else if (!lstrcmpi(plane, "G")) mode = GToY8;
    else if (!lstrcmpi(plane, "B")) mode = BToY8;
    else env->ThrowError("PlaneToY: Invalid plane!");

    if (clip->GetVideoInfo().IsYUY2() && mode == YToY8)
        env->ThrowError("PlaneToY: Y plane not allowed for YUY2!");

    return new SwapUVToY(clip, mode, env);
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

  if (vi.NumComponents() == 1)
    env->ThrowError("PlaneToY: There are no chroma channels in greyscale clip!");

  if(YUVmode) {
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
    const int offset = src->GetOffset(source_plane) - src->GetOffset(target_plane); // very naughty - don't do this at home!!
                                                                                    // Abuse Subframe to snatch the U/V/R/G/B/A plane
    return env->Subframe(src, offset, src->GetPitch(source_plane), src->GetRowSize(source_plane), src->GetHeight(source_plane));
  }

  PVideoFrame dst = env->NewVideoFrame(vi);

  if (mode == YUY2UToY8 || mode == YUY2VToY8 || vi.IsYUY2()) {
    const BYTE* srcp = src->GetReadPtr();
    BYTE* dstp = dst->GetWritePtr();
    int src_pitch = src->GetPitch();
    int dst_pitch = dst->GetPitch();
    int pos = mode == YUY2UToY8 ? 1 : 3;

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
    fill_plane<float>(dstp_u, rowsize, height, pitch, 0.5f);
    fill_plane<float>(dstp_v, rowsize, height, pitch, 0.5f);
  }

  return dst;
}


AVSValue __cdecl SwapYToUV::CreateYToUV(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), NULL , env);
}

AVSValue __cdecl SwapYToUV::CreateYToYUV(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new SwapYToUV(args[0].AsClip(), args[1].AsClip(), args[2].AsClip(), env);
}


SwapYToUV::SwapYToUV(PClip _child, PClip _clip, PClip _clipY, IScriptEnvironment* env)
  : GenericVideoFilter(_child), clip(_clip), clipY(_clipY)
{
  if (!vi.IsYUV())
    env->ThrowError("YToUV: Only YUV data accepted");

  const VideoInfo& vi2 = clip->GetVideoInfo();
  if (vi.height != vi2.height)
    env->ThrowError("YToUV: Clips do not have the same height (U & V mismatch) !");
  if (vi.width != vi2.width)
    env->ThrowError("YToUV: Clips do not have the same width (U & V mismatch) !");
  if (vi.IsYUY2() != vi2.IsYUY2()) 
    env->ThrowError("YToUV: YUY2 Clips must have same colorspace (U & V mismatch) !");

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

  // Autogenerate destination colorformat
  switch (vi.BitsPerComponent())
  {
  case 8: vi.pixel_type = VideoInfo::CS_YV12; break;// CS_Sub_Width_2 and CS_Sub_Height_2 are 0
  case 10: vi.pixel_type = VideoInfo::CS_YUV420P10; break;
  case 12: vi.pixel_type = VideoInfo::CS_YUV420P12; break;
  case 14: vi.pixel_type = VideoInfo::CS_YUV420P14; break;
  case 16: vi.pixel_type = VideoInfo::CS_YUV420P16; break;
  case 32: vi.pixel_type = VideoInfo::CS_YUV420PS; break;
  }

  if (vi3.width == vi.width)
    vi.pixel_type |= VideoInfo::CS_Sub_Width_1;
  else if (vi3.width == vi.width * 2)
    vi.width *= 2;
  else if (vi3.width == vi.width * 4) {
    vi.pixel_type |= VideoInfo::CS_Sub_Width_4;
    vi.width *= 4;
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

  if (clipY) {
    src = clipY->GetFrame(n, env);
    env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
                src->GetReadPtr(PLANAR_Y), src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
    return dst;
  }

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

