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

#include "color.h"

#include <math.h>
#include <float.h>
#include <malloc.h>
#include <stdio.h>
#include <avs/win.h>
#include <avs/minmax.h>
#include "../core/internal.h"
#include <algorithm>
#include <sstream> // stringstream
#include <iomanip> // setprecision

static void coloryuv_showyuv(BYTE* pY, BYTE* pU, BYTE* pV, int y_pitch, int u_pitch, int v_pitch, int framenumber, bool full_range, int bits_per_pixel)
{
    int internal_bitdepth = bits_per_pixel == 8 ? 8 : 10;
    int luma_min = (full_range ? 0 : 16) << (internal_bitdepth - 8);
    int luma_max = ((full_range ? 256: 236) << (internal_bitdepth - 8)) - 1;

    int chroma_min = (full_range ? 0 : 16) << (internal_bitdepth - 8);
    int chroma_max = ((full_range ? 256: 241) << (internal_bitdepth - 8)) - 1;

    int luma_range   = luma_max - luma_min + 1; // 256/220 ,1024/880
    int chroma_range = chroma_max - chroma_min + 1; // 256/225 ,1024/900

    const int luma_size = chroma_range * 2; // YV12 subsampling

    int luma;
    // Calculate luma cycle
    // 0,1..255,254,..1 = 2x256-2
    // 0,1..1023,1022,..1 = 2*1024-2
    luma = framenumber % (luma_range*2 -2);
    if (luma > luma_range-1)
      luma = (luma_range*2 -2) - luma;
    luma += luma_min;

    // Set luma value
    for (int y = 0; y < luma_size; y++)
    {
      switch(bits_per_pixel) {
      case 8:
        memset(pY, luma, luma_size); break;
      case 10: case 12: case 14: case 16:
        std::fill_n((uint16_t *)pY, luma_size, luma << (bits_per_pixel-internal_bitdepth)); break;
      case 32:
        std::fill_n((float *)pY, luma_size, (float)luma / ((1 << internal_bitdepth) - 1)); break;
      }
      pY += y_pitch;
    }
    // Set chroma
    for (int y = 0; y < chroma_range; y++)
    {
      switch(bits_per_pixel) {
      case 8:
        for (int x = 0; x < chroma_range; x++)
          pU[x] = x + chroma_min;
        memset(pV, y + chroma_min, chroma_range);
        break;
      case 10: case 12: case 14: case 16:
        for (int x = 0; x < chroma_range; x++) {
          reinterpret_cast<uint16_t *>(pU)[x] = (x + chroma_min) << (bits_per_pixel - internal_bitdepth);
        }
        std::fill_n((uint16_t *)pV, chroma_range, (y + chroma_min) << (bits_per_pixel-internal_bitdepth));
        break;
      case 32:
        for (int x = 0; x < chroma_range; x++) {
          reinterpret_cast<float *>(pU)[x] = (float)(x + chroma_min) / ((1 << internal_bitdepth) - 1);
        }
        std::fill_n((float *)pV, chroma_range, (float)(y + chroma_min) / ((1 << internal_bitdepth) - 1));
        break;
      }
      pU += u_pitch;
      pV += v_pitch;
    }
}

