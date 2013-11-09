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

#include "convert_yuy2.h"
#include "../core/internal.h"
#include "avs/alignment.h"
#include <emmintrin.h>

//these are to be used only in asm routines
const int cyb_rec601 = int(0.114 * 219 / 255 * 65536 + 0.5);
const int cyg_rec601 = int(0.587 * 219 / 255 * 65536 + 0.5);
const int cyr_rec601 = int(0.299 * 219 / 255 * 65536 + 0.5);

const int ku_rec601  = int(112.0 / (255.0 * (1.0 - 0.114)) * 65536 + 0.5);
const int kv_rec601  = int(112.0 / (255.0 * (1.0 - 0.299)) * 65536 + 0.5);

const int cyb_rec709 = int(0.0722 * 219 / 255 * 65536 + 0.5);
const int cyg_rec709 = int(0.7152 * 219 / 255 * 65536 + 0.5);
const int cyr_rec709 = int(0.2126 * 219 / 255 * 65536 + 0.5);

const int ku_rec709  = int(112.0 / (255.0 * (1.0 - 0.0722)) * 65536 + 0.5);
const int kv_rec709  = int(112.0 / (255.0 * (1.0 - 0.2126)) * 65536 + 0.5);


const int cyb_pc601 = int(0.114 * 65536 + 0.5);
const int cyg_pc601 = int(0.587 * 65536 + 0.5);
const int cyr_pc601 = int(0.299 * 65536 + 0.5);

const int ku_pc601  = int(127.0 / (255.0 * (1.0 - 0.114)) * 65536 + 0.5);
const int kv_pc601  = int(127.0 / (255.0 * (1.0 - 0.299)) * 65536 + 0.5);

const int cyb_pc709 = int(0.0722 * 65536 + 0.5);
const int cyg_pc709 = int(0.7152 * 65536 + 0.5);
const int cyr_pc709 = int(0.2126 * 65536 + 0.5);

const int ku_pc709  = int(127.0 / (255.0 * (1.0 - 0.0722)) * 65536 + 0.5);
const int kv_pc709  = int(127.0 / (255.0 * (1.0 - 0.2126)) * 65536 + 0.5);


static const int cyb_values[4] = {cyb_rec601 / 2, cyb_rec709 / 2, cyb_pc601 / 2, cyb_pc709 / 2};
static const int cyg_values[4] = {cyg_rec601 / 2, cyg_rec709 / 2, cyg_pc601 / 2, cyg_pc709 / 2};
static const int cyr_values[4] = {cyr_rec601 / 2, cyr_rec709 / 2, cyr_pc601 / 2, cyr_pc709 / 2};

const double luma_rec_scale = 255.0/219.0 * 65536+0.5;

static const int ku_values[4]       = {ku_rec601 / 2, ku_rec709 / 2, ku_pc601 / 2, ku_pc709 / 2};
static const int ku_values_luma[4]  = {-int((ku_rec601/2) * luma_rec_scale) / 65536, -int((ku_rec709/2) * luma_rec_scale) / 65536, -ku_pc601 / 2, -ku_pc709 / 2};
static const int kv_values[4]       = {kv_rec601 / 2, kv_rec709 / 2, kv_pc601 / 2, kv_pc709 / 2};
static const int kv_values_luma[4]  = {-int((kv_rec601/2) * luma_rec_scale) / 65536, -int((kv_rec709/2) * luma_rec_scale) / 65536, -kv_pc601 / 2, -kv_pc709 / 2};


//__declspec(align(8)) const __int64 cybgr_64 = (__int64)cyb|(((__int64)cyg)<<16)|(((__int64)cyr)<<32);
  __declspec(align(8)) static const __int64 cybgr_64[4]  ={0x000020DE40870C88,
                                                           0x0000175F4E9F07F0,
                                                           0x000026464B230E97,
                                                           0x00001B365B8C093E};

  __declspec(align(8)) static const __int64 y1y2_fpix[4] ={0x5033A29E3F74B61E,    //=(1/((1-0.299)*255/112)<<15+0.5),  (1/((1-0.114)*255/112)<<15+0.5)
                                                           0x4766ACDD3C6EB9A3,    //=(1/((1-0.2126)*255/112)<<15+0.5), (1/((1-0.0722)*255/112)<<15+0.5)
                                                           0x5AF1A50F47F4B80C,    //=(1/((1-0.299)*255/127)<<15+0.5),  (1/((1-0.114)*255/127)<<15+0.5)
                                                           0x50F6AF0A44B6BB4A};   //=(1/((1-0.2126)*255/127)<<15+0.5), (1/((1-0.0722)*255/127)<<15+0.5)

  __declspec(align(16)) static const __int64 fpix_add    = 0x0080800000808000;    //=(128.5) << 16
  __declspec(align(16)) static const __int64 fpix_rnd    = 0x0101000001010000;    //=(128.5) << 17

  __declspec(align(16)) static const __int64 sub_64      = 0x0000FFC00000FFC0; // -64

//static const int sub_32      = 0x0000FFE0;    // -32
  static const int sub_16      = 0x0000FFF0;    // -16

  static const int fraction[4] ={0x00084000,    //=(16.5) << 15 = 0x84000
                                 0x00084000,
                                 0x00004000,    //=(0.5) << 15 = 0x4000
                                 0x00004000};



/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, bool _dupl, bool _interlaced, const char *matrix, IScriptEnvironment* env)
  : GenericVideoFilter(_child), interlaced(_interlaced),src_cs(vi.pixel_type)
{
  if (vi.height&3 && vi.IsYV12() && interlaced)
    env->ThrowError("ConvertToYUY2: Cannot convert from interlaced YV12 if height is not multiple of 4. Use Crop!");

  if (vi.height&1 && vi.IsYV12() )
    env->ThrowError("ConvertToYUY2: Cannot convert from YV12 if height is not even. Use Crop!");

  if (vi.width & 1)
    env->ThrowError("ConvertToYUY2: Image width must be even. Use Crop!");

  theMatrix = Rec601;
  if (matrix) {
    if (!vi.IsRGB())
      env->ThrowError("ConvertToYUY2: invalid \"matrix\" parameter (RGB data only)");

    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "PC.601"))
      theMatrix = PC_601;
    else if (!lstrcmpi(matrix, "PC.709"))
      theMatrix = PC_709;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("ConvertToYUY2: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\", \"PC.601\" or \"PC.709\")");
  }

#ifdef X86_32
  if ((env->GetCPUFlags() & CPUF_MMX) && vi.IsRGB())    // Generate MMX
  {
    this->GenerateAssembly(vi.IsRGB24(), _dupl, (theMatrix < 2), vi.width,
                           &cybgr_64[theMatrix], &y1y2_fpix[theMatrix],
                           &fraction[theMatrix], env);
  }
#else
  //TODO
  env->ThrowError("ConvertToYUY2 is not yet ported to 64-bit.");
#endif

  vi.pixel_type = VideoInfo::CS_YUY2;
}

ConvertToYUY2::~ConvertToYUY2() {
#ifdef X86_32
  assembly.Free();
#endif
}

PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if (((src_cs&VideoInfo::CS_YV12)==VideoInfo::CS_YV12)||((src_cs&VideoInfo::CS_I420)==VideoInfo::CS_I420)) {
    PVideoFrame dst = env->NewVideoFrame(vi,32); 
    BYTE* dstp = dst->GetWritePtr();
    const BYTE* srcp_y = src->GetReadPtr(PLANAR_Y);
    const BYTE* srcp_u = src->GetReadPtr(PLANAR_U);
    const BYTE* srcp_v = src->GetReadPtr(PLANAR_V);
    int src_pitch_y = src->GetPitch(PLANAR_Y);
    int src_pitch_uv = src->GetPitch(PLANAR_U);
    int dst_pitch = dst->GetPitch();
    int src_heigh = dst->GetHeight();

    //todo: maybe check for source width being mod16/8?
    if (interlaced) {
      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp_y, 16))
      {
        convert_yv12_to_yuy2_interlaced_sse2(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
      }
      else
#ifdef X86_32
      if (env->GetCPUFlags() & CPUF_INTEGER_SSE)
      {
        convert_yv12_to_yuy2_interlaced_isse(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
      }
      else
#endif
      {
        convert_yv12_to_yuy2_interlaced_c(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
      }
    } else {
      if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp_y, 16))
      {
        convert_yv12_to_yuy2_progressive_sse2(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
      }
      else
#ifdef X86_32
        if (env->GetCPUFlags() & CPUF_INTEGER_SSE)
        {
          convert_yv12_to_yuy2_progressive_isse(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
        }
        else
#endif
        {
          convert_yv12_to_yuy2_progressive_c(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
        }
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();

#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX)
  {
    mmx_ConvertRGBtoYUY2(src->GetReadPtr(), yuv, src->GetPitch(), dst->GetPitch(), vi.height);
    return dst;
  }
#endif

// non MMX machines.

  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch();

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize();
  const int rgb_inc = ((src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32) ? 4 : 3;

  if (theMatrix == PC_601) {
    const int cyb = int(0.114*65536+0.5);
    const int cyg = int(0.587*65536+0.5);
    const int cyr = int(0.299*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.299))*65536+0.5);

    inline_rgbtoyuy2(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == PC_709) {
    const int cyb = int(0.0722*65536+0.5);
    const int cyg = int(0.7152*65536+0.5);
    const int cyr = int(0.2126*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.2126))*65536+0.5);

    inline_rgbtoyuy2(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == Rec709) {
    const int cyb = int(0.0722*219/255*65536+0.5);
    const int cyg = int(0.7152*219/255*65536+0.5);
    const int cyr = int(0.2126*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.2126))*65536+0.5);

    inline_rgbtoyuy2(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  } else if (theMatrix == Rec601) {
    const int cyb = int(0.114*219/255*65536+0.5);
    const int cyg = int(0.587*219/255*65536+0.5);
    const int cyr = int(0.299*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.299))*65536+0.5);

    inline_rgbtoyuy2(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc);

  }

  return dst;
}

// 1-2-1 Kernel version

inline void ConvertToYUY2::inline_rgbtoyuy2(const bool pcrange, const int cyb, const int cyg, const int cyr,
                                            const int ku, const int kv, const BYTE* rgb,
                                            BYTE* yuv, const int yuv_offset,
                                            const int rgb_offset, const int rgb_inc) {

  const int bias = pcrange ? 0x8000 : 0x108000; //  0.5 * 65536 : 16.5 * 65536

  for (int y=vi.height; y>0; --y)
  {
    // Use left most pixel for edge condition
    int y0                 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + bias) >> 16;
    const BYTE* rgb_prev   = rgb;
    for (int x = 0; x < vi.width; x += 2)
    {
      const BYTE* const rgb_next = rgb + rgb_inc;
      // y1 and y2 can't overflow
      const int y1         = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + bias) >> 16;
      yuv[0]               = y1;
      const int y2         = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + bias) >> 16;
      yuv[2]               = y2;
      if (pcrange) { // This is okay, the compiler optimises out the unused path when pcrange is a constant
        const int scaled_y = y0+y1*2+y2;
        const int b_y      = (rgb_prev[0]+rgb[0]*2+rgb_next[0]) - scaled_y;
        yuv[1]             = PixelClip((b_y * ku + (128<<18) + (1<<17)) >> 18);  // u
        const int r_y      = (rgb_prev[2]+rgb[2]*2+rgb_next[2]) - scaled_y;
        yuv[3]             = PixelClip((r_y * kv + (128<<18) + (1<<17)) >> 18);  // v
      }
      else {
        const int scaled_y = (y0+y1*2+y2 - 64) * int(255.0/219.0*65536+0.5);
        const int b_y      = ((rgb_prev[0]+rgb[0]*2+rgb_next[0]) << 16) - scaled_y;
        yuv[1]             = PixelClip(((b_y >> 12) * ku + (128<<22) + (1<<21)) >> 22);  // u
        const int r_y      = ((rgb_prev[2]+rgb[2]*2+rgb_next[2]) << 16) - scaled_y;
        yuv[3]             = PixelClip(((r_y >> 12) * kv + (128<<22) + (1<<21)) >> 22);  // v
      }
      y0       = y2;

      rgb_prev = rgb_next;
      rgb      = rgb_next + rgb_inc;
      yuv     += 4;
    }
    rgb += rgb_offset;
    yuv += yuv_offset;
  }
}


AVSValue __cdecl ConvertToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;

  const bool haveOpts = args[3].Defined() || args[4].Defined();

  if (clip->GetVideoInfo().IsPlanar()) {
    if (haveOpts || !clip->GetVideoInfo().IsYV12()) {
      // We have no direct conversions. Go to YV16.
      AVSValue new_args[5] = { clip, args[1], args[2], args[3], args[4] };
      clip = ConvertToPlanarGeneric::CreateYV16(AVSValue(new_args, 5), NULL,  env).AsClip();
    }
  }

  if (clip->GetVideoInfo().IsYV16())
    return new ConvertYV16ToYUY2(clip,  env);

  if (haveOpts)
    env->ThrowError("ConvertToYUY2: ChromaPlacement and ChromaResample options are not supported.");

  const bool i=args[1].AsBool(false);
  return new ConvertToYUY2(clip, false, i, args[2].AsString(0), env);
}





/****************************************************
 ******* Convert back to YUY2                  ******
 ******* this only uses Chroma from left pixel ******
 ******* to be used, when signal already has   ******
 ******* been YUY2 to avoid deterioration      ******
 ****************************************************/

ConvertBackToYUY2::ConvertBackToYUY2(PClip _child, const char *matrix, IScriptEnvironment* env)
  : ConvertToYUY2(_child, true, false, matrix, env)
{
  if (!_child->GetVideoInfo().IsRGB() && !_child->GetVideoInfo().IsYV24())
    env->ThrowError("ConvertBackToYUY2: Use ConvertToYUY2 to convert non-RGB material to YUY2.");
}

static void convert_yv24_back_to_yuy2_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int pitchY, int pitchUV, int dpitch, int height, int width) {
  int mod16_width = width / 16 * 16;
  __m128i ff = _mm_set1_epi16(0x00ff);

  for (int y=0; y < height; y++) {
    for (int x=0; x < mod16_width; x+=16) {
      __m128i y = _mm_load_si128(reinterpret_cast<const __m128i*>(srcY+x));
      __m128i u = _mm_load_si128(reinterpret_cast<const __m128i*>(srcU+x));
      __m128i v = _mm_load_si128(reinterpret_cast<const __m128i*>(srcV+x));
      u = _mm_and_si128(u, ff);
      v = _mm_slli_epi16(v, 8);
      __m128i uv = _mm_or_si128(u, v); //VUVUVUVUVU

      __m128i yuv_lo = _mm_unpacklo_epi8(y, uv);
      __m128i yuv_hi = _mm_unpackhi_epi8(y, uv);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*2), yuv_lo);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*2+16), yuv_hi);
    }

    if (mod16_width != width) {
      __m128i y = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcY+width-16));
      __m128i u = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcU+width-16));
      __m128i v = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcV+width-16));
      u = _mm_and_si128(u, ff);
      v = _mm_slli_epi16(v, 8);
      __m128i uv = _mm_or_si128(u, v); //VUVUVUVUVU

      __m128i yuv_lo = _mm_unpacklo_epi8(y, uv);
      __m128i yuv_hi = _mm_unpackhi_epi8(y, uv);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+width*2-32), yuv_lo);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+width*2-16), yuv_hi);
    }
    srcY += pitchY;
    srcU += pitchUV;
    srcV += pitchUV;
    dstp += dpitch;
  }
}

