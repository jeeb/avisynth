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

#include "../convert_yuy2.h"
#include "convert_yv12_sse.h"
#include "../convert.h"
#include "../convert_matrix.h"
#include <avs/alignment.h>

#ifdef AVS_WINDOWS
    #include <avs/win.h>
#else
    #include <avs/posix.h>
#endif

#include <emmintrin.h>


template<int rgb_bytes>
static void convert_rgb_line_to_yuy2_sse2(const BYTE *srcp, BYTE *dstp, int width, const ConversionMatrix& matrix) {
  const bool TV_range = 0 != matrix.offset_y; // Rec601, Rec709, Rec2020
  const __m128i luma_round_mask = _mm_set1_epi32(TV_range ? 0x84000 : 0x4000); //  16.5 * 32768 : 0.5 * 32768
  const __m128i tv_scale = _mm_set1_epi32(matrix.offset_y << 2); // 64/0

  __m128i luma_coefs = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i chroma_coefs = _mm_set_epi16(matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma, matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma);
  __m128i chroma_round_mask = _mm_set1_epi32(0x808000);

  __m128i upper_dword_mask = _mm_set1_epi32(0xFFFF0000);
  __m128i zero = _mm_setzero_si128();

  //main processing
  __m128i src = _mm_cvtsi32_si128(*reinterpret_cast<const int*>(srcp));
  src = _mm_unpacklo_epi8(src, zero); //xx | 00xx 00r0 00g0 00b0
  __m128i t1 = _mm_madd_epi16(src, luma_coefs); //xx | xx | xx*0 + r0*cyr | g0*cyg + b0*cyb

  __m128i t1_r = _mm_shuffle_epi32(t1, _MM_SHUFFLE(3, 3, 1, 1)); //xx | xx | xx | r0*cyr

  t1 = _mm_add_epi32(t1, luma_round_mask); //xx | xx | xx | g0*cyg + b0*cyb + round
  t1 = _mm_add_epi32(t1, t1_r); //xx | xx | xx | r0*cyr + g0*cyg + b0*cyb + round

  __m128i y0 = _mm_srli_epi32(t1, 15); //xx | xx | xx | 0 0 0 0 y0
  y0 = _mm_shuffle_epi32(y0, _MM_SHUFFLE(0, 0, 0, 0)); //0 0 0 0 y0 | 0 0 0 0 y0 | 0 0 0 0 y0 | 0 0 0 0 y0
  __m128i rb_prev = _mm_shuffle_epi32(src, _MM_SHUFFLE(1, 0, 1, 0)); //00xx 00r0 00g0 00b0 | xx

  for (int x = 0; x < width; x+=4) {

    __m128i rgb_p1, rgb_p2;
    if constexpr(rgb_bytes == 4) {
      //RGB32
      __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(srcp+x*4)); //xxr3 g3b3 xxr2 g2b2 | xxr1 g1b1 xxr0 g0b0

      rgb_p1 = _mm_unpacklo_epi8(src, zero); //00xx 00r2 00g2 00b2 | 00xx 00r1 00g1 00b1
      rgb_p2 = _mm_unpackhi_epi8(src, zero); //00xx 00r4 00g4 00b4 | 00xx 00r3 00g3 00b3
    } else {
      //RGB24
      __m128i pixel01 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3)); //pixels 0 and 1
      __m128i pixel23 = _mm_loadl_epi64(reinterpret_cast<const __m128i*>(srcp+x*3+6)); //pixels 2 and 3

      //0 0 0 0 0 0 0 0 | x x r1 g1 b1 r0 g0 b0  -> 0 x 0 x 0 r1 0 g1 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 g1 0 b1 0 r0 | 0 b1 0 r0 0 g0 0 b0 -> 0 r1 0 r1 0 g1 0 b1 | 0 b1 0 r0 0 g0 0 b0
      rgb_p1 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel01, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
      rgb_p2 = _mm_shufflehi_epi16(_mm_shuffle_epi32(_mm_unpacklo_epi8(pixel23, zero), _MM_SHUFFLE(2, 1, 1, 0)), _MM_SHUFFLE(0, 3, 2, 1));
    }

    __m128i rb13 = _mm_unpacklo_epi64(rgb_p1, rgb_p2); //00xx 00r3 00g3 00b3 | 00xx 00r1 00g1 00b1
    __m128i rb24 = _mm_unpackhi_epi64(rgb_p1, rgb_p2); //00xx 00r4 00g4 00b4 | 00xx 00r2 00g2 00b2
    __m128i rb02 = _mm_unpackhi_epi64(rb_prev, rgb_p1); //00xx 00r2 00g2 00b2 | 00xx 00r0 00g0 00b0
    rb_prev = rgb_p2;
    __m128i rb = _mm_add_epi16(rb13, rb13); //xxxx r3*2 | xxxx b3*2 | 00xx r1*2 | xxxx b1*2
    rb = _mm_add_epi16(rb, rb24); //xxxx r3*2 + r4 | xxxx b3*2 + b4 | 00xx r1*2 + r2 | xxxx b1*2 + b2
    rb = _mm_add_epi16(rb, rb02); //xxxx r2 + r3*2 + r4 | xxxx b2 + b3*2 + b4 | 00xx r0 + r1*2 + r2 | xxxx b0 + b1*2 + b2

    rb = _mm_slli_epi32(rb, 16); //r2+r3*2+r4 0000 | b2 + b3*2 + b4 0000 | r0+r1*2+r2 0000 | b0 + b1*2 + b2 0000

    __m128i t1 = _mm_madd_epi16(rgb_p1, luma_coefs); //xx*0 + r2*cyr | g2*cyg + b2*cyb | xx*0 + r1*cyr | g1*cyg + b1*cyb
    __m128i t2 = _mm_madd_epi16(rgb_p2, luma_coefs); //xx*0 + r4*cyr | g4*cyg + b4*cyb | xx*0 + r3*cyr | g3*cyg + b3*cyb

    __m128i r_temp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2), _MM_SHUFFLE(3, 1, 3, 1))); // r4*cyr | r3*cyr | r2*cyr | r1*cyr
    __m128i gb_temp = _mm_castps_si128(_mm_shuffle_ps(_mm_castsi128_ps(t1), _mm_castsi128_ps(t2), _MM_SHUFFLE(2, 0, 2, 0))); // g4*cyg + b4*cyb | g3*cyg + b3*cyb | g2*cyg + b2*cyb | g1*cyg + b1*cyb

    __m128i luma = _mm_add_epi32(r_temp, gb_temp); //r4*cyr + g4*cyg + b4*cyb | r3*cyr + g3*cyg + b3*cyb | r2*cyr + g2*cyg + b2*cyb | r1*cyr + g1*cyg + b1*cyb
    luma = _mm_add_epi32(luma, luma_round_mask); //r4*cyr + g4*cyg + b4*cyb + round | r3*cyr + g3*cyg + b3*cyb + round | r2*cyr + g2*cyg + b2*cyb + round | r1*cyr + g1*cyg + b1*cyb + round
    luma = _mm_srli_epi32(luma, 15); //0000 00y4 | 0000 00y3 | 0000 00y2 | 0000 00y1


    __m128i y13 = _mm_shuffle_epi32(luma, _MM_SHUFFLE(2, 2, 0, 0)); //0000 00y3 | 0000 00y3 | 0000 00y1 | 0000 00y1
    __m128i y02 = _mm_castps_si128(_mm_shuffle_ps(
      _mm_castsi128_ps(y0),
      _mm_castsi128_ps(luma),
      _MM_SHUFFLE(1, 1, 3, 3)
      )); //0000 00y2 | 0000 00y2 | 0000 00y0 | 0000 00y0
    __m128i y24 = _mm_shuffle_epi32(luma, _MM_SHUFFLE(3, 3, 1, 1)); //0000 00y4 | 0000 00y4 | 0000 00y2 | 0000 00y2
    y0 = luma;
    __m128i scaled_y = _mm_add_epi16(y13, y13);
    scaled_y = _mm_add_epi16(scaled_y, y02);
    scaled_y = _mm_add_epi16(scaled_y, y24);//0000 y2+y3*2+y4 | 0000 y2+y3*2+y4 | 0000 y0+y1*2+y2 | 0000 y0+y1*2+y2

    scaled_y = _mm_sub_epi16(scaled_y, tv_scale);

    __m128i rby = _mm_or_si128(rb, scaled_y); //00 rr 00 yy 00 bb 00 yy

    __m128i uv = _mm_madd_epi16(rby, chroma_coefs);
    uv = _mm_srai_epi32(uv, 1);

    uv = _mm_add_epi32(uv, chroma_round_mask);
    uv = _mm_and_si128(uv, upper_dword_mask);
    __m128i yuv = _mm_or_si128(uv, luma);

    yuv = _mm_packus_epi16(yuv, yuv);

   _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), yuv);
  }
}