// luts are only for integer bits 8/10/12/14/16. float will be realtime
template<typename pixel_t>
static void coloryuv_create_lut(BYTE* lut8, const ColorYUVPlaneConfig* config, int bits_per_pixel, bool tweaklike_params)
{
    pixel_t *lut = reinterpret_cast<pixel_t *>(lut8);

    // parameters are not scaled by bitdepth (legacy 8 bit behaviour)

    const double value_scale = (1 << bits_per_pixel); // scale is 256/1024/4096/16384/65536

    const int lookup_size = (1 << bits_per_pixel); // 256, 1024, 4096, 16384, 65536
    const int pixel_max = lookup_size - 1;

    int tv_range_lo_luma_8 = 16;
    int tv_range_hi_luma_8 = 235;
    int tv_range_lo_chroma_8 = tv_range_lo_luma_8; // 16-240,64–960, 256–3852,... 4096-61692
    int tv_range_hi_chroma_8 = 240;


    int tv_range_lo_luma = (tv_range_lo_luma_8 << (bits_per_pixel - 8));
    int tv_range_hi_luma = (tv_range_hi_luma_8 << (bits_per_pixel - 8));
    int tv_range_lo_chroma = tv_range_lo_luma; // 16-240,64–960, 256–3852,... 4096-61692
    int tv_range_hi_chroma = (tv_range_hi_chroma_8 << (bits_per_pixel - 8));

    double gain = tweaklike_params ? config->gain : (config->gain / 256 + 1.0);
    double contrast = tweaklike_params ? config->contrast : (config->contrast / 256 + 1.0);
    double gamma = tweaklike_params ? config->gamma : (config->gamma / 256 + 1.0);
    double offset = config->offset / 256;

    int range = config->range;
    if (range == COLORYUV_RANGE_PC_TVY)
    {
      // only luma is treated as PC range
        range = config->plane == PLANAR_Y ? COLORYUV_RANGE_PC_TV : COLORYUV_RANGE_NONE;
    }

    double range_factor = 1.0;
    double range_factor_tv_to_pc;
    double range_factor_pc_to_tv;

    if (config->plane == PLANAR_Y) {
      // 8 bit 219 = 235-16, 10 bit: 64–960
      range_factor_tv_to_pc = (double)pixel_max / (tv_range_hi_luma - tv_range_lo_luma); // 255.0 / 219.0
      range_factor_pc_to_tv = (tv_range_hi_luma - tv_range_lo_luma) / (double)pixel_max; // 219.0 / 255.0
    }
    else {
      // 224 = 240-16
      range_factor_tv_to_pc = (double)pixel_max / (tv_range_hi_chroma - tv_range_lo_chroma); // 255.0 / 224.0
      range_factor_pc_to_tv = (tv_range_hi_chroma - tv_range_lo_chroma) / (double)pixel_max;
    }

    if (range != COLORYUV_RANGE_NONE)
    {
      if (range == COLORYUV_RANGE_PC_TV)
        range_factor = range_factor_pc_to_tv;
      else
        range_factor = range_factor_tv_to_pc;
    }

    double tv_range_lo_current = (config->plane == PLANAR_Y ? tv_range_lo_luma : tv_range_lo_chroma);

    // For gamma pre-post correction. we are in [0..1) working range
    double tv_range_lo_normalized = (config->plane == PLANAR_Y) ? tv_range_lo_luma_8 / 256.0 : tv_range_lo_chroma_8 / 256.0;

    for (int i = 0; i < lookup_size; i++) {

        double value = double(i);

        // normalize
        value /= value_scale;

        // Applying gain
        value *= gain;

        // Applying contrast
        value = (value - 0.5) * contrast + 0.5; // integer here, float would be different when its chroma if -0.5..0.5
        // in Classic AVS: constract is applied on the original value and not on the already gained value
        // value = (value * gain) + ((value - 0.5) * contrast + 0.5) - value + (bright - 1);

        // Applying offset
        value += offset;

        // Applying gamma
        if (gamma != 0) {
          // We know that the input is TV range for sure: (coring=true and !PC->TV), levels="TV->PC" or (new!) levels="TV"
          if ((config->clip_tv && range != COLORYUV_RANGE_PC_TV) || range == COLORYUV_RANGE_TV_PC || config->force_tv_range) {
            // avs+ 180301- use gamma on the proper 0.0 based value
            if (value > tv_range_lo_normalized)
            {
              // tv->pc
              value = (value - tv_range_lo_normalized) * range_factor_tv_to_pc; // (v-16)*range
              value = pow(value, 1.0 / gamma);
              // pc->tv
              value = value * range_factor_pc_to_tv + tv_range_lo_normalized; // v*range - 16
            }
          }
          else {
            if (value > 0) {
              value = pow(value, 1.0 / gamma);
            }
          }
        }

        value *= value_scale;

        // Range conversion
        if (range == COLORYUV_RANGE_PC_TV)
        {
          value = value * range_factor + tv_range_lo_current; // v*range - 16
        }
        else if (range == COLORYUV_RANGE_TV_PC)
        {
          value = (value - tv_range_lo_current) * range_factor; // (v-16)*range
        }

        // Convert back to int
        //int iValue = int(value); // hmm P.F. 20180226 no rounding?
        int iValue = int(value + 0.5); // P.F. 20180611 yes, round!

        // Clamp
        iValue = clamp(iValue, 0, pixel_max);

        if (config->clip_tv) // set when coring
        {
            iValue = clamp(iValue, tv_range_lo_luma, config->plane == PLANAR_Y ? tv_range_hi_luma : tv_range_hi_chroma);
        }

        lut[i] = iValue;
    }
}