#ifdef X86_32

static void convert_yv24_back_to_yuy2_mmx(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int pitchY, int pitchUV, int dpitch, int height, int width) {
  int mod8_width = width / 8 * 8;
  __m64 ff = _mm_set1_pi16(0x00ff);

  for (int y=0; y < height; y++) {
    for (int x=0; x < mod8_width; x+=8) {
      __m64 y = *reinterpret_cast<const __m64*>(srcY+x);
      __m64 u = *reinterpret_cast<const __m64*>(srcU+x);
      __m64 v = *reinterpret_cast<const __m64*>(srcV+x);
      u = _mm_and_si64(u, ff);
      v = _mm_slli_pi16(v, 8);
      __m64 uv = _mm_or_si64(u, v); //VUVUVUVUVU

      __m64 yuv_lo = _mm_unpacklo_pi8(y, uv);
      __m64 yuv_hi = _mm_unpackhi_pi8(y, uv);

      *reinterpret_cast<__m64*>(dstp+x*2) = yuv_lo;
      *reinterpret_cast<__m64*>(dstp+x*2+8) = yuv_hi;
    }

    if (mod8_width != width) {
      __m64 y = *reinterpret_cast<const __m64*>(srcY+width-8);
      __m64 u = *reinterpret_cast<const __m64*>(srcU+width-8);
      __m64 v = *reinterpret_cast<const __m64*>(srcV+width-8);
      u = _mm_and_si64(u, ff);
      v = _mm_slli_pi16(v, 8);
      __m64 uv = _mm_or_si64(u, v); //VUVUVUVUVU

      __m64 yuv_lo = _mm_unpacklo_pi8(y, uv);
      __m64 yuv_hi = _mm_unpackhi_pi8(y, uv);

      *reinterpret_cast<__m64*>(dstp+width*2-16) = yuv_lo;
      *reinterpret_cast<__m64*>(dstp+width*2-8) = yuv_hi;
    }
    srcY += pitchY;
    srcU += pitchUV;
    srcV += pitchUV;
    dstp += dpitch;
  }
  _mm_empty();
}

#endif // X86_32

static void convert_yv24_back_to_yuy2_c(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int pitchY, int pitchUV, int dpitch, int height, int width) {
  for (int y=0; y < height; ++y) {
    for (int x=0; x < width; x+=2) {
      dstp[x*2+0] = srcY[x];
      dstp[x*2+1] = srcU[x];
      dstp[x*2+2] = srcY[x+1];
      dstp[x*2+3] = srcV[x];
    }
    srcY += pitchY;
    srcU += pitchUV;
    srcV += pitchUV;
    dstp += dpitch;
  }
}


static void convert_rgb_back_to_yuy2_c(BYTE* yuv, const BYTE* rgb, int rgb_offset, int yuv_offset, int height, int width, int rgb_inc, int matrix) {
  /* Existing 0-1-0 Kernel version */
  int cyb, cyg, cyr, ku, kv;

  if (matrix == PC_601 || matrix == PC_709) {
    if (matrix == PC_601) {
      cyb = int(0.114 * 65536 + 0.5);
      cyg = int(0.587 * 65536 + 0.5);
      cyr = int(0.299 * 65536 + 0.5);

      ku  = int(127.0 / (255.0 * (1.0 - 0.114)) * 65536 + 0.5);
      kv  = int(127.0 / (255.0 * (1.0 - 0.299)) * 65536 + 0.5);
    } else {
      cyb = int(0.0722 * 65536 + 0.5);
      cyg = int(0.7152 * 65536 + 0.5);
      cyr = int(0.2126 * 65536 + 0.5);

      ku  = int(127.0 / (255.0 * (1.0 - 0.0722)) * 65536 + 0.5);
      kv  = int(127.0 / (255.0 * (1.0 - 0.2126)) * 65536 + 0.5);
    }
    for (int y = height; y>0; --y)
    {
      for (int x = 0; x < width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        yuv[0] = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x8000) >> 16;
        yuv[2] = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x8000) >> 16;

        int scaled_y = yuv[0];
        int b_y = rgb[0] - scaled_y;
        yuv[1] = ScaledPixelClip(b_y * ku + 0x800000);  // u
        int r_y = rgb[2] - scaled_y;
        yuv[3] = ScaledPixelClip(r_y * kv + 0x800000);  // v
        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  } else {
    if (matrix == Rec709) {
      cyb = int(0.0722 * 219 / 255 * 65536 + 0.5);
      cyg = int(0.7152 * 219 / 255 * 65536 + 0.5);
      cyr = int(0.2126 * 219 / 255 * 65536 + 0.5);

      ku  = int(112.0 / (255.0 * (1.0 - 0.0722)) * 32768 + 0.5);
      kv  = int(112.0 / (255.0 * (1.0 - 0.2126)) * 32768 + 0.5);
    } else {
      cyb = int(0.114 * 219 / 255 * 65536 + 0.5);
      cyg = int(0.587 * 219 / 255 * 65536 + 0.5);
      cyr = int(0.299 * 219 / 255 * 65536 + 0.5);

      ku  = int(1 / 2.018 * 32768 + 0.5);
      kv  = int(1 / 1.596 * 32768 + 0.5);
    }

    for (int y = height; y>0; --y)
    {
      for (int x = 0; x < width; x += 2)
      {
        const BYTE* const rgb_next = rgb + rgb_inc;
        // y1 and y2 can't overflow
        yuv[0] = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000) >> 16;
        yuv[2] = (cyb*rgb_next[0] + cyg*rgb_next[1] + cyr*rgb_next[2] + 0x108000) >> 16;

        int scaled_y = (yuv[0] - 16) * int(255.0 / 219.0 * 65536 + 0.5);
        int b_y = ((rgb[0]) << 16) - scaled_y;
        yuv[1] = ScaledPixelClip((b_y >> 15) * ku + 0x800000);  // u
        int r_y = ((rgb[2]) << 16) - scaled_y;
        yuv[3] = ScaledPixelClip((r_y >> 15) * kv + 0x800000);  // v

        rgb = rgb_next + rgb_inc;
        yuv += 4;
      }
      rgb += rgb_offset;
      yuv += yuv_offset;
    }
  }
}


