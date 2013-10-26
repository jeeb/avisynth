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


#include "greyscale.h"
#include <avs/win.h>
#include <cstdlib>
#include "../core/internal.h"
#include <emmintrin.h>
#include "avs/alignment.h"
#include "avs/minmax.h"


/*************************************
 *******   Convert to Greyscale ******
 ************************************/

extern const AVSFunction Greyscale_filters[] = {
  { "Greyscale", "c[matrix]s", Greyscale::Create },       // matrix can be "rec601", "rec709" or "Average"
  { "Grayscale", "c[matrix]s", Greyscale::Create },
  { 0 }
};

Greyscale::Greyscale(PClip _child, const char* matrix, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  theMatrix = Rec601;
  if (matrix) {
    if (!vi.IsRGB())
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (RGB data only)");
    if (!lstrcmpi(matrix, "rec709"))
      theMatrix = Rec709;
    else if (!lstrcmpi(matrix, "Average"))
      theMatrix = Average;
    else if (!lstrcmpi(matrix, "rec601"))
      theMatrix = Rec601;
    else
      env->ThrowError("GreyScale: invalid \"matrix\" parameter (must be matrix=\"Rec601\", \"Rec709\" or \"Average\")");
  }
}

//this is not really faster than MMX but a lot cleaner
static void greyscale_yuy2_sse2(BYTE *srcp, size_t /*width*/, size_t height, size_t pitch) {
  __m128i luma_mask = _mm_set1_epi16(0x00FF);
#pragma warning(disable: 4309)
  __m128i chroma_value = _mm_set1_epi16(0x8000);
#pragma warning(default: 4309)
  BYTE* end_point = srcp + pitch * height;

  while(srcp < end_point) {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp));
    src = _mm_and_si128(src, luma_mask);
    src = _mm_or_si128(src, chroma_value);
    _mm_store_si128(reinterpret_cast<__m128i*>(srcp), src);

    srcp += 16;
  }
}

static void greyscale_rgb32_sse2(BYTE *srcp, size_t /*width*/, size_t height, size_t pitch, int cyb, int cyg, int cyr) {
  __m128i matrix = _mm_set_epi16(0, cyr, cyg, cyb, 0, cyr, cyg, cyb);
  __m128i zero = _mm_setzero_si128();
  __m128i round_mask = _mm_set1_epi32(16384);
  __m128i alpha_mask = _mm_set1_epi32(0xFF000000);

  BYTE* end_point = srcp + pitch * height;

  while(srcp < end_point) { 
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp));
    __m128i alpha = _mm_and_si128(src, alpha_mask);
    __m128i pixel01 = _mm_unpacklo_epi8(src, zero);
    __m128i pixel23 = _mm_unpackhi_epi8(src, zero);

    pixel01 = _mm_madd_epi16(pixel01, matrix);
    pixel23 = _mm_madd_epi16(pixel23, matrix);

    __m128i tmp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01), _mm_castsi128_ps(pixel23), _MM_SHUFFLE(3, 1, 3, 1))); // r3*cyr | r2*cyr | r1*cyr | r0*cyr
    __m128i tmp2 = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(pixel01), _mm_castsi128_ps(pixel23), _MM_SHUFFLE(2, 0, 2, 0)));

    tmp = _mm_add_epi32(tmp, tmp2); 
    tmp = _mm_add_epi32(tmp, round_mask); 
    tmp = _mm_srli_epi32(tmp, 15); // 0 0 0 p3 | 0 0 0 p2 | 0 0 0 p1 | 0 0 0 p0

    //todo: pshufb?
    __m128i result = _mm_or_si128(tmp, _mm_slli_si128(tmp, 1)); 
    result = _mm_or_si128(result, _mm_slli_si128(tmp, 2));
    result = _mm_or_si128(alpha, result);

    _mm_store_si128(reinterpret_cast<__m128i*>(srcp), result);

    srcp += 16;
  }
}


#ifdef X86_32
static void greyscale_yuy2_mmx(BYTE *srcp, size_t width, size_t height, size_t pitch) {
  bool not_mod8 = false;
  size_t loop_limit = min((pitch / 8) * 8, ((width*4 + 7) / 8) * 8);

  __m64 luma_mask = _mm_set1_pi16(0x00FF);
#pragma warning(disable: 4309)
  __m64 chroma_value = _mm_set1_pi16(0x8000);
#pragma warning(default: 4309)

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
     __m64 src = *reinterpret_cast<const __m64*>(srcp+x);
     src = _mm_and_si64(src, luma_mask);
     src = _mm_or_si64(src, chroma_value);
     *reinterpret_cast<__m64*>(srcp+x) = src;
    }

    if (loop_limit < width) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+width-8);
      src = _mm_and_si64(src, luma_mask);
      src = _mm_or_si64(src, chroma_value);
      *reinterpret_cast<__m64*>(srcp+width-8) = src;
    }

    srcp += pitch;
  }
 _mm_empty();
}