static std::string coloryuv_create_lut_expr(const ColorYUVPlaneConfig* config, int bits_per_pixel, bool tweaklike_params)
{
  // parameters are not scaled by bitdepth (legacy 8 bit behaviour)

  const bool f32 = (bits_per_pixel == 32);

  const double value_scale = f32 ? 1.0 : (1 << bits_per_pixel); // scale is 256/1024/4096/16384/65536

  double pixel_max;
  double tv_range_lo_luma;
  double tv_range_hi_luma;
  double tv_range_lo_chroma;
  double tv_range_hi_chroma;

  int tv_range_lo_luma_8 = 16;
  int tv_range_hi_luma_8 = 235;
  int tv_range_lo_chroma_8 = tv_range_lo_luma_8; // 16-240,64–960, 256–3852,... 4096-61692
  int tv_range_hi_chroma_8 = 240;

  if (f32) {
    pixel_max = 1.0;
    tv_range_lo_luma = tv_range_lo_luma_8 / 256.0;
    tv_range_hi_luma = tv_range_hi_luma_8 / 256.0;
    tv_range_lo_chroma = (tv_range_lo_chroma_8 - 128.0) / 256.0; // -112
    tv_range_hi_chroma = (tv_range_hi_chroma_8 - 128.0) / 256.0; // 112
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
    tv_range_lo_chroma = tv_range_lo_chroma + 0.5; // -112
    tv_range_hi_chroma = tv_range_hi_chroma + 0.5; // 112
#endif
  }
  else {
    pixel_max = (double)((1 << bits_per_pixel) - 1);
    tv_range_lo_luma = (double)(tv_range_lo_luma_8 << (bits_per_pixel - 8));
    tv_range_hi_luma = (double)(tv_range_hi_luma_8 << (bits_per_pixel - 8));
    tv_range_lo_chroma = tv_range_lo_luma; // 16-240,64–960, 256–3852,... 4096-61692
    tv_range_hi_chroma = (double)(tv_range_hi_chroma_8 << (bits_per_pixel - 8));
  }

  double gain = tweaklike_params ? config->gain : (config->gain / 256 + 1.0);
  double contrast = tweaklike_params ? config->contrast : (config->contrast / 256 + 1.0);
  double gamma = tweaklike_params ? config->gamma : (config->gamma / 256 + 1.0);
  double offset = config->offset / 256; // always in the 256 range

  int range = config->range; // enum
  if (range == COLORYUV_RANGE_PC_TVY)
  {
    range = config->plane == PLANAR_Y ? COLORYUV_RANGE_PC_TV : COLORYUV_RANGE_NONE;
  }

  double range_factor = 1.0;
  double range_factor_tv_to_pc;
  double range_factor_pc_to_tv;

  if (config->plane == PLANAR_Y) {
    // 8 bit 219 = 235-16, 10 bit: 64–960
    range_factor_tv_to_pc = (double)pixel_max / (tv_range_hi_luma - tv_range_lo_luma); // 255.0 / 219.0
    range_factor_pc_to_tv = (tv_range_hi_luma - tv_range_lo_luma) / (double)pixel_max; // 219.0 / 255.0
  }
  else {
    // 224 = 240-16
    range_factor_tv_to_pc = (double)pixel_max / (tv_range_hi_chroma - tv_range_lo_chroma); // 255.0 / 224.0
    range_factor_pc_to_tv = (tv_range_hi_chroma - tv_range_lo_chroma) / (double)pixel_max;
  }

  if (range != COLORYUV_RANGE_NONE)
  {
    if (range == COLORYUV_RANGE_PC_TV)
      range_factor = range_factor_pc_to_tv;
    else
      range_factor = range_factor_tv_to_pc;
  }

  double tv_range_lo_current = (config->plane == PLANAR_Y ? tv_range_lo_luma : tv_range_lo_chroma);

  // For gamma pre-post correction. we are in [0..1) working range
  double tv_range_lo_normalized = (config->plane == PLANAR_Y) ? tv_range_lo_luma_8 / 256.0 : tv_range_lo_chroma_8 / 256.0;

  std::stringstream ss;

  // value = double(i)
  ss << "x";

  if (!f32) {
    // value = value / value_scale;
    ss << " " << value_scale << " /";
  }

  if (gain != 1.0) {
    // Applying gain
    // value *= gain;
    ss << " " << gain << " *";
  }

  // Applying contrast
  //  value = (value - 0.5) * contrast + 0.5;
  if (f32) {
    // Although it is possible, it doesn't make sense to apply this setting to the luma of the signal. 
    if (config->plane == PLANAR_Y) {
      ss << " 0.5 - " << contrast << " * 0.5 +";
    }
    else {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
      ss << " 0.5 - " << contrast << " * 0.5 +";
#else
      ss << " " << contrast << " *";
#endif
    }
  }
  else {
    ss << " 0.5 - " << contrast << " * 0.5 +";
  }

  // Applying offset
  // value += offset;
  ss << " " << offset << " +";

  // Applying gamma
  if (gamma != 0) {
    // We know that the input is TV range for sure: (coring=true and not pc->tv), levels="TV->PC" or (new!) levels="TV"
    if ((config->clip_tv && range != COLORYUV_RANGE_PC_TV) || range == COLORYUV_RANGE_TV_PC || config->force_tv_range) {
      // avs+ 180301- use gamma on the proper 0.0 based value
      // value = value > 16scaled ? (pow((value - 16scl)*range_tv_pc,(1/gamma))*range_pc_tv+16d : value
      // tv->pc, power, pc->tv
      ss << " A@ " << tv_range_lo_normalized << " > A " << tv_range_lo_normalized << " - " << range_factor_tv_to_pc << " * " << (1.0 / gamma) << " pow " << range_factor_pc_to_tv << " * " << tv_range_lo_normalized << " + " << " A ?";
    }
    else {
      //if (gamma != 0 && value > 0)
      //  value = pow(value, 1.0 / gamma);
      // value = value > 0 ? pow(value,(1/gamma)) : value
      ss << " A@ 0 > A " << (1.0 / gamma) << " pow A ?";
    }
  }

  if (!f32) {
    // value *= value_scale;
    ss << " " << value_scale << " *";
  }

  if (config->clip_tv) // set when coring
  {
    ss << " " << (config->plane == PLANAR_Y ? tv_range_lo_luma : tv_range_lo_chroma) << " max " << (config->plane == PLANAR_Y ? tv_range_hi_luma : tv_range_hi_chroma) << " min ";
    //iValue = clamp(iValue, tv_range_lo_luma, config->plane == PLANAR_Y ? tv_range_hi_luma : tv_range_hi_chroma);
  }

  // clamp to the original valid bitdepth is done by Expr

  // Range conversion
  if (range == COLORYUV_RANGE_PC_TV)
  {
    // value = value * range_factor + tv_range_lo_luma; // v*range - 16
    if (config->plane == PLANAR_Y)
      ss << " " << range_factor << " * " << tv_range_lo_luma << " +";
    else
      ss << " " << range_factor << " * " << tv_range_lo_chroma << " +";
  }
  else if (range == COLORYUV_RANGE_TV_PC)
  {
    //value = (value - tv_range_lo_luma) * range_factor; // (v-16)*range
    if (config->plane == PLANAR_Y)
      ss << " " << tv_range_lo_luma << " - " << range_factor << " *";
    else
      ss << " " << tv_range_lo_chroma << " - " << range_factor << " *";
  }

  std::string exprString = ss.str();
  return exprString;

}

// for float, only loose_min and loose_max is counted
template<bool forFloat>
static void coloryuv_analyse_core(const int* freq, const int pixel_num, ColorYUVPlaneData* data, int bits_per_pixel_for_stat)
{
  // 32 bit float reached here as split into ranges like a 16bit clip
  int pixel_value_count = 1 << bits_per_pixel_for_stat; // size of freq table

  const int pixel_256th = pixel_num / 256; // For loose max/min yes, still 1/256!

  double avg = 0.0;
  int real_min, real_max;

  if (!forFloat) {
    real_min = -1;
    real_max = -1;
  }
  data->loose_max = -1;
  data->loose_min = -1;

  int px_min_c = 0, px_max_c = 0;

  for (int i = 0; i < pixel_value_count; i++)
  {
    if (!forFloat) {
      avg += freq[i] * double(i);

      if (freq[i] > 0 && real_min == -1)
      {
        real_min = i;
      }
    }

    if (data->loose_min == -1)
    {
      px_min_c += freq[i];

      if (px_min_c > pixel_256th)
      {
        data->loose_min = i;
      }
    }

    if (!forFloat) {
      if (freq[pixel_value_count - 1 - i] > 0 && real_max == -1)
      {
        real_max = pixel_value_count - 1 - i;
      }
    }

    if (data->loose_max == -1)
    {
      px_max_c += freq[pixel_value_count - 1 - i];

      if (px_max_c > pixel_256th)
      {
        data->loose_max = pixel_value_count - 1 - i;
      }
    }
  }

  if (!forFloat) {
    avg /= pixel_num;
    data->average = avg;
    data->real_min = (float)real_min;
    data->real_max = (float)real_max;
  }
}


