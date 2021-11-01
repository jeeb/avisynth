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

#ifndef __Convert_bits_H__
#define __Convert_bits_H__

#include <avisynth.h>
#include <stdint.h>
#include "convert.h"

typedef void (*BitDepthConvFuncPtr)(const BYTE *srcp, BYTE *dstp, int src_rowsize, int src_height, int src_pitch, int dst_pitch, int source_bitdepth, int target_bitdepth);

class ConvertBits : public GenericVideoFilter
{
public:
  ConvertBits(PClip _child, const int _dither_mode, const int _target_bitdepth, bool _truerange, bool _fulls, bool _fulld, int _dither_bitdepth, IScriptEnvironment* env);
  PVideoFrame __stdcall GetFrame(int n,IScriptEnvironment* env);

  int __stdcall SetCacheHints(int cachehints, int frame_range) override {
    AVS_UNUSED(frame_range);
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);
private:
  BitDepthConvFuncPtr conv_function;
  BitDepthConvFuncPtr conv_function_chroma; // 32bit float YUV chroma
  BitDepthConvFuncPtr conv_function_a;
  int dither_mode;
  int pixelsize;
  int bits_per_pixel;
  int target_bitdepth;
  int dither_bitdepth;
  bool truerange; // if 16->10 range reducing or e.g. 14->16 bit range expansion needed
  bool format_change_only;
  bool fulls; // source is full range (defaults: rgb=true, yuv=false (bit shift))
  bool fulld; // destination is full range (defaults: rgb=true, yuv=false (bit shift))
};

/**********************************
******  Bitdepth conversions  *****
**********************************/
// 10->8
// repeated 4x for sse size 16
static const struct dither2x2_t
{
  const BYTE data[4] = {
    0, 2,
    3, 1,
  };
  // cycle: 2
  alignas(16) const BYTE data_sse2[2 * 16] = {
    0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2,
    3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1, 3, 1
  };
  dither2x2_t() {};
} dither2x2;


// 12->8
static const struct dither4x4_t
{
  const BYTE data[16] = {
     0,  8,  2, 10,
    12,  4, 14,  6,
     3, 11,  1,  9,
    15,  7, 13,  5
  };
  // cycle: 4
  alignas(16) const BYTE data_sse2[4 * 16] = {
     0,  8,  2, 10,  0,  8,  2, 10,  0,  8,  2, 10,  0,  8,  2, 10,
    12,  4, 14,  6, 12,  4, 14,  6, 12,  4, 14,  6, 12,  4, 14,  6,
     3, 11,  1,  9,  3, 11,  1,  9,  3, 11,  1,  9,  3, 11,  1,  9,
    15,  7, 13,  5, 15,  7, 13,  5, 15,  7, 13,  5, 15,  7, 13,  5
  };
  dither4x4_t() {};
} dither4x4;

// 14->8
static const struct dither8x8_t
{
  const BYTE data[8][8] = {
    { 0, 32,  8, 40,  2, 34, 10, 42}, /* 8x8 Bayer ordered dithering */
    {48, 16, 56, 24, 50, 18, 58, 26}, /* pattern. Each input pixel */
    {12, 44,  4, 36, 14, 46,  6, 38}, /* is scaled to the 0..63 range */
    {60, 28, 52, 20, 62, 30, 54, 22}, /* before looking in this table */
    { 3, 35, 11, 43,  1, 33,  9, 41}, /* to determine the action. */
    {51, 19, 59, 27, 49, 17, 57, 25},
    {15, 47,  7, 39, 13, 45,  5, 37},
    {63, 31, 55, 23, 61, 29, 53, 21}
  };
  // cycle: 8
  alignas(16) const BYTE data_sse2[8][16] = {
    {  0, 32,  8, 40,  2, 34, 10, 42,  0, 32,  8, 40,  2, 34, 10, 42 }, /* 8x8 Bayer ordered dithering */
    { 48, 16, 56, 24, 50, 18, 58, 26, 48, 16, 56, 24, 50, 18, 58, 26 }, /* pattern. Each input pixel */
    { 12, 44,  4, 36, 14, 46,  6, 38, 12, 44,  4, 36, 14, 46,  6, 38 }, /* is scaled to the 0..63 range */
    { 60, 28, 52, 20, 62, 30, 54, 22, 60, 28, 52, 20, 62, 30, 54, 22 }, /* before looking in this table */
    {  3, 35, 11, 43,  1, 33,  9, 41,  3, 35, 11, 43,  1, 33,  9, 41 }, /* to determine the action. */
    { 51, 19, 59, 27, 49, 17, 57, 25, 51, 19, 59, 27, 49, 17, 57, 25 },
    { 15, 47,  7, 39, 13, 45,  5, 37, 15, 47,  7, 39, 13, 45,  5, 37 },
    { 63, 31, 55, 23, 61, 29, 53, 21, 63, 31, 55, 23, 61, 29, 53, 21 }
  };
  dither8x8_t() {};
} dither8x8;

// 16->8
static const struct dither16x16_t
{
  // cycle: 16x
  alignas(16) const BYTE data[16][16] = {
    {   0,192, 48,240, 12,204, 60,252,  3,195, 51,243, 15,207, 63,255 },
    { 128, 64,176,112,140, 76,188,124,131, 67,179,115,143, 79,191,127 },
    {  32,224, 16,208, 44,236, 28,220, 35,227, 19,211, 47,239, 31,223 },
    { 160, 96,144, 80,172,108,156, 92,163, 99,147, 83,175,111,159, 95 },
    {   8,200, 56,248,  4,196, 52,244, 11,203, 59,251,  7,199, 55,247 },
    { 136, 72,184,120,132, 68,180,116,139, 75,187,123,135, 71,183,119 },
    {  40,232, 24,216, 36,228, 20,212, 43,235, 27,219, 39,231, 23,215 },
    { 168,104,152, 88,164,100,148, 84,171,107,155, 91,167,103,151, 87 },
    {   2,194, 50,242, 14,206, 62,254,  1,193, 49,241, 13,205, 61,253 },
    { 130, 66,178,114,142, 78,190,126,129, 65,177,113,141, 77,189,125 },
    {  34,226, 18,210, 46,238, 30,222, 33,225, 17,209, 45,237, 29,221 },
    { 162, 98,146, 82,174,110,158, 94,161, 97,145, 81,173,109,157, 93 },
    {  10,202, 58,250,  6,198, 54,246,  9,201, 57,249,  5,197, 53,245 },
    { 138, 74,186,122,134, 70,182,118,137, 73,185,121,133, 69,181,117 },
    {  42,234, 26,218, 38,230, 22,214, 41,233, 25,217, 37,229, 21,213 },
    { 170,106,154, 90,166,102,150, 86,169,105,153, 89,165,101,149, 85 }
  };
  dither16x16_t() {};
} dither16x16;

#endif // __Convert_bits_H__