static __forceinline __m64 greyscale_rgb32_core_mmx(__m64 &src, __m64 &alpha_mask, __m64 &zero, __m64 &matrix, __m64 &round_mask) {
  __m64 alpha = _mm_and_si64(src, alpha_mask);
  __m64 pixel0 = _mm_unpacklo_pi8(src, zero); 
  __m64 pixel1 = _mm_unpackhi_pi8(src, zero);

  pixel0 = _mm_madd_pi16(pixel0, matrix); //a0*0 + r0*cyr | g0*cyg + b0*cyb
  pixel1 = _mm_madd_pi16(pixel1, matrix); //a1*0 + r1*cyr | g1*cyg + b1*cyb

  __m64 tmp = _mm_unpackhi_pi32(pixel0, pixel1); // r1*cyr | r0*cyr
  __m64 tmp2 = _mm_unpacklo_pi32(pixel0, pixel1); // g1*cyg + b1*cyb | g0*cyg + b0*cyb

  tmp = _mm_add_pi32(tmp, tmp2); // r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
  tmp = _mm_add_pi32(tmp, round_mask); // r1*cyr + g1*cyg + b1*cyb + 32768 | r0*cyr + g0*cyg + b0*cyb + 32768
  tmp = _mm_srli_pi32(tmp, 15); // 0 0 0 p2 | 0 0 0 p1

  __m64 shifted = _mm_slli_si64(tmp, 8);
  tmp = _mm_or_si64(tmp, shifted); // 0 0 p2 p2 | 0 0 p1 p1
  tmp = _mm_or_si64(tmp, _mm_slli_si64(shifted, 8)); // 0 p2 p2 p2 | 0 p1 p1 p1
  return _mm_or_si64(tmp, alpha);
}

static void greyscale_rgb32_mmx(BYTE *srcp, size_t width, size_t height, size_t pitch, int cyb, int cyg, int cyr) {
  __m64 matrix = _mm_set_pi16(0, cyr, cyg, cyb);
  __m64 zero = _mm_setzero_si64();
  __m64 round_mask = _mm_set1_pi32(16384);
  __m64 alpha_mask = _mm_set1_pi32(0xFF000000);

  size_t loop_limit = min((pitch / 8) * 8, ((width*4 + 7) / 8) * 8);

  for (size_t y = 0; y < height; ++y) {
    for (size_t x = 0; x < loop_limit; x+=8) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+x); //pixels 0 and 1
      __m64 result = greyscale_rgb32_core_mmx(src, alpha_mask, zero, matrix, round_mask);

      *reinterpret_cast<__m64*>(srcp+x) = result;
    }

    if (loop_limit < width) {
      __m64 src = *reinterpret_cast<const __m64*>(srcp+width-8); //pixels 0 and 1
      __m64 result = greyscale_rgb32_core_mmx(src, alpha_mask, zero, matrix, round_mask);

      *reinterpret_cast<__m64*>(srcp+width-8) = result;
    }

    srcp += pitch;
  }
  _mm_empty();
}
#endif


