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

// ConvertNativeToStacked, ConvertStackedToNative 2016 by pinterf

#include <avisynth.h>
#include <avs/alignment.h>
#include <cstdint>
#include <emmintrin.h>

class ConvertToStacked : public GenericVideoFilter
{
public:

    ConvertToStacked(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src)
    {

        if (vi.IsColorSpace(VideoInfo::CS_YUV420P16)) vi.pixel_type = VideoInfo::CS_YV12;
        else if (vi.IsColorSpace(VideoInfo::CS_YUV422P16)) vi.pixel_type = VideoInfo::CS_YV16;
        else if (vi.IsColorSpace(VideoInfo::CS_YUV444P16)) vi.pixel_type = VideoInfo::CS_YV24;
        else if (vi.IsColorSpace(VideoInfo::CS_Y16)) vi.pixel_type = VideoInfo::CS_Y8;
        else env->ThrowError("ConvertNativeToStacked: Input clip must be native 16 bit: YUV420P16, YUV422P16, YUV444P16, Y16");

        vi.height = vi.height << 1; // * 2 stacked
                                    // back from native 16 bit to stacked 8 bit

        return;
    }

    PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment* env)
    {
        PVideoFrame src = child->GetFrame(n, env);

        PVideoFrame dst = env->NewVideoFrame(vi);

        const int planes[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
        const int plane_count = vi.IsY8() ? 1 : 3; // checking the stacked 8 bit format constants
        for (int p = 0; p < plane_count; ++p) {
            const int plane = planes[p];
            uint16_t* srcp = (uint16_t*)src->GetReadPtr(plane);
            uint8_t* msb = dst->GetWritePtr(plane);
            const int src_pitch = src->GetPitch(plane); // in bytes
            const int dst_pitch = dst->GetPitch(plane);
            const int height = (vi.height >> vi.GetPlaneHeightSubsampling(plane)) / 2; // non-stacked real height
            const int width = vi.width >> vi.GetPlaneWidthSubsampling(plane);
            uint8_t* lsb = msb + dst_pitch*height;

            bool use_sse2 = (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(msb, 16) && IsPtrAligned(lsb, 16) && IsPtrAligned(srcp, 16);

            if (use_sse2)
            {
                int wMod16 = (width / 16) * 16;

                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < wMod16; x += 16) {
                        __m128i data16_1, data16_2;
                        __m128i masklo = _mm_set1_epi16(0x00FF);
                        // no gain when sse4.1 _mm_stream_load_si128 is used
                        data16_1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x));       // 16 bytes, 8 words ABCDEFGH
                        data16_2 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp + x + 8));   // 16 bytes, 8 words
                        _mm_stream_si128(reinterpret_cast<__m128i*>(msb + x), _mm_packus_epi16(_mm_srli_epi16(data16_1, 8), _mm_srli_epi16(data16_2, 8))); // ABCDEFGH Hi
                        _mm_stream_si128(reinterpret_cast<__m128i*>(lsb + x), _mm_packus_epi16(_mm_and_si128(data16_1, masklo), _mm_and_si128(data16_2, masklo))); // ABCDEFGH Lo
                    }
                    // the rest
                    for (int x = wMod16; x < width; ++x) {
                        const uint16_t out = srcp[x];
                        msb[x] = out >> 8;
                        lsb[x] = (uint8_t)out;
                    }

                    srcp += src_pitch / sizeof(uint16_t);
                    msb += dst_pitch;
                    lsb += dst_pitch;
                } // y
            }
            else
            {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        const uint16_t out = srcp[x];
                        msb[x] = out >> 8;
                        lsb[x] = (uint8_t)out;
                    }

                    srcp += src_pitch / sizeof(uint16_t);
                    msb += dst_pitch;
                    lsb += dst_pitch;
                }
            }
        }
        return dst;
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }

    static AVSValue __cdecl ConvertToStacked::Create(AVSValue args, void*, IScriptEnvironment* env)
    {
        PClip clip = args[0].AsClip();
        /*if (clip->GetVideoInfo().IsY8())
        return clip;*/
        return new ConvertToStacked(clip, env);
    }
};


class ConvertFromStacked : public GenericVideoFilter
{
public:

