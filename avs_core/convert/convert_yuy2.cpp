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
#include "convert.h"
#include <avs/alignment.h>

#ifdef AVS_WINDOWS
    #include <avs/win.h>
#else
    #include <avs/posix.h>
#endif


//these are to be used only in asm routines
static const int cyb_rec601 = int(0.114 * 219 / 255 * 65536 + 0.5);
static const int cyg_rec601 = int(0.587 * 219 / 255 * 65536 + 0.5);
static const int cyr_rec601 = int(0.299 * 219 / 255 * 65536 + 0.5);

static const int ku_rec601  = int(112.0 / (255.0 * (1.0 - 0.114)) * 65536 + 0.5);
static const int kv_rec601  = int(112.0 / (255.0 * (1.0 - 0.299)) * 65536 + 0.5);

static const int cyb_rec709 = int(0.0722 * 219 / 255 * 65536 + 0.5);
static const int cyg_rec709 = int(0.7152 * 219 / 255 * 65536 + 0.5);
static const int cyr_rec709 = int(0.2126 * 219 / 255 * 65536 + 0.5);

static const int ku_rec709  = int(112.0 / (255.0 * (1.0 - 0.0722)) * 65536 + 0.5);
static const int kv_rec709  = int(112.0 / (255.0 * (1.0 - 0.2126)) * 65536 + 0.5);


static const int cyb_pc601 = int(0.114 * 65536 + 0.5);
static const int cyg_pc601 = int(0.587 * 65536 + 0.5);
static const int cyr_pc601 = int(0.299 * 65536 + 0.5);

static const int ku_pc601  = int(127.0 / (255.0 * (1.0 - 0.114)) * 65536 + 0.5);
static const int kv_pc601  = int(127.0 / (255.0 * (1.0 - 0.299)) * 65536 + 0.5);

static const int cyb_pc709 = int(0.0722 * 65536 + 0.5);
static const int cyg_pc709 = int(0.7152 * 65536 + 0.5);
static const int cyr_pc709 = int(0.2126 * 65536 + 0.5);

static const int ku_pc709  = int(127.0 / (255.0 * (1.0 - 0.0722)) * 65536 + 0.5);
static const int kv_pc709  = int(127.0 / (255.0 * (1.0 - 0.2126)) * 65536 + 0.5);


static const int cyb_values[4] = {cyb_rec601 / 2, cyb_rec709 / 2, cyb_pc601 / 2, cyb_pc709 / 2};
static const int cyg_values[4] = {cyg_rec601 / 2, cyg_rec709 / 2, cyg_pc601 / 2, cyg_pc709 / 2};
static const int cyr_values[4] = {cyr_rec601 / 2, cyr_rec709 / 2, cyr_pc601 / 2, cyr_pc709 / 2};

static const double luma_rec_scale = 255.0/219.0 * 65536+0.5;

static const int ku_values[4]       = {ku_rec601 / 2, ku_rec709 / 2, ku_pc601 / 2, ku_pc709 / 2};
static const int ku_values_luma[4]  = {-int((ku_rec601/2) * luma_rec_scale) / 65536, -int((ku_rec709/2) * luma_rec_scale) / 65536, -ku_pc601 / 2, -ku_pc709 / 2};
static const int kv_values[4]       = {kv_rec601 / 2, kv_rec709 / 2, kv_pc601 / 2, kv_pc709 / 2};
static const int kv_values_luma[4]  = {-int((kv_rec601/2) * luma_rec_scale) / 65536, -int((kv_rec709/2) * luma_rec_scale) / 65536, -kv_pc601 / 2, -kv_pc709 / 2};

/**********************************
 *******   Convert to YUY2   ******
 *********************************/

ConvertToYUY2::ConvertToYUY2(PClip _child, bool _dupl, bool _interlaced, const char *matrix, IScriptEnvironment* env)
  : GenericVideoFilter(_child), interlaced(_interlaced),src_cs(vi.pixel_type)
{
  AVS_UNUSED(_dupl);
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

  vi.pixel_type = VideoInfo::CS_YUY2;
}

// 1-2-1 Kernel version