static void coloryuv_analyse_planar(const BYTE* pSrc, int src_pitch, int width, int height, ColorYUVPlaneData* data, int bits_per_pixel, bool chroma)
{
    // We can gather statistics from float, but for population count we have to
    // split the range. We decide to split it into 2^16 ranges.
    int bits_per_pixel_for_freq = bits_per_pixel <= 16 ? bits_per_pixel : 16;
    int statistics_size = 1 << bits_per_pixel_for_freq; // float: 65536
    int *freq = new int[statistics_size];
    std::fill_n(freq, statistics_size, 0);

    double sum = 0.0; // for float
    float real_min;
    float real_max;

    if(bits_per_pixel==8) {
      for (int y = 0; y < height; y++)
      {
          for (int x = 0; x < width; x++)
          {
              freq[pSrc[x]]++;
          }

          pSrc += src_pitch;
      }
    }
    else if (bits_per_pixel >= 10 && bits_per_pixel <= 14) {
      uint16_t mask = statistics_size - 1; // e.g. 0x3FF for 10 bit
      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          freq[clamp(reinterpret_cast<const uint16_t *>(pSrc)[x],(uint16_t)0,mask)]++;
        }

        pSrc += src_pitch;
      }
    }
    else if (bits_per_pixel == 16) {
      // no clamp, faster
      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          freq[reinterpret_cast<const uint16_t *>(pSrc)[x]]++;
        }

        pSrc += src_pitch;
      }
    } else if(bits_per_pixel==32) {
      // 32 bits: we populate pixels only for loose_min and loose_max
      // real_min and real_max, average (sum) is computed differently
      real_min = reinterpret_cast<const float *>(pSrc)[0];
      real_max = real_min;
      if (chroma) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
        const float shift = 0.0f;
#else
        const float shift = 32768.0f;
#endif
        for (int y = 0; y < height; y++)
        {
          for (int x = 0; x < width; x++)
          {
            // -0.5..0.5 (0..1.0 when FLOAT_CHROMA_IS_HALF_CENTERED) to 0..65535
            // see also: ConditionalFunctions MinMax
            const float pixel = reinterpret_cast<const float *>(pSrc)[x];
            freq[clamp((int)(65535.0f*pixel + shift + 0.5f), 0, 65535)]++;
            // todo: SSE2
            real_min = min(real_min, pixel);
            real_max = max(real_max, pixel);
            sum += pixel;
          }
          pSrc += src_pitch;
        }
      }
      else {
        for (int y = 0; y < height; y++)
        {
          for (int x = 0; x < width; x++)
          {
            // 0..1 -> 0..65535
            const float pixel = reinterpret_cast<const float *>(pSrc)[x];
            freq[clamp((int)(65535.0f*pixel + 0.5f), 0, 65535)]++;
            // todo: SSE2
            real_min = min(real_min, pixel);
            real_max = max(real_max, pixel);
            sum += pixel;
          }

          pSrc += src_pitch;
        }
      }
    }


    if (bits_per_pixel == 32) {
      coloryuv_analyse_core<true>(freq, width*height, data, bits_per_pixel_for_freq);
      data->average = sum / (height * width);
      data->real_max = real_max;
      data->real_min = real_min;
#ifndef FLOAT_CHROMA_IS_HALF_CENTERED
      // loose min and max was shifted by half of 16bit range. We still keep here the range
      if (chroma) {
        data->loose_max = data->loose_max - 32768;
        data->loose_min = data->loose_min - 32768;
      }
#endif
      // autogain treats it as a value of 16bit magnitude, show=true as well
      //data->loose_min = data->loose_min / 65535.0f; not now.
      //data->loose_max = data->loose_max / 65535.0f;
    }
    else {
      coloryuv_analyse_core<false>(freq, width*height, data, bits_per_pixel_for_freq);
    }

    delete[] freq;
}

static void coloryuv_analyse_yuy2(const BYTE* pSrc, int src_pitch, int width, int height, ColorYUVPlaneData* dataY, ColorYUVPlaneData* dataU, ColorYUVPlaneData* dataV)
{
    int freqY[256], freqU[256], freqV[256];
    memset(freqY, 0, sizeof(freqY));
    memset(freqU, 0, sizeof(freqU));
    memset(freqV, 0, sizeof(freqV));

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width*2; x+=4)
        {
            freqY[pSrc[x+0]]++;
            freqU[pSrc[x+1]]++;
            freqY[pSrc[x+2]]++;
            freqV[pSrc[x+3]]++;
        }

        pSrc += src_pitch;
    }

    coloryuv_analyse_core<false>(freqY, width*height, dataY, 1);
    coloryuv_analyse_core<false>(freqU, width*height/2, dataU, 1);
    coloryuv_analyse_core<false>(freqV, width*height/2, dataV, 1);
}