template<int matrix, int rgb_bytes, bool aligned>
static __forceinline __m128i convert_rgb_block_back_to_yuy2_sse2(const BYTE* srcp, const __m128i &luma_coefs, const __m128i &chroma_coefs, const __m128i &upper_dword_mask, 
                                                                 const __m128i &chroma_round_mask, __m128i &luma_round_mask, const __m128i &tv_scale, const __m128i &zero) {
  __m128i rgb_p1, rgb_p2;
  if (rgb_bytes == 4) {
    //RGB32
    __m128i src;
    if (aligned) {
      src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp)); //xxr3 g3b3 xxr2 g2b2 | xxr1 g1b1 xxr0 g0b0
    } else {
      src = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp)); //xxr3 g3b3 xxr2 g2b2 | xxr1 g1b1 xxr0 g0b0
    }

    rgb_p1 = _mm_unpacklo_epi8(src, zero); //00xx 00r1 00g1 00b1 | 00xx 00r0 00g0 00b0
    rgb_p2 = _mm_unpackhi_epi8(src, zero); //00xx 00r3 00g3 00b3 | 00xx 00r2 00g2 00b2
  } else {
    //RGB24
    __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp)); //pixels 0 and 1
    __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+6)); //pixels 2 and 3

    //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
    rgb_p1 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
    rgb_p2 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
  }

  __m128i t1 = _mm_madd_epi16(rgb_p1, luma_coefs); //xx*0 + r1*cyr | g1*cyg + b1*cyb | xx*0 + r0*cyr | g0*cyg + b0*cyb
  __m128i t2 = _mm_madd_epi16(rgb_p2, luma_coefs); //xx*0 + r3*cyr | g3*cyg + b3*cyb | xx*0 + r2*cyr | g2*cyg + b2*cyb

  __m128i r_temp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2), _MM_SHUFFLE(3, 1, 3, 1))); // r3*cyr | r2*cyr | r1*cyr | r0*cyr
  __m128i gb_temp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2), _MM_SHUFFLE(2, 0, 2, 0))); // g3*cyg + b3*cyb | g2*cyg + b2*cyb | g1*cyg + b1*cyb | g0*cyg + b0*cyb

  __m128i luma = _mm_add_epi32(r_temp, gb_temp); //r3*cyr + g3*cyg + b3*cyb | r2*cyr + g2*cyg + b2*cyb | r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  luma = _mm_add_epi32(luma, luma_round_mask); //r3*cyr + g3*cyg + b3*cyb + round | r2*cyr + g2*cyg + b2*cyb + round | r1*cyr + g1*cyg + b1*cyb + round | r0*cyr + g0*cyg + b0*cyb + round
  luma = _mm_srli_epi32(luma, 15); //00 00 00 y3 00 00 00 y2 00 00 00 y1 00 00 00 y0

  __m128i rb_p1 = _mm_slli_epi32(rgb_p1, 16); //00r1 0000 00b1 0000 | 00r0 0000 00b0 0000
  __m128i rb_p2 = _mm_slli_epi32(rgb_p2, 16); //00r3 0000 00b3 0000 | 00r2 0000 00b2 0000
  __m128i rb_p = _mm_unpacklo_epi64(rb_p1, rb_p2);  //00r2 0000 00b2 0000 | 00r0 0000 00b0 0000

  __m128i y_scaled;
  if (matrix == Rec601 || matrix == Rec709) {
    y_scaled = _mm_sub_epi16(luma, tv_scale);
  } else {
    y_scaled = luma;
  }

  __m128i y0 = _mm_shuffle_epi32(y_scaled, _MM_SHUFFLE(2, 2, 0, 0)); //00 00 00 y2 00 00 00 y2 | 00 00 00 y0 00 00 00 y0

  __m128i rby = _mm_or_si128(rb_p, y0); //00 rr 00 y2 00 b2 00 y2 | 00 r0 00 y0 00 b0 00 y0

  rby = _mm_adds_epu16(rby, rby); //2*r2 | 2*y2 | 2*b2 | 2*y2 | 2*r0 | 2*y0 | 2*b0 | 2*y0

  __m128i uv = _mm_madd_epi16(rby, chroma_coefs);

  uv = _mm_add_epi32(uv, chroma_round_mask);
  uv = _mm_and_si128(uv, upper_dword_mask);
  __m128i yuv = _mm_or_si128(uv, luma); ///00 v1 00 y3 00 u1 00 y2 | 00 v0 00 y1 00 u0 00 y0

  return _mm_packus_epi16(yuv, yuv);
}

//////////////////////////////////////////////////////////////////////////
//Optimization note: matrix is a template argument only to avoid subtraction for PC matrices. Compilers tend to generate ~10% faster code in this case. 
//MMX version is not optimized this way because who'll be using MMX anyway?
//////////////////////////////////////////////////////////////////////////
template<int matrix, int rgb_bytes>
static void convert_rgb_line_back_to_yuy2_sse2(const BYTE *srcp, BYTE *dstp, int width) {
  int mod4_width = width / 4 * 4;

  __m128i luma_round_mask;
  if (matrix == Rec601 || matrix == Rec709) {
    luma_round_mask = _mm_set1_epi32(0x84000);
  } else {
    luma_round_mask = _mm_set1_epi32(0x4000);
  }

  __m128i luma_coefs = _mm_set_epi16(0, cyr_values[matrix], cyg_values[matrix], cyb_values[matrix], 0, cyr_values[matrix], cyg_values[matrix], cyb_values[matrix]);
  __m128i chroma_coefs = _mm_set_epi16(kv_values[matrix], kv_values_luma[matrix], ku_values[matrix], ku_values_luma[matrix], kv_values[matrix], kv_values_luma[matrix], ku_values[matrix], ku_values_luma[matrix]);
  __m128i chroma_round_mask = _mm_set1_epi32(0x808000);

  __m128i upper_dword_mask = _mm_set1_epi32(0xFFFF0000);
  __m128i zero = _mm_setzero_si128();
  __m128i tv_scale = _mm_set1_epi32(16);

  for (int x = 0; x < mod4_width; x+=4) {
    __m128i yuv = convert_rgb_block_back_to_yuy2_sse2<matrix, rgb_bytes, true>(srcp + x * rgb_bytes, luma_coefs, chroma_coefs, upper_dword_mask, chroma_round_mask, luma_round_mask, tv_scale, zero);

    _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), yuv);
  }

  if (width != mod4_width) {
    const BYTE* ptr = srcp + (width-4) * rgb_bytes;

    __m128i yuv = convert_rgb_block_back_to_yuy2_sse2<matrix, rgb_bytes, false>(ptr, luma_coefs, chroma_coefs, upper_dword_mask, chroma_round_mask, luma_round_mask, tv_scale, zero);

    _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width*2 - 8), yuv);
  }
}

template<int matrix, int rgb_bytes>
static void convert_rgb_back_to_yuy2_sse2(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_back_to_yuy2_sse2<matrix, rgb_bytes>(src, dst, width);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
}