static void convert_rgb_to_yuy2_c(const bool pcrange, const int cyb, const int cyg, const int cyr,
                                  const int ku, const int kv, const BYTE* rgb,
                                  BYTE* yuv, const int yuv_offset,
                                  const int rgb_offset, const int rgb_inc,
                                  int width, int height) {

  const int bias = pcrange ? 0x8000 : 0x108000; //  0.5 * 65536 : 16.5 * 65536

  for (int y= height; y>0; --y)
  {
    // Use left most pixel for edge condition
    int y0                 = (cyb*rgb[0] + cyg*rgb[1] + cyr*rgb[2] + bias) >> 16;
    const BYTE* rgb_prev   = rgb;
    for (int x = 0; x < width; x += 2)
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


/*
 Optimization note: you can template matrix parameter like in ConvertBackToYUY2 to get ~5% better performance
*/
PVideoFrame __stdcall ConvertToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if (((src_cs&VideoInfo::CS_YV12)==VideoInfo::CS_YV12)||((src_cs&VideoInfo::CS_I420)==VideoInfo::CS_I420)) {
    PVideoFrame dst = env->NewVideoFrameP(vi, &src);
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
      {
        convert_yv12_to_yuy2_interlaced_c(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
      }
    } else {
        {
          convert_yv12_to_yuy2_progressive_c(srcp_y, srcp_u, srcp_v, src->GetRowSize(PLANAR_Y), src_pitch_y, src_pitch_uv, dstp, dst_pitch ,src_heigh);
        }
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  BYTE* yuv = dst->GetWritePtr();

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

    convert_rgb_to_yuy2_c(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc, vi.width, vi.height);

  } else if (theMatrix == PC_709) {
    const int cyb = int(0.0722*65536+0.5);
    const int cyg = int(0.7152*65536+0.5);
    const int cyr = int(0.2126*65536+0.5);

    const int ku  = int(127./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(127./(255.*(1.0-0.2126))*65536+0.5);

    convert_rgb_to_yuy2_c(true, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc, vi.width, vi.height);

  } else if (theMatrix == Rec709) {
    const int cyb = int(0.0722*219/255*65536+0.5);
    const int cyg = int(0.7152*219/255*65536+0.5);
    const int cyr = int(0.2126*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.0722))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.2126))*65536+0.5);

    convert_rgb_to_yuy2_c(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc, vi.width, vi.height);

  } else if (theMatrix == Rec601) {
    const int cyb = int(0.114*219/255*65536+0.5);
    const int cyg = int(0.587*219/255*65536+0.5);
    const int cyr = int(0.299*219/255*65536+0.5);

    const int ku  = int(112./(255.*(1.0-0.114))*65536+0.5);
    const int kv  = int(112./(255.*(1.0-0.299))*65536+0.5);

    convert_rgb_to_yuy2_c(false, cyb, cyg, cyr, ku, kv, rgb, yuv, yuv_offset, rgb_offset, rgb_inc, vi.width, vi.height);

  }

  return dst;
}


AVSValue __cdecl ConvertToYUY2::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  if (clip->GetVideoInfo().IsYUY2())
    return clip;

  const bool haveOpts = args[3].Defined() || args[4].Defined();

  if (clip->GetVideoInfo().BitsPerComponent() != 8) {
    env->ThrowError("ConvertToYUY2: only 8 bit sources are supported");
  }

  if (clip->GetVideoInfo().IsPlanar()) {
    if (haveOpts || !clip->GetVideoInfo().IsYV12()) {
      // We have no direct conversions. Go to YV16.
      AVSValue new_args[5] = { clip, args[1], args[2], args[3], args[4] };
      clip = ConvertToPlanarGeneric::CreateYUV422(AVSValue(new_args, 5), (void *)0,  env).AsClip(); // (void *)0: restricted to 8 bits
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


PVideoFrame __stdcall ConvertBackToYUY2::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame src = child->GetFrame(n, env);

  if ((src_cs&VideoInfo::CS_YV24)==VideoInfo::CS_YV24)
  {
    PVideoFrame dst = env->NewVideoFrameP(vi, &src);
    BYTE* dstp = dst->GetWritePtr();
    const int dpitch  = dst->GetPitch();

    const BYTE* srcY = src->GetReadPtr(PLANAR_Y);
    const BYTE* srcU = src->GetReadPtr(PLANAR_U);
    const BYTE* srcV = src->GetReadPtr(PLANAR_V);

    const int pitchY  = src->GetPitch(PLANAR_Y);
    const int pitchUV = src->GetPitch(PLANAR_U);

    {
      convert_yv24_back_to_yuy2_c(srcY, srcU, srcV, dstp, pitchY, pitchUV, dpitch, vi.height, vi.width);
    }
    return dst;
  }

  PVideoFrame dst = env->NewVideoFrameP(vi, &src);
  BYTE* yuv = dst->GetWritePtr();

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
