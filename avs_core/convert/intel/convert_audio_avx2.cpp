// Avisynth+
// https://avs-plus.net
//
// This file is part of Avisynth+ which is released under GPL2+ with exception.

// Convert Audio helper functions (AVX2)
// Copyright (c) 2020 Xinyue Lu

#include <avs/types.h>
#include <avs/config.h>
#include <immintrin.h> // AVX2 at most

// Easy: 32-16, 16-32
// Float: 32-FLT, FLT-32

void convert32To16_AVX2(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<int32_t *>(inbuf);
  auto in16 = reinterpret_cast<int16_t *>(inbuf);
  auto out = reinterpret_cast<int16_t *>(outbuf);

  const int c_loop = count & ~15;

  for (int i = c_loop; i < count; i++)
    out[i] = in16[i * 2 + 1];

  for (int i = 0; i < c_loop; i += 16) {
    __m256i in32a = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(in)); in += 8;
    __m256i in32b = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(in)); in += 8;
    __m256i in16a = _mm256_srai_epi32(in32a, 16);
    __m256i in16b = _mm256_srai_epi32(in32b, 16);
    __m256i out16 = _mm256_packs_epi32(in16a, in16b);
    out16 = _mm256_permute4x64_epi64(out16, 216);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out), out16); out += 16;
  }

  _mm256_zeroupper();
}

void convert16To32_AVX2(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<int16_t *>(inbuf);
  auto out = reinterpret_cast<int32_t *>(outbuf);
  auto out16 = reinterpret_cast<int16_t *>(outbuf);

  const int c_loop = count & ~15;

  for (int i = c_loop; i < count; i++) {
    out16[i * 2] = 0;
    out16[i * 2 + 1] = in[i];
  }

  __m256i zero = _mm256_set1_epi16(0);
  for (int i = 0; i < c_loop; i += 16) {
    __m256i in16 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(in)); in += 16;
    in16 = _mm256_permute4x64_epi64(in16, 216);
    __m256i out32a = _mm256_unpacklo_epi16(zero, in16);
    __m256i out32b = _mm256_unpackhi_epi16(zero, in16);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out), out32a); out += 8;
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out), out32b); out += 8;
  }

  _mm256_zeroupper();
}

void convert32ToFLT_AVX2(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<int32_t *>(inbuf);
  auto out = reinterpret_cast<SFLOAT *>(outbuf);
  const float divisor = 1.0f/2147483648.0f;

  const int c_loop = count & ~7;

  for (int i = c_loop; i < count; i++)
    out[i] = in[i] * divisor;

  __m256 divv = _mm256_set1_ps(divisor);
  for (int i = 0; i < c_loop; i += 8) {
    __m256i in32 = _mm256_loadu_si256(reinterpret_cast<const __m256i *>(in)); in += 8;
    __m256 infl = _mm256_cvtepi32_ps(in32);
    __m256 outfl = _mm256_mul_ps(infl, divv);
    _mm256_storeu_ps(out, outfl); out += 8;
  }

  _mm256_zeroupper();
}

void convertFLTTo32_AVX2(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<SFLOAT *>(inbuf);
  auto out = reinterpret_cast<int32_t *>(outbuf);
  const float multiplier = 2147483648.0f;
  const float max32 = 2147483647.0f;
  const float min32 = -2147483648.0f;

  const int c_loop = count & ~7;

  for (int i = c_loop; i < count; i++) {
    float val = in[i] * multiplier;
    if (val > max32) val = max32;
    if (val < min32) val = min32;
    out[i] = static_cast<int32_t>(val);
  }

  __m256 mulv = _mm256_set1_ps(multiplier);
  __m256 maxv = _mm256_set1_ps(max32);
  __m256 minv = _mm256_set1_ps(min32);
  for (int i = 0; i < c_loop; i += 8) {
    __m256 infl = _mm256_loadu_ps(in); in += 8;
    __m256 outfl = _mm256_mul_ps(infl, mulv);
    outfl = _mm256_min_ps(outfl, maxv);
    outfl = _mm256_max_ps(outfl, minv);
    __m256i out32 = _mm256_cvttps_epi32(outfl);
    _mm256_storeu_si256(reinterpret_cast<__m256i *>(out), out32); out += 8;
  }

  _mm256_zeroupper();
}