static void coloryuv_autogain(const ColorYUVPlaneData* dY, const ColorYUVPlaneData* dU, const ColorYUVPlaneData* dV, ColorYUVPlaneConfig* cY, ColorYUVPlaneConfig* cU, ColorYUVPlaneConfig* cV,
  int bits_per_pixel, bool tweaklike_params)
{
  int bits_per_pixel_for_freq = bits_per_pixel <= 16 ? bits_per_pixel : 16; // for float: "loose" statistics like uint16_t
  // always 16..235
  int loose_max_limit = (235 + 1) << (bits_per_pixel_for_freq - 8);
  int loose_min_limit = 16 << (bits_per_pixel_for_freq - 8);
  double gain_corr = 1 << bits_per_pixel_for_freq;
  int maxY = min(dY->loose_max, loose_max_limit);
  int minY = max(dY->loose_min, loose_min_limit);

  int range = maxY - minY;

  if (range > 0) {
    double scale = double(loose_max_limit - loose_min_limit) / range;
    cY->offset = (loose_min_limit - scale * minY) / (1 << (bits_per_pixel_for_freq - 8)); // good for float also, 0..256 range
    cY->gain = tweaklike_params ? scale : (256 * (scale - 1.0));
    cY->changed = true;
  }
}

static void coloryuv_autowhite(const ColorYUVPlaneData* dY, const ColorYUVPlaneData* dU, const ColorYUVPlaneData* dV, ColorYUVPlaneConfig* cY, ColorYUVPlaneConfig* cU, ColorYUVPlaneConfig* cV,
  int bits_per_pixel)
{
  if (bits_per_pixel == 32) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
    double middle = 0.5;
#else
    double middle = 0.0;
#endif
    cU->offset = (middle - dU->average) * 256; // parameter is in 256 range
    cV->offset = (middle - dV->average) * 256;
  }
  else {
    double middle = (1 << (bits_per_pixel - 1)) - 1; // 128-1, 2048-1 ...
    cU->offset = (middle - dU->average) / (1 << (bits_per_pixel - 8)); // parameter is in 256 range
    cV->offset = (middle - dV->average) / (1 << (bits_per_pixel - 8));
  }
  cU->changed = true;
  cV->changed = true;
}

// only for integer samples
static void coloryuv_apply_lut_planar(BYTE* pDst, const BYTE* pSrc, int dst_pitch, int src_pitch, int width, int height, const BYTE* lut, int bits_per_pixel)
{
    if(bits_per_pixel==8)
    {
      for (int y = 0; y < height; y++)
      {
          for (int x = 0; x < width; x++)
          {
              pDst[x] = lut[pSrc[x]];
          }

          pSrc += src_pitch;
          pDst += dst_pitch;
      }
    }
    else if (bits_per_pixel >= 10 && bits_per_pixel <= 14) {
      uint16_t max_pixel_value = (1 << bits_per_pixel) - 1;
      // protection needed for lut
      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          uint16_t pixel = reinterpret_cast<const uint16_t *>(pSrc)[x];
          pixel = pixel <= max_pixel_value ? pixel : max_pixel_value;
          reinterpret_cast<uint16_t *>(pDst)[x] = reinterpret_cast<const uint16_t *>(lut)[pixel];
        }

        pSrc += src_pitch;
        pDst += dst_pitch;
      }
    }
    else if (bits_per_pixel == 16) {
      // no protection, faster
      for (int y = 0; y < height; y++)
      {
        for (int x = 0; x < width; x++)
        {
          reinterpret_cast<uint16_t *>(pDst)[x] = reinterpret_cast<const uint16_t *>(lut)[reinterpret_cast<const uint16_t *>(pSrc)[x]];
        }

        pSrc += src_pitch;
        pDst += dst_pitch;
      }
    }
}

static void coloryuv_apply_lut_yuy2(BYTE* pDst, const BYTE* pSrc, int dst_pitch, int src_pitch, int width, int height, const BYTE* lutY, const BYTE* lutU, const BYTE* lutV)
{
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width * 2; x += 4)
        {
            pDst[x+0] = lutY[pSrc[x + 0]];
            pDst[x+1] = lutU[pSrc[x + 1]];
            pDst[x+2] = lutY[pSrc[x + 2]];
            pDst[x+3] = lutV[pSrc[x + 3]];
        }

        pSrc += src_pitch;
        pDst += dst_pitch;
    }
}