#ifdef X86_32
#pragma warning(disable: 4799)
template<int rgb_bytes>
static void convert_rgb_line_back_to_yuy2_mmx(const BYTE *srcp, BYTE *dstp, int width, int matrix) {
  __m64 luma_round_mask;
  if (matrix == Rec601 || matrix == Rec709) {
    luma_round_mask = _mm_set1_pi32(0x84000);
  } else {
    luma_round_mask = _mm_set1_pi32(0x4000);
  }

  __m64 luma_coefs = _mm_set_pi16(0, cyr_values[matrix], cyg_values[matrix], cyb_values[matrix] );
  __m64 chroma_coefs = _mm_set_pi16(kv_values[matrix], kv_values_luma[matrix], ku_values[matrix], ku_values_luma[matrix]);
  __m64 chroma_round_mask = _mm_set1_pi32(0x808000);

  __m64 upper_dword_mask = _mm_set1_pi32(0xFFFF0000);
  __m64 zero = _mm_setzero_si64();
  __m64 tv_scale = _mm_set1_pi32((matrix == Rec601 || matrix == Rec709) ? 16 : 0);
  
  for (int x = 0; x < width; x+=2) {
    __m64 src = *reinterpret_cast<const __m64*>(srcp+x*rgb_bytes); //xxr1 g1b1 xxr0 g0b0

    __m64 rgb_p1 = _mm_unpacklo_pi8(src, zero); //00xx 00r0 00g0 00b0
    if (rgb_bytes == 3) {
      src = _mm_slli_si64(src, 8);
    }
    __m64 rgb_p2 = _mm_unpackhi_pi8(src, zero); //00xx 00r1 00g1 00b1

    __m64 t1 = _mm_madd_pi16(rgb_p1, luma_coefs); //xx*0 + r0*cyr | g0*cyg + b0*cyb
    __m64 t2 = _mm_madd_pi16(rgb_p2, luma_coefs); //xx*0 + r1*cyr | g1*cyg + b1*cyb

    __m64 r_temp = _mm_unpackhi_pi32(t1, t2); //r1*cyr | r0*cyr
    __m64 gb_temp = _mm_unpacklo_pi32(t1, t2); //g1*cyg + b1*cyb | g0*cyg + b0*cyb

    __m64 luma = _mm_add_pi32(r_temp, gb_temp); //r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
    luma = _mm_add_pi32(luma, luma_round_mask); //r1*cyr + g1*cyg + b1*cyb + round | r0*cyr + g0*cyg + b0*cyb + round
    luma = _mm_srli_pi32(luma, 15); //00 00 00 y1 00 00 00 y0

    __m64 rb_p1 = _mm_slli_pi32(rgb_p1, 16); //00r0 0000 00b0 0000

    __m64 y_scaled = _mm_sub_pi16(luma, tv_scale);
    __m64 y0 = _mm_unpacklo_pi32(y_scaled, y_scaled); //00 00 00 y0 00 00 00 y0

    __m64 rby = _mm_or_si64(rb_p1, y0); //00 rr 00 yy 00 bb 00 yy

    rby = _mm_adds_pu16(rby, rby); //2*r | 2*y | 2*b | 2*y

    __m64 uv = _mm_madd_pi16(rby, chroma_coefs);

    uv = _mm_add_pi32(uv, chroma_round_mask);
    uv = _mm_and_si64(uv, upper_dword_mask);
    __m64 yuv = _mm_or_si64(uv, luma);

    yuv = _mm_packs_pu16(yuv, yuv);

    *reinterpret_cast<int*>(dstp+x*2) = _mm_cvtsi64_si32(yuv);
  }
}
#pragma warning(default: 4799)

template<int rgb_bytes>
static void convert_rgb_back_to_yuy2_mmx(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height, int matrix) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_back_to_yuy2_mmx<rgb_bytes>(src, dst, width, matrix);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
  _mm_empty();
}

#endif

PVideoFrame __stdcall ConvertBackToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if ((src_cs&VideoInfo::CS_YV24)==VideoInfo::CS_YV24) 
  {
    PVideoFrame dst = env->NewVideoFrame(vi);
    BYTE* dstp = dst->GetWritePtr();
    const int dpitch  = dst->GetPitch();

    const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
    const BYTE* srcU = src->GetReadPtr(PLANAR_U);
    const BYTE* srcV = src->GetReadPtr(PLANAR_V);

    const int pitchY  = src->GetPitch(PLANAR_Y);
    const int pitchUV = src->GetPitch(PLANAR_U);

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcY, 16) && IsPtrAligned(srcU, 16) && IsPtrAligned(srcV, 16))
    {  // Use MMX
      convert_yv24_back_to_yuy2_sse2(srcY, srcU, srcV, dstp, pitchY, pitchUV, dpitch, vi.height, vi.width);
    } 
    else 
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_MMX)
    {  // Use MMX
      convert_yv24_back_to_yuy2_mmx(srcY, srcU, srcV, dstp, pitchY, pitchUV, dpitch, vi.height, vi.width);
    } 
    else 
