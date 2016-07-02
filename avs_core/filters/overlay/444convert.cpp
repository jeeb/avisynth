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

// Overlay (c) 2003, 2004 by Klaus Post

#include "444convert.h"
#include "../../core/internal.h"
#include <emmintrin.h>
#include <avs/alignment.h>


//this isn't really faster than mmx
static void convert_yv12_chroma_to_yv24_sse2(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int src_width, int src_height) {
  int mod8_width = src_width / 8 * 8;
  for (int y = 0; y < src_height; ++y) {
    for (int x = 0; x < mod8_width; x+=8) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x)); //0 0 0 0 0 0 0 0 U8 U7 U6 U5 U4 U3 U2 U1 U0
      src = _mm_unpacklo_epi8(src, src); //U8 U8 U7 U7 U6 U6 U5 U5 U4 U4 U3 U3 U2 U2 U1 U1 U0 U0

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*2), src);
      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x*2 + dst_pitch), src);
    }

    if (mod8_width != src_width) {
      __m128i src = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+src_width - 8)); //0 0 0 0 0 0 0 0 U8 U7 U6 U5 U4 U3 U2 U1 U0
      src = _mm_unpacklo_epi8(src, src); //U8 U8 U7 U7 U6 U6 U5 U5 U4 U4 U3 U3 U2 U2 U1 U1 U0 U0

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + (src_width * 2) - 16), src);
      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp + (src_width * 2) - 16 + dst_pitch), src);
    }

    dstp += dst_pitch*2;
    srcp += src_pitch;
  }
}


#ifdef X86_32
/***** YV12 -> YUV 4:4:4   ******/

static void convert_yv12_chroma_to_yv24_mmx(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int src_width, int src_height) {
  int mod4_width = src_width / 4 * 4;
  for (int y = 0; y < src_height; ++y) {
    for (int x = 0; x < mod4_width; x+=4) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp+x)); //0 0 0 0 U3 U2 U1 U0
      src = _mm_unpacklo_pi8(src, src); //U3 U3 U2 U2 U1 U1 U0 U0

      *reinterpret_cast<__m64*>(dstp+x*2) = src;
      *reinterpret_cast<__m64*>(dstp+x*2 + dst_pitch) = src;
    }

    if (mod4_width != src_width) {
      __m64 src = _mm_cvtsi32_si64(*reinterpret_cast<const int*>(srcp-4)); //0 0 0 0 U3 U2 U1 U0
      src = _mm_unpacklo_pi8(src, src); //U3 U3 U2 U2 U1 U1 U0 U0

      *reinterpret_cast<__m64*>(dstp + (src_width * 2) - 8) = src;
      *reinterpret_cast<__m64*>(dstp + (src_width * 2) - 8 + dst_pitch) = src;
    }

    dstp += dst_pitch*2;
    srcp += src_pitch;
  }
  _mm_empty();
}

#endif // X86_32


static void convert_yv12_chroma_to_yv24_c(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int src_width, int src_height) {
  for (int y = 0; y < src_height; ++y) {
    for (int x = 0; x < src_width; ++x) {
      dstp[x*2]             = srcp[x];
      dstp[x*2+1]           = srcp[x];
      dstp[x*2+dst_pitch]   = srcp[x];
      dstp[x*2+dst_pitch+1] = srcp[x];
    }
    dstp += dst_pitch*2;
    srcp += src_pitch;
  }
}

void Convert444FromYV12::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env)
{

  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch, src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());

  const BYTE* srcU = src->GetReadPtr(PLANAR_U);
  const BYTE* srcV = src->GetReadPtr(PLANAR_V);

  int srcUVpitch = src->GetPitch(PLANAR_U);

  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstUVpitch = dst->pitch;

  int width = src->GetRowSize(PLANAR_U);
  int height = src->GetHeight(PLANAR_U);
  
  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(dstU, 16) && IsPtrAligned(dstV, 16))  
  {
    convert_yv12_chroma_to_yv24_sse2(dstU, srcU, dstUVpitch, srcUVpitch, width, height);
    convert_yv12_chroma_to_yv24_sse2(dstV, srcV, dstUVpitch, srcUVpitch, width, height);
  }
  else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_MMX) 
  {
    convert_yv12_chroma_to_yv24_mmx(dstU, srcU, dstUVpitch, srcUVpitch, width, height);
    convert_yv12_chroma_to_yv24_mmx(dstV, srcV, dstUVpitch, srcUVpitch, width, height);
  } 
  else 
