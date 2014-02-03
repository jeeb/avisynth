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


static void coloryuv_showyuv(BYTE* pY, BYTE* pU, BYTE* pV, int y_pitch, int u_pitch, int v_pitch, int frame, bool full_range)
{
    const int frame_size = full_range ? 256 : 224;
    const int luma_size = frame_size * 2;
    const int chroma_offset = full_range ? 0 : 16;

    int luma;

    // Calculate luma cycle
    if (full_range)
    {
        luma = frame % 510;
        if (luma > 255)
        {
            luma = 510 - luma;
        }
    }
    else
    {
        luma = frame % 438;
        if (luma > 219)
        {
            luma = 438 - luma;
        }
        luma += 16;
    }

    // Set luma value
    for (int y = 0; y < luma_size; y++)
    {
        memset(pY, luma, luma_size);
        pY += y_pitch;
    }

    // Set chroma
    for (int y = 0; y < frame_size; y++)
    {
        for (int x = 0; x < frame_size; x++) {
            pU[x] = x + chroma_offset;
        }

        memset(pV, y + chroma_offset, frame_size);

        pU += u_pitch;
        pV += v_pitch;
    }
}

static void coloryuv_create_lut(BYTE* lut, const ColorYUVPlaneConfig* config)
{
    const double scale = 256;

    double gain = config->gain / scale + 1.0;
    double contrast = config->contrast / scale + 1.0;
    double gamma = config->gamma / scale + 1.0;
    double offset = config->offset / scale;

    int range = config->range;
    if (range == COLORYUV_RANGE_PC_TVY)
    {
        range = config->plane == PLANAR_Y ? COLORYUV_RANGE_PC_TV : COLORYUV_RANGE_NONE;
    }

    double range_factor = 1.0;

    if (range != COLORYUV_RANGE_NONE)
    {
        if (range == COLORYUV_RANGE_PC_TV)
        {
            if (config->plane == PLANAR_Y)
            {
                range_factor = 219.0 / 255.0;
            }
            else
            {
                range_factor = 224.0 / 255.0;
            }
        }
        else
        {
            if (config->plane == PLANAR_Y)
            {
                range_factor = 255.0 / 219.0;
            }
            else
            {
                range_factor = 255.0 / 224.0;
            }
        }
    }

    for (int i = 0; i < 256; i++) {
        double value = double(i) / 256.0;

        // Applying gain
        value *= gain;

        // Applying contrast
        value = (value - 0.5) * contrast + 0.5;

        // Applying offset
        value += offset;

        // Applying gamma
        if (gamma != 0 && value > 0)
        {
            value = pow(value, 1.0 / gamma);
        }

        value *= 256.0;

        // Range conversion
        if (range == COLORYUV_RANGE_PC_TV)
        {
            value = value*range_factor + 16.0;
        }
        else if (range == COLORYUV_RANGE_TV_PC)
        {
            value = (value - 16.0) * range_factor;
        }

        // Convert back to int
        int iValue = int(value);

        // Clamp
        iValue = clamp(iValue, 0, 255);

        if (config->clip_tv)
        {
            iValue = clamp(iValue, 16, config->plane == PLANAR_Y ? 235 : 240);
        }

        lut[i] = iValue;
    }
}

static void coloryuv_analyse_core(const int* freq, const int pixel_num, ColorYUVPlaneData* data)
{
    const int pixel_256th = pixel_num / 256; // For loose max/min

    double avg = 0.0;
    data->real_min = -1;
    data->real_max = -1;
    data->loose_max = -1;
    data->loose_min = -1;

    int px_min_c = 0, px_max_c = 0;

    for (int i = 0; i < 256; i++)
    {
        avg += freq[i] * i;

        if (freq[i] > 0 && data->real_min == -1)
        {
            data->real_min = i;
        }

        if (data->loose_min == -1)
        {
            px_min_c += freq[i];

            if (px_min_c > pixel_256th)
            {
                data->loose_min = i;
            }
        }

        if (freq[255 - i] > 0 && data->real_max == -1)
        {
            data->real_max = 255 - i;
        }

        if (data->loose_max == -1)
        {
            px_max_c += freq[255 - i];

            if (px_max_c > pixel_256th)
            {
                data->loose_max = 255 - i;
            }
        }
    }

    avg /= pixel_num;
    data->average = avg;
}

static void coloryuv_analyse_planar(const BYTE* pSrc, int src_pitch, int width, int height, ColorYUVPlaneData* data)
{
    int freq[256];
    memset(freq, 0, sizeof(freq));

    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            freq[pSrc[x]]++;
        }

        pSrc += src_pitch;
    }

    coloryuv_analyse_core(freq, width*height, data);
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

    coloryuv_analyse_core(freqY, width*height, dataY);
    coloryuv_analyse_core(freqU, width*height/2, dataU);
    coloryuv_analyse_core(freqV, width*height/2, dataV);
}