PVideoFrame Greyscale::GetFrame(int n, IScriptEnvironment* env)
{
  PVideoFrame frame = child->GetFrame(n, env);
  if (vi.IsY8())
    return frame;

  env->MakeWritable(&frame);
  BYTE* srcp = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  int height = vi.height;
  int width = vi.width;

  if (vi.IsPlanar()) {
    pitch = frame->GetPitch(PLANAR_U)/4;
    int *srcpUV = (int*)frame->GetWritePtr(PLANAR_U);
    width = frame->GetRowSize(PLANAR_U_ALIGNED)/4;
    height = frame->GetHeight(PLANAR_U);
    for (int y=0; y<height; y++) {
      for (int x=0; x<width; x++) {
        srcpUV[x] = 0x80808080;  // mod 8
      }
      srcpUV += pitch;
    }
    pitch = frame->GetPitch(PLANAR_V)/4;
    srcpUV = (int*)frame->GetWritePtr(PLANAR_V);
    width = frame->GetRowSize(PLANAR_V_ALIGNED)/4;
    height = frame->GetHeight(PLANAR_V);
    for (int y=0; y<height; ++y) {
      for (int x=0; x<width; x++) {
        srcpUV[x] = 0x80808080;  // mod 8
      }
      srcpUV += pitch;
    }

    return frame;
  }

  if (vi.IsYUY2()) {
    if ((env->GetCPUFlags() & CPUF_SSE2) && width > 4 && IsPtrAligned(srcp, 16)) {
      greyscale_yuy2_sse2(srcp, width, height, pitch);
    } else
#ifdef X86_32
    if ((env->GetCPUFlags() & CPUF_MMX) && width > 2) {
      greyscale_yuy2_mmx(srcp, width, height, pitch);
    } else
#endif
    {
      for (int y=0; y<height; ++y)
      {
        for (int x=0; x<width; x++)
          srcp[x*2+1] = 128;
        srcp += pitch;
      }
    }

    return frame;
  } 

  if (vi.IsRGB32()) {
    const int cyav = int(0.33333*32768+0.5);

    const int cyb = int(0.114*32768+0.5);
    const int cyg = int(0.587*32768+0.5);
    const int cyr = int(0.299*32768+0.5);

    const int cyb709 = int(0.0722*32768+0.5);
    const int cyg709 = int(0.7152*32768+0.5);
    const int cyr709 = int(0.2126*32768+0.5);

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      if (theMatrix == Rec709)
        greyscale_rgb32_sse2(srcp, width, height, pitch, cyb709, cyg709, cyr709);
      else if (theMatrix == Average)
        greyscale_rgb32_sse2(srcp, width, height, pitch, cyav, cyav, cyav);
      else
        greyscale_rgb32_sse2(srcp, width, height, pitch, cyb, cyg, cyr);
      return frame;
    } 
#ifdef X86_32
    else if (env->GetCPUFlags() & CPUF_MMX) {
      if (theMatrix == Rec709)
        greyscale_rgb32_mmx(srcp, width, height, pitch, cyb709, cyg709, cyr709);
      else if (theMatrix == Average)
        greyscale_rgb32_mmx(srcp, width, height, pitch, cyav, cyav, cyav);
      else
        greyscale_rgb32_mmx(srcp, width, height, pitch, cyb, cyg, cyr);
      return frame;
    }
#endif
  }

  if (vi.IsRGB())
  {  // RGB C.
    BYTE* p_count = srcp;

    const int rgb_inc = vi.IsRGB32() ? 4 : 3;
    if (theMatrix == Rec709) {
      //	  const int cyb709 = int(0.0722*65536+0.5); //  4732
      //	  const int cyg709 = int(0.7152*65536+0.5); // 46871
      //	  const int cyr709 = int(0.2126*65536+0.5); // 13933

      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<vi.width; x++) {
          int greyscale=((srcp[0]*4732)+(srcp[1]*46871)+(srcp[2]*13933)+32768)>>16; // This is the correct brigtness calculations (standardized in Rec. 709)
          srcp[0]=srcp[1]=srcp[2]=greyscale;
          srcp += rgb_inc;
        }
        p_count+=pitch;
        srcp=p_count;
      }
    }
    else if (theMatrix == Average) {
      //	  const int cyav = int(0.333333*65536+0.5); //  21845

      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<vi.width; x++) {
          int greyscale=((srcp[0]+srcp[1]+srcp[2])*21845+32768)>>16; // This is the average of R, G & B
          srcp[0]=srcp[1]=srcp[2]=greyscale;
          srcp += rgb_inc;
        }
        p_count+=pitch;
        srcp=p_count;
      }
    }
    else {
      //	  const int cyb = int(0.114*65536+0.5); //  7471
      //	  const int cyg = int(0.587*65536+0.5); // 38470
      //	  const int cyr = int(0.299*65536+0.5); // 19595

      for (int y=0; y<vi.height; ++y) {
        for (int x=0; x<vi.width; x++) {
          int greyscale=((srcp[0]*7471)+(srcp[1]*38470)+(srcp[2]*19595)+32768)>>16; // This produces similar results as YUY2 (luma calculation)
          srcp[0]=srcp[1]=srcp[2]=greyscale;
          srcp += rgb_inc;
        }
        p_count+=pitch;
        srcp=p_count;
      }
    }
  }
  return frame;
}


AVSValue __cdecl Greyscale::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  PClip clip = args[0].AsClip();
  const VideoInfo& vi = clip->GetVideoInfo();

  if (vi.IsY8())
    return clip;

  return new Greyscale(clip, args[1].AsString(0), env);
}