#endif
  {
    convert_yv12_chroma_to_yv24_c(dstU, srcU, dstUVpitch, srcUVpitch, width, height);
    convert_yv12_chroma_to_yv24_c(dstV, srcV, dstUVpitch, srcUVpitch, width, height);
  }
}

void Convert444FromYV12::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight());
}


void Convert444FromYV24::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetPtr(PLANAR_U), dst->pitch,
    src->GetReadPtr(PLANAR_U),src->GetPitch(PLANAR_U), src->GetRowSize(PLANAR_U), src->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetPtr(PLANAR_V), dst->pitch,
    src->GetReadPtr(PLANAR_V),src->GetPitch(PLANAR_V), src->GetRowSize(PLANAR_V), src->GetHeight(PLANAR_V));
}

void Convert444FromYV24::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

void Convert444FromY8::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
  memset((void *)dst->GetPtr(PLANAR_U), 0x80, dst->pitch * dst->h());
  memset((void *)dst->GetPtr(PLANAR_V), 0x80, dst->pitch * dst->h());
}

void Convert444FromY8::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  env->BitBlt(dst->GetPtr(PLANAR_Y), dst->pitch,
    src->GetReadPtr(PLANAR_Y),src->GetPitch(PLANAR_Y), src->GetRowSize(PLANAR_Y), src->GetHeight(PLANAR_Y));
}

/***** YUY2 -> YUV 4:4:4   ******/


void Convert444FromYUY2::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstU[x]   = dstU[x+1] = srcP[x2+1];
      dstV[x]   = dstV[x+1] = srcP[x2+3];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}


void Convert444FromYUY2::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstY[x]   = srcP[x2];
      dstY[x+1] = srcP[x2+2];
    }
    srcP+=srcPitch;
    dstY+=dstPitch;
  }
}

/****** YUV 4:4:4 -> YUV 4:4:4   - Perhaps the easiest job in the world ;)  *****/
PVideoFrame Convert444ToYV24::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight(PLANAR_Y));
  env->BitBlt(dst->GetWritePtr(PLANAR_U), dst->GetPitch(PLANAR_U),
    src->GetPtr(PLANAR_U), src->pitch, dst->GetRowSize(PLANAR_U), dst->GetHeight(PLANAR_U));
  env->BitBlt(dst->GetWritePtr(PLANAR_V), dst->GetPitch(PLANAR_V),
    src->GetPtr(PLANAR_V), src->pitch, dst->GetRowSize(PLANAR_V), dst->GetHeight(PLANAR_V));
  return dst;
}

PVideoFrame Convert444ToY8::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight());
  return dst;
}

static __forceinline __m128i convert_yv24_chroma_block_to_yv12_sse2(const __m128i &src_line0_p0, const __m128i &src_line1_p0, const __m128i &src_line0_p1, const __m128i &src_line1_p1, const __m128i &ffff, const __m128i &mask) {
  __m128i avg1 = _mm_avg_epu8(src_line0_p0, src_line1_p0);
  __m128i avg2 = _mm_avg_epu8(src_line0_p1, src_line1_p1);

  __m128i avg1x = _mm_xor_si128(avg1, ffff);
  __m128i avg2x = _mm_xor_si128(avg2, ffff);

  __m128i avg1_sh = _mm_srli_epi16(avg1x, 8);
  __m128i avg2_sh = _mm_srli_epi16(avg2x, 8);

  avg1 = _mm_avg_epu8(avg1x, avg1_sh);
  avg2 = _mm_avg_epu8(avg2x, avg2_sh);

  avg1 = _mm_and_si128(avg1, mask);
  avg2 = _mm_and_si128(avg2, mask);

  __m128i packed = _mm_packus_epi16(avg1, avg2);
  return _mm_xor_si128(packed, ffff);
}

