// Avisynth+
// https://avs-plus.net
//
// This file is part of Avisynth+ which is released under GPL2+ with exception.

// Convert Audio helper functions (Pure C)
// Copyright (c) 2020 Xinyue Lu

#include <avs/types.h>

/* Supported fast route conversions:
 *
 * |    From: | U 8 | S16 | S24 | S32 | FLT |
 * | To:      |     |     |     |     |     |
 * |  U 8     |  -  | CS  | CS  | CS  |     |
 * |  S16     | CS  |  -  | CS  | CS  |     |
 * |  S24     | CS  | CS  |  -  | CS  |     |
 * |  S32     | CS  | CS  | CS  |  -  | CS  |
 * |  FLT     |     |     |     | CS  |  -  |
 * 
 * * C = C, S = SSE2+, A = AVX2
 */

void convert32To16(void *inbuf, void *outbuf, int count) {
  auto in16 = reinterpret_cast<int16_t *>(inbuf);
  auto out = reinterpret_cast<int16_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in16[i * 2 + 1];
}

void convert16To32(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<int16_t *>(inbuf);
  auto out16 = reinterpret_cast<int16_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out16[i * 2] = 0;
    out16[i * 2 + 1] = in[i];
  }
}

void convert32To8(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out = reinterpret_cast<uint8_t *>(outbuf);

  const int c_loop = count & ~15;

  for (int i = c_loop; i < count; i++)
    out[i] = in8[i * 4 + 3] + 128;
}

void convert8To32(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 4] = 0;
    out8[i * 4 + 1] = 0;
    out8[i * 4 + 2] = 0;
    out8[i * 4 + 3] = in[i] - 128;
  }
}

void convert16To8(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out = reinterpret_cast<uint8_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in8[i * 2 + 1] + 128;
}

void convert8To16(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 2] = 0;
    out8[i * 2 + 1] = in[i] - 128;
  }
}

void convert32To24(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3 + 0] = in8[i * 4 + 1];
    out8[i * 3 + 1] = in8[i * 4 + 2];
    out8[i * 3 + 2] = in8[i * 4 + 3];
  }
}

void convert24To32(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 4] = 0;
    out8[i * 4 + 1] = in8[i * 3 + 0];
    out8[i * 4 + 2] = in8[i * 3 + 1];
    out8[i * 4 + 3] = in8[i * 3 + 2];
  }
}

void convert24To16(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 2 + 0] = in8[i * 3 + 1];
    out8[i * 2 + 1] = in8[i * 3 + 2];
  }
}

void convert16To24(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3] = 0;
    out8[i * 3 + 1] = in8[i * 2 + 0];
    out8[i * 3 + 2] = in8[i * 2 + 1];
  }
}

void convert24To8(void *inbuf, void *outbuf, int count) {
  auto in8 = reinterpret_cast<int8_t *>(inbuf);
  auto out = reinterpret_cast<uint8_t *>(outbuf);

  for (int i = 0; i < count; i++)
    out[i] = in8[i * 3 + 2] + 128;
}

void convert8To24(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<uint8_t *>(inbuf);
  auto out8 = reinterpret_cast<int8_t *>(outbuf);

  for (int i = 0; i < count; i++) {
    out8[i * 3] = 0;
    out8[i * 3 + 1] = 0;
    out8[i * 3 + 2] = in[i] - 128;
  }
}

void convert32ToFLT(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<int32_t *>(inbuf);
  auto out = reinterpret_cast<SFLOAT *>(outbuf);
  const float divisor = 1.0f/2147483648.0f;

  for (int i = 0; i < count; i++)
    out[i] = in[i] * divisor;
}

void convertFLTTo32(void *inbuf, void *outbuf, int count) {
  auto in = reinterpret_cast<SFLOAT *>(inbuf);
  auto out = reinterpret_cast<int32_t *>(outbuf);
  const float multiplier = 2147483648.0f;
  const float max32 = 2147483647.0f;
  const float min32 = -2147483648.0f;

  for (int i = 0; i < count; i++) {
    float val = in[i] * multiplier;
    if (val > max32) val = max32;
    if (val < min32) val = min32;
    out[i] = static_cast<int32_t>(val);
  }
}