static void coloryuv_autogain(const ColorYUVPlaneData* dY, const ColorYUVPlaneData* dU, const ColorYUVPlaneData* dV, ColorYUVPlaneConfig* cY, ColorYUVPlaneConfig* cU, ColorYUVPlaneConfig* cV)
{
    int maxY = min(dY->loose_max, 236);
    int minY = max(dY->loose_min, 16);

    int range = maxY - minY;

    if (range > 0) {
        double scale = 220.0 / range;
        cY->offset = 16.0 - scale*minY;
        cY->gain = 256.0*scale - 256.0;
    }
}

static void coloryuv_autowhite(const ColorYUVPlaneData* dY, const ColorYUVPlaneData* dU, const ColorYUVPlaneData* dV, ColorYUVPlaneConfig* cY, ColorYUVPlaneConfig* cU, ColorYUVPlaneConfig* cV)
{
    cU->offset = 127 - dU->average;
    cV->offset = 127 - dV->average;
}

static void coloryuv_apply_lut_planar(BYTE* pDst, const BYTE* pSrc, int dst_pitch, int src_pitch, int width, int height, const BYTE* lut)
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
            c_##plane##->internal_name = t;               \
            c_##plane##->changed = true;                  \
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
                     bool colorbar, bool analyse, bool autowhite, bool autogain, bool conditional,
                     IScriptEnvironment* env)
 : GenericVideoFilter(child),
   colorbar(colorbar), analyse(analyse), autowhite(autowhite), autogain(autogain), conditional(conditional)
{
    if (!vi.IsYUV())
    {
        env->ThrowError("ColorYUV: Only work with YUV colorspace.");
    }

    configY.gain = gain_y;
    configY.offset = offset_y;
    configY.gamma = gamma_y;
    configY.contrast = contrast_y;
    configY.changed = false;
    configY.clip_tv = false;
    configY.plane = PLANAR_Y;

    configU.gain = gain_u;
    configU.offset = offset_u;
    configU.gamma = gamma_u;
    configU.contrast = contrast_u;
    configU.changed = false;
    configU.clip_tv = false;
    configU.plane = PLANAR_U;

    configV.gain = gain_v;
    configV.offset = offset_v;
    configV.gamma = gamma_v;
    configV.contrast = contrast_v;
    configV.changed = false;
    configV.clip_tv = false;
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
    else if (lstrcmpi(level, "") != 0)
    {
        env->ThrowError("ColorYUV: invalid parameter : levels");
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

    if (colorbar)
    {
        vi.width = 224 * 2;
        vi.height = 224 * 2;
        vi.pixel_type = VideoInfo::CS_YV12;
    }
}

PVideoFrame __stdcall ColorYUV::GetFrame(int n, IScriptEnvironment* env)
{
    if (colorbar)
    {
        PVideoFrame dst = env->NewVideoFrame(vi);
        coloryuv_showyuv(dst->GetWritePtr(), dst->GetWritePtr(PLANAR_U), dst->GetWritePtr(PLANAR_V), dst->GetPitch(), dst->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_V), n, false);
        return dst;
    }

    PVideoFrame src = child->GetFrame(n, env);
    PVideoFrame dst = env->NewVideoFrame(vi);

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
            coloryuv_analyse_planar(src->GetReadPtr(), src->GetPitch(), vi.width, vi.height, &dY);
            if (!vi.IsY8())
            {
                const int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
                const int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

                coloryuv_analyse_planar(src->GetReadPtr(PLANAR_U), src->GetPitch(PLANAR_U), width, height, &dU);
                coloryuv_analyse_planar(src->GetReadPtr(PLANAR_V), src->GetPitch(PLANAR_V), width, height, &dV);
            }
        }

        if (analyse)
        {
            if (!vi.IsY8())
            {
                sprintf(text,
                        "        Frame: %-8u ( Luma Y / ChromaU / ChromaV )\n"
                        "      Average:      ( %7.2f / %7.2f / %7.2f )\n"
                        "      Minimum:      (   %3d   /   %3d   /   %3d    )\n"
                        "      Maximum:      (   %3d   /   %3d   /   %3d    )\n"
                        "Loose Minimum:      (   %3d   /   %3d   /   %3d    )\n"
                        "Loose Maximum:      (   %3d   /   %3d   /   %3d    )\n",
                        n,
                        dY.average, dU.average, dV.average,
                        dY.real_min, dU.real_min, dV.real_min,
                        dY.real_max, dU.real_max, dV.real_max,
                        dY.loose_min, dU.loose_min, dV.loose_min,
                        dY.loose_max, dU.loose_max, dV.loose_max
                        );
            } 
            else
            {
                sprintf(text,
                        "        Frame: %-8u\n"
                        "      Average: %7.2f\n"
                        "      Minimum: %3d\n"
                        "      Maximum: %3d\n"
                        "Loose Minimum: %3d\n"
                        "Loose Maximum: %3d\n",
                        n,
                        dY.average,
                        dY.real_min,
                        dY.real_max,
                        dY.loose_min,
                        dY.loose_max
                        );
            }
        }

        if (autowhite && !vi.IsY8())
        {
            coloryuv_autowhite(&dY, &dU, &dV, &cY, &cU, &cV);
        }

        if (autogain)
        {
            coloryuv_autogain(&dY, &dU, &dV, &cY, &cU, &cV);
        }
    }

    // Read conditional variables
    coloryuv_read_conditional(env, &cY, &cU, &cV);

    BYTE *lutY, *lutU, *lutV;

    // FIXME: Should I use env2->Allocate?
    lutY = static_cast<BYTE*>(alloca(256));
    lutU = static_cast<BYTE*>(alloca(256));
    lutV = static_cast<BYTE*>(alloca(256));

    coloryuv_create_lut(lutY, &cY);
    if (!vi.IsY8())
    {
        coloryuv_create_lut(lutU, &cU);
        coloryuv_create_lut(lutV, &cV);
    }

    if (vi.IsYUY2())
    {
        coloryuv_apply_lut_yuy2(dst->GetWritePtr(), src->GetReadPtr(), dst->GetPitch(), src->GetPitch(), vi.width, vi.height, lutY, lutU, lutV);
    }
    else
    {
        coloryuv_apply_lut_planar(dst->GetWritePtr(), src->GetReadPtr(), dst->GetPitch(), src->GetPitch(), vi.width, vi.height, lutY);
        if (!vi.IsY8())
        {
            const int width = vi.width >> vi.GetPlaneWidthSubsampling(PLANAR_U);
            const int height = vi.height >> vi.GetPlaneHeightSubsampling(PLANAR_U);

            coloryuv_apply_lut_planar(dst->GetWritePtr(PLANAR_U), src->GetReadPtr(PLANAR_U), dst->GetPitch(PLANAR_U), src->GetPitch(PLANAR_U), width, height, lutU);
            coloryuv_apply_lut_planar(dst->GetWritePtr(PLANAR_V), src->GetReadPtr(PLANAR_V), dst->GetPitch(PLANAR_V), src->GetPitch(PLANAR_V), width, height, lutV);
        }
    }

    if (analyse)
    {
        env->ApplyMessage(&dst, vi, text, vi.width / 4, 0xa0a0a0, 0, 0);
    }

    return dst;
}