static void convert_yv24_chroma_to_yv12_sse2(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int dst_width, const int dst_height) {
  int mod16_width = dst_width / 16 * 16;

#pragma warning(push)
#pragma warning(disable:4309)
  __m128i ffff = _mm_set1_epi8(0xFF);
#pragma warning(pop)
  __m128i mask = _mm_set1_epi16(0x00FF);

  for (int y = 0; y < dst_height; ++y) {
    for (int x = 0; x < mod16_width; x+=16) {
      __m128i src_line0_p0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2));
      __m128i src_line0_p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+16));
      __m128i src_line1_p0 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+src_pitch));
      __m128i src_line1_p1 = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*2+src_pitch+16));

      __m128i avg = convert_yv24_chroma_block_to_yv12_sse2(src_line0_p0, src_line1_p0, src_line0_p1, src_line1_p1, ffff, mask);

      _mm_store_si128(reinterpret_cast<__m128i*>(dstp+x), avg);
    }

    if (mod16_width != dst_width) {
      __m128i src_line0_p0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+dst_width*2-32));
      __m128i src_line0_p1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+dst_width*2-16));
      __m128i src_line1_p0 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+dst_width*2+src_pitch-32));
      __m128i src_line1_p1 = _mm_loadu_si128(reinterpret_cast<const __m128i*>(srcp+dst_width*2+src_pitch-16));

      __m128i avg = convert_yv24_chroma_block_to_yv12_sse2(src_line0_p0, src_line1_p0, src_line0_p1, src_line1_p1, ffff, mask);

      _mm_storeu_si128(reinterpret_cast<__m128i*>(dstp+dst_width-16), avg);
    }

    dstp += dst_pitch;
    srcp += src_pitch*2;
  }
}

#ifdef X86_32

static __forceinline __m64 convert_yv24_chroma_block_to_yv12_isse(const __m64 &src_line0_p0, const __m64 &src_line1_p0, const __m64 &src_line0_p1, const __m64 &src_line1_p1, const __m64 &ffff, const __m64 &mask) {
  __m64 avg1 = _mm_avg_pu8(src_line0_p0, src_line1_p0);
  __m64 avg2 = _mm_avg_pu8(src_line0_p1, src_line1_p1);

  __m64 avg1x = _mm_xor_si64(avg1, ffff);
  __m64 avg2x = _mm_xor_si64(avg2, ffff);

  __m64 avg1_sh = _mm_srli_pi16(avg1x, 8);
  __m64 avg2_sh = _mm_srli_pi16(avg2x, 8);

  avg1 = _mm_avg_pu8(avg1x, avg1_sh);
  avg2 = _mm_avg_pu8(avg2x, avg2_sh);

  avg1 = _mm_and_si64(avg1, mask);
  avg2 = _mm_and_si64(avg2, mask);

  __m64 packed = _mm_packs_pu16(avg1, avg2);
  return _mm_xor_si64(packed, ffff);
}

static void convert_yv24_chroma_to_yv12_isse(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int dst_width, const int dst_height) {
  int mod8_width = dst_width / 8 * 8;

#pragma warning(push)
#pragma warning(disable:4309)
  __m64 ffff = _mm_set1_pi8(0xFF);
#pragma warning(pop)
  __m64 mask = _mm_set1_pi16(0x00FF);

  for (int y = 0; y < dst_height; ++y) {
    for (int x = 0; x < mod8_width; x+=8) {
      __m64 src_line0_p0 = *reinterpret_cast<const __m64*>(srcp+x*2);
      __m64 src_line0_p1 = *reinterpret_cast<const __m64*>(srcp+x*2+8);
      __m64 src_line1_p0 = *reinterpret_cast<const __m64*>(srcp+x*2+src_pitch);
      __m64 src_line1_p1 = *reinterpret_cast<const __m64*>(srcp+x*2+src_pitch+8);

      __m64 avg = convert_yv24_chroma_block_to_yv12_isse(src_line0_p0, src_line1_p0, src_line0_p1, src_line1_p1, ffff, mask);

      *reinterpret_cast<__m64*>(dstp+x) = avg;
    }

    if (mod8_width != dst_width) {
      __m64 src_line0_p0 = *reinterpret_cast<const __m64*>(srcp+dst_width*2-16);
      __m64 src_line0_p1 = *reinterpret_cast<const __m64*>(srcp+dst_width*2-8);
      __m64 src_line1_p0 = *reinterpret_cast<const __m64*>(srcp+dst_width*2+src_pitch-16);
      __m64 src_line1_p1 = *reinterpret_cast<const __m64*>(srcp+dst_width*2+src_pitch-8);

      __m64 avg = convert_yv24_chroma_block_to_yv12_isse(src_line0_p0, src_line1_p0, src_line0_p1, src_line1_p1, ffff, mask);

      *reinterpret_cast<__m64*>(dstp+dst_width-8) = avg;
    }

    dstp += dst_pitch;
    srcp += src_pitch*2;
  }
  _mm_empty();
}