#define READ_CONDITIONAL(plane, var_name, internal_name)  \
    {                                                     \
        const double t = env2->GetVar("coloryuv_" #var_name "_" #plane, DBL_MIN); \
        if (t != DBL_MIN) {                               \
            c_##plane->internal_name = t;               \
            c_##plane->changed = true;                  \
        }                                                 \
    }

static void coloryuv_read_conditional(IScriptEnvironment* env, ColorYUVPlaneConfig* c_y, ColorYUVPlaneConfig* c_u, ColorYUVPlaneConfig* c_v)
{
    auto env2 = static_cast<IScriptEnvironment2*>(env);

    READ_CONDITIONAL(y, gain, gain);
    READ_CONDITIONAL(y, off, offset);
    READ_CONDITIONAL(y, gamma, gamma);
    READ_CONDITIONAL(y, cont, contrast);

    READ_CONDITIONAL(u, gain, gain);
    READ_CONDITIONAL(u, off, offset);
    READ_CONDITIONAL(u, gamma, gamma);
    READ_CONDITIONAL(u, cont, contrast);

    READ_CONDITIONAL(v, gain, gain);
    READ_CONDITIONAL(v, off, offset);
    READ_CONDITIONAL(v, gamma, gamma);
    READ_CONDITIONAL(v, cont, contrast);
}

#undef READ_CONDITIONAL

ColorYUV::ColorYUV(PClip child,
  double gain_y, double offset_y, double gamma_y, double contrast_y,
  double gain_u, double offset_u, double gamma_u, double contrast_u,
  double gain_v, double offset_v, double gamma_v, double contrast_v,
  const char* level, const char* opt,
  bool showyuv, bool analyse, bool autowhite, bool autogain, bool conditional,
  int bits, bool showyuv_fullrange,
  bool tweaklike_params, // ColorYUV2: 0.0/0.5/1.0/2.0/3.0 instead of -256/-128/0/256/512
  IScriptEnvironment* env)
 : GenericVideoFilter(child),
   colorbar_bits(showyuv ? bits : 0), analyse(analyse), autowhite(autowhite), autogain(autogain), conditional(conditional), colorbar_fullrange(showyuv_fullrange), tweaklike_params(tweaklike_params)
{
    luts[0] = luts[1] = luts[2] = nullptr;

    if (!vi.IsYUV() && !vi.IsYUVA())
        env->ThrowError("ColorYUV: Only work with YUV colorspace.");

    configY.gain = gain_y;
    configY.offset = offset_y;
    configY.gamma = gamma_y;
    configY.contrast = contrast_y;
    configY.changed = false;
    configY.clip_tv = false;
    configY.force_tv_range = false;
    configY.plane = PLANAR_Y;

    configU.gain = gain_u;
    configU.offset = offset_u;
    configU.gamma = gamma_u;
    configU.contrast = contrast_u;
    configU.changed = false;
    configU.clip_tv = false;
    configU.force_tv_range = false; // n/a. in chroma. For gamma
    configU.plane = PLANAR_U;

    configV.gain = gain_v;
    configV.offset = offset_v;
    configV.gamma = gamma_v;
    configV.contrast = contrast_v;
    configV.changed = false;
    configV.clip_tv = false;
    configV.force_tv_range = false; // n/a. in chroma. For gamma
    configV.plane = PLANAR_V;

    // Range
    if (lstrcmpi(level, "TV->PC") == 0)
    {
        configV.range = configU.range = configY.range = COLORYUV_RANGE_TV_PC;
    }
    else if (lstrcmpi(level, "PC->TV") == 0)
    {
        configV.range = configU.range = configY.range = COLORYUV_RANGE_PC_TV;
    }
    else if (lstrcmpi(level, "PC->TV.Y") == 0)
    {
        configV.range = configU.range = configY.range = COLORYUV_RANGE_PC_TVY;
    }
    else if (lstrcmpi(level, "TV") == 0)
    {
      configV.force_tv_range = configU.force_tv_range = configY.force_tv_range = true;
    }
    else if (lstrcmpi(level, "") != 0)
    {
        env->ThrowError("ColorYUV: invalid parameter : levels");
    }
    else {
      // avs+: missing init to none
      configV.range = configU.range = configY.range = COLORYUV_RANGE_NONE;
    }

    // Option
    if (lstrcmpi(opt, "coring") == 0)
    {
        configY.clip_tv = true;
        configU.clip_tv = true;
        configV.clip_tv = true;
    }
    else if (lstrcmpi(opt, "") != 0)
    {
        env->ThrowError("ColorYUV: invalid parameter : opt");
    }

    if(showyuv && colorbar_bits !=8 && colorbar_bits != 10 && colorbar_bits != 12 && colorbar_bits != 14 && colorbar_bits != 16 && colorbar_bits != 32)
      env->ThrowError("ColorYUV: bits parameter for showyuv must be 8, 10, 12, 14, 16 or 32");

    if (colorbar_bits>0 && showyuv)
    {
        // pre-avs+: coloryuv_showyuv is always called with full_range false
        int chroma_range = colorbar_fullrange ? 256 : (240 - 16 + 1); // 0..255, 16..240
        // size limited to either 8 or 10 bits, independenly of 12/14/16 or 32bit float bit-depth
        vi.width = (colorbar_bits == 8 ? chroma_range : (chroma_range*4)) * 2;
        vi.height = vi.width;
        switch (colorbar_bits) {
        case 8: vi.pixel_type = VideoInfo::CS_YV12; break;
        case 10: vi.pixel_type = VideoInfo::CS_YUV420P10; break;
        case 12: vi.pixel_type = VideoInfo::CS_YUV420P12; break;
        case 14: vi.pixel_type = VideoInfo::CS_YUV420P14; break;
        case 16: vi.pixel_type = VideoInfo::CS_YUV420P16; break;
        case 32: vi.pixel_type = VideoInfo::CS_YUV420PS; break;
        }
    }

    if (!showyuv) {
      // prepare basic LUT
      int pixelsize = vi.ComponentSize();
      int bits_per_pixel = vi.BitsPerComponent();

      if (pixelsize == 1 || pixelsize == 2) {
        // no float lut. float will be realtime
        int lut_size = pixelsize * (1 << bits_per_pixel); // 256*1 / 1024*2 .. 65536*2
        luts[0] = new BYTE[lut_size];
        if (!vi.IsY()) {
          luts[1] = new BYTE[lut_size];
          luts[2] = new BYTE[lut_size];
        }

        if (pixelsize == 1) {
          coloryuv_create_lut<uint8_t>(luts[0], &configY, bits_per_pixel, tweaklike_params);
          if (!vi.IsY())
          {
            coloryuv_create_lut<uint8_t>(luts[1], &configU, bits_per_pixel, tweaklike_params);
            coloryuv_create_lut<uint8_t>(luts[2], &configV, bits_per_pixel, tweaklike_params);
          }
        }
        else if (pixelsize == 2) { // pixelsize==2
          coloryuv_create_lut<uint16_t>(luts[0], &configY, bits_per_pixel, tweaklike_params);
          if (!vi.IsY())
          {
            coloryuv_create_lut<uint16_t>(luts[1], &configU, bits_per_pixel, tweaklike_params);
            coloryuv_create_lut<uint16_t>(luts[2], &configV, bits_per_pixel, tweaklike_params);
          }
        }
      }
    }
}

ColorYUV::~ColorYUV() {
  if (luts[0]) delete[] luts[0];
  if (luts[1]) delete[] luts[1];
  if (luts[2]) delete[] luts[2];
}


PVideoFrame __stdcall ColorYUV::GetFrame(int n, IScriptEnvironment* env)
{
    if (colorbar_bits>0)
    {
        PVideoFrame dst = env->NewVideoFrame(vi);
        // pre AVS+: full_range is always false
        // AVS+: showyuv_fullrange bool parameter
        // AVS+: bits parameter
        coloryuv_showyuv(dst->GetWritePtr(), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(), dst->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_V), n, colorbar_fullrange, colorbar_bits);
        return dst;
    }

    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst;
    
    int pixelsize = vi.ComponentSize();
    int bits_per_pixel = vi.BitsPerComponent();

    ColorYUVPlaneConfig // Yes, we copy these struct
        cY = configY,
        cU = configU,
        cV = configV;

    // for analysing data
    char text[512];

    if (analyse || autowhite || autogain)
    {
        ColorYUVPlaneData dY, dU, dV;

        if (vi.IsYUY2())
        {
            coloryuv_analyse_yuy2(src->GetReadPtr(), src->GetPitch(), vi.width, vi.height, &dY, &dU, &dV);
        }
        else
        {
            coloryuv_analyse_planar(src->GetReadPtr(), src->GetPitch(), vi.width, vi.height, &dY, bits_per_pixel, false); // false: not chroma
            if (!vi.IsY())
            {
                const int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
                const int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

                coloryuv_analyse_planar(src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), width, height, &dU, bits_per_pixel, true); // true: chroma
                coloryuv_analyse_planar(src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), width, height, &dV, bits_per_pixel, true);
            }
        }

        if (analyse)
        {
          if (!vi.IsY())
          {
            if (bits_per_pixel == 32)
              sprintf(text,
                "        Frame: %-8u ( Luma Y / ChromaU / ChromaV )\n"
                "      Average:      ( %7.5f / %7.5f / %7.5f )\n"
                "      Minimum:      ( %7.5f / %7.5f / %7.5f )\n"
                "      Maximum:      ( %7.5f / %7.5f / %7.5f )\n"
                "Loose Minimum:      ( %7.5f / %7.5f / %7.5f )\n"
                "Loose Maximum:      ( %7.5f / %7.5f / %7.5f )\n",
                n,
                dY.average, dU.average, dV.average,
                dY.real_min, dU.real_min, dV.real_min,
                dY.real_max, dU.real_max, dV.real_max,
                (float)dY.loose_min / 65535.0f, (float)dU.loose_min / 65535.0f, (float)dV.loose_min / 65535.0f,
                (float)dY.loose_max / 65535.0f, (float)dU.loose_max / 65535.0f, (float)dV.loose_max / 65535.0f
              );
            else
              sprintf(text,
                "        Frame: %-8u ( Luma Y / ChromaU / ChromaV )\n"
                "      Average:      ( %7.2f / %7.2f / %7.2f )\n"
                "      Minimum:      (  %5d  /  %5d  /  %5d   )\n"
                "      Maximum:      (  %5d  /  %5d  /  %5d   )\n"
                "Loose Minimum:      (  %5d  /  %5d  /  %5d   )\n"
                "Loose Maximum:      (  %5d  /  %5d  /  %5d   )\n",
                n,
                dY.average, dU.average, dV.average,
                (int)dY.real_min, (int)dU.real_min, (int)dV.real_min,
                (int)dY.real_max, (int)dU.real_max, (int)dV.real_max,
                dY.loose_min, dU.loose_min, dV.loose_min,
                dY.loose_max, dU.loose_max, dV.loose_max
              );
          }
          else
          {
            if (bits_per_pixel == 32)
              sprintf(text,
                "        Frame: %-8u\n"
                "      Average: %7.5f\n"
                "      Minimum: %7.5f\n"
                "      Maximum: %7.5f\n"
                "Loose Minimum: %7.5f\n"
                "Loose Maximum: %7.5f\n",
                n,
                dY.average,
                dY.real_min,
                dY.real_max,
                (float)dY.loose_min / 65535.0f,
                (float)dY.loose_max / 65535.0f
              );
            else
              sprintf(text,
                "        Frame: %-8u\n"
                "      Average: %7.2f\n"
                "      Minimum: %5d\n"
                "      Maximum: %5d\n"
                "Loose Minimum: %5d\n"
                "Loose Maximum: %5d\n",
                n,
                dY.average,
                (int)dY.real_min,
                (int)dY.real_max,
                dY.loose_min,
                dY.loose_max
              );
          }
        }

        if (autowhite && !vi.IsY())
        {
            coloryuv_autowhite(&dY, &dU, &dV, &cY, &cU, &cV, bits_per_pixel);
        }

        if (autogain)
        {
            coloryuv_autogain(&dY, &dU, &dV, &cY, &cU, &cV, bits_per_pixel, tweaklike_params);
        }
    }

    // Read conditional variables
    coloryuv_read_conditional(env, &cY, &cU, &cV);

    // no float lut. float will be realtime
    if (pixelsize == 1 || pixelsize == 2) {

      BYTE *luts_live[3] = { nullptr };

      BYTE *lutY = luts[0];
      BYTE *lutU = luts[1];
      BYTE *lutV = luts[2];

      // recalculate plane LUT only if changed
      int lut_size = pixelsize * (1 << bits_per_pixel); // 256*1 / 1024*2 .. 65536*2
      if (cY.changed) {
        luts_live[0] = new BYTE[lut_size];
        lutY = luts_live[0];
        if (pixelsize == 1)
          coloryuv_create_lut<uint8_t>(lutY, &cY, bits_per_pixel, tweaklike_params);
        else if (pixelsize == 2)
          coloryuv_create_lut<uint16_t>(lutY, &cY, bits_per_pixel, tweaklike_params);
      }

      if (!vi.IsY())
      {
        if (cU.changed) {
          luts_live[1] = new BYTE[lut_size];
          lutU = luts_live[1];
          if (pixelsize == 1)
            coloryuv_create_lut<uint8_t>(lutU, &cU, bits_per_pixel, tweaklike_params);
          else if (pixelsize == 2)
            coloryuv_create_lut<uint16_t>(lutU, &cU, bits_per_pixel, tweaklike_params);
        }
        if (cV.changed) {
          luts_live[2] = new BYTE[lut_size];
          lutV = luts_live[2];
          if (pixelsize == 1)
            coloryuv_create_lut<uint8_t>(lutV, &cV, bits_per_pixel, tweaklike_params);
          else if (pixelsize == 2)
            coloryuv_create_lut<uint16_t>(lutV, &cV, bits_per_pixel, tweaklike_params);
        }
      }
      dst = env->NewVideoFrame(vi); // go live dst here!

      if (vi.IsYUY2())
      {
        coloryuv_apply_lut_yuy2(dst->GetWritePtr(), src->GetReadPtr(), dst->GetPitch(), src->GetPitch(), vi.width, vi.height, lutY, lutU, lutV);
      }
      else
      {
        coloryuv_apply_lut_planar(dst->GetWritePtr(), src->GetReadPtr(), dst->GetPitch(), src->GetPitch(), vi.width, vi.height, lutY, bits_per_pixel);
        if (!vi.IsY())
        {
          const int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
          const int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

          coloryuv_apply_lut_planar(dst->GetWritePtr(PLANAR_U), src->GetReadPtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetPitch(PLANAR_U), width, height, lutU, bits_per_pixel);
          coloryuv_apply_lut_planar(dst->GetWritePtr(PLANAR_V), src->GetReadPtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetPitch(PLANAR_V), width, height, lutV, bits_per_pixel);
        }
        if (vi.IsYUVA()) {
          env->BitBlt(dst->GetWritePtr(PLANAR_A), dst->GetPitch(PLANAR_A), src->GetReadPtr(PLANAR_A), src->GetPitch(PLANAR_A), src->GetRowSize(PLANAR_A), src->GetHeight(PLANAR_A));
        }
      }

      if (luts_live[0]) delete[] luts_live[0];
      if (luts_live[1]) delete[] luts_live[1];
      if (luts_live[2]) delete[] luts_live[2];
    } // lut create and use
    else {
      // 32 bit float: expr
      std::string exprY = coloryuv_create_lut_expr(&cY, bits_per_pixel, tweaklike_params);
      std::string exprU = !vi.IsY() ? coloryuv_create_lut_expr(&cU, bits_per_pixel, tweaklike_params) : "";
      std::string exprV = !vi.IsY() ? coloryuv_create_lut_expr(&cV, bits_per_pixel, tweaklike_params) : "";
      std::string exprA = ""; // copy
      // Invoke Expr
      AVSValue child2;
      if (vi.IsY()) {
        AVSValue new_args[2] = { child, exprY.c_str() };
        child2 = env->Invoke("Expr", AVSValue(new_args, 2)).AsClip();
      }
      else if(vi.IsYUV()) {
        AVSValue new_args[4] = { child, exprY.c_str(), exprU.c_str(), exprV.c_str() };
        child2 = env->Invoke("Expr", AVSValue(new_args, 4)).AsClip();
      }
      else if (vi.IsYUVA()) {
        AVSValue new_args[5] = { child, exprY.c_str(), exprU.c_str(), exprV.c_str(), exprA.c_str() };
        child2 = env->Invoke("Expr", AVSValue(new_args, 5)).AsClip();
      }
      dst = child2.AsClip()->GetFrame(n, env);
    }

    if (analyse)
    {
        env->ApplyMessage(&dst, vi, text, vi.width / 4, 0xa0a0a0, 0, 0);
    }

    return dst;
}