template<int rgb_bytes>
void convert_rgb_to_yuy2_sse2(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix &matrix) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_to_yuy2_sse2<rgb_bytes>(src, dst, width, matrix);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
}

//instantiate
//template<int rgb_bytes>
template void convert_rgb_to_yuy2_sse2<3>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);
template void convert_rgb_to_yuy2_sse2<4>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);

#ifdef X86_32

#pragma warning(push)
#pragma warning(disable: 4799 4700)

template<int rgb_bytes>
static void convert_rgb_line_to_yuy2_mmx(const BYTE *srcp, BYTE *dstp, int width, const ConversionMatrix& matrix) {
  const bool TV_range = 0 != matrix.offset_y; // Rec601, Rec709, Rec2020
  const __m64 luma_round_mask = _mm_set1_pi32(TV_range ? 0x84000 : 0x4000); //  16.5 * 32768 : 0.5 * 32768
  const __m64 tv_scale = _mm_set1_pi32(matrix.offset_y << 2); // 64/0

  __m64 luma_coefs = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 chroma_coefs = _mm_set_pi16(matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma);
  __m64 chroma_round_mask = _mm_set1_pi32(0x808000);

  __m64 upper_dword_mask = _mm_set1_pi32(0xFFFF0000);
  __m64 zero = _mm_setzero_si64();
  __m64 dont_care;

  __m64 src = *reinterpret_cast<const __m64*>(srcp);
  src = _mm_unpacklo_pi8(src, zero); //00xx 00r0 00g0 00b0
  __m64 t1 = _mm_madd_pi16(src, luma_coefs); //xx*0 + r0*cyr | g0*cyg + b0*cyb
  __m64 t1_r = _mm_unpackhi_pi32(t1, dont_care); //xx | r0*cyr

  t1 = _mm_add_pi32(t1, luma_round_mask); //xx | g0*cyg + b0*cyb + round
  t1 = _mm_add_pi32(t1, t1_r); //xx | r0*cyr + g0*cyg + b0*cyb + round

  __m64 y0 = _mm_srli_pi32(t1, 15); //xx | 0 0 0 0 y0
  __m64 rb_prev = src;

  for (int x = 0; x < width; x+=2) {
    __m64 src = *reinterpret_cast<const __m64*>(srcp+x*rgb_bytes); //xxr1 g1b1 xxr0 g0b0

    __m64 rgb_p1 = _mm_unpacklo_pi8(src, zero); //00xx 00r0 00g0 00b0
    if (rgb_bytes == 3) {
      src = _mm_slli_si64(src, 8);
    }
    __m64 rgb_p2 = _mm_unpackhi_pi8(src, zero); //00xx 00r1 00g1 00b1

    __m64 rb  = _mm_add_pi16(rgb_p1, rgb_p1);
    __m64 rb_part2 = _mm_add_pi16(rgb_p2, rb_prev);
    rb_prev = rgb_p2;
    rb = _mm_add_pi16(rb, rb_part2);
    rb = _mm_slli_pi32(rb, 16); //00 r0+r1*2+r2 00 00 || 00 b0 + b1*2 + b2  00 00

    __m64 t1 = _mm_madd_pi16(rgb_p1, luma_coefs); //xx*0 + r0*cyr | g0*cyg + b0*cyb
    __m64 t2 = _mm_madd_pi16(rgb_p2, luma_coefs); //xx*0 + r1*cyr | g1*cyg + b1*cyb

    __m64 r_temp = _mm_unpackhi_pi32(t1, t2); //r1*cyr | r0*cyr
    __m64 gb_temp = _mm_unpacklo_pi32(t1, t2); //g1*cyg + b1*cyb | g0*cyg + b0*cyb

    __m64 luma = _mm_add_pi32(r_temp, gb_temp); //r1*cyr + g1*cyg + b1*cyb | r0*cyr + g0*cyg + b0*cyb
    luma = _mm_add_pi32(luma, luma_round_mask); //r1*cyr + g1*cyg + b1*cyb + round | r0*cyr + g0*cyg + b0*cyb + round
    luma = _mm_srli_pi32(luma, 15); //00 00 00 y1 00 00 00 y0  //correspond to y1 and y2 in C code

    __m64 y2 = _mm_unpackhi_pi32(luma, dont_care); //00 00 00 00 y2 00 00 00 y2

    __m64 scaled_y = _mm_add_pi16(y2, luma); //xx | 00 00 y2+y1
    scaled_y = _mm_add_pi16(scaled_y, y0); //00 00 y2 + y1 + y0 | 00 00 y2 + y1 + y0
    y0 = y2;
    scaled_y = _mm_add_pi16(scaled_y, luma);  //00 00 y2 + y1*2 + y0 | 00 00 y2 + y1*2 + y0
    scaled_y = _mm_unpacklo_pi32(scaled_y, scaled_y);
    scaled_y = _mm_sub_pi16(scaled_y, tv_scale);

    __m64 rby = _mm_or_si64(rb, scaled_y); //00 rr 00 yy 00 bb 00 yy

    __m64 uv = _mm_madd_pi16(rby, chroma_coefs);
    uv = _mm_srai_pi32(uv, 1);

    uv = _mm_add_pi32(uv, chroma_round_mask);
    uv = _mm_and_si64(uv, upper_dword_mask);
    __m64 yuv = _mm_or_si64(uv, luma);

    yuv = _mm_packs_pu16(yuv, yuv);

    *reinterpret_cast<int*>(dstp+x*2) = _mm_cvtsi64_si32(yuv);
  }
}
#pragma warning(pop)