#endif
    {
      convert_yv24_back_to_yuy2_c(srcY, srcU, srcV, dstp, pitchY, pitchUV, dpitch, vi.height, vi.width);
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrame(vi);
  BYTE* yuv = dst->GetWritePtr();


  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(src->GetReadPtr(), 16)) 
  {
    if ((src_cs & VideoInfo::CS_BGR32) == VideoInfo::CS_BGR32) {
      if (theMatrix == Rec601) {
        convert_rgb_back_to_yuy2_sse2<Rec601, 4>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else if (theMatrix == Rec709) {
        convert_rgb_back_to_yuy2_sse2<Rec709, 4>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else if (theMatrix == PC_601) {
        convert_rgb_back_to_yuy2_sse2<PC_601, 4>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else {
        convert_rgb_back_to_yuy2_sse2<PC_709, 4>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      }
    } else {
      if (theMatrix == Rec601) {
        convert_rgb_back_to_yuy2_sse2<Rec601, 3>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else if (theMatrix == Rec709) {
        convert_rgb_back_to_yuy2_sse2<Rec709, 3>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else if (theMatrix == PC_601) {
        convert_rgb_back_to_yuy2_sse2<PC_601, 3>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      } else {
        convert_rgb_back_to_yuy2_sse2<PC_709, 3>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height);
      }
    }
    return dst;
  }

#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX)
  {
    if ((src_cs & VideoInfo::CS_BGR32) == VideoInfo::CS_BGR32) {
      convert_rgb_back_to_yuy2_mmx<4>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height, theMatrix);
    } else {
      convert_rgb_back_to_yuy2_mmx<3>(src->GetReadPtr(), dst->GetWritePtr(), src->GetPitch(), dst->GetPitch(), vi.width, vi.height, theMatrix);
    }
    return dst;
  }
#endif

  const BYTE* rgb = src->GetReadPtr() + (vi.height-1) * src->GetPitch(); // Last line

  const int yuv_offset = dst->GetPitch() - dst->GetRowSize();
  const int rgb_offset = -src->GetPitch() - src->GetRowSize(); // moving upwards
  const int rgb_inc = (src_cs&VideoInfo::CS_BGR32)==VideoInfo::CS_BGR32 ? 4 : 3;

  convert_rgb_back_to_yuy2_c(yuv, rgb, rgb_offset, yuv_offset, vi.height, vi.width, rgb_inc, theMatrix);

  return dst;
}

AVSValue __cdecl ConvertBackToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (!clip->GetVideoInfo().IsYUY2())
    return new ConvertBackToYUY2(clip, args[1].AsString(0), env);

  return clip;
}

  /********************************
   * Dynamic compiled RGB to YUY2 convertion.
   *
   * (c) 2002- 2005 Klaus Post and Ian Brabham
   * (c) 2011       Ian Brabham added 1-2-1 mode
   *
   * rgb24: If true, BGR24 is assumed, otherwise BGR32 is assumed.
   * dupl: Only calculate chroma from the leftmost pixel. Use if material has already been 4:2:2 subsampled.
   * sub: Set to true, if sub_32 has to be subtracted.
   ********************************/

  /********************************
   * - Notes on MMX:
   * Fractions are one bit less than integer code,
   *  but otherwise the algorithm is the same, except
   *  r_y and b_y are calculated at the same time.
   * Order of executin has been changed much for better pairing possibilities.
   * It is important that the 64bit values are 8 byte-aligned
   *  otherwise it will give a huge penalty when accessing them.
   * Instructions pair rather ok, instructions from the top is merged
   *  into last part, to avoid dependency stalls.
   *****************************/


#ifdef X86_32
void ConvertToYUY2::mmx_ConvertRGBtoYUY2(const BYTE *src,BYTE *dst,int src_pitch, int dst_pitch, int h)
{

  src += src_pitch*(h-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0;y<h;y++) {
    assembly.Call(src, dst);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
}
#endif

/* Code for 1-2-1 & 0-1-0 kernels */
#ifdef X86_32
void ConvertToYUY2::GenerateAssembly(bool rgb24, bool dupl, bool sub, int w,
                                     const __int64* ptr_cybgr, const __int64* ptr_y1y2_fpix,
                                     const int* ptr_fraction, IScriptEnvironment* env)  {

  bool sse  = !!(env->GetCPUFlags() & CPUF_SSE);
  bool sse2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool fast128 = !!(env->GetCPUFlags() & (CPUF_SSE3|CPUF_SSSE3|CPUF_SSE4_1|CPUF_SSE4_2));
  //dupl is true for BackToYUY2 and false for ConvertToYUY2
 // if (!fast128 && !dupl)
    sse2 = false; // 1-2-1 SSE2 code is slower than MMX on P4 etc.

  int lwidth_bytes = w;
  lwidth_bytes *= (rgb24) ? 3 : 4;    // Width in bytes

#define SRC esi
#define DST edi
#define RGBEND eax

  Assembler x86;   // This is the class that assembles the code.

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7

  if (!sse2) { // MMX Code

    // Store registers and get arg pointers
    x86.push(        eax);
    x86.mov(         eax,dword_ptr[esp+4+4]);      // Pointer to args list
    x86.push(        SRC);
    x86.mov(         SRC,dword_ptr[eax+0]);        // arg1
    x86.push(        DST);
    x86.mov(         DST,dword_ptr[eax+4]);      // arg2

    x86.movq(        mm2,qword_ptr[SRC]);        //mm2= XXR2 G2B2 XXR1 G1B1
    x86.movq(        mm7,qword_ptr[ptr_cybgr]);
    x86.mov(         RGBEND,lwidth_bytes);
    x86.movd(        mm0,dword_ptr[ptr_fraction]);
    x86.add(         RGBEND,SRC);
    x86.sub(         DST,4);                     // Compensate for early +=4 below
    x86.punpcklbw(   mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
    if (rgb24) {     
      x86.psllq(     mm2,8);                     // Compensate for RGB24
    }                
    x86.cmp(         SRC,RGBEND);
    x86.psrlw(       mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
    x86.jae(         "outloop");                 // Jump out of loop if true (width==0)

    if(!dupl) { // 1-2-1 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
      x86.punpckhbw( mm2,mm0);                   // mm2= 00XX 00R2 00G2 00B2
      x86.movq(      mm6,mm1);                   // mm6 = 00XX 00R0 00G0 00B0
      x86.movq(      mm5,mm2);                   // mm5 = 00XX 00R0 00G0 00B0 for next time
      x86.pmaddwd(   mm2,mm7);                   // mm2 = partial y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1 = partial y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.punpckldq( mm4,mm2);                   // mm4 = 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm4);                   // mm2 = 0000y2.. 0000???? 
      x86.punpckldq( mm4,mm1);                   // mm4 = 00G1+00B1 xxxx xxxx
      x86.psrld(     mm2,15);                    // mm2 = 000000y2 00000000 final value
      x86.paddd(     mm1,mm4);                   // mm1 = 0000y1.. 0000???? 
      x86.movq(      mm3,mm2);                   // mm3 = y2
      x86.psrld(     mm1,15);                    // mm1 = 000000y1 00000000 final value
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm4,qword_ptr[&sub_64]);    // mm4 = -64's
      } else { // PC scale mode
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y2

      x86.align(     16);
      x86.label("loop");

      if (sse)
        x86.prefetchnta(byte_ptr[SRC+64]);

      x86.paddw(     mm6,mm5);                   // mm6 += 00XX 00R2 00G2 00B2
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y1 + y2
      x86.pslld(     mm6,16);                    // Clear out G-value mm6= RRRR 0000 BBBB 0000
      x86.paddw(     mm3,mm1);                   // mm3 = y0 + y1*2 + y2
      x86.punpckhdq( mm1,mm2);                   // mm1= 000000y2 000000y1
      x86.movq(      mm2,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm4);                   // mm3 = y0 + y1*2 - 64
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.punpckhdq( mm3,mm3);                   // Copy scaled_y to lower dword mm3=SCALED_Y SCALED_Y
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.por(       mm6,mm3);                   // mm6 = 00r1 00y1 00b1 00y1
      x86.cmp(       SRC,RGBEND);
      x86.pmaddwd(   mm6,mm2);                   // Mult b_y and r_y 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      x86.pcmpeqd(   mm3,mm3);                   // mm3  = ffff ffff ffff ffff
      x86.psrad(     mm6,1);                     // mm6 /= 2
      x86.pslld(     mm3,16);                    // mm3  = ffff 0000 ffff 0000
      x86.paddd(     mm6,mm4);                   // Add 0x808000 to r_y and b_y
      x86.movq(      mm2,qword_ptr[SRC]);        // mm2= XXR2 G2B2 XXR1 G1B1
      x86.pand(      mm6,mm3);                   // Clear out fractions
      x86.movq(      mm3,mm1);                   // Y0 = Y2 for next time
      x86.por(       mm6,mm1);                   // mm6 = 00vv 00Y2 00uu 00Y1
      x86.punpcklbw( mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
      x86.packuswb(  mm6,mm6);                   // mm6 = VVY2 UUY1 VVY2 UUY1
      x86.psrlw(     mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      if (rgb24) {
        x86.psllq(   mm2,8);                     // Compensate for RGB24
      }
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.punpckhbw( mm2,mm0);                   // mm2 = 00XX 00R2 00G2 00B2
      x86.movq(      mm6,mm5);                   // mm6 = 00XX 00R0 00G0 00B0
      x86.movq(      mm5,mm2);                   // mm5 = 00XX 00R0 00G0 00B0 for next time
      x86.pmaddwd(   mm2,mm7);                   // mm2 = partial y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddw(     mm6,mm1);                   // mm6 += 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1 = partial y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.punpckldq( mm4,mm2);                   // mm4 = 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm4);                   // mm2 = 0000y2.. 0000???? 
      x86.punpckldq( mm4,mm1);                   // mm4 = 00G1+00B1 xxxx xxxx
      x86.psrld(     mm2,15);                    // mm2 = 000000y2 00000000 final value
      x86.paddd(     mm1,mm4);                   // mm1 = 0000y1.. 0000???? 
      x86.paddw(     mm3,mm2);                   // mm3 = y0 + y2
      x86.psrld(     mm1,15);                    // mm1 = 000000y1 00000000 final value
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm4,qword_ptr[&sub_64]);    // mm4 = -64's
      } else { // PC scale mode
        x86.movq(    mm4,qword_ptr[&fpix_add]);  // 0x808000 for r_y and b_y
      }
      x86.jmp(       "loop");                // loop if true

    } else { // 0-1-0 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
      x86.movq(      mm5,qword_ptr[&fpix_add]);  // 0x808000 for add to r_y and b_y

      x86.align(     16);
      x86.label("loop");

      if (sse)
        x86.prefetchnta(byte_ptr[SRC+64]);

      x86.punpckhbw( mm2,mm0);                   // mm2= 00XX 00R2 00G2 00B2

      x86.movq(      mm6,mm1);                   // mm6  = 00XX 00R1 00G1 00B1
      x86.pmaddwd(   mm1,mm7);                   // mm1= v2v2 v2v2 v1v1 v1v1   y1 //(cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + 0x108000)
      x86.pmaddwd(   mm2,mm7);                   // mm2= w2w2 w2w2 w1w1 w1w1   y2 //(cyb*rgbnext[0] + cyg*rgbnext[1] + cyr*rgbnext[2] + 0x108000)
      x86.paddd(     mm1,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.paddd(     mm2,mm0);                   // Add rounding fraction (16.5)<<15 to lower dword only
      x86.punpckldq( mm3,mm1);                   // mm3= 00G1+00B1 xxxx xxxx
      x86.punpckldq( mm4,mm2);                   // mm4= 00G2+00B2 xxxx xxxx
      x86.paddd(     mm1,mm3);
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movd(    mm3,dword_ptr[&sub_16]);    // mm3 = -16
      }
      x86.paddd(     mm2,mm4);
      x86.pslld(     mm6,16);                    // Clear out G-value mm6= RRRR 0000 BBBB 0000
      x86.punpckhdq( mm1,mm2);                   // mm1= 000y2... 000y1...
      x86.movq(      mm4,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E
      x86.psrld(     mm1,15);                    // mm1= 000000y2 000000y1 final value

      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm1);                   // mm3 = y1 - 16
      } else { // PC scale mode
        x86.movq(    mm3,mm1);                   // mm3 = y1
      }

      x86.punpckldq( mm3,mm3);                   // Copy scaled_y to upper dword mm3=SCALED_Y SCALED_Y
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.por(       mm6,mm3);                   // mm6 = 00r1 00y1 00b1 00y1
      x86.pcmpeqd(   mm3,mm3);                   // mm3  = ffff ffff ffff ffff
      x86.paddw(     mm6,mm6);                   // mm6 = 0r10 0y10 0b10 0y10 -- Shift up
      x86.pslld(     mm3,16);                    // mm3  = ffff 0000 ffff 0000
      x86.pmaddwd(   mm6,mm4);                   // Mult b_y and r_y 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddd(     mm6,mm5);                   // Add 0x808000 to r_y and b_y
      x86.movq(      mm2,qword_ptr[SRC]);        // mm2= XXR2 G2B2 XXR1 G1B1
      x86.pand(      mm6,mm3);                   // Clear out fractions
      x86.cmp(       SRC,RGBEND);
      x86.por(       mm6,mm1);                   // mm6 = 00vv 00Y2 00uu 00Y1
      x86.punpcklbw( mm1,mm2);                   // mm1= XXxx R1xx G1xx B1xx
      x86.packuswb(  mm6,mm6);                   // mm6 = VVY2 UUY1 VVY2 UUY1
      x86.psrlw(     mm1,8);                     // mm1= 00XX 00R1 00G1 00B1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      if (rgb24) {
        x86.psllq(   mm2,8);                     // Compensate for RGB24
      }
      x86.jb(        "loop");                    // Jump loop if SRC below RGBEND
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
    }

  } else { // SSE2 code

    // Store registers
    x86.push(        eax);
    x86.mov(         eax,dword_ptr[esp+4+4]);    // Pointer to args list
    x86.push(        SRC);
    x86.push(        DST);

    x86.mov(         SRC,dword_ptr[eax+0]);      // arg1
    x86.mov(         DST,dword_ptr[eax+4]);      // arg2
    if (rgb24) {
      x86.sub(       SRC,1);                     // Compensate for RGB24
    }
    x86.sub(         DST,4);                     // Compensate for early +=4 below

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
    x86.movq(        xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 xxr1 g1b1
    x86.movq(        xmm7,qword_ptr[ptr_cybgr]);
    x86.mov(         RGBEND,lwidth_bytes);
    x86.movd(        xmm0,dword_ptr[ptr_fraction]);
    x86.add(         RGBEND,SRC);
    x86.movq(        mm5,qword_ptr[ptr_y1y2_fpix]);// 0x5033 A29E 3F74 B61E, =(1/((1-0.299)*255/112)<<15+0.5), (1/((1-0.114)*255/112)<<15+0.5)
    x86.prefetchnta( byte_ptr[SRC+64]);
//_______________________________________
    if(!dupl) { // 1-2-1 mode
      x86.cmp(       SRC,RGBEND);
      x86.pxor(      xmm1,xmm1);
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)

      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.movq(    mm0,qword_ptr[&sub_64]);    // mm0  = -64
      }
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.punpcklqdq(xmm7,xmm7);                 // Copy to high qword
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pshufd(    xmm0,xmm0,0x00); // 0000    // Copy to all dwords
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+cyr*R|cyg*G+cyb*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.movq(      mm4,mm3);                   // mm4  = 00XX 00R0 00G0 00B0 first time
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [0000|0000|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.movdq2q(   mm4,xmm3);                  // mm4  = 00XX 00R2 00G2 00B2
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.movq(      mm7,qword_ptr[&fpix_rnd]);  // mm7   = 128.5 << 17 -- 0x0101000001010000
      x86.paddd(     xmm2,xmm4);
      x86.pcmpeqd(   mm6,mm6);                   // mm6  = ffffffff ffffffff
      x86.paddw(     mm3,mm4);                   // mm3  = accumulated RGB values (for b_y and r_y)
      x86.psrld(     xmm2,15);                   // xmm2 = 0000 00y2 0000 00y2 0000 00y1 0000 00y1 final value
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 0000 00y1 0000 00y1
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.pslld(     mm6,16);                    // mm6  = ffff0000 ffff0000
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2
      x86.paddw(     mm3,mm1);                   // Y0 = Y1 for first time

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 XMM6 xmm7
      x86.align(     16);
      x86.label("loop");
//-----------------------------------------------   Unroll 1 mm4, mm6------------------------
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm0);                   // mm3  = y0 - 64
      }
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1 - 64
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 XXr1 g1b1
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1*2 - 64
      x86.punpckldq( mm1,mm2);                   // mm1  = 000000y2 000000y1
      x86.paddw(     mm3,mm2);                   // mm3  = y0 + y1*2 + y2 - 64
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.cmp(       SRC,RGBEND);
      x86.paddd(     mm3,mm7);                   // Add  128.5 to r_y and b_y -- (r_y ?? b_y ?? + 256 0 256 0 + 1 1 1 1) >> 1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00XX 00r1 00g1 00b1
      x86.psrad(     mm3,1);                     // mm3 /= 2

      x86.prefetchnta(byte_ptr[SRC+128]);
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pand(      mm6,mm3);                   // mm6  = .r_y0000 .b_y0000
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.por(       mm6,mm1);                   // mm6  = VVVV 00y2 UUUU 00y1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.packuswb(  mm6,mm6);                   // mm6  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm6);        // Store final pixel
      x86.movdq2q(   mm6,xmm3);                  // mm6  = 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+CYR*R|CYG*G+CYB*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [CYG*G+CYB*B|0*A+CYR*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.paddw(     mm3,mm6);                   // mm3 += 00XX 00R2 00G2 00B2 accumulated RGB values (for b_y and r_y)
      x86.pcmpeqd(   mm4,mm4);                   // mm4  = ffffffff ffffffff
      x86.paddd(     xmm2,xmm4);
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.pslld(     mm4,16);                    // mm4  = ffff0000 ffff0000
      x86.psrld(     xmm2,15);                   // xmm2 = 000000y2 000000y2 000000y1 000000y1 final value
      x86.paddw(     mm3,mm2);                   // mm3  = RRRR 00Y0 BBBB 00Y0
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 000000y1 000000y1
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2

//-----------------------------------------------   Unroll 2 mm6, mm4------------------------
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm0);                   // mm3  = y0 - 64
      }
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1 - 64
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 XXr1 g1b1
      x86.paddw(     mm3,mm1);                   // mm3  = y0 + y1*2 - 64
      x86.punpckldq( mm1,mm2);                   // mm1  = 000000y2 000000y1
      x86.paddw(     mm3,mm2);                   // mm3  = y0 + y1*2 + y2 - 64
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.cmp(       SRC,RGBEND);
      x86.paddd(     mm3,mm7);                   // Add  128.5 to r_y and b_y -- (r_y ?? b_y ?? + 256 0 256 0 + 1 1 1 1) >> 1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00XX 00r1 00g1 00b1
      x86.psrad(     mm3,1);                     // mm3 /= 2

      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.pand(      mm4,mm3);                   // mm4  = .r_y0000 .b_y0000
      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.por(       mm4,mm1);                   // mm4  = VVVV 00y2 UUUU 00y1
      x86.pshufd(    xmm3,xmm2,0x4E); // 1023    // xmm3 = 00XX 00r1 00g1 00b1 00XX 00R2 00G2 00B2
      x86.packuswb(  mm4,mm4);                   // mm4  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.paddw(     mm3,mm3);                   // mm3 += 00XX 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm4);        // Store final pixel
      x86.movdq2q(   mm4,xmm3);                  // mm4  = 00XX 00R2 00G2 00B2
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+CYR*R|CYG*G+CYB*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.paddw(     mm3,mm6);                   // mm3 += 00XX 00R0 00G0 00B0
      x86.pshufd(    xmm4,xmm2,0xB1); // 2301    // xmm4 = [CYG*G+CYB*B|0*A+CYR*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      x86.paddw(     mm3,mm4);                   // mm3 += 00XX 00R2 00G2 00B2 accumulated RGB values (for b_y and r_y)
      x86.pcmpeqd(   mm6,mm6);                   // mm6  = ffffffff ffffffff
      x86.paddd(     xmm2,xmm4);
      x86.pslld(     mm3,16);                    // mm3  = RRRR 0000 BBBB 0000
      x86.pslld(     mm6,16);                    // mm6  = ffff0000 ffff0000
      x86.psrld(     xmm2,15);                   // xmm2 = 000000y2 000000y2 000000y1 000000y1 final value
      x86.paddw(     mm3,mm2);                   // mm3  = RRRR 00Y0 BBBB 00Y0
      x86.pshufd(    xmm5,xmm2,0x4E); // 1023    // xmm5 = 000000y1 000000y1 000000y2 000000y2
      x86.movdq2q(   mm1,xmm2);                  // mm1  = 000000y1 000000y1
      x86.movdq2q(   mm2,xmm5);                  // mm2  = 000000y2 000000y2