AVSValue __cdecl ColorYUV::Create(AVSValue args, void*, IScriptEnvironment* env)
{
    return new ColorYUV(args[0].AsClip(),
                        args[1].AsFloat(0.0f),                // gain_y
                        args[2].AsFloat(0.0f),                // off_y      bright
                        args[3].AsFloat(0.0f),                // gamma_y
                        args[4].AsFloat(0.0f),                // cont_y
                        args[5].AsFloat(0.0f),                // gain_u
                        args[6].AsFloat(0.0f),                // off_u      bright
                        args[7].AsFloat(0.0f),                // gamma_u
                        args[8].AsFloat(0.0f),                // cont_u
                        args[9].AsFloat(0.0f),                // gain_v
                        args[10].AsFloat(0.0f),                // off_v
                        args[11].AsFloat(0.0f),                // gamma_v
                        args[12].AsFloat(0.0f),                // cont_v
                        args[13].AsString(""),                // levels = "", "TV->PC", "PC->TV"
                        args[14].AsString(""),                // opt = "", "coring"
//                      args[15].AsString(""),                // matrix = "", "rec.709"
                        args[16].AsBool(false),                // colorbars
                        args[17].AsBool(false),                // analyze
                        args[18].AsBool(false),                // autowhite
                        args[19].AsBool(false),                // autogain
                        args[20].AsBool(false),                // conditional
                        env);
}

extern const AVSFunction Color_filters[] = {
    { "ColorYUV", "c[gain_y]f[off_y]f[gamma_y]f[cont_y]f" \
                  "[gain_u]f[off_u]f[gamma_u]f[cont_u]f" \
                  "[gain_v]f[off_v]f[gamma_v]f[cont_v]f" \
                  "[levels]s[opt]s[matrix]s[showyuv]b" \
                  "[analyze]b[autowhite]b[autogain]b[conditional]b",
                  ColorYUV::Create },
    { 0 }
};