    ConvertFromStacked::ConvertFromStacked(PClip src, IScriptEnvironment* env) : GenericVideoFilter(src)
    {
        if (vi.IsYV12()) vi.pixel_type = VideoInfo::CS_YUV420P16;
        else if (vi.IsYV16()) vi.pixel_type = VideoInfo::CS_YUV422P16;
        else if (vi.IsYV24()) vi.pixel_type = VideoInfo::CS_YUV444P16;
        else if (vi.IsY8()) vi.pixel_type = VideoInfo::CS_Y16;
        else env->ThrowError("ConvertStackedToNative: Input stacked clip must be YV12, YV16, YV24 or Y8");

        vi.height = vi.height >> 1; // div 2 non stacked

        return;
    }

    PVideoFrame __stdcall ConvertFromStacked::GetFrame(int n, IScriptEnvironment* env)
    {
        PVideoFrame src = child->GetFrame(n, env);

        PVideoFrame dst = env->NewVideoFrame(vi);

        const int planes[] = { PLANAR_Y, PLANAR_U, PLANAR_V };
        const int plane_count = vi.IsColorSpace(VideoInfo::CS_Y16) ? 1 : 3;
        for (int p = 0; p < plane_count; ++p) {
            const int plane = planes[p];
            uint8_t* msb = const_cast<uint8_t *>(src->GetReadPtr(plane));
            uint16_t* dstp = (uint16_t*)dst->GetWritePtr(plane);
            const int src_pitch = src->GetPitch(plane);
            const int dst_pitch = dst->GetPitch(plane);
            const int height = (vi.height >> vi.GetPlaneHeightSubsampling(plane)); // real non-stacked height
            const int width = vi.width >> vi.GetPlaneWidthSubsampling(plane);
            uint8_t* lsb = msb + src_pitch*height;

            bool use_sse2 = (env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(msb, 16) && IsPtrAligned(lsb, 16) && IsPtrAligned(dstp, 16);

            if (use_sse2)
            {
                // sse2
                int wMod16 = (width / 16) * 16;

                // pf my very first intrinsic, hey
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < wMod16; x += 16) {
                        __m128i data_hi, data_lo;
                        data_hi = _mm_load_si128(reinterpret_cast<const __m128i*>(msb + x)); // 16 bytes
                        data_lo = _mm_load_si128(reinterpret_cast<const __m128i*>(lsb + x));
                        // Interleaves the lower 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
                        _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x), _mm_unpacklo_epi8(data_lo, data_hi));
                        // Interleaves the higher 8 signed or unsigned 8-bit integers in a with the lower 8 signed or unsigned 8-bit integers in b.
                        _mm_stream_si128(reinterpret_cast<__m128i*>(dstp + x + 8), _mm_unpackhi_epi8(data_lo, data_hi));
                    }
                    // remaining, 
                    for (int x = wMod16; x < width; ++x) {
                        dstp[x] = msb[x] << 8 | lsb[x];
                    } // x
                    msb += src_pitch;
                    lsb += src_pitch;
                    dstp += dst_pitch / sizeof(uint16_t); // uint16_t *
                }
            }
            else {
                for (int y = 0; y < height; ++y) {
                    for (int x = 0; x < width; ++x) {
                        dstp[x] = msb[x] << 8 | lsb[x];
                    }
                    msb += src_pitch;
                    lsb += src_pitch;
                    dstp += dst_pitch / sizeof(uint16_t); // uint16_t *
                }
            }
        }
        return dst;
    }

    int __stdcall SetCacheHints(int cachehints, int frame_range) override
    {
        return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
    }


    static AVSValue __cdecl ConvertFromStacked::Create(AVSValue args, void*, IScriptEnvironment* env)
    {
        PClip clip = args[0].AsClip();
        return new ConvertFromStacked(clip, env);
    }
};


static const AVS_Linkage * AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
    AVS_linkage = vectors;

    env->AddFunction("ConvertFromStacked", "c", ConvertFromStacked::Create, 0);
    env->AddFunction("ConvertToStacked", "c", ConvertToStacked::Create, 0);

    return "`ConvertStacked' Stacked format conversion for 16-bit formats.";
}