#endif X86_32

static void convert_yv24_chroma_to_yv12_c(BYTE *dstp, const BYTE *srcp, int dst_pitch, int src_pitch, int dst_width, const int dst_hegiht) {
  for (int y=0; y < dst_hegiht; y++) {
    for (int x=0; x < dst_width; x++) {
      dstp[x] = (srcp[x*2] + srcp[x*2+1] + srcp[x*2+src_pitch] + srcp[x*2+src_pitch+1] + 2)>>2;
    }
    srcp+=src_pitch*2;
    dstp+=dst_pitch;
  }
}

PVideoFrame Convert444ToYV12::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env)
{
  env->MakeWritable(&dst);

  env->BitBlt(dst->GetWritePtr(PLANAR_Y), dst->GetPitch(PLANAR_Y),
    src->GetPtr(PLANAR_Y), src->pitch, dst->GetRowSize(PLANAR_Y), dst->GetHeight());

  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcUVpitch = src->pitch;

  BYTE* dstU = dst->GetWritePtr(PLANAR_U);
  BYTE* dstV = dst->GetWritePtr(PLANAR_V);

  int dstUVpitch = dst->GetPitch(PLANAR_U);

  int w = dst->GetRowSize(PLANAR_U);
  int h = dst->GetHeight(PLANAR_U);

  if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcU, 16) && IsPtrAligned(srcV, 16) && IsPtrAligned(dstU, 16) && IsPtrAligned(dstV, 16)) 
  {
    convert_yv24_chroma_to_yv12_sse2(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
    convert_yv24_chroma_to_yv12_sse2(dstV, srcV, dstUVpitch, srcUVpitch, w, h);
  }
  else
#ifdef X86_32
  if (env->GetCPUFlags() & CPUF_INTEGER_SSE)
  {
    convert_yv24_chroma_to_yv12_isse(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
    convert_yv24_chroma_to_yv12_isse(dstV, srcV, dstUVpitch, srcUVpitch, w, h);
  } 
  else
#endif
  {
    convert_yv24_chroma_to_yv12_c(dstU, srcU, dstUVpitch, srcUVpitch, w, h);
    convert_yv24_chroma_to_yv12_c(dstV, srcV, dstUVpitch, srcUVpitch, w, h);
  }
  return dst;
}


/*****   YUV 4:4:4 -> YUY2   *******/

PVideoFrame Convert444ToYUY2::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();

  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  for (int y=0; y<h; y++) {
    for (int x=0; x<w; x+=2) {
      int x2 = x<<1;
      dstP[x2]   = srcY[x];
      dstP[x2+1] = (srcU[x] + srcU[x+1] + 1)>>1;
      dstP[x2+2] = srcY[x+1];
      dstP[x2+3] = (srcV[x] + srcV[x+1] + 1)>>1;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP+=dstPitch;
  }
  return dst;
}

/*****   YUV 4:4:4 -> RGB24/32   *******/

#define Kr 0.299
#define Kg 0.587
#define Kb 0.114

PVideoFrame Convert444ToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 255.0/224.0 * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 255.0/224.0 * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 255.0/224.0 * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 255.0/224.0 * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = (srcY[x] -  16) * int(255.0/219.0*65536+0.5);
      const int U =  srcU[x] - 128;
      const int V =  srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}