//-------------------------------------------------------------------------------------------
      x86.jmp(       "loop");                    // loop if true

    } else { // 0-1-0 mode

// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.pcmpeqd( mm1,mm1);                   // 0xFFFFFFFF
        x86.movq(    mm6,qword_ptr[&fpix_add]);  // Add 128.5 to r_y and b_y -- 0x0080800000808000
        x86.pslld(   mm1,21);                    // 0xFFE00000
        x86.cmp(     SRC,RGBEND);
        x86.psrld(   mm1,16);                    // 0x0000FFE0 = 0 -32 0 -32
      } else {
        x86.movq(    mm6,qword_ptr[&fpix_add]);  // Add 128.5 to r_y and b_y -- 0x0080800000808000
        x86.cmp(     SRC,RGBEND);
      }
      x86.pxor(      xmm1,xmm1);
      x86.jae(       "outloop");                 // Jump out of loop if true (width==0)
      x86.pcmpeqd(   mm4,mm4);                   // mm4 = ffffffff ffffffff
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.punpcklqdq(xmm7,xmm7);                 // Copy to high qword
      x86.pshufd(    xmm0,xmm0,0x00); // 0000    // Copy to all dwords
      x86.pslld(     mm4,16);                    // mm4 = ffff0000 ffff0000
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }

      x86.align(     16);
      x86.label("loop");

      x86.movdq2q(   mm3,xmm2);                  // mm3  = 00XX 00r1 00g1 00b1
      x86.pmaddwd(   xmm2,xmm7);                 // xmm2 = [0*A+cyr*R|cyg*G+cyb*B|0*a+cyr*r|cyg*g+cyb*b]
      x86.pslld(     mm3,17);                    // mm3  = 00r1 0000 00b1 0000 *2
      x86.pshufd(    xmm3,xmm2,0xB1); // 2301    // xmm3 = [cyg*G+cyb*B|0*A+cyr*R|cyg*g+cyb*b|0*a+cyr*r]
      x86.paddd(     xmm2,xmm0);                 // Add rounding fraction (16.5)<<15
      if (sub) { // TV Scale, (X-16)*255/219, mode
        x86.paddw(   mm3,mm1);                   // mm3  += 0 -32 0 -32
      }
      x86.paddd(     xmm2,xmm3);
      x86.add(       SRC, rgb24 ? 6 : 8);
      x86.movdq2q(   mm2,xmm2);                  // mm2  = 00y1.... 00y1....
      x86.psrldq(    xmm2,4);                    // xmm2 = 00000000 00y2.... 00y2.... 00y1....
      x86.psrld(     mm2,14);                    // mm2  = 000000y1 000000y1 *2
      x86.movdq2q(   mm0,xmm2);                  // mm0  = 00y2.... 00y1....
      x86.paddw(     mm3,mm2);                   // mm3  = 00r1 00y1 00b1 00y1 *2
      x86.movq(      xmm2,qword_ptr[SRC]);       // xmm2 = XXR2 G2B2 xxr1 g1b1
      x86.pmaddwd(   mm3,mm5);                   // mm3  = scaled_b-y scaled_r-y
      x86.psrld(     mm0,15);                    // mm0  = 0000 00y1 0000 00y1 final value
      x86.paddd(     mm3,mm6);                   // Add 128.5 to r_y and b_y -- 0x0080800000808000
      x86.prefetchnta(byte_ptr[SRC+128]);
      x86.pand(      mm3,mm4);                   // mm3  = .r_y0000 .b_y0000
      x86.add(       DST,4);                     // Two pixels(packed)
      x86.por(       mm3,mm0);                   // mm3  = VVVV 00y2 UUUU 00y1
      x86.cmp(       SRC,RGBEND);
      x86.packuswb(  mm3,mm3);                   // mm3  = VV Y2 UU Y1 VV Y2 UU Y1
      x86.punpcklbw( xmm2,xmm1);                 // xmm2 = 00XX 00R2 00G2 00B2 00xx 00r1 00g1 00b1
      x86.movd(      dword_ptr[DST],mm3);        // Store final pixel
      if (rgb24) {
        x86.pshuflw( xmm2,xmm2,0x39); // 0321    // Compensate for RGB24
      }
      x86.jb(        "loop");                    // Jump loop if SRC below RGBEND
    }

  }
  x86.align(     16);
  x86.label("outloop");
// xmm0 xmm1 xmm2 xmm3 xmm4 xmm5 xmm6 xmm7
// mm0 mm1 mm2 mm3 mm4 mm5 mm6 mm7

  x86.emms(        );
  // Restore registers
  x86.pop(         DST);
  x86.pop(         SRC);
  x86.pop(         eax);
  x86.ret(         );

  assembly = DynamicAssembledCode(x86, env, "ConvertToYUY2: Dynamic MMX code could not be compiled.");
}
#endif