AVSValue __cdecl ColorYUV::Create(AVSValue args, void* , IScriptEnvironment* env)
{
    const bool tweaklike_params = args[23].AsBool(false); // f2c = true: for tweak-like parameter interpretation
    const float def = tweaklike_params ? 1.0f : 0.0f;
    return new ColorYUV(args[0].AsClip(),
                        args[1].AsFloat(def),                // gain_y
                        args[2].AsFloat(0.0f),                // off_y      bright
                        args[3].AsFloat(def),                // gamma_y
                        args[4].AsFloat(def),                // cont_y
                        args[5].AsFloat(def),                // gain_u
                        args[6].AsFloat(0.0f),                // off_u      bright
                        args[7].AsFloat(def),                // gamma_u
                        args[8].AsFloat(def),                // cont_u
                        args[9].AsFloat(def),                // gain_v
                        args[10].AsFloat(0.0f),                // off_v
                        args[11].AsFloat(def),                // gamma_v
                        args[12].AsFloat(def),                // cont_v
                        args[13].AsString(""),                // levels = "", "TV->PC", "PC->TV"
                        args[14].AsString(""),                // opt = "", "coring"
//                      args[15].AsString(""),                // matrix = "", "rec.709"
                        args[16].AsBool(false),                // colorbars
                        args[17].AsBool(false),                // analyze
                        args[18].AsBool(false),                // autowhite
                        args[19].AsBool(false),                // autogain
                        args[20].AsBool(false),                // conditional
                        args[21].AsInt(8),                     // bits avs+
                        args[22].AsBool(false),                // showyuv_fullrange avs+
                        tweaklike_params,                      // for gain, gamma, cont: 0.0/0.5/1.0/2.0/3.0 instead of -256/-128/0/256/512
                        env);
}

extern const AVSFunction Color_filters[] = {
    { "ColorYUV", BUILTIN_FUNC_PREFIX,
                  "c[gain_y]f[off_y]f[gamma_y]f[cont_y]f" \
                  "[gain_u]f[off_u]f[gamma_u]f[cont_u]f" \
                  "[gain_v]f[off_v]f[gamma_v]f[cont_v]f" \
                  "[levels]s[opt]s[matrix]s[showyuv]b" \
                  "[analyze]b[autowhite]b[autogain]b[conditional]" \
                  "b[bits]i[showyuv_fullrange]b[f2c]b", // avs+ 20180226- f2c-like parameters
                  ColorYUV::Create},
    { 0 }
};