PVideoFrame Convert444NonCCIRToRGB::ConvertImage(Image444* src, PVideoFrame dst, IScriptEnvironment* env) {
  const int crv = int(2*(1-Kr)       * 65536+0.5);
  const int cgv = int(2*(1-Kr)*Kr/Kg * 65536+0.5);
  const int cgu = int(2*(1-Kb)*Kb/Kg * 65536+0.5);
  const int cbu = int(2*(1-Kb)       * 65536+0.5);

  env->MakeWritable(&dst);

  const BYTE* srcY = src->GetPtr(PLANAR_Y);
  const BYTE* srcU = src->GetPtr(PLANAR_U);
  const BYTE* srcV = src->GetPtr(PLANAR_V);

  int srcPitch = src->pitch;

  BYTE* dstP = dst->GetWritePtr();
  int dstPitch = dst->GetPitch();

  int w = src->w();
  int h = src->h();

  dstP += h*dstPitch-dstPitch;
  int bpp = dst->GetRowSize()/w;

  for (int y=0; y<h; y++) {
    int xRGB = 0;
    for (int x=0; x<w; x++) {
      const int Y = srcY[x]*65536;
      const int U = srcU[x] - 128;
      const int V = srcV[x] - 128;

      dstP[xRGB+0] = ScaledPixelClip(Y + U * cbu);
      dstP[xRGB+1] = ScaledPixelClip(Y - U * cgu - V * cgv);
      dstP[xRGB+2] = ScaledPixelClip(Y           + V * crv);
      xRGB += bpp;
    }
    srcY+=srcPitch;
    srcU+=srcPitch;
    srcV+=srcPitch;
    dstP-=dstPitch;
  }
  return dst;
}


/******* RGB 24/32 -> YUV444   *******/

void Convert444FromRGB::ConvertImageLumaOnly(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);

  int dstPitch = dst->pitch;

  int w = dst->w();
  int h = dst->h();

  int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      dstY[x] = srcP[RGBx]; // Blue channel only ???
      RGBx+=bpp;
    }
    srcP-=srcPitch;
    dstY+=dstPitch;
  }
}

void Convert444FromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyr = int(Kr * 219/255 * 65536 + 0.5);
  const int cyg = int(Kg * 219/255 * 65536 + 0.5);
  const int cyb = int(Kb * 219/255 * 65536 + 0.5);

  const int kv = int(32768 / (2*(1-Kr) * 255.0/224.0) + 0.5); // 20531
  const int ku = int(32768 / (2*(1-Kb) * 255.0/224.0) + 0.5); // 16244

  const BYTE* srcP = src->GetReadPtr();
  int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x108000) >> 16; // 0x108000 = 16.5 * 65536
      const int scaled_y = (y - 16) * int(255.0/219.0*65536+0.5);
      const int b_y = (b << 16) - scaled_y;
      const int r_y = (r << 16) - scaled_y;

      dstY[x] = BYTE(y);
      dstU[x] = BYTE(((b_y>>11) * ku + 0x8080000)>>20); // 0x8080000 = 128.5 << 20
      dstV[x] = BYTE(((r_y>>11) * kv + 0x8080000)>>20);

      RGBx += bpp;
    }

    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

void Convert444NonCCIRFromRGB::ConvertImage(PVideoFrame src, Image444* dst, IScriptEnvironment* env) {
  const int cyb = int(Kb * 65536 + 0.5);
  const int cyg = int(Kg * 65536 + 0.5);
  const int cyr = int(Kr * 65536 + 0.5);

  const int kv = int(65536 / (2*(1-Kr)) + 0.5);
  const int ku = int(65536 / (2*(1-Kb)) + 0.5);

  const BYTE* srcP = src->GetReadPtr();
  const int srcPitch = src->GetPitch();

  BYTE* dstY = dst->GetPtr(PLANAR_Y);
  BYTE* dstU = dst->GetPtr(PLANAR_U);
  BYTE* dstV = dst->GetPtr(PLANAR_V);

  const int dstPitch = dst->pitch;

  const int w = dst->w();
  const int h = dst->h();

  const int bpp = src->GetRowSize()/w;
  srcP += h*srcPitch-srcPitch;

  for (int y=0; y<h; y++) {
    int RGBx = 0;
    for (int x=0; x<w; x++) {
      const int b = srcP[RGBx+0];
      const int g = srcP[RGBx+1];
      const int r = srcP[RGBx+2];

      const int y = (cyb*b + cyg*g + cyr*r + 0x8000) >> 16; // 0x8000 = 0.5 * 65536

      dstY[x] = BYTE(y);
      dstU[x] = BYTE(((b - y) * ku + 0x808000)>>16); // 0x808000 = 128.5 * 65536
      dstV[x] = BYTE(((r - y) * kv + 0x808000)>>16);

      RGBx+=bpp;
    }
    srcP-=srcPitch;

    dstY+=dstPitch;
    dstU+=dstPitch;
    dstV+=dstPitch;
  }
}