template<int rgb_bytes>
void convert_rgb_to_yuy2_mmx(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_to_yuy2_mmx<rgb_bytes>(src, dst, width, matrix);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
  _mm_empty();
}

//instantiate
//template<int rgb_bytes>
template void convert_rgb_to_yuy2_mmx<3>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);
template void convert_rgb_to_yuy2_mmx<4>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);

#endif



/****************************************************
 ******* Convert back to YUY2                  ******
 ******* this only uses Chroma from left pixel ******
 ******* to be used, when signal already has   ******
 ******* been YUY2 to avoid deterioration      ******
 ****************************************************/



void convert_yv24_back_to_yuy2_sse2(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int pitchY, int pitchUV, int dpitch, int height, int width) {
  int mod16_width = width / 16 * 16;
  __m128i ff = _mm_set1_epi16(0x00ff);

  for (int yy=0; yy < height; yy++) {
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

void convert_yv24_back_to_yuy2_mmx(const BYTE* srcY, const BYTE* srcU, const BYTE* srcV, BYTE* dstp, int pitchY, int pitchUV, int dpitch, int height, int width) {
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




template<int rgb_bytes, bool aligned>
static AVS_FORCEINLINE __m128i convert_rgb_block_back_to_yuy2_sse2(const BYTE* srcp, const __m128i &luma_coefs, const __m128i &chroma_coefs, const __m128i &upper_dword_mask,
                                                                 const __m128i &chroma_round_mask, const __m128i &luma_round_mask, const __m128i &tv_scale, const __m128i &zero) {
  __m128i rgb_p1, rgb_p2;
  if constexpr(rgb_bytes == 4) {
    //RGB32
    __m128i src;
    if constexpr(aligned) {
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

  __m128i y_scaled = _mm_sub_epi16(luma, tv_scale); // need_tv_scale could go to template

  __m128i y0 = _mm_shuffle_epi32(y_scaled, _MM_SHUFFLE(2, 2, 0, 0)); //00 00 00 y2 00 00 00 y2 | 00 00 00 y0 00 00 00 y0

  __m128i rby = _mm_or_si128(rb_p, y0); //00 rr 00 y2 00 b2 00 y2 | 00 r0 00 y0 00 b0 00 y0

  rby = _mm_adds_epu16(rby, rby); //2*r2 | 2*y2 | 2*b2 | 2*y2 | 2*r0 | 2*y0 | 2*b0 | 2*y0

  __m128i uv = _mm_madd_epi16(rby, chroma_coefs);

  uv = _mm_add_epi32(uv, chroma_round_mask);
  uv = _mm_and_si128(uv, upper_dword_mask);
  __m128i yuv = _mm_or_si128(uv, luma); ///00 v1 00 y3 00 u1 00 y2 | 00 v0 00 y1 00 u0 00 y0

  return _mm_packus_epi16(yuv, yuv);
}

template<int rgb_bytes>
static AVS_FORCEINLINE void convert_rgb_line_back_to_yuy2_sse2(const BYTE *srcp, BYTE *dstp, int width, const ConversionMatrix &matrix) {
  int mod4_width = width / 4 * 4;

  const bool TV_range = 0 != matrix.offset_y; // Rec601, Rec709, Rec2020
  const __m128i luma_round_mask = _mm_set1_epi32(TV_range ? 0x84000 : 0x4000); //  16.5 * 32768 : 0.5 * 32768
  const __m128i tv_scale = _mm_set1_epi32(matrix.offset_y); // 16/0

  __m128i luma_coefs = _mm_set_epi16(0, matrix.y_r, matrix.y_g, matrix.y_b, 0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m128i chroma_coefs = _mm_set_epi16(matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma, matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma);
  __m128i chroma_round_mask = _mm_set1_epi32(0x808000);

  __m128i upper_dword_mask = _mm_set1_epi32(0xFFFF0000);
  __m128i zero = _mm_setzero_si128();

  for (int x = 0; x < mod4_width; x+=4) {
    __m128i yuv = convert_rgb_block_back_to_yuy2_sse2<rgb_bytes, true>(srcp + x * rgb_bytes, luma_coefs, chroma_coefs, upper_dword_mask, chroma_round_mask, luma_round_mask, tv_scale, zero);

    _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+x*2), yuv);
  }

  if (width != mod4_width) {
    const BYTE* ptr = srcp + (width-4) * rgb_bytes;

    __m128i yuv = convert_rgb_block_back_to_yuy2_sse2<rgb_bytes, false>(ptr, luma_coefs, chroma_coefs, upper_dword_mask, chroma_round_mask, luma_round_mask, tv_scale, zero);

    _mm_storel_epi64(reinterpret_cast<__m128i*>(dstp+width*2 - 8), yuv);
  }
}

template<int rgb_bytes>
void convert_rgb_back_to_yuy2_sse2(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height, ConversionMatrix& matrix) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_back_to_yuy2_sse2<rgb_bytes>(src, dst, width, matrix);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
}

//instantiate
//template<int rgb_bytes>
template void convert_rgb_back_to_yuy2_sse2<3>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, ConversionMatrix& matrix);
template void convert_rgb_back_to_yuy2_sse2<4>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, ConversionMatrix& matrix);

#ifdef X86_32
#pragma warning(disable: 4799)
template<int rgb_bytes>
static void convert_rgb_line_back_to_yuy2_mmx(const BYTE *srcp, BYTE *dstp, int width, const ConversionMatrix &matrix) {

  const bool TV_range = 0 != matrix.offset_y; // Rec601, Rec709, Rec2020
  const __m64 luma_round_mask = _mm_set1_pi32(TV_range ? 0x84000 : 0x4000); //  16.5 * 32768 : 0.5 * 32768
  const __m64 tv_scale = _mm_set1_pi32(matrix.offset_y); // 16/0

  __m64 luma_coefs = _mm_set_pi16(0, matrix.y_r, matrix.y_g, matrix.y_b);
  __m64 chroma_coefs = _mm_set_pi16(matrix.kv, matrix.kv_luma, matrix.ku, matrix.ku_luma);
  __m64 chroma_round_mask = _mm_set1_pi32(0x808000);

  __m64 upper_dword_mask = _mm_set1_pi32(0xFFFF0000);
  __m64 zero = _mm_setzero_si64();

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
void convert_rgb_back_to_yuy2_mmx(const BYTE *src, BYTE *dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix &matrix) {
  src += src_pitch*(height-1);       // ;Move source to bottom line (read top->bottom)

  for (int y=0; y < height; ++y) {
    convert_rgb_line_back_to_yuy2_mmx<rgb_bytes>(src, dst, width, matrix);
    src -= src_pitch;           // ;Move upwards
    dst += dst_pitch;
  } // end for y
  _mm_empty();
}

//instantiate
//template<int rgb_bytes>
template void convert_rgb_back_to_yuy2_mmx<3>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);
template void convert_rgb_back_to_yuy2_mmx<4>(const BYTE* src, BYTE* dst, int src_pitch, int dst_pitch, int width, int height, const ConversionMatrix& matrix);

#endif

