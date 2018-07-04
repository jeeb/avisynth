/*
*
* Avisynth+ Expression filter, ported from the VapourSynth project
* Copyright (c) 2012-2015 Fredrik Mellbin
* 
* Additions and differences to VS r39 version:
* ------------------------------
* (similar features to the masktools mt_lut family syntax)
* Operator aliases:
*   Caret (^) can be used like pow
*   For equality check "==" can be used like "="
*   & same as and
*   | same as or
* New operator: != (not equal)
* Built-in constants
*   ymin, ymax (ymin_a .. ymin_z for individual clips) - the usual luma limits (16..235 or scaled equivalents)
*   cmin, cmax (cmin_a .. cmin_z) - chroma limits (16..240 or scaled equivalents)
*   range_half (range_half_a .. range_half_z) - half of the range, (128 or scaled equivalents)
*   range_size, range_half, range_min, range_max (range_size_a .. range_size_z , etc..)
* Autoscale helper functions (operand is treated as being a 8 bit constant unless i8..i16 or f32 is specified)
*   scaleb (scale by bit shift - mul or div by 2, 4, 6, 8...)
*   scalef (scale by stretch full scale - mul or div by source_max/target_max
* Keywords for modifying base bit depth for scaleb and scalef
*   i8, i10, i12, i14, i16, f32
* Built-in math constant
*   pi
* Alpha plane handling. When no separate expression is supplied for alpha, plane is copied instead of reusing last expression parameter
* Proper clamping when storing 10,12 or 14 bit outputs
* Faster storing of results for 8 and 10-16 bit outputs
* 16 pixels/cycle instead of 8 when avx2, with fallback to 8-pixel case on the right edge. Thus no need for 64 byte alignment for 32 bit float.
* (Load zeros for nonvisible pixels, when simd block size goes beyond image width, to prevent garbage input for simd calculation)
* Optimizations: x^0.5 is sqrt, ^1 +0 -0 *1 /1 to nothing, ^2, ^3, ^4 is done by faster and more precise multiplication
* spatial input variables in expr syntax:
*    sx, sy (absolute x and y coordinates, 0 to width-1 and 0 to height-1)
*    sxr, syr (relative x and y coordinates, from 0 to 1.0)
* Optimize: recognize constant plane expression: use fast memset instead of generic simd process. Approx. 3-4x (32 bits) to 10-12x (8 bits) speedup
* Optimize: Recognize single clip letter in expression: use fast plane copy (BitBlt) 
*   (e.g. for 8-16 bits: instead of load-convert_to_float-clamp-convert_to_int-store). Approx. 1.4x (32 bits), 3x (16 bits), 8-9x (8 bits) speedup
* Optimize: do not call GetFrame for input clips that are not referenced or plane-copied
* 20171211: Implement relative pixel indexing e.g. x[-1,-3], requires SSSE3
*           Fix jitasm code generation (corrupt code when doing register reorders)
* 20171212: Variables:  A..Z
*           To store the current top value of the stack to a variable: A@ .. Z@
*           To store the current top value and pop it from the top of the stack: A^.. Z^
*           To use a stored variable: single uppercase letter. E.g. A
* 20171214: Trig.functions (C only): sin, cos, tan, asin, acos, atan
*           '%' The implementation is fmod-like: x - trunc(x/d)*d.
*             Note: SSE2 and up is using trunc for float->integer conversion, works for usual width/height magnitude.
*             (A float can hold a 24 bit integer w/o losing precision)
*           expr constants: 'width', 'height' for current plane width and height
*           expr auto variable: 'frameno' holds the current frame number 0..total number of frames-1
*           expr auto variable: 'time' relative time in clip, 0 <= time < 1 
*                calculation: time = frameno/total number of frames)
* 20180614 new parameters: scale_inputs, clamp_float
*          implement 'clip' three operand operator like in masktools2: x minvalue maxvalue clip -> max(min(x, maxvalue), minvalue)
*
* Differences from masktools 2.2.15
* ---------------------------------
*   Up to 26 clips are allowed (x,y,z,a,b,...w). Masktools handles only up to 4 clips with its mt_lut, my_lutxy, mt_lutxyz, mt_lutxyza
*   Clips with different bit depths are allowed
*   works with 32 bit floats instead of 64 bit double internally 
*   less functions (e.g. no bit shifts)
*   logical 'false' is 0 instead of -1
*   avs+: ymin, ymax, etc built-in constants can have a _X suffix, where X is the corresponding clip designator letter. E.g. cmax_z, range_half_x
*   mt_lutspa-like functionality is available through "sx", "sy", "sxr", "syr"
*/

#include <iostream>
#include <locale>
#include <sstream>
#include <vector>
#include <list>
#include <string>
#include <algorithm>
#include <stdexcept>
#include <memory>
#include <cmath>
#include <unordered_map>

#include <avisynth.h>
#include <avs/win.h>
#include <stdlib.h>
#include "../core/internal.h"
#include "../../convert/convert_planar.h" // fill_plane
#include "avs/alignment.h"

#define VS_TARGET_CPU_X86
#define VS_TARGET_OS_WINDOWS
#include "exprfilter.h"

#ifdef VS_TARGET_CPU_X86
#define NOMINMAX
#include "jitasm.h"
#ifndef VS_TARGET_OS_WINDOWS
#include <sys/mman.h>
#endif
#endif

//#define TEST_AVX2_CODEGEN_IN_AVX

#include <immintrin.h>

#ifdef VS_TARGET_CPU_X86

// normal versions work with two xmm or ymm registers (2*4 or 2*8 pixels per cycle)
// _Single suffixed versions work only one xmm or ymm registers at a time (1*4 or 1*8 pixels per cycle)

#define OneArgOp(instr) \
auto &t1 = stack.back(); \
instr(t1.first, t1.first); \
instr(t1.second, t1.second);

#define OneArgOp_Single(instr) \
auto &t1 = stack1.back(); \
instr(t1, t1);

#define TwoArgOp(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto &t2 = stack.back(); \
instr(t2.first, t1.first); \
instr(t2.second, t1.second);

#define TwoArgOp_Single(instr) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto &t2 = stack1.back(); \
instr(t2, t1);

#define TwoArgOp_Avx(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto &t2 = stack.back(); \
instr(t2.first, t2.first, t1.first); \
instr(t2.second, t2.second, t1.second);

#define TwoArgOp_Single_Avx(instr) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto &t2 = stack1.back(); \
instr(t2, t2, t1);

#define CmpOp(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto t2 = stack.back(); \
stack.pop_back(); \
instr(t1.first, t2.first); \
instr(t1.second, t2.second); \
andps(t1.first, CPTR(elfloat_one)); \
andps(t1.second, CPTR(elfloat_one)); \
stack.push_back(t1);

#define CmpOp_Single(instr) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto t2 = stack1.back(); \
stack1.pop_back(); \
instr(t1, t2); \
andps(t1, CPTR(elfloat_one)); \
stack1.push_back(t1);

#define CmpOp_Avx(instr, op) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto t2 = stack.back(); \
stack.pop_back(); \
instr(t1.first, t1.first, t2.first, op); \
instr(t1.second, t1.second, t2.second, op); \
vandps(t1.first, t1.first, CPTR_AVX(elfloat_one)); \
vandps(t1.second, t1.second, CPTR_AVX(elfloat_one)); \
stack.push_back(t1);

#define CmpOp_Single_Avx(instr, op) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto t2 = stack1.back(); \
stack1.pop_back(); \
instr(t1, t1, t2, op); \
vandps(t1, t1, CPTR_AVX(elfloat_one)); \
stack1.push_back(t1);

#define LogicOp(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto t2 = stack.back(); \
stack.pop_back(); \
cmpnleps(t1.first, zero); \
cmpnleps(t1.second, zero); \
cmpnleps(t2.first, zero); \
cmpnleps(t2.second, zero); \
instr(t1.first, t2.first); \
instr(t1.second, t2.second); \
andps(t1.first, CPTR(elfloat_one)); \
andps(t1.second, CPTR(elfloat_one)); \
stack.push_back(t1);

#define LogicOp_Single(instr) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto t2 = stack1.back(); \
stack1.pop_back(); \
cmpnleps(t1, zero); \
cmpnleps(t2, zero); \
instr(t1, t2); \
andps(t1, CPTR(elfloat_one)); \
stack1.push_back(t1);

#define LogicOp_Avx(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto t2 = stack.back(); \
stack.pop_back(); \
vcmpps(t1.first, t1.first, zero, _CMP_GT_OQ); \
vcmpps(t1.second, t1.second, zero, _CMP_GT_OQ); \
vcmpps(t2.first, t2.first, zero, _CMP_GT_OQ); \
vcmpps(t2.second, t2.second, zero, _CMP_GT_OQ); \
instr(t1.first, t1.first, t2.first); \
instr(t1.second, t1.second, t2.second); \
vandps(t1.first, t1.first, CPTR_AVX(elfloat_one)); \
vandps(t1.second, t1.second, CPTR_AVX(elfloat_one)); \
stack.push_back(t1);

#define LogicOp_Single_Avx(instr) \
auto t1 = stack1.back(); \
stack1.pop_back(); \
auto t2 = stack1.back(); \
stack1.pop_back(); \
vcmpps(t1, t1, zero, _CMP_GT_OQ); \
vcmpps(t2, t2, zero, _CMP_GT_OQ); \
instr(t1, t1, t2); \
vandps(t1, t1, CPTR_AVX(elfloat_one)); \
stack1.push_back(t1);

enum {
    elabsmask, elc7F, elmin_norm_pos, elinv_mant_mask,
    elfloat_one, elfloat_half, elstore8, elstore10, elstore12, elstore14, elstore16,
    spatialX, spatialX2,
    loadmask1000, loadmask1100, loadmask1110,
    elShuffleForRight0, elShuffleForRight1, elShuffleForRight2, elShuffleForRight3, elShuffleForRight4, elShuffleForRight5, elShuffleForRight6,
    elShuffleForLeft0, elShuffleForLeft1, elShuffleForLeft2, elShuffleForLeft3, elShuffleForLeft4, elShuffleForLeft5, elShuffleForLeft6,
    elexp_hi, elexp_lo, elcephes_LOG2EF, elcephes_exp_C1, elcephes_exp_C2, elcephes_exp_p0, elcephes_exp_p1, elcephes_exp_p2, elcephes_exp_p3, elcephes_exp_p4, elcephes_exp_p5, elcephes_SQRTHF,
    elcephes_log_p0, elcephes_log_p1, elcephes_log_p2, elcephes_log_p3, elcephes_log_p4, elcephes_log_p5, elcephes_log_p6, elcephes_log_p7, elcephes_log_p8, elcephes_log_q1 = elcephes_exp_C2, elcephes_log_q2 = elcephes_exp_C1
};

// constants for xmm

#define XCONST(x) { x, x, x, x }
#define MAKEDWORD(ch0, ch1, ch2, ch3)                              \
                ((DWORD)(BYTE)(ch0) | ((DWORD)(BYTE)(ch1) << 8) |   \
                ((DWORD)(BYTE)(ch2) << 16) | ((DWORD)(BYTE)(ch3) << 24 ))
#define XBYTECONST(a0,a1,a2,a3,a4,a5,a6,a7,a8,a9,a10,a11,a12,a13,a14,a15) \
  { (int)MAKEDWORD(a0,a1,a2,a3), (int)MAKEDWORD(a4,a5,a6,a7), (int)MAKEDWORD(a8,a9,a10,a11), (int)MAKEDWORD(a12,a13,a14,a15) }


alignas(16) static const FloatIntUnion logexpconst[][4] = {
    XCONST(0x7FFFFFFF), // absmask
    XCONST(0x7F), // c7F
    XCONST(0x00800000), // min_norm_pos
    XCONST(~0x7f800000), // inv_mant_mask
    XCONST(1.0f), // float_one
    XCONST(0.5f), // float_half
    XCONST(255.0f), // store8
    XCONST(1023.0f), // store10 (avs+)
    XCONST(4095.0f), // store12 (avs+)
    XCONST(16383.0f), // store14 (avs+)
    XCONST(65535.0f), // store16
    { 0.0f, 1.0f, 2.0f, 3.0f }, // spatialX
    { 4.0f, 5.0f, 6.0f, 7.0f }, // spatialX2
    { (int)0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000 }, // loadmask1000
    { (int)0xFFFFFFFF, (int)0xFFFFFFFF, 0x00000000, 0x00000000 }, // loadmask1100
    { (int)0xFFFFFFFF, (int)0xFFFFFFFF, (int)0xFFFFFFFF, 0x00000000 }, // loadmask1110
    XBYTECONST(0,1,2,3,4,5,6,7,  8,9,10,11,12,13,12,13), // elShuffleForRight0
    XBYTECONST(0,1,2,3,4,5,6,7,  8,9,10,11,10,11,10,11), // elShuffleForRight1
    XBYTECONST(0,1,2,3,4,5,6,7,  8,9,8,9,8,9,8,9), // elShuffleForRight2
    XBYTECONST(0,1,2,3,4,5,6,7,  6,7,6,7,6,7,6,7), // elShuffleForRight3
    XBYTECONST(0,1,2,3,4,5,4,5,  4,5,4,5,4,5,4,5), // elShuffleForRight4
    XBYTECONST(0,1,2,3,2,3,2,3,  2,3,2,3,2,3,2,3), // elShuffleForRight5
    XBYTECONST(0,1,0,1,0,1,0,1,  0,1,0,1,0,1,0,1), // elShuffleForRight6
    XBYTECONST(2,3,2,3,4,5,6,7,  8,9,10,11,12,13,14,15), // elShuffleForLeft0
    XBYTECONST(4,5,4,5,4,5,6,7,  8,9,10,11,12,13,14,15), // elShuffleForLeft1
    XBYTECONST(6,7,6,7,6,7,6,7,  8,9,10,11,12,13,14,15), // elShuffleForLeft2
    XBYTECONST(8,9,8,9,8,9,8,9,  8,9,10,11,12,13,14,15), // elShuffleForLeft3
    XBYTECONST(10,11,10,11,10,11,10,11, 10,11,10,11,12,13,14,15), // elShuffleForLeft4
    XBYTECONST(12,13,12,13,12,13,12,13, 12,13,12,13,12,13,14,15), // elShuffleForLeft5
    XBYTECONST(14,15,14,15,14,15,14,15, 14,15,14,15,14,15,14,15), // elShuffleForLeft6
    XCONST(88.3762626647949f), // exp_hi
    XCONST(-88.3762626647949f), // exp_lo
    XCONST(1.44269504088896341f), // cephes_LOG2EF
    XCONST(0.693359375f), // cephes_exp_C1
    XCONST(-2.12194440e-4f), // cephes_exp_C2
    XCONST(1.9875691500E-4f), // cephes_exp_p0
    XCONST(1.3981999507E-3f), // cephes_exp_p1
    XCONST(8.3334519073E-3f), // cephes_exp_p2
    XCONST(4.1665795894E-2f), // cephes_exp_p3
    XCONST(1.6666665459E-1f), // cephes_exp_p4
    XCONST(5.0000001201E-1f), // cephes_exp_p5
    XCONST(0.707106781186547524f), // cephes_SQRTHF
    XCONST(7.0376836292E-2f), // cephes_log_p0
    XCONST(-1.1514610310E-1f), // cephes_log_p1
    XCONST(1.1676998740E-1f), // cephes_log_p2
    XCONST(-1.2420140846E-1f), // cephes_log_p3
    XCONST(+1.4249322787E-1f), // cephes_log_p4
    XCONST(-1.6668057665E-1f), // cephes_log_p5
    XCONST(+2.0000714765E-1f), // cephes_log_p6
    XCONST(-2.4999993993E-1f), // cephes_log_p7
    XCONST(+3.3333331174E-1f) // cephes_log_p8
};


#define CPTR(x) (xmmword_ptr[constptr + (x) * 16])

// AVX2 stuff
// constants for ymm

#undef XCONST
#define XCONST(x) { x, x, x, x, x, x, x, x }

alignas(32) static const FloatIntUnion logexpconst_avx[][8] = {
  XCONST(0x7FFFFFFF), // absmask
  XCONST(0x7F), // c7F
  XCONST(0x00800000), // min_norm_pos
  XCONST(~0x7f800000), // inv_mant_mask
  XCONST(1.0f), // float_one
  XCONST(0.5f), // float_half
  XCONST(255.0f), // store8
  XCONST(1023.0f), // store10 (avs+)
  XCONST(4095.0f), // store12 (avs+)
  XCONST(16383.0f), // store14 (avs+)
  XCONST(65535.0f), // store16
  { 0.0f, 1.0f, 2.0f, 3.0f, 4.0f, 5.0f, 6.0f, 7.0f }, // spatialX
  { 8.0f, 9.0f, 10.0f, 11.0f, 12.0f, 13.0f, 14.0f, 15.0f }, // spatialX2
  { (int)0xFFFFFFFF, 0x00000000, 0x00000000, 0x00000000, 0, 0, 0, 0 }, // loadmask1000 not used, avx supports blendps
  { (int)0xFFFFFFFF, (int)0xFFFFFFFF, 0x00000000, 0x00000000, 0, 0, 0, 0 }, // loadmask1100 not used, avx supports blendps
  { (int)0xFFFFFFFF, (int)0xFFFFFFFF, (int)0xFFFFFFFF, 0x00000000, 0, 0, 0, 0 }, // loadmask1110 not used, avx supports blendps
  XCONST(0), // n/a elShuffleForRight0
  XCONST(0), // n/a elShuffleForRight1
  XCONST(0), // n/a elShuffleForRight2
  XCONST(0), // n/a elShuffleForRight3
  XCONST(0), // n/a elShuffleForRight4
  XCONST(0), // n/a elShuffleForRight5
  XCONST(0), // n/a elShuffleForRight6
  XCONST(0), // n/a elShuffleForLeft0
  XCONST(0), // n/a elShuffleForLeft1
  XCONST(0), // n/a elShuffleForLeft2
  XCONST(0), // n/a elShuffleForLeft3
  XCONST(0), // n/a elShuffleForLeft4
  XCONST(0), // n/a elShuffleForLeft5
  XCONST(0), // n/a elShuffleForLeft6
  XCONST(88.3762626647949f), // exp_hi
  XCONST(-88.3762626647949f), // exp_lo
  XCONST(1.44269504088896341f), // cephes_LOG2EF
  XCONST(0.693359375f), // cephes_exp_C1
  XCONST(-2.12194440e-4f), // cephes_exp_C2
  XCONST(1.9875691500E-4f), // cephes_exp_p0
  XCONST(1.3981999507E-3f), // cephes_exp_p1
  XCONST(8.3334519073E-3f), // cephes_exp_p2
  XCONST(4.1665795894E-2f), // cephes_exp_p3
  XCONST(1.6666665459E-1f), // cephes_exp_p4
  XCONST(5.0000001201E-1f), // cephes_exp_p5
  XCONST(0.707106781186547524f), // cephes_SQRTHF
  XCONST(7.0376836292E-2f), // cephes_log_p0
  XCONST(-1.1514610310E-1f), // cephes_log_p1
  XCONST(1.1676998740E-1f), // cephes_log_p2
  XCONST(-1.2420140846E-1f), // cephes_log_p3
  XCONST(+1.4249322787E-1f), // cephes_log_p4
  XCONST(-1.6668057665E-1f), // cephes_log_p5
  XCONST(+2.0000714765E-1f), // cephes_log_p6
  XCONST(-2.4999993993E-1f), // cephes_log_p7
  XCONST(+3.3333331174E-1f) // cephes_log_p8
};

#define CPTR_AVX(x) (ymmword_ptr[constptr + (x) * 32])

#define EXP_PS(x) { \
XmmReg fx, emm0, etmp, y, mask, z; \
minps(x, CPTR(elexp_hi)); \
maxps(x, CPTR(elexp_lo)); \
movaps(fx, x); \
mulps(fx, CPTR(elcephes_LOG2EF)); \
addps(fx, CPTR(elfloat_half)); \
cvttps2dq(emm0, fx); \
cvtdq2ps(etmp, emm0); \
movaps(mask, etmp); \
cmpnleps(mask, fx); \
andps(mask, CPTR(elfloat_one)); \
movaps(fx, etmp); \
subps(fx, mask); \
movaps(etmp, fx); \
mulps(etmp, CPTR(elcephes_exp_C1)); \
movaps(z, fx); \
mulps(z, CPTR(elcephes_exp_C2)); \
subps(x, etmp); \
subps(x, z); \
movaps(z, x); \
mulps(z, z); \
movaps(y, CPTR(elcephes_exp_p0)); \
mulps(y, x); \
addps(y, CPTR(elcephes_exp_p1)); \
mulps(y, x); \
addps(y, CPTR(elcephes_exp_p2)); \
mulps(y, x); \
addps(y, CPTR(elcephes_exp_p3)); \
mulps(y, x); \
addps(y, CPTR(elcephes_exp_p4)); \
mulps(y, x); \
addps(y, CPTR(elcephes_exp_p5)); \
mulps(y, z); \
addps(y, x); \
addps(y, CPTR(elfloat_one)); \
cvttps2dq(emm0, fx); \
paddd(emm0, CPTR(elc7F)); \
pslld(emm0, 23); \
mulps(y, emm0); \
x = y; }

#define LOG_PS(x) { \
XmmReg emm0, invalid_mask, mask, y, etmp, z; \
xorps(invalid_mask, invalid_mask); \
cmpnleps(invalid_mask, x); \
maxps(x, CPTR(elmin_norm_pos)); \
movaps(emm0, x); \
psrld(emm0, 23); \
andps(x, CPTR(elinv_mant_mask)); \
orps(x, CPTR(elfloat_half)); \
psubd(emm0, CPTR(elc7F)); \
cvtdq2ps(emm0, emm0); \
addps(emm0, CPTR(elfloat_one)); \
movaps(mask, x); \
cmpltps(mask, CPTR(elcephes_SQRTHF)); \
movaps(etmp, x); \
andps(etmp, mask); \
subps(x, CPTR(elfloat_one)); \
andps(mask, CPTR(elfloat_one)); \
subps(emm0, mask); \
addps(x, etmp); \
movaps(z, x); \
mulps(z, z); \
movaps(y, CPTR(elcephes_log_p0)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p1)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p2)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p3)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p4)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p5)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p6)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p7)); \
mulps(y, x); \
addps(y, CPTR(elcephes_log_p8)); \
mulps(y, x); \
mulps(y, z); \
movaps(etmp, emm0); \
mulps(etmp, CPTR(elcephes_log_q1)); \
addps(y, etmp); \
mulps(z, CPTR(elfloat_half)); \
subps(y, z); \
mulps(emm0, CPTR(elcephes_log_q2)); \
addps(x, y); \
addps(x, emm0); \
orps(x, invalid_mask); }

#define EXP_PS_AVX(x) { \
YmmReg fx, emm0, etmp, y, mask, z; \
vminps(x, x, CPTR_AVX(elexp_hi)); \
vmaxps(x, x, CPTR_AVX(elexp_lo)); \
/*vmovaps(fx, x);*/ \
/*vmulps(fx, fx, CPTR_AVX(elcephes_LOG2EF));*/ \
vmulps(fx, x, CPTR_AVX(elcephes_LOG2EF)); /* simplified above 2 lines */ \
vaddps(fx, fx, CPTR_AVX(elfloat_half)); \
vcvttps2dq(emm0, fx); \
vcvtdq2ps(etmp, emm0); \
vmovaps(mask, etmp); \
vcmpps(mask, mask, fx, _CMP_GT_OQ); /* cmpnleps */ \
vandps(mask, mask, CPTR_AVX(elfloat_one)); \
vmovaps(fx, etmp); \
vsubps(fx, fx, mask); \
/*vmovaps(etmp, fx);*/ \
/*vmulps(etmp, etmp, CPTR_AVX(elcephes_exp_C1));*/ \
vmulps(etmp, fx, CPTR_AVX(elcephes_exp_C1)); /* simplified above 2 lines */ \
/*vmovaps(z, fx); */\
/*vmulps(z, z, CPTR_AVX(elcephes_exp_C2));*/ \
vmulps(z, fx, CPTR_AVX(elcephes_exp_C2));  /* simplified above 2 lines */ \
vsubps(x, x, etmp); \
vsubps(x, x, z); \
/*vmovaps(z, x);*/ \
/*vmulps(z, z); */ \
vmulps(z, x, x); /* simplified above 2 lines */ \
vmovaps(y, CPTR_AVX(elcephes_exp_p0)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_exp_p1)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_exp_p2)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_exp_p3)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_exp_p4)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_exp_p5)); \
vmulps(y, y, z); \
vaddps(y, y, x); \
vaddps(y, y, CPTR_AVX(elfloat_one)); \
vcvttps2dq(emm0, fx); \
vpaddd(emm0, emm0, CPTR_AVX(elc7F)); \
vpslld(emm0, emm0, 23); \
vmulps(y, y, emm0); \
x = y; }

#define LOG_PS_AVX(x) { \
YmmReg emm0, invalid_mask, mask, y, etmp, z; \
vxorps(invalid_mask, invalid_mask, invalid_mask); \
vcmpps(invalid_mask, invalid_mask, x, _CMP_GT_OQ); /* cmpnleps */ \
vmaxps(x, x, CPTR_AVX(elmin_norm_pos)); \
vmovaps(emm0, x); \
vpsrld(emm0, emm0, 23); \
vandps(x, x, CPTR_AVX(elinv_mant_mask)); \
vorps(x, x, CPTR_AVX(elfloat_half)); \
vpsubd(emm0, emm0, CPTR_AVX(elc7F)); \
vcvtdq2ps(emm0, emm0); \
vaddps(emm0, emm0, CPTR_AVX(elfloat_one)); \
vmovaps(mask, x); \
vcmpps(mask, mask, CPTR_AVX(elcephes_SQRTHF), _CMP_LT_OQ); /* cmpltps */ \
vmovaps(etmp, x); \
vandps(etmp, etmp, mask); \
vsubps(x, x, CPTR_AVX(elfloat_one)); \
vandps(mask, mask, CPTR_AVX(elfloat_one)); \
vsubps(emm0, emm0, mask); \
vaddps(x, x, etmp); \
vmovaps(z, x); \
vmulps(z, z, z); \
vmovaps(y, CPTR_AVX(elcephes_log_p0)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p1)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p2)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p3)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p4)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p5)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p6)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p7)); \
vmulps(y, y, x); \
vaddps(y, y, CPTR_AVX(elcephes_log_p8)); \
vmulps(y, y, x); \
vmulps(y, y, z); \
vmovaps(etmp, emm0); \
vmulps(etmp, etmp, CPTR_AVX(elcephes_log_q1)); \
vaddps(y, y, etmp); \
vmulps(z, z, CPTR_AVX(elfloat_half)); \
vsubps(y, y, z); \
vmulps(emm0, emm0, CPTR_AVX(elcephes_log_q2)); \
vaddps(x, x, y); \
vaddps(x, x, emm0); \
vorps(x, x, invalid_mask); }

// return (x - std::round(x / d)*d);
#define FMOD_PS(x, d) { \
XmmReg aTmp; \
movaps(aTmp, x); \
divps(aTmp, d); \
cvttps2dq(aTmp,aTmp); \
cvtdq2ps(aTmp,aTmp); \
mulps(aTmp, d); \
subps(x, aTmp); }

#define FMOD_PS_AVX(x, d) { \
YmmReg aTmp; \
vdivps(aTmp, x, d); \
vcvttps2dq(aTmp,aTmp); \
vcvtdq2ps(aTmp,aTmp); \
vmulps(aTmp, aTmp, d); \
vsubps(x, x, aTmp); }

struct ExprEval : public jitasm::function<void, ExprEval, uint8_t *, const intptr_t *, intptr_t, intptr_t> {

  std::vector<ExprOp> ops;
  int numInputs;
  int cpuFlags;
  int planeheight;
  int planewidth;
  bool singleMode;
  int labelCount; // to have unique label strings

  std::string getLabelCount()
  {
    return std::to_string(++labelCount);
  }

  ExprEval(std::vector<ExprOp> &ops, int numInputs, int cpuFlags, int planewidth, int planeheight, bool singleMode) : ops(ops), numInputs(numInputs), cpuFlags(cpuFlags), 
    planewidth(planewidth), planeheight(planeheight), singleMode(singleMode), labelCount(0) {}

  __forceinline void doMask(XmmReg &r, Reg &constptr, int _planewidth)
  {
    switch (_planewidth & 3) {
    case 1: andps(r, CPTR(loadmask1000)); break;
    case 2: andps(r, CPTR(loadmask1100)); break;
    case 3: andps(r, CPTR(loadmask1110)); break;
    }
  }

  template<bool processSingle, bool maskUnused>
  __forceinline void processingLoop(Reg &regptrs, XmmReg &zero, Reg &constptr, Reg &SpatialY)
  {
    std::list<std::pair<XmmReg, XmmReg>> stack;
    std::list<XmmReg> stack1;

    const int pixels_per_cycle = processSingle ? 4 : 8;

    const bool maskIt = (maskUnused && ((planewidth & 3) != 0));

    for (const auto &iter : ops) {
      if (iter.op == opLoadSpatialX) {
        if (processSingle) {
          XmmReg r1;
          movd(r1, dword_ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
          shufps(r1, r1, 0);
          cvtdq2ps(r1, r1);
          addps(r1, CPTR(spatialX));
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          movd(r1, dword_ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
          shufps(r1, r1, 0);
          cvtdq2ps(r1, r1);
          movaps(r2, r1);
          addps(r1, CPTR(spatialX));
          addps(r2, CPTR(spatialX2));
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSpatialY) {
        if (processSingle) {
          XmmReg r1;
          movd(r1, SpatialY);
          shufps(r1, r1, 0);
          cvtdq2ps(r1, r1);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          movd(r1, SpatialY);
          shufps(r1, r1, 0);
          cvtdq2ps(r1, r1);
          movaps(r2, r1);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadInternalVar) {
        if (processSingle) {
          XmmReg r1;
          movd(r1, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INTERNAL_VARIABLES)]);
          shufps(r1, r1, 0);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          movd(r1, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INTERNAL_VARIABLES)]);
          shufps(r1, r1, 0);
          movaps(r2, r1);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16 || iter.op == opLoadRelSrcF32) {
        // either dx or dy is nonzero
        // common part follows for single 4 pixels/cycle and dual 8 pixels/cycle
        Reg newx;
        if (iter.dx != 0) {
          mov(newx, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]); // original base
          add(newx, iter.dx); // new base
        }

        Reg a;
        mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]); // current pixel group of current line
                                                                                       // adjust read pointer vertically for nonzero dy, keep 0..height-1 limits
        if (iter.dy < 0) {
          // Read from above
          Reg dy, sy;
          mov(sy, SpatialY);
          mov(dy, -iter.dy); // dy = -dy; 
          cmp(dy, sy);
          cmovg(dy, sy); // mov if greater: if (dy > SpatialY) dy = SpatialY;
#ifdef _M_X64
          imul(dy, qword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_STRIDES)]); // dy * stride
#else
          imul(dy, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_STRIDES)]); // dy * stride
#endif
          sub(a, dy); // a -= dy * stride
        }
        else if (iter.dy > 0) {
          // Read from bottom
          Reg dy, sy;
          mov(sy, planeheight - 1);
          sub(sy, SpatialY);
          mov(dy, iter.dy);
          cmp(dy, sy);
          cmovg(dy, sy); // mov if greater: if (dy > (planeheight - 1) - SpatialY) dy = SpatialY;
#ifdef _M_X64
          imul(dy, qword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_STRIDES)]); // dy * stride
#else
          imul(dy, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_STRIDES)]); // dy * stride
#endif
          add(a, dy); // a += dy * stride
        }

        // dy shift is done already. newx holds xcounter + dx
        // Cases: 
        // ReadBefore: xcounter + dx < 0 (only when dx < 0): 
        //   FullReadBefore: dx <= pixels_per_cycle: clone leftmost pixel to each pixel posision in the group
        //   PartialReadBefore: pixels_per_cycle < dx < 0: close leftmost pixel to -dx positions
        // NormalRead: 0 <= xcounter + dx < planewidth - (pixels_per_cycle - 1) (can read whole pixel group)
        // OverRead:
        //   PartialOverRead when pixel at (planewidth-1) is current read position
        //   PartialOverRead when pixel at (planewidth-1) is after current read position
        //   FullOverRead: clone pixel at (planewidth-1) to each pixel posision in the group
        if (processSingle) {
          // LoadRel8/16/32, single register mode 1x4 pixels

          // Use getLabelCount: names should be unique across multiple calls to processingLoop
          std::string LabelNeg = "neg" + getLabelCount();
          std::string LabelOver = "over" + getLabelCount();
          std::string LabelEnd = "end" + getLabelCount();

          XmmReg r1;

          if (iter.dx < 0) { // Optim: read from left is possible only for dx<0 case
            cmp(newx, 0);
            jl(LabelNeg); // newx < 0, read (partially or fully) from before the leftmost pixel
          }
          if (iter.dx != 0) { // Optim: read after rightmost pixel is possible only for dx>0 case
                              // Also check for dx<0, because of possible memory overread
                              // e.g.: planewidth = 64, dx = -1, 16 bit pixels, 16 bytes/cycle, reading from offsets -1(0), 15, 31, 47, then 63
                              // When we read 16 bytes from offset 63, we are overaddressing the 64 byte scanline, 
                              // which may give access violation when pointer is in the most bottom line.
            cmp(newx, planewidth - (pixels_per_cycle - 1)); // read (partially of fully) after the rightmost pixel
            jge(LabelOver);
          }

          // It's safe to read the whole pixel group
          int offset;
          if (iter.op == opLoadRelSrc8)
            offset = iter.dx;
          else if (iter.op == opLoadRelSrc16)
            offset = iter.dx * sizeof(uint16_t);
          else if (iter.op == opLoadRelSrcF32)
            offset = iter.dx * sizeof(float);

          if (iter.op == opLoadRelSrc8) {
            movd(r1, dword_ptr[a + offset]); // 4 pixels, 4 bytes
            punpcklbw(r1, zero);
            punpcklwd(r1, zero);
            cvtdq2ps(r1, r1);
          }
          else if (iter.op == opLoadRelSrc16) {
            movq(r1, mmword_ptr[a + offset]); // 4 pixels, 8 bytes
            punpcklwd(r1, zero);
            cvtdq2ps(r1, r1);
          }
          else if (iter.op == opLoadRelSrcF32) {
            if (iter.dx % 4 == 0)
              movdqa(r1, xmmword_ptr[a + offset]); // 4 pixels, 16 bytes aligned
            else
              movdqu(r1, xmmword_ptr[a + offset]); // 4 pixels, 16 bytes unaligned
          }
          if (iter.dx != 0) {
            jmp(LabelEnd); // generate jump only when over/negative branches exist
          }

          if (iter.dx != 0) {
            L(LabelOver);
            std::string PartialOverread = "PartialOverread" + getLabelCount();
            std::string NoFullOverReadFromNewX = "NoFullOverReadFromNewX" + getLabelCount();
            std::string labelDoOver = "DoOver" + getLabelCount();

            if (iter.dx > 0) { // FullOverRead possible only when dx>0
              cmp(newx, planewidth);
              jl(PartialOverread); // if newx < planewidth ->

                                   // case: FullOver
                                   // even the first pixel to read is beyond the end of line
                                   // We have to clone the rightmost pixel from (planewidth-1)
              if (iter.op == opLoadRelSrc8) {
                sub(a, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                add(a, planewidth - 1);
                // reuse newx
                movzx(newx, byte_ptr[a]);
                movd(r1, newx);
                punpcklbw(r1, zero); // words
                pshufb(r1, CPTR(elShuffleForRight6)); // duplicate last word to all

                punpcklwd(r1, zero);
                cvtdq2ps(r1, r1);
              }
              else if (iter.op == opLoadRelSrc16) {
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 1); // for 16 bit 2*xcounter
                sub(a, tmp);
                add(a, (planewidth - 1) * 2);
                // reuse newx
                movzx(newx, word_ptr[a]);
                movd(r1, newx);
                pshufb(r1, CPTR(elShuffleForRight6)); // duplicate last word to all

                punpcklwd(r1, zero);
                cvtdq2ps(r1, r1);
              }
              else if (iter.op == opLoadRelSrcF32) {
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 2); // for 32 bit 4*xcounter
                sub(a, tmp);
                add(a, (planewidth - 1) * 4);
                movd(r1, dword_ptr[a]);
                pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));
              }
              jmp(LabelEnd);
            } // full OverRead path, needed when iter.dx>0

              // case: Partial overread
              // read the block, then clone the last valid pixel from position (planewidth-1)
              // problem: newx is not aligned
            L(PartialOverread);

            // planewidth == 14  dx=7   newx = 0+7=7, newx>=planewidth-7, then not newx>=planewidth => newx = 13 => planewidth-newx = 1
            // sample 1: newx is in different segment than planewidth-1
            // [xcounter]
            // [a]           [newx]    [pw-1]
            // V             V           V
            // 0 1 2 3 4 5 6 7 8 9 A B C D e f g h i j k l
            //               P Q R S T U V w              need this
            //               0 1 2 3 4 5 6 7              we can read this
            //               P Q R S T U V w              last pixel is beyond
            //               P Q R S T U V V              need this
            // sample 2: newx is in the same segment than planewidth-1
            // planewidth == 13  dx=3
            //           [xcounter] [newx]
            //                [a][newx][pw-1]
            //                 V     V V
            // 0 1 2 3 4 5 6 7 8 9 A B C d e f g h i j
            //                       P Q x x x x x x        need this
            //                       P Q Q Q Q Q Q Q        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            // when newx and (planewidth-1) are in different segments then we read from newx
            Reg tmp;
            mov(tmp, newx);
            and_(tmp, ~(pixels_per_cycle - 1));
            cmp(tmp, (planewidth & ~(pixels_per_cycle - 1)));
            jle(NoFullOverReadFromNewX); // jump if (newx and ~0x07) < (planewidth & ~0x07) (in another segment)

                                         // read from current (last) pointer,
            if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16) {
              if (iter.op == opLoadRelSrc8) {
                movd(r1, dword_ptr[a]); // 4 pixels, 4 bytes
                punpcklbw(r1, zero); // words
              }
              else { // opLoadRel16
                movq(r1, mmword_ptr[a]); // 8 pixels, 16 bytes, here still aligned
              }
              /*
              psrldq(r1, ((planewidth - 1) & (pixels_per_cycle - 1)) * sizeof(uint16_t)); // Shift right by (planewidth - 1) & 7 to lose low words
              sub(newx, planewidth - (pixels_per_cycle - 1)); // find out shuffle pointer -1, ... -7 -> 6 ... 0
              shl(newx, 4); // *16 for shuffle table
              // LabelDoOver copied here
              // reuse a : Reg shuffleTable;
              lea(a, CPTR(elShuffleForRight0)); // ptr for word shuffle
              pshufb(r1, xmmword_ptr[a + newx]);
              */
              punpcklwd(r1, zero);
              cvtdq2ps(r1, r1);
              //jmp(LabelEnd);
              //jmp(labelDoOver);
            }
            else if (iter.op == opLoadRelSrcF32) {
              // omg it's complicated
              movdqa(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes, here still aligned 
            }
            int bytes_to_shift = ((planewidth - 1) & (pixels_per_cycle - 1)) * sizeof(float);
            if (bytes_to_shift > 0) {
              psrldq(r1, bytes_to_shift);
              switch (bytes_to_shift) { // 4, 8, 12
              case 4:
                pshufd(r1, r1, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6));
                break;
              case 8:
                pshufd(r1, r1, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6));
                break;
              case 12:
                pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));
                break;
              }
            }
            jmp(LabelEnd);
            //}

            L(NoFullOverReadFromNewX);
            // read from newx
            if (iter.op == opLoadRelSrc8) {
              sub(a, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]); // back x counter bytes to the beginning
              add(a, newx);     // new position
              movd(r1, dword_ptr[a]); // 4 pixels, 4 bytes 
              punpcklbw(r1, zero); // words
                                   /*
                                   // no shift here, just duplicate appropriate pixel into the high ones
                                   mov(newx, (6 - ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1))) << 4); // find out shuffle pointer
                                   // shuffle by the pattern, table offset in newx, and finalizes
                                   // here r1 contains words
                                   // todo direct load
                                   Reg shuffleTable;
                                   lea(shuffleTable, CPTR(elShuffleForRight4)); // for dual: elShuffleForRight0
                                   add(shuffleTable, newx);
                                   pshufb(r1, xmmword_ptr[shuffleTable]);
                                   */
              punpcklwd(r1, zero);
              cvtdq2ps(r1, r1);
            }
            else if (iter.op == opLoadRelSrc16) {
              //a = a - 2 * xcounter + 2 * newx;
              sub(newx, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
              shl(newx, 1);
              add(a, newx);
              movq(r1, mmword_ptr[a]); // 4 pixels, 8 bytes 
                                       // no shift here, just duplicate appropriate pixel into the high ones
                                       /*
                                       // reuse newx
                                       mov(newx, (6 - ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1))) << 4); // find out shuffle pointer
                                       // ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1))    keep
                                       //                         0                                ShuffleForRight6     keep word #0, spread it to 1..7
                                       //                         1                                ShuffleForRight5     keep word #0..1, spread it to 2..7
                                       //
                                       //                         6                                ShuffleForRight0     keep word #0..6, spread it to 7..7
                                       // continues on labelDoOver
                                       // todo direct load
                                       Reg shuffleTable;
                                       lea(shuffleTable, CPTR(elShuffleForRight4)); // for dual: elShuffleForRight0
                                       add(shuffleTable, newx);
                                       pshufb(r1, xmmword_ptr[shuffleTable]);
                                       */
              punpcklwd(r1, zero);
              cvtdq2ps(r1, r1);
            }
            else if (iter.op == opLoadRelSrcF32) {
              //a = a - 4 * xcounter + 4 * newx;
              sub(newx, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
              shl(newx, 2);
              add(a, newx);


              // no real shift here, just duplicate appropriate pixel into the high ones. But we have two registers
              movdqu(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes, no need for upper 4 pixels
            }
            // we have 4 floats here in r1, common part
            int what = ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1));
            //                        what
            //                         0     ShuffleForRight2_32  keep r1 dword #0   , spread it to 1..3, then spread r1.3 to r2 (ShuffleForRight2_32(r2,r1)
            //                         1     ShuffleForRight1_32  keep r1 dword #0..1, spread it to 2..3, then spread r1.3 to r2
            //                         2     ShuffleForRight0_32  keep r1 dword #0..2, spread it to 3   , then spread r1.3 to r2
            //                         3                          keep r1 dword #0..3,                   , then spread r1.3 to r2
            //                         4                          keep r1 dword #0..3,                   , keep r2 dword #0   , spread it to 1..3
            //                         5                          keep r1 dword #0..3,                   , keep r2 dword #0..1, spread it to 2..3
            //                         6                          keep r1 dword #0..3,                   , keep r2 dword #0..2, spread it to    3

            switch (what) {
            case 0:
              pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // fill 3 upper dwords of r1 from r1.0
              break;
            case 1:
              pshufd(r1, r1, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6)); // fill 2 upper dwords of r1 from r1.1
              break;
            case 2:
              pshufd(r1, r1, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6)); // fill 1 upper dwords of r1 from r1.2
              break;
            }
            // continues on labelEnd
            //}
            if (iter.dx < 0)
              jmp(LabelEnd);
          } // over: iter.dx != 0
          if (iter.dx < 0) {
            L(LabelNeg);
            // read from negative area on the left side, read exactly from 0th, then shift
            // When reading from negative x coordinates we read exactly from 0th, then shift and duplicate
            // For extreme minus offsets we duplicate 0th (leftmost) pixel to each position
            // example: dx = -1
            // -1 0  1  2  3  4  5  6  7 
            // A  A  B  C  D  E  F  G         we need this
            //    A  B  C  D  E  F  G  H      read [0]
            //    0  A  B  C  D  E  F  G  H   shift
            //    A  A  B  C  D  E  F  G  H   duplicate by shuffle
            if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16) {
              if (iter.op == opLoadRelSrc8) {
                sub(a, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]); // go back to the beginning
                movd(r1, dword_ptr[a]); // 8 pixels, 8 bytes
                punpcklbw(r1, zero); // bytes to words
              }
              else if (iter.op == opLoadRelSrc16) {
                // go back to the beginning, in 16 bit, *2 
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 1); // for 16 bit 2*xcounter
                sub(a, tmp);
                movq(r1, mmword_ptr[a]); // 8 pixels, 16 bytes
              }
              punpcklwd(r1, zero);
              cvtdq2ps(r1, r1);
            }
            else if (iter.op == opLoadRelSrcF32) {
              Reg tmp;
              mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
              shl(tmp, 2); // *4
              sub(a, tmp);
              movdqa(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes aligned
            }
            std::string PartialReadBefore = "PartialReadBefore" + getLabelCount();

            cmp(newx, -pixels_per_cycle);
            jg(PartialReadBefore);
            // FullReadBefore: newx <= -pixels_per_cycle, clone 0th (leftmost) pixel to all
            pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));
            jmp(LabelEnd);

            L(PartialReadBefore);
            // -pixels_per_cycle < newx < 0
            int bytes_to_shift = sizeof(float) * min(pixels_per_cycle - 1, (-iter.dx) & (pixels_per_cycle - 1));
            //    shift bytes             
            //         4                  r1 << 4     shuffle r1.1 to r1.0-0
            //         8                  r1 << 8     shuffle r1.2 to r1.0-1
            //         12                 r1 << 12    shuffle r1.3 to r1.0-2
            pslldq(r1, bytes_to_shift); // todo: shift + shuffle = single shuffle

            switch (bytes_to_shift) { // 4, 8, 12
            case 4:
              pshufd(r1, r1, (1 << 0) | (1 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft0_32 // shuffle r1.1 to r1.0-0
              break;
            case 8:
              pshufd(r1, r1, (2 << 0) | (2 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft1_32 // shuffle r1.2 to r1.0-1
              break;
            case 12:
              pshufd(r1, r1, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // elShuffleForLeft2_32 // shuffle r1.3 to r1.0-2
              break;
            }
          } // negative
          L(LabelEnd);
          stack1.push_back(r1);
        } // end of single Relative Mode
        else {
          // LoadRel8/16/32, dual register mode 2x4 pixels

          // Use getLabelCount: names should be unique across multiple calls to processingLoop
          std::string LabelNeg = "neg" + getLabelCount();
          std::string LabelOver = "over" + getLabelCount();
          std::string LabelEnd = "end" + getLabelCount();

          XmmReg r1, r2;

          // damn, when the order of the two comparisons is exchanged, bad code is generated for dx=-1 (expr=x[-1]).
          // jitasm cannot guess the proper register for 'a', it uses register 'xcounter' instead
          // maybe the jump order has to match the label order?
          // good: LabelOver, LabelNeg. Bad: LabelNeg, LabelOver
          // But it's not true. Now x[-2] fails for 32 bit clip
          if (iter.dx < 0) { // Optim: read from left is possible only for dx<0 case
            cmp(newx, 0);
            jl(LabelNeg); // newx < 0, read (partially or fully) from before the leftmost pixel
          }
          if (iter.dx != 0) {
            // Also check for dx<0, because of possible memory overread
            // e.g.: planewidth = 64, dx = -1, 16 bit pixels, 16 bytes/cycle, reading from offsets -1(0), 15, 31, 47, then 63
            // When we read 16 bytes from offset 63, we are overaddressing the 64 byte scanline, 
            // which may give access violation when pointer is in the most bottom line.
            cmp(newx, planewidth - (pixels_per_cycle - 1)); // read (partially of fully) after the rightmost pixel
            jge(LabelOver);
          }
          /*
          if (iter.dx < 0) { // Optim: read from left is possible only for dx<0 case
          cmp(newx, 0);
          jl(LabelNeg); // newx < 0, read (partially or fully) from before the leftmost pixel
          }
          */

          // It's safe to read the whole pixel group
          int offset;
          if (iter.op == opLoadRelSrc8)
            offset = iter.dx;
          else if (iter.op == opLoadRelSrc16)
            offset = iter.dx * sizeof(uint16_t);
          else if (iter.op == opLoadRelSrcF32)
            offset = iter.dx * sizeof(float);

          if (iter.op == opLoadRelSrc8) {
            movq(r1, mmword_ptr[a + offset]); // 8 pixels, 8 bytes
            punpcklbw(r1, zero);
            movdqa(r2, r1);
            punpcklwd(r1, zero);
            punpckhwd(r2, zero);
            cvtdq2ps(r1, r1);
            cvtdq2ps(r2, r2);
          }
          else if (iter.op == opLoadRelSrc16) {
            if (iter.dx % 8 == 0)
              movdqa(r1, xmmword_ptr[a + offset]); // 8 pixels 16 byte boundary, aligned
            else
              movdqu(r1, xmmword_ptr[a + offset]);
            movdqa(r2, r1);
            punpcklwd(r1, zero);
            punpckhwd(r2, zero);
            cvtdq2ps(r1, r1);
            cvtdq2ps(r2, r2);
          }
          else if (iter.op == opLoadRelSrcF32) {
            if (iter.dx % 4 == 0) {
              movdqa(r1, xmmword_ptr[a + offset]); // // 4 pixels 16 byte boundary, aligned
              movdqa(r2, xmmword_ptr[a + offset + 16]);
            }
            else {
              movdqu(r1, xmmword_ptr[a + offset]); // unaligned
              movdqu(r2, xmmword_ptr[a + offset + 16]);
            }
          }
          if (iter.dx != 0) {
            jmp(LabelEnd); // Optim: generate jump only when over/negative branches exist
          }

          if (iter.dx != 0) {
            L(LabelOver);

            // x  dx   newx
            // 8   1  8+1=9
            // planewidth == 16
            // 0 1 2 3 4 5 6 7 8 9 A B C D E F g h i j
            //                   P Q R S T U V x            need this
            //                   P Q R S T U V V            duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 1 2 3 4 5 6 7 -              Shift right by dx to lose low bytes
            //                 1 2 3 4 5 6 7 7              have to make this one from it. Only the first planewidth-newx (7) bytes valid
            // x  dx   newx
            // 8   3  8+3=11
            // planewidth == 15
            // 0 1 2 3 4 5 6 7 8 9 A B C D E f g h i j
            //                       P Q R S T x x x        need this
            //                       P Q R S S S S S        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 3 4 5 6 7 - - -              Shift right by dx to lose low bytes
            //                 3 4 5 6 6 6 6 6              have to make this one from it. Only the first planewidth-newx (4) bytes valid
            // planewidth == 14
            // 0 1 2 3 4 5 6 7 8 9 A B C D e f g h i j
            //                       P Q R x x x x x        need this
            //                       P Q R R R R R R        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 3 4 5 6 7 - - -              Shift right by dx to lose low bytes
            //                 3 4 5 5 5 5 5 5              have to make this one from it. Only the first planewidth-newx (3) bytes valid
            // special case: full read from beyond line
            // planewidth == 14  dx=6   newx = 8+6=14, newx>=planewidth => newx = 13 => planewidth-newx = 1
            // 0 1 2 3 4 5 6 7 8 9 A B C D e f g h i j k l
            //                             ? ? ? ? ? ? ? ?  need this, but overread
            //                             ? ? ? ? ? ? ? ?  duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 5 6 7 - - - - -              Shift right by not dx but (planewidth-1)&7, it's 5 in this example, to lose low bytes
            //                 5 5 5 5 5 5 5 5              case(1): duplicate very first
            // planewidth == 14  dx=7   newx = 0+7=7, newx>=planewidth-7, then not newx>=planewidth => newx = 13 => planewidth-newx = 1
            // 0 1 2 3 4 5 6 7 8 9 A B C D e f g h i j k l
            //               P Q R S T U V w                need this
            //               P Q R S T U V V                duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 5 6 7 - - - - -              Shift right by not dx but (planewidth-1)&7, it's 5 in this example, to lose low bytes
            //                 5 5 5 5 5 5 5 5              case(1): duplicate very first
            // planewidth == 14  dx=7   newx = 8+7=15, newx>=planewidth => newx = 13 => planewidth-newx = 1
            //                             ? ? ? ? ? ? ? ?  need this, but overread
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 5 - - - - - - -              Shift right to have the last pixel
            //                 5 5 5 5 5 5 5 5              case(1): duplicate very first
            // planewidth == 13
            // 0 1 2 3 4 5 6 7 8 9 A B C d e f g h i j
            //                       P Q x x x x x x        need this
            //                       P Q Q Q Q Q Q Q        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 3 4 5 6 7 - - -              Shift right by dx to lose low bytes
            //                 3 4 4 4 4 4 4 4              have to make this one from it. Only the first planewidth-newx (2) bytes valid
            // planewidth == 12
            // 0 1 2 3 4 5 6 7 8 9 A B c d e f g h i j
            //                       P x x x x x x x        need this
            //                       P P P P P P P P        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            //                 3 4 5 6 7 - - -              Shift right by dx to lose low bytes
            //                 3 3 3 3 3 3 3 3              have to make this one from it. Only the first planewidth-newx (1) bytes valid

            // duplicate highest, make a shuffle table by planewidth-newx (1..7)
            // planewidth - newx    newx-pw  newx-pw+7    shuffle
            //                               newx-(pw-7)
            //           1            -1         6        0->0 0->1 0->2 0->3 0->4 0->5 0->6 0->7 elSuffleForRight6 (lowest to everywhere)
            //           2            -2         5        0->0 1->1 1->2 1->3 1->4 1->5 1->6 1->7 elSuffleForRight5 (lowest two remains then second duplicates)
            //           3            -3         4        0->0 1->1 2->2 2->3 2->4 2->5 2->6 2->7 elSuffleForRight4 (lowest three remains then third duplicates)
            //           4            -4         3        0->0 1->1 2->2 3->3 3->4 3->5 3->6 3->7 elSuffleForRight3 
            //           5            -5         2        0->0 1->1 2->2 3->3 4->4 4->5 4->6 4->7 elSuffleForRight2 
            //           6            -6         1        0->0 1->1 2->2 3->3 4->4 5->5 5->6 5->7 elSuffleForRight1 
            //           7            -7         0        0->0 1->1 2->2 3->3 4->4 5->5 6->6 6->7 elSuffleForRight0 (lowest seven remains then seventh duplicates)
            // in extreme case (read all beyond last pixel): newx >= planewidth: ==> case of elSuffleForRight6

            // shuffleTable = elShuffleForRight0 + 16*(newx-(planewidth-7))
            std::string PartialOverread = "PartialOverread" + getLabelCount();
            std::string NoFullOverReadFromNewX = "NoFullOverReadFromNewX" + getLabelCount();
            std::string labelDoOver = "DoOver" + getLabelCount();

            if (iter.dx > 0) { // FullOverRead possible only when dx>0
              cmp(newx, planewidth);
              jl(PartialOverread); // if newx < planewidth ->

                                   // case: FullOver
                                   // even the first pixel to read is beyond the end of line
                                   // We have to clone the rightmost pixel from (planewidth-1)
              if (iter.op == opLoadRelSrc8) {
                sub(a, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                add(a, planewidth - 1);
                // reuse newx
                movzx(newx, byte_ptr[a]);
                movd(r1, newx);
                punpcklbw(r1, zero); // words
                pshufb(r1, CPTR(elShuffleForRight6)); // duplicate last word to all

                movdqa(r2, r1);
                punpcklwd(r1, zero);
                punpckhwd(r2, zero);
                cvtdq2ps(r1, r1);
                cvtdq2ps(r2, r2);
              }
              else if (iter.op == opLoadRelSrc16) {
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 1); // for 16 bit 2*xcounter
                sub(a, tmp);
                add(a, (planewidth - 1) * 2);
                // reuse newx
                movzx(newx, word_ptr[a]);
                movd(r1, newx);
                pshufb(r1, CPTR(elShuffleForRight6)); // duplicate last word to all

                movdqa(r2, r1);
                punpcklwd(r1, zero);
                punpckhwd(r2, zero);
                cvtdq2ps(r1, r1);
                cvtdq2ps(r2, r2);
              }
              else if (iter.op == opLoadRelSrcF32) {
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 2); // for 32 bit 4*xcounter
                sub(a, tmp);
                add(a, (planewidth - 1) * 4);
                movd(r1, dword_ptr[a]);
                pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));
                movdqa(r2, r1);
              }
              jmp(LabelEnd);
            } // full OverRead path, needed when iter.dx>0

              // case: Partial overread
              // read the block, then clone the last valid pixel from position (planewidth-1)
              // problem: newx is not aligned
            L(PartialOverread);
            // planewidth == 14  dx=7   newx = 0+7=7, newx>=planewidth-7, then not newx>=planewidth => newx = 13 => planewidth-newx = 1
            // sample 1: newx is in different segment than planewidth-1
            // [xcounter]
            // [a]           [newx]    [pw-1]
            // V             V           V
            // 0 1 2 3 4 5 6 7 8 9 A B C D e f g h i j k l
            //               P Q R S T U V w              need this
            //               0 1 2 3 4 5 6 7              we can read this
            //               P Q R S T U V w              last pixel is beyond
            //               P Q R S T U V V              need this
            // sample 2: newx is in the same segment than planewidth-1
            // planewidth == 13  dx=3
            //           [xcounter] [newx]
            //                [a][newx][pw-1]
            //                 V     V V
            // 0 1 2 3 4 5 6 7 8 9 A B C d e f g h i j
            //                       P Q x x x x x x        need this
            //                       P Q Q Q Q Q Q Q        duplicated the last valid pixel
            //                 0 1 2 3 4 5 6 7              we can read this
            // when newx and (planewidth-1) are in different segments then we read from newx
            Reg tmp;
            mov(tmp, newx);
            and_(tmp, ~(pixels_per_cycle - 1));
            cmp(tmp, (planewidth & ~(pixels_per_cycle - 1)));
            jle(NoFullOverReadFromNewX); // jump if (newx and ~0x07) < (planewidth & ~0x07) (in another segment)

                                         // read from current (last) pointer,
            if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16) {
              if (iter.op == opLoadRelSrc8) {
                movq(r1, mmword_ptr[a]); // 8 pixels, 8 bytes
                punpcklbw(r1, zero); // words
              }
              else { // opLoadRel16
                movdqa(r1, xmmword_ptr[a]); // 8 pixels, 16 bytes, here still aligned
              }
              psrldq(r1, ((planewidth - 1) & (pixels_per_cycle - 1)) * sizeof(uint16_t)); // Shift right by (planewidth - 1) & 7 to lose low words
              sub(newx, planewidth - (pixels_per_cycle - 1)); // find out shuffle pointer -1, ... -7 -> 6 ... 0
              shl(newx, 4); // *16 for shuffle table
                            // LabelDoOver copied here
                            // reuse a : Reg shuffleTable;
              lea(a/*shuffleTable*/, CPTR(elShuffleForRight0)); // ptr for word shuffle
                                                                //add(a/*shuffleTable*/, newx);
              pshufb(r1, xmmword_ptr[a/*shuffleTable*/ + newx]);

              movdqa(r2, r1);
              punpcklwd(r1, zero);
              punpckhwd(r2, zero);
              cvtdq2ps(r1, r1);
              cvtdq2ps(r2, r2);
              jmp(LabelEnd);
            }
            else if (iter.op == opLoadRelSrcF32) {
              // omg it's complicated

              // palignr memo
              // temp1[255:0]  ((DEST[127:0] << 128) OR SRC[127:0]) >> (imm8*8);
              // DEST[127:0]  temp1[127:0]

              int bytes_to_shift = ((planewidth - 1) & (pixels_per_cycle - 1)) * sizeof(float);
              if (bytes_to_shift < 16) {
                // src              dst
                // r2               r1
                // 15 14 13.... 0   15 14 13 ... 0
                //    15 14 13  1   0  15 14.... 1  palignr(dst, src, 1)
                //              15  14 13  ..... 15 palignr(dst, src, 15)
                movdqa(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes, here still aligned 
                movdqa(r2, xmmword_ptr[a + 16]); // 4 pixels, 16 bytes
                if (bytes_to_shift > 0) {
                  palignr(r1, r2, bytes_to_shift); // shift right dualreg. r1 is ready.
                  psrldq(r2, bytes_to_shift); // Shift right upper part
                  switch (bytes_to_shift) { // 4, 8, 12
                  case 4:
                    pshufd(r2, r2, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6)); // elShuffleForRight0_32
                    break;
                  case 8:
                    pshufd(r2, r2, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6));
                    break;
                  case 12:
                    pshufd(r2, r2, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // elShuffleForRight2_32
                    break;
                  }
                }
              }
              else if (bytes_to_shift == 16) {
                // src              dst
                // r2               r1
                // 15 14 13.... 0   15 14 13 ... 0  --> 16 bytes: r1 = r2
                movdqa(r1, xmmword_ptr[a + 16]); // 4 pixels, 16 bytes, no need [a + 0], here still aligned
                pshufd(r2, r1, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // fill r2 with highest dword of r1
              }
              else {
                // bytes to shift > 16 (20, 24, 28), ignore lower 4 pixels, move and shift and spread from upper 4 pixels
                movdqa(r1, xmmword_ptr[a + 16]); // 4 pixels, 16 bytes, no need [a + 0], here still aligned
                psrldq(r1, bytes_to_shift - 16); // Shift right upper part
                switch (bytes_to_shift) { // 0, 4, 8, 12
                case 20:
                  pshufd(r1, r1, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6)); // elShuffleForRight0_32
                  break;
                case 24:
                  pshufd(r1, r1, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6));
                  break;
                case 28:
                  pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // elShuffleForRight2_32
                  break;
                }
                pshufd(r2, r1, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // fill r2 with highest dword of r1
              }
              jmp(LabelEnd);
            }

            L(NoFullOverReadFromNewX);
            // read from newx
            //a = a - 1,2,4 * xcounter + 1,2,4 * newx;
            sub(newx, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
            if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16) {
              if (iter.op == opLoadRelSrc8)
              {
                add(a, newx);     // new position
                movq(r1, mmword_ptr[a]); // 8 pixels, 8 bytes 
                punpcklbw(r1, zero); // words
              }
              else {
                shl(newx, 1); //a = a - 2 * xcounter + 2 * newx;
                add(a, newx);
                movdqu(r1, xmmword_ptr[a]); // 8 pixels, 16 bytes 
              }
              // no shift here, just duplicate appropriate pixel into the high ones
              int what = ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1));
              switch (what) {
              case 0:
                pshufb(r1, CPTR(elShuffleForRight6));
                break;
              case 1:
                pshufb(r1, CPTR(elShuffleForRight5));
                break;
              case 2:
                pshufb(r1, CPTR(elShuffleForRight4));
                break;
              case 3:
                pshufb(r1, CPTR(elShuffleForRight3));
                break;
              case 4:
                pshufb(r1, CPTR(elShuffleForRight2));
                break;
              case 5:
                pshufb(r1, CPTR(elShuffleForRight1));
                break;
              case 6:
                pshufb(r1, CPTR(elShuffleForRight0));
                break;
              }
              movdqa(r2, r1);
              punpcklwd(r1, zero);
              punpckhwd(r2, zero);
              cvtdq2ps(r1, r1);
              cvtdq2ps(r2, r2);
              //jmp(LabelEnd);
            }
            else if (iter.op == opLoadRelSrcF32) {
              shl(newx, 2); // float: *4
              add(a, newx);

              int what = ((planewidth - iter.dx - 1) & (pixels_per_cycle - 1));
              //                        what
              //                         0     ShuffleForRight2_32  keep r1 dword #0   , spread it to 1..3, then spread r1.3 to r2 (ShuffleForRight2_32(r2,r1)
              //                         1     ShuffleForRight1_32  keep r1 dword #0..1, spread it to 2..3, then spread r1.3 to r2
              //                         2     ShuffleForRight0_32  keep r1 dword #0..2, spread it to 3   , then spread r1.3 to r2
              //                         3                          keep r1 dword #0..3,                   , then spread r1.3 to r2
              //                         4                          keep r1 dword #0..3,                   , keep r2 dword #0   , spread it to 1..3
              //                         5                          keep r1 dword #0..3,                   , keep r2 dword #0..1, spread it to 2..3
              //                         6                          keep r1 dword #0..3,                   , keep r2 dword #0..2, spread it to    3

              // no real shift here, just duplicate appropriate pixel into the high ones. But we have two registers
              if (what <= 3) {
                movdqu(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes, no need for upper 4 pixels
                switch (what) {
                case 0:
                  pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // elShuffleForRight2_32 // fill 3 upper dwords of r1 from r1.0
                  break;
                case 1:
                  pshufd(r1, r1, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6)); // fill 2 upper dwords of r1 from r1.1
                  break;
                case 2:
                  pshufd(r1, r1, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6)); // elShuffleForRight0_32 // fill 1 upper dwords of r1 from r1.2
                  break;
                }
                pshufd(r2, r1, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // fill all dwords of r2 from r1.3
              }
              else {
                movdqu(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes, low 4 pixels keep them as is
                movdqu(r2, xmmword_ptr[a + 16]); // 4 pixels, 16 bytes
                switch (what) {
                case 4:
                  pshufd(r2, r2, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // elShuffleForRight2_32 // fill 3 upper dwords of r1 from r2.0
                  break;
                case 5:
                  pshufd(r2, r2, (0 << 0) | (1 << 2) | (1 << 4) | (1 << 6)); // fill 2 upper dwords of r1 from r2.1
                  break;
                case 6:
                  pshufd(r2, r2, (0 << 0) | (1 << 2) | (2 << 4) | (2 << 6)); // elShuffleForRight0_32 // fill 1 upper dwords of r1 from r2.2
                  break;
                }
              }
              // continues on labelEnd
            }
            if (iter.dx < 0)
              jmp(LabelEnd);
          } // over: iter.dx != 0
          if (iter.dx < 0) {
            L(LabelNeg);
            // When reading from negative x coordinates we read exactly from 0th, then shift and duplicate
            // For extreme minus offsets we duplicate 0th (leftmost) pixel to each position
            // example: dx = -1
            // -1 0  1  2  3  4  5  6  7 
            // A  A  B  C  D  E  F  G         we need this
            //    A  B  C  D  E  F  G  H      read [0]
            //    0  A  B  C  D  E  F  G  H   shift
            //    A  A  B  C  D  E  F  G  H   duplicate by shuffle
            if (iter.op == opLoadRelSrc8 || iter.op == opLoadRelSrc16) {
              if (iter.op == opLoadRelSrc8) {
                sub(a, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]); // go back to the beginning
                movq(r1, mmword_ptr[a]); // 8 pixels, 8 bytes
                punpcklbw(r1, zero); // bytes to words
              }
              else if (iter.op == opLoadRelSrc16) {
                // go back to the beginning, in 16 bit, *2 
                Reg tmp;
                mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
                shl(tmp, 1); // for 16 bit 2*xcounter
                sub(a, tmp);
                movdqa(r1, xmmword_ptr[a]); // 8 pixels, 16 bytes
              }

              std::string PartialReadBefore = "PartialReadBefore" + getLabelCount();
              std::string Finalize = "Finalize" + getLabelCount();
              cmp(newx, -pixels_per_cycle); // pixels_per_cycle words
              jg(PartialReadBefore);
              // FullReadBefore: newx <= -pixels_per_cycle, clone 0th (leftmost) pixel to all
              pshufb(r1, CPTR(elShuffleForRight6)); // lowest word to all
              jmp(Finalize);
              L(PartialReadBefore);
              // -pixels_per_cycle < newx < 0
              int toShift = min(pixels_per_cycle - 1, (-iter.dx) & (pixels_per_cycle - 1));
              pslldq(r1, toShift * 2); // shift in word domain
              switch (toShift) {
              case 1: pshufb(r1, CPTR(elShuffleForLeft0)); break;
              case 2: pshufb(r1, CPTR(elShuffleForLeft1)); break;
              case 3: pshufb(r1, CPTR(elShuffleForLeft2)); break;
              case 4: pshufb(r1, CPTR(elShuffleForLeft3)); break;
              case 5: pshufb(r1, CPTR(elShuffleForLeft4)); break;
              case 6: pshufb(r1, CPTR(elShuffleForLeft5)); break;
              case 7: pshufb(r1, CPTR(elShuffleForLeft6)); break;
              }
              L(Finalize);

              movdqa(r2, r1);
              punpcklwd(r1, zero);
              punpckhwd(r2, zero);
              cvtdq2ps(r1, r1);
              cvtdq2ps(r2, r2);
            }
            else if (iter.op == opLoadRelSrcF32) {
              // negative
              // go back to the beginning, in 16 bit, *2 
              Reg tmp;
              mov(tmp, ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
              shl(tmp, 2); // go back to the beginning, in 32 bit, *4
              sub(a, tmp);

              std::string PartialReadBefore = "PartialReadBefore" + getLabelCount();

              cmp(newx, -pixels_per_cycle);
              jg(PartialReadBefore);
              // FullReadBefore: newx <= -pixels_per_cycle, clone 0th (leftmost) pixel to all
              movdqa(r1, xmmword_ptr[a]);
              pshufd(r1, r1, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6));
              movdqa(r2, r1);
              jmp(LabelEnd);

              L(PartialReadBefore);
              // -pixels_per_cycle < newx < 0
              int bytes_to_shift = sizeof(float) * min(pixels_per_cycle - 1, (-iter.dx) & (pixels_per_cycle - 1));
              //    shift bytes             
              //         4                  r2r1 << 4     shuffle r1.1 to r1.0-0
              //         8                  r2r1 << 8     shuffle r1.2 to r1.0-1
              //         12                 r2r1 << 12    shuffle r1.3 to r1.0-2
              //         16                 r2r1 << 16    -> r2 = r1,                                     , shuffle r2.0 to all r1
              //         20                 r2r1 << 20    -> r2 = r1, r2 << (20-4), shuffle r2.1 to r2.0-0, shuffle r2.0 to all r1
              //         24                 r2r1 << 24    -> r2 = r1, r2 << (24-4), shuffle r2.2 to r2.0-1, shuffle r2.0 to all r1
              //         28                 r2r1 << 28    -> r2 = r1, r2 << (28-4), shuffle r2.3 to r2.0-2, shuffle r2.0 to all r1
              if (bytes_to_shift < 16) {
                movdqa(r1, xmmword_ptr[a]); // 4 pixels, 16 bytes
                movdqa(r2, xmmword_ptr[a + 16]); // 4 pixels, 16 bytes
                                                 // 4 floats
                                                 // r2            r1
                                                 // H3 H2 H1 H0   L3 L2 L1 L0   << 1*4 byte
                                                 // H2 H1 H0 L3   L2 L1 L0 00
                                                 // H2 H1 H0 00   or
                                                 // 00 00 00 L3   L2 L1 L0 00
                psrldq(r1, 16 - bytes_to_shift);
                pslldq(r2, bytes_to_shift);
                por(r2, r1);
                movdqa(r1, xmmword_ptr[a]); // load again
                pslldq(r1, bytes_to_shift); // todo: shift + shuffle = single shuffle

                switch (bytes_to_shift) { // 4, 8, 12
                case 4:
                  pshufd(r1, r1, (1 << 0) | (1 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft0_32 // shuffle r1.1 to r1.0-0
                  break;
                case 8:
                  pshufd(r1, r1, (2 << 0) | (2 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft1_32 // shuffle r1.2 to r1.0-1
                  break;
                case 12:
                  pshufd(r1, r1, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // elShuffleForLeft2_32 // shuffle r1.3 to r1.0-2
                  break;
                }
              }
              else {
                // toShift >= 16
                //movdqa(r1, xmmword_ptr[a]); // no need for 15..31
                movdqa(r2, xmmword_ptr[a]); // 4 pixels, 16 bytes
                if (bytes_to_shift > 16)
                  pslldq(r2, bytes_to_shift - 16);

                switch (bytes_to_shift) { // 20, 24, 28
                case 20:
                  pshufd(r2, r2, (1 << 0) | (1 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft0_32 // shuffle r2.1 to r2.0-0
                  break;
                case 24:
                  pshufd(r2, r2, (2 << 0) | (2 << 2) | (2 << 4) | (3 << 6)); // elShuffleForLeft1_32 // shuffle r2.2 to r2.0-1
                  break;
                case 28:
                  pshufd(r2, r2, (3 << 0) | (3 << 2) | (3 << 4) | (3 << 6)); // elShuffleForLeft2_32 // shuffle r2.3 to r2.0-2
                  break;
                }
                pshufd(r1, r2, (0 << 0) | (0 << 2) | (0 << 4) | (0 << 6)); // shuffle r2.0 to all r1
              }
            }
          }
          L(LabelEnd);
          stack.push_back(std::make_pair(r1, r2));
        }
      } // oploadRel8/16/32
      else if (iter.op == opLoadSrc8) {
        if (processSingle) {
          XmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movd(r1, dword_ptr[a]); // 4 pixels, 4 bytes
          punpcklbw(r1, zero);
          punpcklwd(r1, zero);
          cvtdq2ps(r1, r1);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movq(r1, mmword_ptr[a]);
          punpcklbw(r1, zero);
          movdqa(r2, r1);
          punpcklwd(r1, zero);
          punpckhwd(r2, zero);
          cvtdq2ps(r1, r1);
          cvtdq2ps(r2, r2);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrc16) {
        if (processSingle) {
          XmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movq(r1, mmword_ptr[a]); // 4 pixels, 8 bytes
          punpcklwd(r1, zero);
          cvtdq2ps(r1, r1);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movdqa(r1, xmmword_ptr[a]);
          movdqa(r2, r1);
          punpcklwd(r1, zero);
          punpckhwd(r2, zero);
          cvtdq2ps(r1, r1);
          cvtdq2ps(r2, r2);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrcF32) {
        if (processSingle) {
          XmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movdqa(r1, xmmword_ptr[a]);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          movdqa(r1, xmmword_ptr[a]);
          movdqa(r2, xmmword_ptr[a + 16]);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrcF16) { // not supported in avs+
        if (processSingle) {
          XmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          vcvtph2ps(r1, qword_ptr[a]);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          vcvtph2ps(r1, qword_ptr[a]);
          vcvtph2ps(r2, qword_ptr[a + 8]);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadVar) {
        if (processSingle) {
          XmmReg r1;
          // 16 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 16 * iter.e.ival;
          movdqa(r1, xmmword_ptr[regptrs + offset]);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          // 32 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 32 * iter.e.ival;
          movdqa(r1, xmmword_ptr[regptrs + offset]);
          movdqa(r2, xmmword_ptr[regptrs + offset + 16]);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadConst) {
        if (processSingle) {
          XmmReg r1;
          Reg a;
          mov(a, iter.e.ival);
          movd(r1, a);
          shufps(r1, r1, 0);
          if (maskIt)
            doMask(r1, constptr, planewidth);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1, r2;
          Reg a;
          mov(a, iter.e.ival);
          movd(r1, a);
          shufps(r1, r1, 0);
          movaps(r2, r1);
          if (maskIt)
            doMask(r2, constptr, planewidth);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opDup) {
        if (processSingle) {
          auto p = std::next(stack1.rbegin(), iter.e.ival);
          XmmReg r1;
          movaps(r1, *p);
          stack1.push_back(r1);
        }
        else {
          auto p = std::next(stack.rbegin(), iter.e.ival);
          XmmReg r1, r2;
          movaps(r1, p->first);
          movaps(r2, p->second);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opSwap) {
        if (processSingle) {
          std::swap(stack1.back(), *std::next(stack1.rbegin(), iter.e.ival));
        }
        else {
          std::swap(stack.back(), *std::next(stack.rbegin(), iter.e.ival));
        }
      }
      else if (iter.op == opAdd) {
        if (processSingle) {
          TwoArgOp_Single(addps)
        }
        else {
          TwoArgOp(addps)
        }
      }
      else if (iter.op == opSub) {
        if (processSingle) {
          TwoArgOp_Single(subps)
        }
        else {
          TwoArgOp(subps)
        }
      }
      else if (iter.op == opMul) {
        if (processSingle) {
          TwoArgOp_Single(mulps)
        }
        else {
          TwoArgOp(mulps)
        }
      }
      else if (iter.op == opDiv) {
        if (processSingle) {
          TwoArgOp_Single(divps)
        }
        else {
          TwoArgOp(divps)
        }
      }
      else if (iter.op == opFmod) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto &t2 = stack1.back();
          FMOD_PS(t2, t1)
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto &t2 = stack.back();
          FMOD_PS(t2.first, t1.first)
          FMOD_PS(t2.second, t1.second)
        }
      }
      else if (iter.op == opMax) {
        if (processSingle) {
          TwoArgOp_Single(maxps)
        }
        else {
          TwoArgOp(maxps)
        }
      }
      else if (iter.op == opMin) {
        if (processSingle) {
          TwoArgOp_Single(minps)
        }
        else {
          TwoArgOp(minps)
        }
      }
      else if (iter.op == opSqrt) {
        if (processSingle) {
          auto &t1 = stack1.back();
          maxps(t1, zero);
          sqrtps(t1, t1);
        }
        else {
          auto &t1 = stack.back();
          maxps(t1.first, zero);
          maxps(t1.second, zero);
          sqrtps(t1.first, t1.first);
          sqrtps(t1.second, t1.second);
        }
      }
      // Integer store operations: Why sometimes C version differs from SSE: convert to int from .5 intermediates.
      // Simd version of float -> int32 (cvtps2dq) is using the SSE rounding mode "round to nearest"
      // C version is using: (uint8_t)(f + 0.5f) which turnes into cvttps, typecast to int uses trunc
      // Even for positive numbers they are not the same, when converting occurs exactly from the halfway
      // SSE is using Banker's rounding, which rounds to the nearest _even_ integer value. https://en.wikipedia.org/wiki/IEEE_754#Roundings_to_nearest
      //        C   SSE
      //  0.5   1   0
      //  1.5   2   2
      //  2.5   3   2
      //  3.5   4   4
      else if (iter.op == opStore8) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          XmmReg r1;
          Reg a;
          maxps(t1, zero);
          minps(t1, CPTR(elstore8));
          mov(a, ptr[regptrs]);
          cvtps2dq(t1, t1);    // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
          packssdw(t1, zero);  // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          packuswb(t1, zero);  // _mm_packus_epi16: 0 0 0 0 0 0 0 0 b7 b6 b5 b4 b3 b2 b1 b0
          movd(dword_ptr[a], t1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          XmmReg r1, r2;
          Reg a;
          maxps(t1.first, zero);
          maxps(t1.second, zero);
          minps(t1.first, CPTR(elstore8));
          minps(t1.second, CPTR(elstore8));
          mov(a, ptr[regptrs]);
          cvtps2dq(t1.first, t1.first);    // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
          cvtps2dq(t1.second, t1.second);  // 00 w7 00 w6 00 w5 00 w4
          // t1.second is the lo
          packssdw(t1.first, t1.second);   // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          packuswb(t1.first, zero);       // _mm_packus_epi16: 0 0 0 0 0 0 0 0 b7 b6 b5 b4 b3 b2 b1 b0
          movq(mmword_ptr[a], t1.first);
        }
      }
      else if (iter.op == opStore10 // avs+
        || iter.op == opStore12 // avs+ 
        || iter.op == opStore14 // avs+
        || iter.op == opStore16
        ) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          XmmReg r1;
          Reg a;
          maxps(t1, zero);
          switch (iter.op) {
          case opStore10:
            minps(t1, CPTR(elstore10));
            break;
          case opStore12:
            minps(t1, CPTR(elstore12));
            break;
          case opStore14:
            minps(t1, CPTR(elstore14));
            break;
          case opStore16:
            minps(t1, CPTR(elstore16));
            break;
          }
          mov(a, ptr[regptrs]);
          cvtps2dq(t1, t1);
          // new
          switch (iter.op) {
          case opStore10:
          case opStore12:
          case opStore14:
            packssdw(t1, zero);   // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
            break;
          case opStore16:
            if (cpuFlags & CPUF_SSE4_1) {
              packusdw(t1, zero);   // _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
            }
            else {
              // old, sse2
              movdqa(r1, t1);   // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
              psrldq(t1, 6);
              por(t1, r1);
              pshuflw(t1, t1, 0b11011000);
              punpcklqdq(t1, zero);
            }
            break;
          }
          movq(mmword_ptr[a], t1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          XmmReg r1, r2;
          Reg a;
          maxps(t1.first, zero);
          maxps(t1.second, zero);
          switch (iter.op) {
          case opStore10:
            minps(t1.first, CPTR(elstore10));
            minps(t1.second, CPTR(elstore10));
            break;
          case opStore12:
            minps(t1.first, CPTR(elstore12));
            minps(t1.second, CPTR(elstore12));
            break;
          case opStore14:
            minps(t1.first, CPTR(elstore14));
            minps(t1.second, CPTR(elstore14));
            break;
          case opStore16:
            minps(t1.first, CPTR(elstore16));
            minps(t1.second, CPTR(elstore16));
            break;
          }
          mov(a, ptr[regptrs]);
          cvtps2dq(t1.first, t1.first);
          cvtps2dq(t1.second, t1.second);
          // new
          switch (iter.op) {
          case opStore10:
          case opStore12:
          case opStore14:
            packssdw(t1.first, t1.second);   // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
            break;
          case opStore16:
            if (cpuFlags & CPUF_SSE4_1) {
              packusdw(t1.first, t1.second);   // _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
            }
            else {
              // old, sse2
              movdqa(r1, t1.first);   // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
              movdqa(r2, t1.second);  // 00 w7 00 w6 00 w5 00 w4
              psrldq(t1.first, 6);
              psrldq(t1.second, 6);
              por(t1.first, r1);
              por(t1.second, r2);
              pshuflw(t1.first, t1.first, 0b11011000);
              pshuflw(t1.second, t1.second, 0b11011000);
              punpcklqdq(t1.first, t1.second);
            }
            break;
          }
          movdqa(xmmword_ptr[a], t1.first);
        }
      }
      else if (iter.op == opStoreF32) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          movaps(xmmword_ptr[a], t1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          movaps(xmmword_ptr[a], t1.first);
          movaps(xmmword_ptr[a + 16], t1.second);
        }
      }
      else if (iter.op == opStoreF16) { // not supported in avs+
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vcvtps2ph(qword_ptr[a], t1, 0);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vcvtps2ph(qword_ptr[a], t1.first, 0);
          vcvtps2ph(qword_ptr[a + 8], t1.second, 0);
        }
      }
      else if (iter.op == opStoreVar || iter.op == opStoreAndPopVar) {
        if (processSingle) {
          auto &t1 = stack1.back();
          if(iter.op == opStoreAndPopVar)
            stack1.pop_back();
          // 16 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 16 * iter.e.ival;
          movaps(xmmword_ptr[regptrs + offset], t1);
        }
        else {
          auto &t1 = stack.back();
          if (iter.op == opStoreAndPopVar)
            stack.pop_back();
          // 32 byte/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 32 * iter.e.ival;
          movaps(xmmword_ptr[regptrs + offset], t1.first);
          movaps(xmmword_ptr[regptrs + offset + 16], t1.second);
        }
      }
      else if (iter.op == opAbs) {
        if (processSingle) {
          auto &t1 = stack1.back();
          andps(t1, CPTR(elabsmask));
        }
        else {
          auto &t1 = stack.back();
          andps(t1.first, CPTR(elabsmask));
          andps(t1.second, CPTR(elabsmask));
        }
      }
      else if (iter.op == opNeg) {
        if (processSingle) {
          auto &t1 = stack1.back();
          cmpleps(t1, zero);
          andps(t1, CPTR(elfloat_one));
        }
        else {
          auto &t1 = stack.back();
          cmpleps(t1.first, zero);
          cmpleps(t1.second, zero);
          andps(t1.first, CPTR(elfloat_one));
          andps(t1.second, CPTR(elfloat_one));
        }
      }
      else if (iter.op == opAnd) {
        if (processSingle) {
          LogicOp_Single(andps)
        }
        else {
          LogicOp(andps)
        }
      }
      else if (iter.op == opOr) {
        if (processSingle) {
          LogicOp_Single(orps)
        }
        else {
          LogicOp(orps)
        }
      }
      else if (iter.op == opXor) {
        if (processSingle) {
          LogicOp_Single(xorps)
        }
        else {
          LogicOp(xorps)
        }
      }
      else if (iter.op == opGt) { // a > b (gt) -> b < (lt) a
        if (processSingle) {
          CmpOp_Single(cmpltps)
        }
        else {
          CmpOp(cmpltps)
        }
      }
      else if (iter.op == opLt) { // a < b (lt) -> b > (gt,nle) a
        if (processSingle) {
          CmpOp_Single(cmpnleps)
        }
        else {
          CmpOp(cmpnleps)
        }
      }
      else if (iter.op == opEq) { // a == b -> b == a
        if (processSingle) {
          CmpOp_Single(cmpeqps)
        }
        else {
          CmpOp(cmpeqps)
        }
      }
      else if (iter.op == opNotEq) { // a != b
        if (processSingle) {
          CmpOp_Single(cmpneqps)
        }
        else {
          CmpOp(cmpneqps)
        }
      }
      else if (iter.op == opLE) { // a <= b -> b >= (ge,nlt) a
        if (processSingle) {
          CmpOp_Single(cmpnltps)
        }
        else {
          CmpOp(cmpnltps)
        }
      }
      else if (iter.op == opGE) { // a >= b -> b <= (le) a
        if (processSingle) {
          CmpOp_Single(cmpleps)
        }
        else {
          CmpOp(cmpleps)
        }
      }
      else if (iter.op == opTernary) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto t2 = stack1.back();
          stack1.pop_back();
          auto t3 = stack1.back();
          stack1.pop_back();
          XmmReg r1;
          xorps(r1, r1);
          cmpltps(r1, t3);
          andps(t2, r1);
          andnps(r1, t1);
          orps(r1, t2);
          stack1.push_back(r1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto t2 = stack.back();
          stack.pop_back();
          auto t3 = stack.back();
          stack.pop_back();
          XmmReg r1, r2;
          xorps(r1, r1);
          xorps(r2, r2);
          cmpltps(r1, t3.first);
          cmpltps(r2, t3.second);
          andps(t2.first, r1);
          andps(t2.second, r2);
          andnps(r1, t1.first);
          andnps(r2, t1.second);
          orps(r1, t2.first);
          orps(r2, t2.second);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opExp) {
        if (processSingle) {
          auto &t1 = stack1.back();
          EXP_PS(t1)
        }
        else {
          auto &t1 = stack.back();
          EXP_PS(t1.first)
            EXP_PS(t1.second)
        }
      }
      else if (iter.op == opLog) {
        if (processSingle) {
          auto &t1 = stack1.back();
          LOG_PS(t1)
        }
        else {
          auto &t1 = stack.back();
          LOG_PS(t1.first)
            LOG_PS(t1.second)
        }
      }
      else if (iter.op == opPow) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto &t2 = stack1.back();
          LOG_PS(t2)
            mulps(t2, t1);
          EXP_PS(t2)
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto &t2 = stack.back();
          LOG_PS(t2.first)
            mulps(t2.first, t1.first);
          EXP_PS(t2.first)
            LOG_PS(t2.second)
            mulps(t2.second, t1.second);
          EXP_PS(t2.second)
        }
      }
      else if (iter.op == opClip) {
        // clip(a, low, high) = min(max(a, low),high)
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto t2 = stack1.back();
          stack1.pop_back();
          auto &t3 = stack1.back();
          maxps(t3, t2);
          minps(t3, t1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto t2 = stack.back();
          stack.pop_back();
          auto &t3 = stack.back();
          maxps(t3.first, t2.first);
          minps(t3.first, t1.first);
          maxps(t3.second, t2.second);
          minps(t3.second, t1.second);
        }
      }

    }
  }

    void main(Reg regptrs, Reg regoffs, Reg niter, Reg SpatialY)
    {
        XmmReg zero;
        pxor(zero, zero);
        Reg constptr;
        mov(constptr, (uintptr_t)logexpconst);

        L("wloop");
        cmp(niter, 0);
        je("wend");
        //sub(niter, 1);
        dec(niter);

        // process two sets, no partial input masking
        if (singleMode)
          processingLoop<true, false>(regptrs, zero, constptr, SpatialY);
        else
          processingLoop<false, false>(regptrs, zero, constptr, SpatialY);

        const int EXTRA = 2; // output pointer, xcounter
        if constexpr(sizeof(void *) == 8) {
            int numIter = (numInputs + EXTRA + 1) / 2;
            for (int i = 0; i < numIter; i++) {
                XmmReg r1, r2;
                movdqu(r1, xmmword_ptr[regptrs + 16 * i]);
                movdqu(r2, xmmword_ptr[regoffs + 16 * i]);
                paddq(r1, r2);
                movdqu(xmmword_ptr[regptrs + 16 * i], r1);
            }
        } else {
            int numIter = (numInputs + EXTRA + 3) / 4;
            for (int i = 0; i < numIter; i++) {
                XmmReg r1, r2;
                movdqu(r1, xmmword_ptr[regptrs + 16 * i]);
                movdqu(r2, xmmword_ptr[regoffs + 16 * i]);
                paddd(r1, r2);
                movdqu(xmmword_ptr[regptrs + 16 * i], r1);
            }
        }

        jmp("wloop");
        L("wend");

        int nrestpixels = planewidth & (singleMode ? 3 : 7);
        if (nrestpixels > 4) // dual process with masking
          processingLoop<false, true>(regptrs, zero, constptr, SpatialY);
        else if (nrestpixels == 4) // single process, no masking
          processingLoop<true, false>(regptrs, zero, constptr, SpatialY);
        else if (nrestpixels > 0) // single process, masking
          processingLoop<true, true>(regptrs, zero, constptr, SpatialY);
    }
};

// avx2 evaluator with two ymm registers
struct ExprEvalAvx2 : public jitasm::function<void, ExprEvalAvx2, uint8_t *, const intptr_t *, intptr_t, intptr_t> {

  std::vector<ExprOp> ops;
  int numInputs;
  int cpuFlags;
  int planewidth;
  int planeheight;
  bool singleMode;

  ExprEvalAvx2(std::vector<ExprOp> &ops, int numInputs, int cpuFlags, int planewidth, int planeheight, bool singleMode) : ops(ops), numInputs(numInputs), cpuFlags(cpuFlags), 
    planewidth(planewidth), planeheight(planeheight), singleMode(singleMode) {}

  template<bool processSingle, bool maskUnused>
  __forceinline void processingLoop(Reg &regptrs, YmmReg &zero, Reg &constptr, Reg &SpatialY)
  {
    std::list<std::pair<YmmReg, YmmReg>> stack;
    std::list<YmmReg> stack1;

    const bool maskIt = (maskUnused && ((planewidth & 7) != 0));
    const int mask = ((1 << (planewidth & 7)) - 1);
    // mask by zero when we have only 1-7 valid pixels
    // 1: 2-1   = 1   // 00000001
    // 2: 4-1   = 3   // 00000011
    // 7: 128-1 = 127 // 01111111

    for (const auto &iter : ops) {
      if (iter.op == opLoadSpatialX) {
        if (processSingle) {
          YmmReg r1;
          XmmReg r1x;
          vmovd(r1x, dword_ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
          vcvtdq2ps(r1x, r1x);
          vbroadcastss(r1, r1x);
          vaddps(r1, r1, CPTR_AVX(spatialX));
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          XmmReg r1x;
          vmovd(r1x, dword_ptr[regptrs + sizeof(void *) * (RWPTR_START_OF_XCOUNTER)]);
          vcvtdq2ps(r1x, r1x);
          vbroadcastss(r1, r1x);
          vmovaps(r2, r1);
          vaddps(r1, r1, CPTR_AVX(spatialX));
          vaddps(r2, r2, CPTR_AVX(spatialX2));
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSpatialY) {
        if (processSingle) {
          YmmReg r1;
          XmmReg r1x;
#if _M_X64
          vmovq(r1x, SpatialY);
#else
          vmovd(r1x, SpatialY);
#endif
          vcvtdq2ps(r1x, r1x);
          vbroadcastss(r1, r1x);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          XmmReg r1x;
#if _M_X64
          vmovq(r1x, SpatialY);
#else
          vmovd(r1x, SpatialY);
#endif
          vcvtdq2ps(r1x, r1x);
          vbroadcastss(r1, r1x);
          vmovaps(r2, r1);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadInternalVar) {
        if (processSingle) {
          YmmReg r1;
          XmmReg r1x;
          vmovd(r1x, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INTERNAL_VARIABLES)]);
          vbroadcastss(r1, r1x);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          XmmReg r1x;
          vmovd(r1x, dword_ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INTERNAL_VARIABLES)]);
          vbroadcastss(r1, r1x);
          vmovaps(r2, r1);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrc8) {
        if (processSingle) {
          XmmReg r1x;
          YmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 8 bytes, 8 pixels * uint8_t
          vmovq(r1x, mmword_ptr[a]);
          // 8->32 bits like _mm256_cvtepu8_epi32
          vpmovzxbd(r1, r1x);
          // int -> float
          vcvtdq2ps(r1, r1);
          if (maskIt)
            vblendps(r1, zero, r1, mask);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1x;
          YmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 16 bytes, 16 pixels * uint8_t
          vmovdqa(r1x, xmmword_ptr[a]);
          // 8->16 bits like _mm256_cvtepu8_epi16
          vpmovzxbw(r1, r1x);
          // 16->32 bit like _mm256_cvtepu16_epi32
          vextracti128(r1x, r1, 1); // upper 128
          vpmovzxwd(r2, r1x);
          vextracti128(r1x, r1, 0); // lower 128
          vpmovzxwd(r1, r1x);
          // int -> float
          vcvtdq2ps(r1, r1);
          vcvtdq2ps(r2, r2);
          if (maskIt)
            vblendps(r2, zero, r2, mask);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrc16) {
        if (processSingle) {
          XmmReg r1x;
          YmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 16 bytes, 8 pixels * uint16_t
          vmovdqa(r1x, xmmword_ptr[a]);
          // 16->32 bit like _mm256_cvtepu16_epi32
          vpmovzxwd(r1, r1x);
          // int -> float
          vcvtdq2ps(r1, r1);
          if (maskIt)
            vblendps(r1, zero, r1, mask);
          stack1.push_back(r1);
        }
        else {
          XmmReg r1x;
          YmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 32 bytes, 16 pixels * uint16_t
          vmovdqa(r1, ymmword_ptr[a]);
          // 16->32 bit like _mm256_cvtepu16_epi32
          vextracti128(r1x, r1, 1); // upper 128
          vpmovzxwd(r2, r1x);
          vextracti128(r1x, r1, 0); // lower 128
          vpmovzxwd(r1, r1x);
          // int -> float
          vcvtdq2ps(r1, r1);
          vcvtdq2ps(r2, r2);
          if (maskIt)
            vblendps(r2, zero, r2, mask);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrcF32) {
        if (processSingle) {
          YmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 32 bytes, 8 * float
          vmovdqa(r1, ymmword_ptr[a]);
          if (maskIt)
            vblendps(r1, zero, r1, mask);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          // 32 bytes, 8 * float
          vmovdqa(r1, ymmword_ptr[a]);
          vmovdqa(r2, ymmword_ptr[a + 32]); // needs 64 byte aligned data to prevent read past valid data!
          if (maskIt)
            vblendps(r2, zero, r2, mask);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadSrcF16) { // not supported in avs+
        if (processSingle) {
          YmmReg r1;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          vcvtph2ps(r1, xmmword_ptr[a]);
          if (maskIt)
            vblendps(r1, zero, r1, mask);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          Reg a;
          mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + RWPTR_START_OF_INPUTS)]);
          vcvtph2ps(r1, xmmword_ptr[a]);
          vcvtph2ps(r2, xmmword_ptr[a + 16]);
          if (maskIt)
            vblendps(r2, zero, r2, mask);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadVar) {
        if (processSingle) {
          YmmReg r1;
          // 32 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 32 * iter.e.ival;
          // 32 bytes, 8 * float
          vmovdqa(r1, ymmword_ptr[regptrs + offset]);
          if (maskIt)
            vblendps(r1, zero, r1, mask);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          // 64 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 64 * iter.e.ival;
          // 32 bytes, 8 * float
          vmovdqa(r1, ymmword_ptr[regptrs + offset]);
          vmovdqa(r2, ymmword_ptr[regptrs + offset + 32]); // needs 64 byte aligned data to prevent read past valid data!
          if (maskIt)
            vblendps(r2, zero, r2, mask);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opLoadConst) {
        if (processSingle) {
          YmmReg r1;
          Reg32 a;
          XmmReg r1x;
          mov(a, iter.e.ival);
          vmovd(r1x, a);
          vbroadcastss(r1, r1x);
          stack1.push_back(r1);
        }
        else {
          YmmReg r1, r2;
          Reg32 a;
          XmmReg r1x;
          mov(a, iter.e.ival);
          vmovd(r1x, a);
          vbroadcastss(r1, r1x);
          vmovaps(r2, r1);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opDup) {
        if (processSingle) {
          auto p = std::next(stack1.rbegin(), iter.e.ival);
          YmmReg r1;
          vmovaps(r1, *p);
          stack1.push_back(r1);
        }
        else {
          auto p = std::next(stack.rbegin(), iter.e.ival);
          YmmReg r1, r2;
          vmovaps(r1, p->first);
          vmovaps(r2, p->second);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opSwap) {
        if(processSingle)
          std::swap(stack1.back(), *std::next(stack1.rbegin(), iter.e.ival));
        else
          std::swap(stack.back(), *std::next(stack.rbegin(), iter.e.ival));
      }
      else if (iter.op == opAdd) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vaddps);
        }
        else {
          TwoArgOp_Avx(vaddps);
        }
      }
      else if (iter.op == opSub) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vsubps);
        }
        else {
          TwoArgOp_Avx(vsubps);
        }
      }
      else if (iter.op == opMul) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vmulps);
        }
        else {
          TwoArgOp_Avx(vmulps);
        }
      }
      else if (iter.op == opDiv) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vdivps);
        }
        else {
          TwoArgOp_Avx(vdivps);
        }
      }
      else if (iter.op == opFmod) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto &t2 = stack1.back();
          FMOD_PS_AVX(t2, t1)
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto &t2 = stack.back();
          FMOD_PS_AVX(t2.first, t1.first)
          FMOD_PS_AVX(t2.second, t1.second)
        }
      }
      else if (iter.op == opMax) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vmaxps);
        }
        else {
          TwoArgOp_Avx(vmaxps);
        }
      }
      else if (iter.op == opMin) {
        if (processSingle) {
          TwoArgOp_Single_Avx(vminps);
        }
        else {
          TwoArgOp_Avx(vminps);
        }
      }
      else if (iter.op == opSqrt) {
        if (processSingle) {
          auto &t1 = stack1.back();
          vmaxps(t1, t1, zero);
          vsqrtps(t1, t1);
        }
        else {
          auto &t1 = stack.back();
          vmaxps(t1.first, t1.first, zero);
          vmaxps(t1.second, t1.second, zero);
          vsqrtps(t1.first, t1.first);
          vsqrtps(t1.second, t1.second);
        }
      }
      else if (iter.op == opStore8) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          vmaxps(t1, t1, zero);
          vminps(t1, t1, CPTR_AVX(elstore8));
          mov(a, ptr[regptrs]);
          vcvtps2dq(t1, t1);   // float to int32
          XmmReg r1x, r2x;
          // 32 -> 16 bits from ymm 8 integers to xmm 8 words
          // first
          vextracti128(r1x, t1, 0);
          vextracti128(r2x, t1, 1);
          vpackusdw(r1x, r1x, r2x); // _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          // 16 -> 8 bits
          vpackuswb(r1x, r1x, r1x); // _mm_packus_epi16: w3 w2 w1 w0 w3 w2 w1 w0
          vmovq(mmword_ptr[a], r1x); // store 8 bytes
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          vmaxps(t1.first, t1.first, zero);
          vmaxps(t1.second, t1.second, zero);
          vminps(t1.first, t1.first, CPTR_AVX(elstore8));
          vminps(t1.second, t1.second, CPTR_AVX(elstore8));
          mov(a, ptr[regptrs]);
          vcvtps2dq(t1.first, t1.first);   // float to int32
          vcvtps2dq(t1.second, t1.second);
          // we have 8 integers in t.first and another 8 in t.second
          // second                           first
          // d15 d14 d13 d12 d11 d10 d9 d8    d7 d6 d5 d4 d3 d2 d1 d0  // 16x32 bit integers in two ymm registers. not really 256 bits, but 2x128 bits
          XmmReg r1x, r2x, r_lo_x;
          // 32 -> 16 bits from ymm 8 integers to xmm 8 words
          // first
          vextracti128(r1x, t1.first, 0);
          vextracti128(r2x, t1.first, 1);
          vpackusdw(r_lo_x, r1x, r2x); // _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          // second
          vextracti128(r1x, t1.second, 0); // not perfect, lower 128 bits of t1 could be used as xmm in packus. Cannot tell jitasm that xxmN is lower ymmN
          vextracti128(r2x, t1.second, 1);
          vpackusdw(r1x, r1x, r2x); //  _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          // 16 -> 8 bits
          vpackuswb(r1x, r_lo_x, r1x); // _mm_packus_epi16: w3 w2 w1 w0 w3 w2 w1 w0
          vmovdqa(xmmword_ptr[a], r1x); // store 16 bytes
        }
      }
      else if (iter.op == opStore10 // avs+
        || iter.op == opStore12 // avs+ 
        || iter.op == opStore14 // avs+
        || iter.op == opStore16
        ) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          vmaxps(t1, t1, zero);
          switch (iter.op) {
          case opStore10:
            vminps(t1, t1, CPTR_AVX(elstore10));
            break;
          case opStore12:
            vminps(t1, t1, CPTR_AVX(elstore12));
            break;
          case opStore14:
            vminps(t1, t1, CPTR_AVX(elstore14));
            break;
          case opStore16:
            vminps(t1, t1, CPTR_AVX(elstore16));
            break;
          }
          mov(a, ptr[regptrs]);
          vcvtps2dq(t1, t1);   // min / max clamp ensures that high words are zero
          XmmReg r1x, r2x;
          // 32 -> 16 bits from ymm 8 integers to xmm 8 words
          vextracti128(r1x, t1, 0); // not perfect, lower 128 bits of t1 could be used as xmm in packus. Cannot tell jitasm that xxmN is lower ymmN
          vextracti128(r2x, t1, 1);
          vpackusdw(r1x, r1x, r2x); //  _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          vmovdqa(xmmword_ptr[a], r1x);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          vmaxps(t1.first, t1.first, zero);
          vmaxps(t1.second, t1.second, zero);
          switch (iter.op) {
          case opStore10:
            vminps(t1.first, t1.first, CPTR_AVX(elstore10));
            vminps(t1.second, t1.second, CPTR_AVX(elstore10));
            break;
          case opStore12:
            vminps(t1.first, t1.first, CPTR_AVX(elstore12));
            vminps(t1.second, t1.second, CPTR_AVX(elstore12));
            break;
          case opStore14:
            vminps(t1.first, t1.first, CPTR_AVX(elstore14));
            vminps(t1.second, t1.second, CPTR_AVX(elstore14));
            break;
          case opStore16:
            vminps(t1.first, t1.first, CPTR_AVX(elstore16));
            vminps(t1.second, t1.second, CPTR_AVX(elstore16));
            break;
          }
          mov(a, ptr[regptrs]);
          vcvtps2dq(t1.first, t1.first);   // min / max clamp ensures that high words are zero
          vcvtps2dq(t1.second, t1.second);
          // we have 8 integers in t.first and another 8 in t.second
          // second                           first
          // d15 d14 d13 d12 d11 d10 d9 d8    d7 d6 d5 d4 d3 d2 d1 d0  // 16x32 bit integers in two ymm registers. not really 256 bits, but 2x128 bits
          XmmReg r1x, r2x;
          // 32 -> 16 bits from ymm 8 integers to xmm 8 words
          // first
          vextracti128(r1x, t1.first, 0); // not perfect, lower 128 bits of t1 could be used as xmm in packus. Cannot tell jitasm that xxmN is lower ymmN
          vextracti128(r2x, t1.first, 1);
          vpackusdw(r1x, r1x, r2x); //  _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          vmovdqa(xmmword_ptr[a], r1x);
          // second
          vextracti128(r1x, t1.second, 0);
          vextracti128(r2x, t1.second, 1);
          vpackusdw(r1x, r1x, r2x); //  _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
          vmovdqa(xmmword_ptr[a + 16], r1x);
        }
      }
      else if (iter.op == opStoreF32) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vmovaps(ymmword_ptr[a], t1);
        } else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vmovaps(ymmword_ptr[a], t1.first);
          vmovaps(ymmword_ptr[a + 32], t1.second); // this needs 64 byte aligned data to prevent overwrite!
        }
      }
      else if (iter.op == opStoreF16) { // not supported in avs+
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vcvtps2ph(xmmword_ptr[a], t1, 0);
        } else {
          auto t1 = stack.back();
          stack.pop_back();
          Reg a;
          mov(a, ptr[regptrs]);
          vcvtps2ph(xmmword_ptr[a], t1.first, 0);
          vcvtps2ph(xmmword_ptr[a + 16], t1.second, 0);
        }
      }
      else if (iter.op == opStoreVar || iter.op == opStoreAndPopVar) {
        if (processSingle) {
          auto &t1 = stack1.back();
          if (iter.op == opStoreAndPopVar)
            stack1.pop_back();
          // 32 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 32 * iter.e.ival;
          vmovaps(ymmword_ptr[regptrs + offset], t1);
        }
        else {
          auto &t1 = stack.back();
          if (iter.op == opStoreAndPopVar)
            stack.pop_back();
          // 64 bytes/variable
          int offset = sizeof(void *) * RWPTR_START_OF_USERVARIABLES + 64 * iter.e.ival;
          vmovaps(ymmword_ptr[regptrs + offset], t1.first);
          vmovaps(ymmword_ptr[regptrs + offset + 32], t1.second); // this needs 64 byte aligned data to prevent overwrite!
        }
      }
      else if (iter.op == opAbs) {
        if (processSingle) {
          auto &t1 = stack1.back();
          vandps(t1, t1, CPTR_AVX(elabsmask));
        }
        else {
          auto &t1 = stack.back();
          vandps(t1.first, t1.first, CPTR_AVX(elabsmask));
          vandps(t1.second, t1.second, CPTR_AVX(elabsmask));
        }
      } else if (iter.op == opNeg) {
        if (processSingle) {
          auto &t1 = stack1.back();
          vcmpps(t1, t1, zero, _CMP_LE_OQ); // cmpleps
          vandps(t1, t1, CPTR_AVX(elfloat_one));
        }
        else {
          auto &t1 = stack.back();
          vcmpps(t1.first, t1.first, zero, _CMP_LE_OQ); // cmpleps
          vcmpps(t1.second, t1.second, zero, _CMP_LE_OQ);
          vandps(t1.first, t1.first, CPTR_AVX(elfloat_one));
          vandps(t1.second, t1.second, CPTR_AVX(elfloat_one));
        }
      }
      else if (iter.op == opAnd) {
        if (processSingle) {
          LogicOp_Single_Avx(vandps);
        }
        else {
          LogicOp_Avx(vandps);
        }
      }
      else if (iter.op == opOr) {
        if (processSingle) {
          LogicOp_Single_Avx(vorps);
        }
        else {
          LogicOp_Avx(vorps);
        }
      }
      else if (iter.op == opXor) {
        if (processSingle) {
          LogicOp_Single_Avx(vxorps);
        }
        else {
          LogicOp_Avx(vxorps);
        }
      }
      else if (iter.op == opGt) { // a > b (gt) -> b < (lt) a
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_LT_OQ); // cmpltps
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_LT_OQ) // cmpltps
        }
      }
      else if (iter.op == opLt) { // a < b (lt) -> b > (gt,nle) a
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_GT_OQ); // cmpnleps
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_GT_OQ); // cmpnleps
        }
      }
      else if (iter.op == opEq) {
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_EQ_OQ);
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_EQ_OQ);
        }
      }
      else if (iter.op == opNotEq) { // avs+
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_NEQ_OQ);
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_NEQ_OQ);
        }
      }
      else if (iter.op == opLE) { // a <= b -> b >= (ge,nlt) a
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_GE_OS); // cmpnltps
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_GE_OS) // cmpnltps
        }
      }
      else if (iter.op == opGE) { // a >= b -> b <= (le) a
        if (processSingle) {
          CmpOp_Single_Avx(vcmpps, _CMP_LE_OS) // cmpleps
        }
        else {
          CmpOp_Avx(vcmpps, _CMP_LE_OS) // cmpleps
        }
      }
      else if (iter.op == opTernary) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto t2 = stack1.back();
          stack1.pop_back();
          auto t3 = stack1.back();
          stack1.pop_back();
          YmmReg r1;
          vxorps(r1, r1, r1);
          vcmpps(r1, r1, t3, _CMP_LT_OQ); // cmpltps -> vcmpps ... _CMP_LT_OQ
          vandps(t2, t2, r1);
          vandnps(r1, r1, t1);
          vorps(r1, r1, t2);
          stack1.push_back(r1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto t2 = stack.back();
          stack.pop_back();
          auto t3 = stack.back();
          stack.pop_back();
          YmmReg r1, r2;
          vxorps(r1, r1, r1);
          vxorps(r2, r2, r2);
          vcmpps(r1, r1, t3.first, _CMP_LT_OQ); // cmpltps -> vcmpps ... _CMP_LT_OQ
          vcmpps(r2, r2, t3.second, _CMP_LT_OQ);
          vandps(t2.first, t2.first, r1);
          vandps(t2.second, t2.second, r2);
          vandnps(r1, r1, t1.first);
          vandnps(r2, r2, t1.second);
          vorps(r1, r1, t2.first);
          vorps(r2, r2, t2.second);
          stack.push_back(std::make_pair(r1, r2));
        }
      }
      else if (iter.op == opExp) {
        if (processSingle) {
          auto &t1 = stack1.back();
          EXP_PS_AVX(t1);
        }
        else {
          auto &t1 = stack.back();
          EXP_PS_AVX(t1.first);
          EXP_PS_AVX(t1.second);
        }
      }
      else if (iter.op == opLog) {
        if (processSingle) {
          auto &t1 = stack1.back();
          LOG_PS_AVX(t1);
        } else {
          auto &t1 = stack.back();
          LOG_PS_AVX(t1.first);
          LOG_PS_AVX(t1.second);
        }
      }
      else if (iter.op == opPow) {
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto &t2 = stack1.back();
          LOG_PS_AVX(t2);
          vmulps(t2, t2, t1);
          EXP_PS_AVX(t2);
        } else {
          auto t1 = stack.back();
          stack.pop_back();
          auto &t2 = stack.back();
          LOG_PS_AVX(t2.first);
          vmulps(t2.first, t2.first, t1.first);
          EXP_PS_AVX(t2.first);
          LOG_PS_AVX(t2.second);
          vmulps(t2.second, t2.second, t1.second);
          EXP_PS_AVX(t2.second);
        }
      }
      else if (iter.op == opClip) {
        // clip(a, low, high) = min(max(a, low),high)
        if (processSingle) {
          auto t1 = stack1.back();
          stack1.pop_back();
          auto t2 = stack1.back();
          stack1.pop_back();
          auto &t3 = stack1.back();
          vmaxps(t3, t3, t2);
          vminps(t3, t3, t1);
        }
        else {
          auto t1 = stack.back();
          stack.pop_back();
          auto t2 = stack.back();
          stack.pop_back();
          auto &t3 = stack.back();
          vmaxps(t3.first, t3.first, t2.first);
          vminps(t3.first, t3.first, t1.first);
          vmaxps(t3.second, t3.second, t2.second);
          vminps(t3.second, t3.second, t1.second);
        }
      }
    }
  }
/*
In brief: 
jitasm was modded to accept avx_epilog_=true for code generation

Why: couldn't use vzeroupper because prolog/epilog was saving all xmm6:xmm15 registers even if they were not used at all
Why2: movaps was generated instead of vmovaps for prolog/epilog
Why3: internal register reordering/saving was non-vex encoded
All these issues resulted in AVX->SSE2 penalty

From MSDN:
XMM6:XMM15, YMM6:YMM15 rules for x64:
Nonvolatile (XMM), Volatile (upper half of YMM)
XMM6:XMM15 Must be preserved as needed by callee. 
YMM registers must be preserved as needed by caller. (they do not need to be preserved)

Problem:
- Jitasm saves xmm6..xmm15 when vzeroupper is used,even if only an xmm0 is used (Why?)
No problem (looking at the disassembly list):
- when there is no vzeroupper, then the xmm6:xmm11 is properly saved/restored in prolog/epilog but only
  if ymm6:ymm11 (in this example) is used. If no register is used over xmm6/ymm6 then xmm registers are not saved at all.
- question: does it have any penalty when movaps is used w/o vzeroupper?
  The epilog generates movaps
  movaps      xmm11,xmmword ptr [rbx-10h] 

0000000002430000  push        rbp
0000000002430001  mov         rbp,rsp
0000000002430004  push        rbx
0000000002430005  lea         rbx,[rsp-8]
000000000243000A  sub         rsp,0A8h
0000000002430011  movaps      xmmword ptr [rbx-0A0h],xmm6
0000000002430018  movaps      xmmword ptr [rbx-90h],xmm7
000000000243001F  movaps      xmmword ptr [rbx-80h],xmm8
0000000002430024  movaps      xmmword ptr [rbx-70h],xmm9
0000000002430029  movaps      xmmword ptr [rbx-60h],xmm10
000000000243002E  movaps      xmmword ptr [rbx-50h],xmm11
0000000002430033  movaps      xmmword ptr [rbx-40h],xmm12
0000000002430038  movaps      xmmword ptr [rbx-30h],xmm13
000000000243003D  movaps      xmmword ptr [rbx-20h],xmm14
0000000002430042  movaps      xmmword ptr [rbx-10h],xmm15
-- end of jitasm generated prolog

-- PF AVX+: passing avx_epilog_ = true for codegen, vmovaps is generated instead of movaps
0000000001E60010  vmovaps     xmmword ptr [rbx-60h],xmm6
0000000001E60015  vmovaps     xmmword ptr [rbx-50h],xmm7
0000000001E6001A  vmovaps     xmmword ptr [rbx-40h],xmm8
0000000001E6001F  vmovaps     xmmword ptr [rbx-30h],xmm9
0000000001E60024  vmovaps     xmmword ptr [rbx-20h],xmm10
0000000001E60029  vmovaps     xmmword ptr [rbx-10h],xmm11

// PF comment: user's code like this:
  YmmReg zero;
  vpxor(zero, zero, zero);
  Reg constptr;
  mov(constptr, (uintptr_t)logexpconst_avx);
  vzeroupper();
And the generated instructions:
0000000002430047  vpxor       ymm0,ymm0,ymm0
000000000243004B  mov         rax,7FECCBD15C0h
0000000002430055  vzeroupper
Note: Don't use vzeroupper manually. When vzeroupper is issued manually, jitasm is not too generous: marks all xmm6:xmm15 registers as used
      and epilog and prolog will save all of them, even if none of those xmm/ymm registers are used in the code.
      Modded jitasm: pass avx_epilog_ = true for codegen, it will issue vzeroupper automatically (and has other benefits)

-- start of jitasm generated epilog (old)
0000000002430058  movaps      xmm15,xmmword ptr [rbx-10h]
000000000243005D  movaps      xmm14,xmmword ptr [rbx-20h]
0000000002430062  movaps      xmm13,xmmword ptr [rbx-30h]
0000000002430067  movaps      xmm12,xmmword ptr [rbx-40h]
000000000243006C  movaps      xmm11,xmmword ptr [rbx-50h]
0000000002430071  movaps      xmm10,xmmword ptr [rbx-60h]
0000000002430076  movaps      xmm9,xmmword ptr [rbx-70h]
000000000243007B  movaps      xmm8,xmmword ptr [rbx-80h]
0000000002430080  movaps      xmm7,xmmword ptr [rbx-90h]
0000000002430087  movaps      xmm6,xmmword ptr [rbx-0A0h]
000000000243008E  add         rsp,0A8h
0000000002430095  pop         rbx
0000000002430096  pop         rbp
0000000002430097  ret  
-- end of jitasm generated epilog (old)

PF: modded jitasm (calling codegen with avx_epilog_ = true) generates vmovaps instead of movaps and and automatic vzeroupper before the ret instruction
generated epilog example (new): 
0000000001E70613  vmovaps     xmm11,xmmword ptr [rbx-10h]
0000000001E70618  vmovaps     xmm10,xmmword ptr [rbx-20h]
0000000001E7061D  vmovaps     xmm9,xmmword ptr [rbx-30h]
0000000001E70622  vmovaps     xmm8,xmmword ptr [rbx-40h]
0000000001E70627  vmovaps     xmm7,xmmword ptr [rbx-50h]
0000000001E7062C  vmovaps     xmm6,xmmword ptr [rbx-60h]
0000000001E70631  add         rsp,68h
0000000001E70635  pop         rdi
0000000001E70636  pop         rsi
0000000001E70637  pop         rbx
0000000001E70638  pop         rbp
0000000001E70639  vzeroupper
0000000001E7063C  ret

*/
  void main(Reg regptrs, Reg regoffs, Reg niter, Reg SpatialY)
  {
    YmmReg zero;
    vpxor(zero, zero, zero);
    Reg constptr;
    mov(constptr, (uintptr_t)logexpconst_avx);
    
    L("wloop");
    cmp(niter, 0); // while(niter>0)
    je("wend");
    sub(niter, 1);

    // process two sets, no partial input masking
    if(singleMode)
      processingLoop<true, false>(regptrs, zero, constptr, SpatialY);
    else
      processingLoop<false, false>(regptrs, zero, constptr, SpatialY);

    // increase read and write pointers by 16 pixels
    const int EXTRA = 2; // output pointer, xcounter
    if constexpr(sizeof(void *) == 8) {
      // x64: two 8 byte pointers in an xmm
      int numIter = (numInputs + EXTRA + 1) / 2;

      for (int i = 0; i < numIter; i++) {
        XmmReg r1, r2;
        vmovdqu(r1, xmmword_ptr[regptrs + 16 * i]);
        vmovdqu(r2, xmmword_ptr[regoffs + 16 * i]);
        vpaddq(r1, r1, r2); // pointers are 64 bits
        vmovdqu(xmmword_ptr[regptrs + 16 * i], r1);
      }
    }
    else {
      // x86: four 4 byte pointers in an xmm
      int numIter = (numInputs + EXTRA + 3) / 4;
      for (int i = 0; i < numIter; i++) {
        XmmReg r1, r2;
        vmovdqu(r1, xmmword_ptr[regptrs + 16 * i]);
        vmovdqu(r2, xmmword_ptr[regoffs + 16 * i]);
        vpaddd(r1, r1, r2); // pointers are 32 bits
        vmovdqu(xmmword_ptr[regptrs + 16 * i], r1);
      }
    }

    jmp("wloop");
    L("wend");
    
    int nrestpixels = planewidth & (singleMode ? 7 : 15);
    if(nrestpixels > 8) // dual process with masking
      processingLoop<false, true>(regptrs, zero, constptr, SpatialY);
    else if (nrestpixels == 8) // single process, no masking
      processingLoop<true, false>(regptrs, zero, constptr, SpatialY);
    else if (nrestpixels > 0) // single process, masking
      processingLoop<true, true>(regptrs, zero, constptr, SpatialY);
    // bug in jitasm?
    // on x64, when this is here, debug throws an assert, that a register save/load has an
    // operand size 8 bit, instead of 128 (XMM) or 256 (YMM)
    // vzeroupper(); // don't use it directly. Generate code with avx_epilog_=true
  }
};

#endif

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Exprfilter_filters[] = {
  { "Expr", BUILTIN_FUNC_PREFIX, "c+s+[format]s[optAvx2]b[optSingleMode]b[optSSE2]b[scale_inputs]s[clamp_float]b", Exprfilter::Create },
  { 0 }
};


AVSValue __cdecl Exprfilter::Create(AVSValue args, void* , IScriptEnvironment* env) {

  std::vector<PClip> children;
  std::vector<std::string> expressions;
  int next_paramindex;

  // one or more clips
  if (args[0].IsArray() && args[0][0].IsClip()) { // c+s+ case
    children.resize(args[0].ArraySize());

    for (int i = 0; i < (int)children.size(); ++i) // Copy all
      children[i] = args[0][i].AsClip();

    next_paramindex = 1;
  }
  else if (args[1].IsArray() && args[1][0].IsClip()) { // cc+s+ case
    children.resize(1 + args[1].ArraySize());

    children[0] = args[0].AsClip(); // Copy 1st
    for (int i = 1; i < (int)children.size(); ++i) // Copy rest
      children[i] = args[1][i - 1].AsClip();

    next_paramindex = 2;
  }
  else if (args[1].IsClip()) { //cc case
    children.resize(2);

    children[0] = args[0].AsClip();
    children[1] = args[1].AsClip();

    next_paramindex = 2;
  }
  else if (args[0].IsClip()) { // single clip, cs+ case
    children.resize(1);
    children[0] = args[0].AsClip();

    next_paramindex = 1;
  }

  // one or more expressions: s+
  if (args[next_paramindex].Defined()) {
    AVSValue exprarg = args[next_paramindex++];
    if (exprarg.IsArray()) {
      int nexpr = exprarg.ArraySize();
      expressions.resize(nexpr);
      for (int i = 0; i < nexpr; i++)
        expressions[i] = exprarg[i].AsString();
    }
    else if (exprarg.IsString()) {
      expressions.resize(1);
      expressions[0] = exprarg.AsString();
    }
    else {
      env->ThrowError("Expr: Invalid parameter type for expression string");
    }
  }

  // optional named argument: format
  const char *newformat = nullptr;
  if (args[next_paramindex].Defined()) {
    // always string
    newformat = args[next_paramindex].AsString();
  }
  next_paramindex++;

  // test parameter for avx2-less mode even with avx2 available
#ifdef TEST_AVX2_CODEGEN_IN_AVX
  bool optAvx2 = !!(env->GetCPUFlags() & CPUF_AVX);
#else
  bool optAvx2 = !!(env->GetCPUFlags() & CPUF_AVX2);
#endif
  if (args[next_paramindex].Defined()) {
    if (optAvx2) // disable only
      optAvx2 = args[next_paramindex].AsBool();
  }
  next_paramindex++;

  bool optSingleMode = false;
  if (args[next_paramindex].Defined()) {
    optSingleMode = args[next_paramindex].AsBool();
  }
  next_paramindex++;

  bool optSSE2 = !!(env->GetCPUFlags() & CPUF_SSE2);
  if (args[next_paramindex].Defined()) {
    if (optSSE2) // disable only
      optSSE2 = args[next_paramindex].AsBool();
  }
  next_paramindex++;

  const std::string scale_inputs = args[next_paramindex].Defined() ? args[next_paramindex].AsString("none") : "none";
  next_paramindex++;

  const bool clamp_float = args[next_paramindex].AsBool(false);

  return new Exprfilter(children, expressions, newformat, optAvx2, optSingleMode, optSSE2, scale_inputs, clamp_float, env);

}


PVideoFrame __stdcall Exprfilter::GetFrame(int n, IScriptEnvironment *env) {
  // ExprData d class variable already filled 
  int numInputs = d.numInputs;

  std::vector<PVideoFrame> src;
  src.reserve(children.size());

  for (size_t i = 0; i < children.size(); i++) {
    const auto &child = children[i];
    src.emplace_back(d.clipsUsed[i] ? child->GetFrame(n, env) : nullptr); // GetFrame only when really referenced
  }

  PVideoFrame dst = env->NewVideoFrame(d.vi);

  const uint8_t *srcp[MAX_EXPR_INPUTS] = {};
  const uint8_t *srcp_orig[MAX_EXPR_INPUTS] = {}; // for C
  int src_stride[MAX_EXPR_INPUTS] = {};
  float variable_area[MAX_USER_VARIABLES] = {}; // for C, place for expr variables A..Z

  const float framecount = (float)n; // max precision: 2^24 (16M) frames (32 bit float precision)
  const float relative_time = (float)((double)n / vi.num_frames); // 0 <= time < 1

  const int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
  const int planes_r[4] = { PLANAR_R, PLANAR_G, PLANAR_B, PLANAR_A }; // expression string order is R G B unlike internal G B R plane order
  const int *plane_enums_d = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;

  for (int plane = 0; plane < d.vi.NumComponents(); plane++) {

    const int plane_enum_d = plane_enums_d[plane];

    if (d.plane[plane] == poProcess) {
      uint8_t *dstp = dst->GetWritePtr(plane_enum_d);
      int dst_stride = dst->GetPitch(plane_enum_d);
      int h = d.vi.height >> d.vi.GetPlaneHeightSubsampling(plane_enum_d);
      int w = d.vi.width >> d.vi.GetPlaneWidthSubsampling(plane_enum_d);

      // for simd:
      const int pixels_per_iter = (optAvx2 && d.planeOptAvx2[plane]) ? (optSingleMode ? 8 : 16) : (optSingleMode ? 4 : 8);
      intptr_t ptroffsets[1 + 1 + MAX_EXPR_INPUTS];
      ptroffsets[RWPTR_START_OF_OUTPUT] = d.vi.ComponentSize() * pixels_per_iter; // stepping for output pointer
      ptroffsets[RWPTR_START_OF_XCOUNTER] = pixels_per_iter; // stepping for xcounter

      for (int i = 0; i < numInputs; i++) {
        if (d.node[i]) {
          if (d.clipsUsed[i]) {
            // when input is a single Y, use PLANAR_Y instead of the plane matching to the output
            const VideoInfo& vi_src = d.node[i]->GetVideoInfo();
            const int *plane_enums_s = (vi_src.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;
            const int plane_enum_s = vi_src.IsY() ? PLANAR_Y : plane_enums_d[plane];

            srcp[i] = src[i]->GetReadPtr(plane_enum_s);
            // C only:
            srcp_orig[i] = srcp[i];
            src_stride[i] = src[i]->GetPitch(plane_enum_s);
            // SIMD only
            ptroffsets[RWPTR_START_OF_INPUTS + i] = d.node[i]->GetVideoInfo().ComponentSize() * pixels_per_iter; // 1..Nth: inputs
          }
          else {
            srcp[i] = nullptr;
            srcp_orig[i] = nullptr;
            src_stride[i] = 0;
            ptroffsets[RWPTR_START_OF_INPUTS + i] = 0;
          }
        }
      }

      if (optSSE2 && d.planeOptSSE2[plane]) {

        int nfulliterations = w / pixels_per_iter;

        ExprData::ProcessLineProc proc = d.proc[plane];

        alignas(32) const uint8_t *rwptrs[RWPTR_SIZE];
        *reinterpret_cast<float *>(&rwptrs[RWPTR_START_OF_INTERNAL_VARIABLES + INTERNAL_VAR_CURRENT_FRAME]) = (float)framecount;
        *reinterpret_cast<float *>(&rwptrs[RWPTR_START_OF_INTERNAL_VARIABLES + INTERNAL_VAR_RELTIME]) = (float)relative_time;
        for (int y = 0; y < h; y++) {
          rwptrs[RWPTR_START_OF_OUTPUT] = dstp + dst_stride * y;
          rwptrs[RWPTR_START_OF_XCOUNTER] = 0; // xcounter internal variable
          for (int i = 0; i < numInputs; i++) {
            rwptrs[i + RWPTR_START_OF_INPUTS] = srcp[i] + src_stride[i] * y; // input pointers 1..Nth
            rwptrs[i + RWPTR_START_OF_STRIDES] = reinterpret_cast<const uint8_t *>((intptr_t)src_stride[i]);
          }
          proc(rwptrs, ptroffsets, nfulliterations, y); // parameters are put directly in registers
        }
      }
      else {
        // C version
        std::vector<float> stackVector(d.maxStackSize);

        const ExprOp *vops = d.ops[plane].data();
        float *stack = stackVector.data();
        float stacktop = 0;

        float internal_vars[6];
        internal_vars[INTERNAL_VAR_CURRENT_FRAME] = (float)framecount;
        internal_vars[INTERNAL_VAR_RELTIME] = (float)relative_time;

        for (int y = 0; y < h; y++) {
          for (int x = 0; x < w; x++) {
            int si = 0;
            int i = -1;
            while (true) {
              i++;
              switch (vops[i].op) {
              case opLoadSpatialX:
                stack[si] = stacktop;
                stacktop = (float)x;
                ++si;
                break;
              case opLoadSpatialY:
                stack[si] = stacktop;
                stacktop = (float)y;
                ++si;
                break;
              case opLoadInternalVar:
                stack[si] = stacktop;
                stacktop = internal_vars[vops[i].e.ival];
                ++si;
                break;
              case opLoadSrc8:
                stack[si] = stacktop;
                stacktop = srcp[vops[i].e.ival][x];
                ++si;
                break;
              case opLoadSrc16:
                stack[si] = stacktop;
                stacktop = reinterpret_cast<const uint16_t *>(srcp[vops[i].e.ival])[x];
                ++si;
                break;
              case opLoadSrcF32:
                stack[si] = stacktop;
                stacktop = reinterpret_cast<const float *>(srcp[vops[i].e.ival])[x];
                ++si;
                break;
              case opLoadRelSrc8:
                stack[si] = stacktop;
                {
                  const int newx = x + vops[i].dx;
                  const int newy = y + vops[i].dy;
                  const int clipIndex = vops[i].e.ival;
                  const uint8_t* srcp2 = srcp_orig[clipIndex] + max(0, min(newy, h - 1)) * src_stride[clipIndex];
                  stacktop = srcp2[max(0, min(newx, w - 1))];
                }
                ++si;
                break;
              case opLoadRelSrc16:
                stack[si] = stacktop;
                {
                  const int newx = x + vops[i].dx;
                  const int newy = y + vops[i].dy;
                  const int clipIndex = vops[i].e.ival;
                  const uint16_t* srcp2 = reinterpret_cast<const uint16_t *>(srcp_orig[clipIndex] + max(0, min(newy, h - 1)) * src_stride[clipIndex]);
                  stacktop = srcp2[max(0, min(newx, w - 1))];
                }
                ++si;
                break;
              case opLoadRelSrcF32:
                stack[si] = stacktop;
                {
                  const int newx = x + vops[i].dx;
                  const int newy = y + vops[i].dy;
                  const int clipIndex = vops[i].e.ival;
                  const float* srcp2 = reinterpret_cast<const float *>(srcp_orig[clipIndex] + max(0, min(newy, h - 1)) * src_stride[clipIndex]);
                  stacktop = srcp2[max(0, min(newx, w - 1))];
                }
                ++si;
                break;
              case opLoadConst:
                stack[si] = stacktop;
                stacktop = vops[i].e.fval;
                ++si;
                break;
              case opLoadVar:
                stack[si] = stacktop;
                stacktop = variable_area[vops[i].e.ival];
                ++si;
                break;
              case opDup:
                stack[si] = stacktop;
                stacktop = stack[si - vops[i].e.ival];
                ++si;
                break;
              case opSwap:
                std::swap(stacktop, stack[si - vops[i].e.ival]);
                break;
              case opAdd:
                --si;
                stacktop += stack[si];
                break;
              case opSub:
                --si;
                stacktop = stack[si] - stacktop;
                break;
              case opMul:
                --si;
                stacktop *= stack[si];
                break;
              case opDiv:
                --si;
                stacktop = stack[si] / stacktop;
                break;
              case opFmod:
                --si;
                stacktop = std::fmod(stack[si], stacktop);
                break;
              case opMax:
                --si;
                stacktop = std::max(stacktop, stack[si]);
                break;
              case opMin:
                --si;
                stacktop = std::min(stacktop, stack[si]);
                break;
              case opExp:
                stacktop = std::exp(stacktop);
                break;
              case opLog:
                stacktop = std::log(stacktop);
                break;
              case opPow:
                --si;
                stacktop = std::pow(stack[si], stacktop);
                break;
              case opClip:
                // clip(a, low, high) = min(max(a, low),high)
                si -= 2;
                stacktop = std::max(std::min(stack[si], stacktop), stack[si + 1]);
                break;
              case opSqrt:
                stacktop = std::sqrt(stacktop);
                break;
              case opAbs:
                stacktop = std::abs(stacktop);
                break;
              case opSin:
                stacktop = std::sin(stacktop);
                break;
              case opCos:
                stacktop = std::cos(stacktop);
                break;
              case opTan:
                stacktop = std::tan(stacktop);
                break;
              case opAsin:
                stacktop = std::asin(stacktop);
                break;
              case opAcos:
                stacktop = std::acos(stacktop);
                break;
              case opAtan:
                stacktop = std::atan(stacktop);
                break;
              case opGt:
                --si;
                stacktop = (stack[si] > stacktop) ? 1.0f : 0.0f;
                break;
              case opLt:
                --si;
                stacktop = (stack[si] < stacktop) ? 1.0f : 0.0f;
                break;
              case opEq:
                --si;
                stacktop = (stack[si] == stacktop) ? 1.0f : 0.0f;
                break;
              case opNotEq:
                --si;
                stacktop = (stack[si] != stacktop) ? 1.0f : 0.0f;
                break;
              case opLE:
                --si;
                stacktop = (stack[si] <= stacktop) ? 1.0f : 0.0f;
                break;
              case opGE:
                --si;
                stacktop = (stack[si] >= stacktop) ? 1.0f : 0.0f;
                break;
              case opTernary:
                si -= 2;
                stacktop = (stack[si] > 0) ? stack[si + 1] : stacktop;
                break;
              case opAnd:
                --si;
                stacktop = (stacktop > 0 && stack[si] > 0) ? 1.0f : 0.0f;
                break;
              case opOr:
                --si;
                stacktop = (stacktop > 0 || stack[si] > 0) ? 1.0f : 0.0f;
                break;
              case opXor:
                --si;
                stacktop = ((stacktop > 0) != (stack[si] > 0)) ? 1.0f : 0.0f;
                break;
              case opNeg:
                stacktop = (stacktop > 0) ? 0.0f : 1.0f;
                break;
              case opStore8:
                dstp[x] = (uint8_t)(std::max(0.0f, std::min(stacktop, 255.0f)) + 0.5f);
                goto loopend;
              case opStore10:
                reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)(std::max(0.0f, std::min(stacktop, 1023.0f)) + 0.5f);
                goto loopend;
              case opStore12:
                reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)(std::max(0.0f, std::min(stacktop, 4095.0f)) + 0.5f);
                goto loopend;
              case opStore14:
                reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)(std::max(0.0f, std::min(stacktop, 16383.0f)) + 0.5f);
                goto loopend;
              case opStore16:
                reinterpret_cast<uint16_t *>(dstp)[x] = (uint16_t)(std::max(0.0f, std::min(stacktop, 65535.0f)) + 0.5f);
                goto loopend;
              case opStoreF32:
                reinterpret_cast<float *>(dstp)[x] = stacktop;
                goto loopend;
              case opStoreVar:
                variable_area[vops[i].e.ival] = stacktop;
                break;
              case opStoreAndPopVar:
                variable_area[vops[i].e.ival] = stacktop;
                --si;
                if(si >= 0)
                  stacktop = stack[si];
                break;
              }
            }
          loopend:;
          }
          dstp += dst_stride;
          for (int i = 0; i < numInputs; i++)
            srcp[i] += src_stride[i];
        }
      }
    }
    // avs+: copy plane here
    else if (d.plane[plane] == poCopy) {
      // avs+ copy from Nth clip
      const int copySource = d.planeCopySourceClip[plane];
      // when input is a single Y, use PLANAR_Y instead of the plane matching to the output
      const VideoInfo& vi_src = d.node[copySource]->GetVideoInfo();
      const int *plane_enums_s = (vi_src.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;
      const int plane_enum_s = vi_src.IsY() ? PLANAR_Y : plane_enums_d[plane];

      env->BitBlt(dst->GetWritePtr(plane_enum_d), dst->GetPitch(plane_enum_d),
        src[copySource]->GetReadPtr(plane_enum_s),
        src[copySource]->GetPitch(plane_enum_s),
        src[copySource]->GetRowSize(plane_enum_s),
        src[copySource]->GetHeight(plane_enum_s)
      );
    }
    else if (d.plane[plane] == poFill) { // avs+
      uint8_t *dstp = dst->GetWritePtr(plane_enum_d);
      const int dst_stride = dst->GetPitch(plane_enum_d);
      const int h = dst->GetHeight(plane_enum_d);

      int val;

      switch (vi.BitsPerComponent())
      {
      case 8:
        val = (uint8_t)(std::max(0.0f, std::min(d.planeFillValue[plane], 255.0f)) + 0.5f);
        fill_plane<BYTE>(dstp, h, dst_stride, val);
        break;
      case 10:
        val = (uint16_t)(std::max(0.0f, std::min(d.planeFillValue[plane], 1023.0f)) + 0.5f);
        fill_plane<uint16_t>(dstp, h, dst_stride, val);
        break;
      case 12:
        val = (uint16_t)(std::max(0.0f, std::min(d.planeFillValue[plane], 4095.0f)) + 0.5f);
        fill_plane<uint16_t>(dstp, h, dst_stride, val);
        break;
      case 14:
        val = (uint16_t)(std::max(0.0f, std::min(d.planeFillValue[plane], 16383.0f)) + 0.5f);
        fill_plane<uint16_t>(dstp, h, dst_stride, val);
        break;
      case 16:
        val = (uint16_t)(std::max(0.0f, std::min(d.planeFillValue[plane], 65535.0f)) + 0.5f);
        fill_plane<uint16_t>(dstp, h, dst_stride, val);
        break;
      case 32:
        fill_plane<float>(dstp, h, dst_stride, d.planeFillValue[plane]);
        break;
      }
    } // plane modes
  } // for planes

  return dst;
}

Exprfilter::~Exprfilter() {
  for (int i = 0; i < MAX_EXPR_INPUTS; i++)
    d.node[i] = nullptr;
}

static SOperation getLoadOp(const VideoInfo *vi, bool relativeKind) {
  if (!vi)
    return relativeKind ? opLoadRelSrcF32 : opLoadSrcF32;
  if (vi->BitsPerComponent() == 32) // float, avs has no f16c float
    return relativeKind ? opLoadRelSrcF32 : opLoadSrcF32;
  else if (vi->BitsPerComponent() == 8)
    return relativeKind ? opLoadRelSrc8 : opLoadSrc8;
  else
    return relativeKind ? opLoadRelSrc16 : opLoadSrc16; // 10..16 bits common
}

static SOperation getStoreOp(const VideoInfo *vi) {
  // avs+ has no f16c float
  switch (vi->BitsPerComponent()) {
  case 8: return opStore8;
  case 10: return opStore10; // avs+
  case 12: return opStore12; // avs+ 
  case 14: return opStore14; // avs+
  case 16: return opStore16;
  case 32: return opStoreF32;
  default: return opStoreF32;
  }
}

#define LOAD_OP(op,v,req) do { if (stackSize < req) env->ThrowError("Expr: Not enough elements on stack to perform operation %s", tokens[i].c_str()); ops.push_back(ExprOp(op, (v))); maxStackSize = std::max(++stackSize, maxStackSize); } while(0)
#define LOAD_REL_OP(op,v,req,dx,dy) do { if (stackSize < req) env->ThrowError("Expr: Not enough elements on stack to perform operation %s", tokens[i].c_str()); ops.push_back(ExprOp(op, (v), (dx), (dy))); maxStackSize = std::max(++stackSize, maxStackSize); } while(0)
#define GENERAL_OP(op, v, req, dec) do { if (stackSize < req) env->ThrowError("Expr: Not enough elements on stack to perform operation %s", tokens[i].c_str()); ops.push_back(ExprOp(op, (v))); stackSize-=(dec); } while(0)
#define ONE_ARG_OP(op) GENERAL_OP(op, 0, 1, 0)
#define VAR_STORE_OP(op,v) GENERAL_OP(op, v, 1, 0)
#define VAR_STORE_SPEC_OP(op,v) GENERAL_OP(op, v, 1, 1)
#define TWO_ARG_OP(op) GENERAL_OP(op, 0, 2, 1)
#define THREE_ARG_OP(op) GENERAL_OP(op, 0, 3, 2)
// defines for special scale-back-before-store where no token is in context:
#define LOAD_OP_NOTOKEN(op,v,req) do { if (stackSize < req) env->ThrowError("Expr: Not enough elements on stack to perform a load operation"); ops.push_back(ExprOp(op, (v))); maxStackSize = std::max(++stackSize, maxStackSize); } while(0)
#define GENERAL_OP_NOTOKEN(op, v, req, dec) do { if (stackSize < req) env->ThrowError("Expr: Not enough elements on stack to perform an operation"); ops.push_back(ExprOp(op, (v))); stackSize-=(dec); } while(0)
#define TWO_ARG_OP_NOTOKEN(op) GENERAL_OP_NOTOKEN(op, 0, 2, 1)


// finds _X suffix (clip letter) and returns 0..25 for x,y,z,a,b,...w
// no suffix means 0
static int getSuffix(std::string token, std::string base) {
  size_t len = base.length();
  
  if (token.substr(0, len) != base)
    return -1; // no match
    
  if (token.length() == len)
    return 0; // no suffix, treat as _x

    // find _X suffix, where X is x,y,z,a..w
  if (token.length() != len + 2 || token[len] != '_')
    return -2; // no proper suffix

  char srcChar = token[len + 1]; // last char
  int loadIndex;
  if (srcChar >= 'x')
    loadIndex = srcChar - 'x';
  else
    loadIndex = srcChar - 'a' + 3;
  return loadIndex;
}

static size_t parseExpression(const std::string &expr, std::vector<ExprOp> &ops, const VideoInfo **vi, const VideoInfo *vi_output, const SOperation storeOp, int numInputs, int planewidth, int planeheight, bool chroma, 
  const bool autoconv_full_scale, const bool autoconv_conv_int, const bool autoconv_conv_float, const bool clamp_float,
  IScriptEnvironment *env)
{
    // vi_output is new in avs+, and is not used yet

    // optional scaling, scale_from it depth, default scale_to bitdepth, used in scaleb and scalef
    int targetBitDepth = vi[0]->BitsPerComponent(); // avs+
    int autoScaleSourceBitDepth = 8; // avs+ scalable constants are in 8 bit range by default

    std::vector<std::string> tokens;
    split(tokens, expr, " ", split1::no_empties);

    size_t maxStackSize = 0;
    size_t stackSize = 0;

    for (size_t i = 0; i < tokens.size(); i++) {
        if (tokens[i] == "+")
            TWO_ARG_OP(opAdd);
        else if (tokens[i] == "-")
            TWO_ARG_OP(opSub);
        else if (tokens[i] == "*")
            TWO_ARG_OP(opMul);
        else if (tokens[i] == "/")
            TWO_ARG_OP(opDiv);
        else if (tokens[i] == "%")
            TWO_ARG_OP(opFmod);
        else if (tokens[i] == "max")
            TWO_ARG_OP(opMax);
        else if (tokens[i] == "min")
            TWO_ARG_OP(opMin);
        else if (tokens[i] == "exp")
            ONE_ARG_OP(opExp);
        else if (tokens[i] == "log")
            ONE_ARG_OP(opLog);
        else if (tokens[i] == "pow" || tokens[i] == "^") // avs+: ^ can be used for power
            TWO_ARG_OP(opPow);
        else if (tokens[i] == "sqrt")
            ONE_ARG_OP(opSqrt);
        else if (tokens[i] == "abs")
            ONE_ARG_OP(opAbs);
        else if (tokens[i] == "sin")
          ONE_ARG_OP(opSin);
        else if (tokens[i] == "cos")
          ONE_ARG_OP(opCos);
        else if (tokens[i] == "tan")
          ONE_ARG_OP(opTan);
        else if (tokens[i] == "asin")
          ONE_ARG_OP(opAsin);
        else if (tokens[i] == "acos")
          ONE_ARG_OP(opAcos);
        else if (tokens[i] == "atan")
          ONE_ARG_OP(opAtan);
        else if (tokens[i] == "clip")
          THREE_ARG_OP(opClip);
        else if (tokens[i] == ">")
            TWO_ARG_OP(opGt);
        else if (tokens[i] == "<")
            TWO_ARG_OP(opLt);
        else if (tokens[i] == "=" || tokens[i] == "==") // avs+: == can be used to equality check
            TWO_ARG_OP(opEq);
        else if (tokens[i] == "!=") // avs+: not equal
          TWO_ARG_OP(opNotEq);
        else if (tokens[i] == ">=")
            TWO_ARG_OP(opGE);
        else if (tokens[i] == "<=")
            TWO_ARG_OP(opLE);
        else if (tokens[i] == "?")
            THREE_ARG_OP(opTernary);
        else if (tokens[i] == "and" || tokens[i] == "&") // avs+: & alias for and
            TWO_ARG_OP(opAnd);
        else if (tokens[i] == "or" || tokens[i] == "|") // avs+: | alias for or
            TWO_ARG_OP(opOr);
        else if (tokens[i] == "xor")
            TWO_ARG_OP(opXor);
        else if (tokens[i] == "not")
            ONE_ARG_OP(opNeg);
        else if (tokens[i].substr(0, 3) == "dup")
            if (tokens[i].size() == 3) {
                LOAD_OP(opDup, 0, 1);
            }
            else {
              try {
                int tmp = std::stoi(tokens[i].substr(3));
                if (tmp < 0)
                  env->ThrowError("Expr: Dup suffix can't be less than 0 '%s'", tokens[i].c_str());
                LOAD_OP(opDup, tmp, (size_t)(tmp + 1));
              }
              catch (std::logic_error &) {
                env->ThrowError("Expr: Failed to convert dup suffix '%s' to valid index", tokens[i].c_str());
              }
            }
        else if (tokens[i].substr(0, 4) == "swap")
            if (tokens[i].size() == 4) {
                GENERAL_OP(opSwap, 1, 2, 0);
            }
            else {
              try {
                int tmp = std::stoi(tokens[i].substr(4));
                if (tmp < 1)
                  env->ThrowError("Expr: Swap suffix can't be less than 1 '%s'", tokens[i].c_str());
                GENERAL_OP(opSwap, tmp, (size_t)(tmp + 1), 0);
              }
              catch (std::logic_error &) {
                env->ThrowError("Expr: Failed to convert swap suffix '%s' to valid index", tokens[i].c_str());
              }
            }
        else if (tokens[i] == "sx") { // avs+
          // spatial
          LOAD_OP(opLoadSpatialX, 0, 0);
        } 
        else if (tokens[i] == "sy") { // avs+
          // spatial
          LOAD_OP(opLoadSpatialY, 0, 0);
        }
        else if (tokens[i] == "sxr") { // avs+
          // spatial X relative 0..1
          LOAD_OP(opLoadSpatialX, 0, 0);
          const float p = planewidth > 1 ? 1.0f / ((float)planewidth - 1.0f) : 1.0f;
          LOAD_OP(opLoadConst, p, 0);
          TWO_ARG_OP(opMul);
        }
        else if (tokens[i] == "syr") { // avs+
          // spatial Y relative 0..1
          LOAD_OP(opLoadSpatialY, 0, 0);
          const float p = planeheight > 1 ? 1.0f / ((float)planeheight - 1.0f) : 1.0f;
          LOAD_OP(opLoadConst, p, 0);
          TWO_ARG_OP(opMul);
        }
        else if (tokens[i] == "frameno") { // avs+
          LOAD_OP(opLoadInternalVar, INTERNAL_VAR_CURRENT_FRAME, 0);
        }
        else if (tokens[i] == "time") { // avs+
          LOAD_OP(opLoadInternalVar, INTERNAL_VAR_RELTIME, 0);
        }
        else if (tokens[i] == "width") { // avs+
          LOAD_OP(opLoadConst, (float)planewidth, 0);
        }
        else if (tokens[i] == "height") { // avs+
          LOAD_OP(opLoadConst, (float)planeheight, 0);
        }
        else if (tokens[i].length() == 1 && tokens[i][0] >= 'a' && tokens[i][0] <= 'z') {
          // loading source clip pixels
          char srcChar = tokens[i][0];
          int loadIndex;
          if (srcChar >= 'x')
            loadIndex = srcChar - 'x';
          else
            loadIndex = srcChar - 'a' + 3;
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied to reference '%s'", tokens[i].c_str());
          LOAD_OP(getLoadOp(vi[loadIndex], false), loadIndex, 0);

          // avs+: 'scale_inputs': converts input pixels to a common specified range
          // Apply to integer and/or float bit depths.
          // For integers bit-shift or full-scale-stretch method can be chosen
          // There is no precision loss, since the multiplication/division occurs when original pixels 
          // are already loaded as float
          const int realSourceBitdepth = vi[loadIndex]->BitsPerComponent();
          // need any conversion?
          if (autoScaleSourceBitDepth != realSourceBitdepth && (autoconv_conv_int || autoconv_conv_float)) {
            if (autoconv_conv_int && realSourceBitdepth != 32) {
              // convert from integer to other integer (float: not supported as an internal scale target)
              if (autoScaleSourceBitDepth == 32)
                env->ThrowError("Expr: cannot use scale_inputs with 32bit float as internal scale target (f32)");
              if (autoconv_full_scale) {
                // e.g. conversion from 8 to 16 bits fullscale: /255.0*65535.0
                const float strech_mul = (float)(1 << (autoScaleSourceBitDepth - 1)) / (float)(1 << (realSourceBitdepth - 1));
                LOAD_OP(opLoadConst, strech_mul, 0);
                TWO_ARG_OP(opMul);
              }
              else {
                if (autoScaleSourceBitDepth > realSourceBitdepth)
                {
                  // not fullscale, e.g. conversion from 8 to 16 bits YUV: *256
                  const float shift_mul = (float)(1 << (autoScaleSourceBitDepth - realSourceBitdepth));
                  LOAD_OP(opLoadConst, shift_mul, 0);
                  TWO_ARG_OP(opMul);
                }
                else {
                  // not fullscale, e.g. conversion from 16 to 8 bits YUV: /256
                  const float shift_mul = 1.0f / (float)(1 << (realSourceBitdepth - autoScaleSourceBitDepth));
                  LOAD_OP(opLoadConst, shift_mul, 0);
                  TWO_ARG_OP(opMul);
                }
              }
            }
            else if (autoconv_conv_float && realSourceBitdepth == 32) {
              // convert from float to 8-16 bit integer
              // no difference between autoconv_full_scale or not
              // scale 32 bits (0..1.0) -> 8-16 bits
              // for new 0-based chroma: -0.5..0.5 -> 8*16 bits 0..max-1;
              // for old 0.5-based chroma: 0.0..1.0 -> 8*16 bits 0..max-1;
              if (chroma) {
                // (x-src_middle_chroma)*factor + target_middle_chroma
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
                LOAD_OP(opLoadConst, 0.5f, 0);
                TWO_ARG_OP(opSub);
#endif
                float q = (float)((1 << autoScaleSourceBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);

                int target_middle_chroma = 1 << (autoScaleSourceBitDepth - 1);
                LOAD_OP(opLoadConst, (float)target_middle_chroma, 0);
                TWO_ARG_OP(opAdd);
              }
              else
              {
                // conversion from float: mul by max_pixel_value
                float q = (float)((1 << autoScaleSourceBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);
              }
            }
          } // end of scale inputs
        }
        else if (tokens[i].length() == 1 && tokens[i][0] >= 'A' && tokens[i][0] <= 'Z') { // avs+
          // use of a variable: single uppercase letter A..Z
          char srcChar = tokens[i][0];
          int loadIndex = srcChar - 'A';
          LOAD_OP(opLoadVar, loadIndex, 0);
        }
        else if (tokens[i].length() == 2 && tokens[i][0] >= 'A' && tokens[i][0] <= 'Z') { // avs+
          // storing a variable: A@ .. Z@
          // storing a variable and remove from stack: A^..Z^
          char srcChar = tokens[i][0];
          int loadIndex = srcChar - 'A';
          if (tokens[i][1] == '^')
            VAR_STORE_SPEC_OP(opStoreAndPopVar, loadIndex);
          else if (tokens[i][1] == '@')
            VAR_STORE_OP(opStoreVar, loadIndex);
          else
            env->ThrowError("Expr: Invalid character, '^' or '@' expected in '%s'", tokens[i].c_str());
        }
        // indexed clips e.g. x[-1,-2]
        else if (tokens[i].length() > 1 && tokens[i][0] >= 'a' && tokens[i][0] <= 'z' && tokens[i][1] == '[') {
          char srcChar = tokens[i][0];
          int loadIndex;
          if (srcChar >= 'x')
            loadIndex = srcChar - 'x';
          else
            loadIndex = srcChar - 'a' + 3;
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied to reference '%s'", tokens[i].c_str());

          int dx, dy;
          std::string s;
          std::istringstream numStream(tokens[i].substr(2)); // after '['
          numStream.imbue(std::locale::classic());
          // first coord
          if (!(numStream >> dx))
            env->ThrowError("Expr: Failed to convert '%s' to integer, relative index dx", tokens[i].c_str());
          // separator ','
          if (numStream.get() != ',')
            env->ThrowError("Expr: Failed to convert '%s', character ',' expected between the coordinates", tokens[i].c_str());
          // second coord
          if (!(numStream >> dy))
            env->ThrowError("Expr: Failed to convert '%s' to integer, relative index dy", tokens[i].c_str());
          // ending ']'
          if (numStream.get() != ']')
            env->ThrowError("Expr: Failed to convert '%s' to [x,y], closing ']' expected ", tokens[i].c_str());
          if(numStream >> s)
            env->ThrowError("Expr: Failed to convert '%s' to [x,y], invalid character after ']'", tokens[i].c_str());

          if(dx <= -vi_output->width || dx >= vi_output->width)
            env->ThrowError("Expr: dx must be between +/- (width-1) in '%s'", tokens[i].c_str());
          if (dy <= -vi_output->height || dy >= vi_output->height) 
            env->ThrowError("Expr: dy must be between +/- (height-1) in '%s'", tokens[i].c_str());
          LOAD_REL_OP(getLoadOp(vi[loadIndex], true), loadIndex, 0, dx, dy);
        }
        else if (tokens[i] == "pi") // avs+
        {
          float pi = 3.141592653589793f;
          LOAD_OP(opLoadConst, pi, 0);
        }
        // avs+
        // bitdepth: automatic silent parameter of the lut expression(clip bit depth)
        // clip-specific bitdepths: bitdepth_x, bitdepth_y, bitdepth_z, bitdepth_a, .. bitdepth_w,
        // sbitdepth : automatic silent parameter of the lut expression(bit depth of values to scale)
        //
        // pre-defined, bit depth aware constants
        //   range_half : autoscaled 128 or 0.5 for float, (or 0.0 for chroma with zero-base float chroma version)
        //   range_max  : 255 / 1023 / 4095 / 16383 / 65535 or 1.0 for float
        //                                                     0.5 for float chroma (new zero-based style)
        //   range_min  : 0 for 8-16bits, or 0 for float luma, or -0.5 for float chroma
        //                -0.5 for float chroma (new zero-based style)
        //   range_size : 256 / 1024...65536
        //   ymin, ymax : 16 / 235 autoscaled.
        //   cmin, cmax : 16 / 240 autoscaled. For 32bits zero based chroma: (16-128)/255.0, (240-128)/255.0

        // ymin or ymin_x, ymin_y, ymin_y, ymin_a....
        // similarly: ymax, range_max, cmin, cmax, range_half
        // without clip index specifier, or with '_'+letter suffix
        else if (tokens[i].substr(0, 8) == "bitdepth") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "bitdepth";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = (float)bitsPerComponent;
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i] == "sbitdepth") // avs+
        {
          float q = (float)autoScaleSourceBitDepth;
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 4) == "ymin") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "ymin";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 16.0f / 255 : (16 << (bitsPerComponent - 8)); // scale luma min 16
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 4) == "ymax") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "ymax";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 235.0f / 255 : (235 << (bitsPerComponent - 8)); // scale luma max 235
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 4) == "cmin") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "cmin";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? uv8tof(16) : (16 << (bitsPerComponent - 8)); // scale chroma min 16
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 4) == "cmax") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "cmax";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? uv8tof(240) : (240 << (bitsPerComponent - 8)); // scale chroma max 240
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 10) == "range_size") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_size";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 1.0f : (1 << bitsPerComponent); // 1.0, 256, 1024,... 65536
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 9) == "range_min") // avs+ > r2636
        {
          int loadIndex = -1;
          std::string toFind = "range_min";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          // 0.0 (or -0.5 for zero based 32bit float chroma)
          float q = bitsPerComponent == 32 ? (chroma ? uv8tof(128) - 0.5f : 0.0f) : 0;
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 9) == "range_max") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_max";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          // 1.0 (or 0.5 for zero based 32bit float chroma), 255, 1023,... 65535
          float q = bitsPerComponent == 32 ? (chroma ? uv8tof(128) + 0.5f : 1.0f) : ((1 << bitsPerComponent) - 1);
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 10) == "range_half") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_half";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Expr: Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Expr: Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          // for chroma: range_half is 0.0 for 32bit float (or 0.5 for old float chroma representation)
          float q = bitsPerComponent == 32 ? (chroma ? uv8tof(128) : 0.5f) : (1 << (bitsPerComponent - 1)); // 0.5f, 128, 512, ... 32768
          LOAD_OP(opLoadConst, q, 0);
        }
        // "scaleb" and "scalef" functions scale their operand from 8 bit to the bit depth of the first clip.
        // "i8", "i10", "i14", "i16" and "f32" (typically at the beginning of the expression) sets the scale-base to 8..16 bits or float, respectively.
        // "i8".."f32" keywords can appear anywhere in the expression, but only the last occurence will be effective for the whole expression.
        else if (tokens[i] == "scaleb") // avs+, scale by bit shift
        {
          if (targetBitDepth > autoScaleSourceBitDepth) // scale constant from 8 bits to 10 bit target: *4
          {
            // upscale
            if (targetBitDepth == 32) { // upscale to float
              // divide by max, e.g. x -> x/255
              // for new 0-based chroma  : x -> (x-src_chroma_middle)/factor;
              // for old 0.5 based chroma: x -> (x-src_chroma_middle)/factor + 0.5;
              if (chroma) {
                int src_middle_chroma = 1 << (autoScaleSourceBitDepth - 1);
                LOAD_OP(opLoadConst, (float)src_middle_chroma, 0);
                TWO_ARG_OP(opSub);

                float q = (float)((1 << autoScaleSourceBitDepth) - 1);
                LOAD_OP(opLoadConst, 1.0f / q, 0);
                TWO_ARG_OP(opMul);
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
                LOAD_OP(opLoadConst, 0.5f, 0);
                TWO_ARG_OP(opAdd);
#endif
                // (x-src_middle_chroma)/factor + target_chroma_middle
              }
              else
              {
                float q = (float)((1 << autoScaleSourceBitDepth) - 1);
                LOAD_OP(opLoadConst, 1.0f / q, 0);
                TWO_ARG_OP(opMul);
              }
            }
            else {
              // shift left by (targetBitDepth - currentBaseBitDepth), that is mul by (1 << (targetBitDepth - currentBaseBitDepth))
              int shifts_to_left = targetBitDepth - autoScaleSourceBitDepth;
              float q = (float)(1 << shifts_to_left);
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opMul);
            }
          }
          else if (targetBitDepth < autoScaleSourceBitDepth) // scale constant from 12 bits to 8 bit target: /16
          {
            // downscale
            if (autoScaleSourceBitDepth == 32) {
              // scale 32 bits (0..1.0) -> 8-16 bits
              // for new 0-based chroma: -0.5..0.5 -> 8*16 bits 0..max-1;
              // for old 0.5-based chroma: 0.0..1.0 -> 8*16 bits 0..max-1;
              if (chroma) {
                // (x-src_middle_chroma)*factor + target_middle_chroma
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
                LOAD_OP(opLoadConst, 0.5f, 0);
                TWO_ARG_OP(opSub);
#endif
                float q = (float)((1 << targetBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);

                int target_middle_chroma = 1 << (targetBitDepth - 1);
                LOAD_OP(opLoadConst, (float)target_middle_chroma, 0);
                TWO_ARG_OP(opAdd);
              }
              else
              {
                float q = (float)((1 << targetBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);
              }
            }
            else {
              // shift right by (targetBitDepth - currentBaseBitDepth), that is div by (1 << (currentBaseBitDepth - targetBitDepth))
              int shifts_to_right = autoScaleSourceBitDepth - targetBitDepth;
              float q = (float)(1 << shifts_to_right); // e.g. / 4.0  -> mul by 1/4.0 faster
              LOAD_OP(opLoadConst, 1.0f / q, 0);
              TWO_ARG_OP(opMul);
            }
          }
          else {
            // no scaling is needed. Bit depth of constant is the same as of the reference clip 
          }
        }
        else if (tokens[i] == "scalef") // avs+, scale by full scale
        {
          if (targetBitDepth > autoScaleSourceBitDepth) // scale constant from 8 bits to 10 bit target: *4
          {
            // upscale
            if (targetBitDepth == 32) { // upscale to float
              if (chroma) {
                int src_middle_chroma = 1 << (autoScaleSourceBitDepth - 1);
                LOAD_OP(opLoadConst, (float)src_middle_chroma, 0);
                TWO_ARG_OP(opSub);

                float q = (float)((1 << autoScaleSourceBitDepth) - 1); // divide by max, e.g. x -> x/255
                LOAD_OP(opLoadConst, 1.0f / q, 0);
                TWO_ARG_OP(opMul);
                // (x-src_middle_chroma)*factor + target_middle_chroma
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
                LOAD_OP(opLoadConst, 0.5f, 0);
                TWO_ARG_OP(opAdd);
#endif
              }
              else {
                float q = (float)((1 << autoScaleSourceBitDepth) - 1); // divide by max, e.g. x -> x/255
                LOAD_OP(opLoadConst, 1.0f / q, 0);
                TWO_ARG_OP(opMul);
              }
            }
            else {
              // keep max pixel value e.g. 8->12 bits: x * 4095.0 / 255.0
              float q = (float)((1 << targetBitDepth) - 1) / (float)((1 << autoScaleSourceBitDepth) - 1);
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opMul);
            }
          }
          else if (targetBitDepth < autoScaleSourceBitDepth)
          {
            // downscale
            if (autoScaleSourceBitDepth == 32) {
              // float->integer
              // scale 32 bits (0..1.0) -> 8-16 bits
              // for new 0-based chroma: -0.5..0.5 -> 8*16 bits 0..max-1;
              // for old 0.5-based chroma: 0.0..1.0 -> 8*16 bits 0..max-1;
              if (chroma) {
                // (x-src_middle_chroma)*factor + target_middle_chroma
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
                LOAD_OP(opLoadConst, 0.5f, 0);
                TWO_ARG_OP(opSub);
#endif
                const float q = (float)((1 << targetBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);

                const int target_middle_chroma = 1 << (targetBitDepth - 1);
                LOAD_OP(opLoadConst, (float)target_middle_chroma, 0);
                TWO_ARG_OP(opAdd);
              }
              else
              {
                const float q = (float)((1 << targetBitDepth) - 1);
                LOAD_OP(opLoadConst, q, 0);
                TWO_ARG_OP(opMul);
              }
            }
            else {
              // integer->integer
              // keep max pixel value e.g. 12->8 bits: x * 255.0 / 4095.0
              const float q = (float)((1 << targetBitDepth) - 1) / (float)((1 << autoScaleSourceBitDepth) - 1);
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opMul);
            }
          }
          else {
            // no scaling is needed. Bit depth of constant is the same as of the reference clip 
          }
        }
        else if (tokens[i] == "i8") // avs+
        {
          autoScaleSourceBitDepth = 8;
        }
        else if (tokens[i] == "i10") // avs+
        {
          autoScaleSourceBitDepth = 10;
        }
        else if (tokens[i] == "i12") // avs+
        {
          autoScaleSourceBitDepth = 12;
        }
        else if (tokens[i] == "i14") // avs+
        {
          autoScaleSourceBitDepth = 14;
        }
        else if (tokens[i] == "i16") // avs+
        {
          autoScaleSourceBitDepth = 16;
        }
        else if (tokens[i] == "f32") // avs+
        {
          autoScaleSourceBitDepth = 32;
        }
        else {
          // parse a number
            float f;
            std::string s;
            std::istringstream numStream(tokens[i]);
            numStream.imbue(std::locale::classic());
            if (!(numStream >> f))
              env->ThrowError("Expr: Failed to convert '%s' to float", tokens[i].c_str());
            if (numStream >> s)
              env->ThrowError("Expr: Failed to convert '%s' to float, not the whole token could be converted", tokens[i].c_str());
            LOAD_OP(opLoadConst, f, 0);
        }
    }

    if (tokens.size() > 0) {
        if (stackSize != 1)
            env->ThrowError("Expr: Stack unbalanced at end of expression. Need to have exactly one value on the stack to return.");

        // When scale_inputs option was used for scaling input to a common internal range, 
        // we have to scale pixels before storing them back
        // need any conversion?
        if (autoScaleSourceBitDepth != targetBitDepth && (autoconv_conv_int || autoconv_conv_float)) {
          // We can be sure that internal source was not 32 bits float: 
          // (was checked during input conversion)
          if (targetBitDepth != 32 && autoconv_conv_int) {
            // convert back internal integer to other integer
            if (autoconv_full_scale) {
              // e.g. conversion from 8 to 16 bits fullscale: /255.0*65535.0
              const float strech_mul = (float)(1 << (targetBitDepth - 1)) / (float)(1 << (autoScaleSourceBitDepth - 1));
              LOAD_OP_NOTOKEN(opLoadConst, strech_mul, 0);
              TWO_ARG_OP_NOTOKEN(opMul);
            }
            else {
              if (targetBitDepth > autoScaleSourceBitDepth)
              {
                // not fullscale, e.g. conversion from 8 to 16 bits YUV: *256
                const float shift_mul = (float)(1 << (targetBitDepth - autoScaleSourceBitDepth));
                LOAD_OP_NOTOKEN(opLoadConst, shift_mul, 0);
                TWO_ARG_OP_NOTOKEN(opMul);
              }
              else {
                // not fullscale, e.g. conversion from 16 to 8 bits YUV: *256
                const float shift_mul = 1.0f / (float)(1 << (autoScaleSourceBitDepth - targetBitDepth));
                LOAD_OP_NOTOKEN(opLoadConst, shift_mul, 0);
                TWO_ARG_OP_NOTOKEN(opMul);
              }
            }
          }
          else if (targetBitDepth == 32 && autoconv_conv_float) {
            // For targetBitDepth == 32 we are converting 8-16 bits integer back to float
            // No difference between full_scale or not
            // 8-16 bits -> scale 32 bits (0..1.0)
            // for new 0-based chroma: 8*16 bits 0..max-1 -> -0.5..0.5
            // for old 0.5-based chroma:8*16 bits 0..max-1 -> 0.0..1.0
            if (chroma) {
              // (x-src_middle_chroma)*factor + target_middle_chroma
              int src_middle_chroma = 1 << (autoScaleSourceBitDepth - 1);
              LOAD_OP_NOTOKEN(opLoadConst, (float)src_middle_chroma, 0);
              TWO_ARG_OP_NOTOKEN(opSub);

              float q = (float)((1 << autoScaleSourceBitDepth) - 1);
              LOAD_OP_NOTOKEN(opLoadConst, 1.0f / q, 0);
              TWO_ARG_OP_NOTOKEN(opMul);

#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
              LOAD_OP_NOTOKEN(opLoadConst, 0.5f, 0);
              TWO_ARG_OP_NOTOKEN(opAdd);
#endif
            }
            else
            {
              // 0..max-1 -> 0.0..1.0
              float q = (float)((1 << autoScaleSourceBitDepth) - 1);
              LOAD_OP_NOTOKEN(opLoadConst, 1.0f / q, 0);
              TWO_ARG_OP_NOTOKEN(opMul);
            }
          }
        } // end of scale inputs

        if (clamp_float && targetBitDepth == 32) {
          if (chroma) {
#ifdef FLOAT_CHROMA_IS_HALF_CENTERED
            LOAD_OP_NOTOKEN(opLoadConst, 0.0f, 0);
            TWO_ARG_OP_NOTOKEN(opMax);
            LOAD_OP_NOTOKEN(opLoadConst, 1.0f, 0);
            TWO_ARG_OP_NOTOKEN(opMin);
#else
            LOAD_OP_NOTOKEN(opLoadConst, -0.5f, 0);
            TWO_ARG_OP_NOTOKEN(opMax);
            LOAD_OP_NOTOKEN(opLoadConst, 0.5f, 0);
            TWO_ARG_OP_NOTOKEN(opMin);
#endif
          }
          else
          {
            LOAD_OP_NOTOKEN(opLoadConst, 0.0f, 0);
            TWO_ARG_OP_NOTOKEN(opMax);
            LOAD_OP_NOTOKEN(opLoadConst, 1.0f, 0);
            TWO_ARG_OP_NOTOKEN(opMin);
          }
        }

        // and finally store it
        ops.push_back(storeOp);
    }

    return maxStackSize;
}

static float calculateOneOperand(uint32_t op, float a) {
    switch (op) {
        case opSqrt:
            return std::sqrt(a);
        case opAbs:
            return std::abs(a);
        case opNeg:
            return (a > 0) ? 0.0f : 1.0f;
        case opExp:
            return std::exp(a);
        case opLog:
            return std::log(a);
        case opSin:
          return std::sin(a);
        case opCos:
          return std::cos(a);
        case opTan:
          return std::tan(a);
        case opAsin:
          return std::asin(a);
        case opAcos:
          return std::acos(a);
        case opAtan:
          return std::atan(a);
    }

    return 0.0f;
}

static float calculateTwoOperands(uint32_t op, float a, float b) {
    switch (op) {
        case opAdd:
            return a + b;
        case opSub:
            return a - b;
        case opMul:
            return a * b;
        case opDiv:
            return a / b;
        case opFmod:
            return std::fmod(a, b);
        case opMax:
            return std::max(a, b);
        case opMin:
            return std::min(a, b);
        case opGt:
            return (a > b) ? 1.0f : 0.0f;
        case opLt:
            return (a < b) ? 1.0f : 0.0f;
        case opEq:
            return (a == b) ? 1.0f : 0.0f;
        case opNotEq:
            return (a != b) ? 1.0f : 0.0f;
        case opLE:
            return (a <= b) ? 1.0f : 0.0f;
        case opGE:
            return (a >= b) ? 1.0f : 0.0f;
        case opAnd:
            return (a > 0 && b > 0) ? 1.0f : 0.0f;
        case opOr:
            return (a > 0 || b > 0) ? 1.0f : 0.0f;
        case opXor:
            return ((a > 0) != (b > 0)) ? 1.0f : 0.0f;
        case opPow:
            return std::pow(a, b);
    }

    return 0.0f;
}

static int numOperands(uint32_t op) {
    switch (op) {
        case opLoadConst:
        case opLoadSrc8:
        case opLoadSrc16:
        case opLoadSrcF32:
        case opLoadSrcF16:
        case opLoadRelSrc8:
        case opLoadRelSrc16:
        case opLoadRelSrcF32:
        case opDup:
        case opLoadSpatialX:
        case opLoadSpatialY:
        case opLoadVar:
        case opLoadInternalVar:
          return 0;

        case opSqrt:
        case opAbs:
        case opNeg:
        case opExp:
        case opLog:
        case opStoreVar:
        case opStoreAndPopVar:
        case opSin:
        case opCos:
        case opTan:
        case opAsin:
        case opAcos:
        case opAtan:
          return 1;

        case opSwap:
        case opAdd:
        case opSub:
        case opMul:
        case opDiv:
        case opFmod:
        case opMax:
        case opMin:
        case opGt:
        case opLt:
        case opEq:
        case opNotEq:
        case opLE:
        case opGE:
        case opAnd:
        case opOr:
        case opXor:
        case opPow:
            return 2;

        case opTernary:
        case opClip:
            return 3;
    }

    return 0;
}

static bool isLoadOp(uint32_t op) {
    switch (op) {
        case opLoadConst:
        case opLoadSrc8:
        case opLoadSrc16:
        case opLoadSrcF32:
        case opLoadSrcF16:
        case opLoadRelSrc8:
        case opLoadRelSrc16:
        case opLoadRelSrcF32:
        case opLoadSpatialX:
        case opLoadSpatialY:
        case opLoadVar:
        case opLoadInternalVar:
          return true;
    }

    return false;
}

static void findBranches(std::vector<ExprOp> &ops, size_t pos, size_t *start1, size_t *start2, size_t *start3) {
    int operands = numOperands(ops[pos].op);

    size_t temp1, temp2, temp3;

    if (operands == 0) {
        *start1 = pos;
    } else if (operands == 1) {
        if (isLoadOp(ops[pos - 1].op)) {
            *start1 = pos - 1;
        } else {
            findBranches(ops, pos - 1, &temp1, &temp2, &temp3);
            *start1 = temp1;
        }
    } else if (operands == 2) {
        if (isLoadOp(ops[pos - 1].op)) {
            *start2 = pos - 1;
        } else {
            findBranches(ops, pos - 1, &temp1, &temp2, &temp3);
            *start2 = temp1;
        }

        if (isLoadOp(ops[*start2 - 1].op)) {
            *start1 = *start2 - 1;
        } else {
            findBranches(ops, *start2 - 1, &temp1, &temp2, &temp3);
            *start1 = temp1;
        }
    } else if (operands == 3) {
        if (isLoadOp(ops[pos - 1].op)) {
            *start3 = pos - 1;
        } else {
            findBranches(ops, pos - 1, &temp1, &temp2, &temp3);
            *start3 = temp1;
        }

        if (isLoadOp(ops[*start3 - 1].op)) {
            *start2 = *start3 - 1;
        } else {
            findBranches(ops, *start3 - 1, &temp1, &temp2, &temp3);
            *start2 = temp1;
        }

        if (isLoadOp(ops[*start2 - 1].op)) {
            *start1 = *start2 - 1;
        } else {
            findBranches(ops, *start2 - 1, &temp1, &temp2, &temp3);
            *start1 = temp1;
        }
    }
}

/*
#define PAIR(x) { x, #x }
static std::unordered_map<uint32_t, std::string> op_strings = {
        PAIR(opLoadSrc8),
        PAIR(opLoadSrc16),
        PAIR(opLoadSrcF32),
        PAIR(opLoadSrcF16),
        PAIR(opLoadRelSrc8),
        PAIR(opLoadRelSrc16),
        PAIR(opLoadRelSrcF32),
        PAIR(opLoadSpatialX),
        PAIR(opLoadSpatialY),
        PAIR(opLoadConst),
        PAIR(opLoadInternalVars),
        PAIR(opLoadVar)
        PAIR(opLoadAndPopVar)
        PAIR(opStoreVar)
        PAIR(opStore8),
        PAIR(opStore10),
        PAIR(opStore12),
        PAIR(opStore14),
        PAIR(opStore16),
        PAIR(opStoreF32),
        PAIR(opStoreF16),
        PAIR(opDup),
        PAIR(opSwap),
        PAIR(opAdd),
        PAIR(opSub),
        PAIR(opMul),
        PAIR(opDiv),
        PAIR(opFmod),
        PAIR(opMax),
        PAIR(opMin),
        PAIR(opSqrt),
        PAIR(opAbs),
        PAIR(opGt),
        PAIR(opLt),
        PAIR(opEq),
        PAIR(opNotEq),
        PAIR(opLE),
        PAIR(opGE),
        PAIR(opTernary),
        PAIR(opClip),
        PAIR(opAnd),
        PAIR(opOr),
        PAIR(opXor),
        PAIR(opNeg),
        PAIR(opExp),
        PAIR(opLog),
        PAIR(opPow)
        PAIR(opSin)
        PAIR(opCos)
        PAIR(opTan)
        PAIR(opAsin)
        PAIR(opAcos)
        PAIR(opAtan)
      };
#undef PAIR


static void printExpression(const std::vector<ExprOp> &ops) {
    fprintf(stderr, "Expression: '");

    for (size_t i = 0; i < ops.size(); i++) {
        fprintf(stderr, " %s", op_strings[ops[i].op].c_str());

        if (ops[i].op == opLoadConst)
            fprintf(stderr, "(%.3f)", ops[i].e.fval);
        else if (isLoadOp(ops[i].op))
            fprintf(stderr, "(%d)", ops[i].e.ival);
    }

    fprintf(stderr, "'\n");
}
*/

static void foldConstants(std::vector<ExprOp> &ops) {
    for (size_t i = 0; i < ops.size(); i++) {
        switch (ops[i].op) {
          // optimize pow
        case opPow:
          if (ops[i - 1].op == opLoadConst) {
            if (ops[i - 1].e.fval == 0.5f) {
              // replace pow 0.5 with sqrt
              ops[i].op = opSqrt;
              ops.erase(ops.begin() + i - 1);
              i--;
            }
            else if (ops[i - 1].e.fval == 1.0f) {
              // replace pow 1 with nothing
              ops.erase(ops.begin() + i - 1, ops.begin() + i + 1);
              i -= 2;
            }
            else if (ops[i - 1].e.fval == 2.0f) {
              // replace pow 2 with dup *
              ops[i].op = opMul;
              ops[i - 1].op = opDup; ops[i - 1].e.ival = 0; // dup 0
              i--;
            }
            else if (ops[i - 1].e.fval == 3.0f) {
              // replace pow 3 with dup dup * *
              ops[i].op = opMul;
              ops[i - 1].op = opMul;
              ExprOp extraDup(opDup, 0);
              ops.insert(ops.begin() + i - 1, extraDup);
              ops.insert(ops.begin() + i - 1, extraDup);
              i--;
            }
            else if (ops[i - 1].e.fval == 4.0f) {
              // replace pow 4 with dup * dup *
              ops[i].op = opMul;
              ops[i - 1].op = opDup; ops[i - 1].e.ival = 0; // dup 0
              ExprOp extraMul(opMul);
              ExprOp extraDup(opDup, 0);
              ops.insert(ops.begin() + i - 1, extraMul);
              ops.insert(ops.begin() + i - 1, extraDup);
              i--;
            }
          }
          break;
          // optimize Mul 1 Div 1
        case opMul: case opDiv:
          if (ops[i - 1].op == opLoadConst) {
            if (ops[i - 1].e.fval == 1.0f) {
              // replace mul 1 or div 1 with nothing
              ops.erase(ops.begin() + i - 1, ops.begin() + i + 1);
              i -= 2;
            }
          }
          break;
          // optimize Add 0 or Sub 0
        case opAdd: case opSub:
          if (ops[i - 1].op == opLoadConst) {
            if (ops[i - 1].e.fval == 0.0f) {
              // replace add 0 or sub 0 with nothing
              ops.erase(ops.begin() + i - 1, ops.begin() + i + 1);
              i -= 2;
            }
          }
          break;
          
        }
        
        // fold constant
        switch (ops[i].op) {
            case opDup:
              if (ops[i - 1].op == opLoadConst && ops[i].e.ival == 0) {
                ops[i] = ops[i - 1];
              }
              break;

            case opSqrt:
            case opAbs:
            case opNeg:
            case opExp:
            case opLog:
            case opSin:
            case opCos:
            case opTan:
            case opAsin:
            case opAcos:
            case opAtan:
              if (ops[i - 1].op == opLoadConst) {
                ops[i].e.fval = calculateOneOperand(ops[i].op, ops[i - 1].e.fval);
                ops[i].op = opLoadConst;
                ops.erase(ops.begin() + i - 1);
                i--;
              }
              break;

            case opSwap:
              if (ops[i - 2].op == opLoadConst && ops[i - 1].op == opLoadConst && ops[i].e.ival == 1) {
                const float temp = ops[i - 2].e.fval;
                ops[i - 2].e.fval = ops[i - 1].e.fval;
                ops[i - 1].e.fval = temp;
                ops.erase(ops.begin() + i);
                i--;
              }
              break;

            case opAdd:
            case opSub:
            case opMul:
            case opDiv:
            case opFmod:
            case opMax:
            case opMin:
            case opGt:
            case opLt:
            case opEq:
            case opNotEq:
            case opLE:
            case opGE:
            case opAnd:
            case opOr:
            case opXor:
            case opPow:
              if (ops[i - 2].op == opLoadConst && ops[i - 1].op == opLoadConst) {
                ops[i].e.fval = calculateTwoOperands(ops[i].op, ops[i - 2].e.fval, ops[i - 1].e.fval);
                ops[i].op = opLoadConst;
                ops.erase(ops.begin() + i - 2, ops.begin() + i);
                i -= 2;
              }
              break;

            case opTernary:
              size_t start1, start2, start3;
              findBranches(ops, i, &start1, &start2, &start3);
              if (ops[start2 - 1].op == opLoadConst) {
                ops.erase(ops.begin() + i);
                if (ops[start1].e.fval > 0.0f) {
                  ops.erase(ops.begin() + start3, ops.begin() + i);
                  i = start3;
                } else {
                  ops.erase(ops.begin() + start2, ops.begin() + start3);
                  i -= start3 - start2;
                }
                ops.erase(ops.begin() + start1);
                i -= 2;
              }
              break;
      }
    }
}

Exprfilter::Exprfilter(const std::vector<PClip>& _child_array, const std::vector<std::string>& _expr_array, const char *_newformat, const bool _optAvx2, 
  const bool _optSingleMode, const bool _optSSE2, const std::string _scale_inputs, const bool _clamp_float, IScriptEnvironment *env) :
  children(_child_array), expressions(_expr_array), optAvx2(_optAvx2), optSingleMode(_optSingleMode), optSSE2(_optSSE2), scale_inputs(_scale_inputs), clamp_float(_clamp_float) {

  vi = children[0]->GetVideoInfo();
  d.vi = vi;

  // parse "scale_inputs"
  autoconv_full_scale = false;
  autoconv_conv_float = false;
  autoconv_conv_int = false;

  if (scale_inputs == "allf") {
    autoconv_full_scale = true;
    autoconv_conv_int = true;
    autoconv_conv_float = true;
  }
  else if (scale_inputs == "intf") {
    autoconv_full_scale = true;
    autoconv_conv_int = true;
  }
  else if (scale_inputs == "floatf") {
    autoconv_full_scale = true;
    autoconv_conv_float = true;
  }
  else if (scale_inputs == "all") {
    autoconv_conv_int = true;
    autoconv_conv_float = true;
  }
  else if (scale_inputs == "int") {
    autoconv_conv_int = true;
  }
  else if (scale_inputs == "float") {
    autoconv_conv_float = true;
  }
  else if (scale_inputs != "none") {
    env->ThrowError("Expr: scale_inputs must be 'all','allf','int','intf','float','floatf' or 'none'");
  }

  try {
    d.numInputs = (int)children.size(); // d->numInputs = vsapi->propNumElements(in, "clips");
    if (d.numInputs > 26)
      env->ThrowError("Expr: More than 26 input clips provided");
    
    for (int i = 0; i < d.numInputs; i++)
      d.node[i] = children[i];

    // checking formats
    const VideoInfo *vi_array[MAX_EXPR_INPUTS] = {};
    for (int i = 0; i < d.numInputs; i++)
      if (d.node[i])
        vi_array[i] = &d.node[i]->GetVideoInfo();


    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A }; // for checking GBR order is OK
    int *plane_enums = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;
    const int plane_enum = plane_enums[1]; // for subsampling check U only

    // check all clips against first one
    for (int i = 0; i < d.numInputs; i++) {

      int *plane_enums_i = (vi_array[i]->IsYUV() || vi_array[i]->IsYUVA()) ? planes_y : planes_r;
      const int plane_enum_i = plane_enums_i[1];

      if (vi_array[0]->NumComponents() != vi_array[i]->NumComponents() // number of planes should match
        || 
        ( !vi_array[0]->IsY() && ( // no subsampling for Y
           vi_array[0]->GetPlaneWidthSubsampling(plane_enum) != vi_array[i]->GetPlaneWidthSubsampling(plane_enum_i)
           || vi_array[0]->GetPlaneHeightSubsampling(plane_enum) != vi_array[i]->GetPlaneHeightSubsampling(plane_enum_i)
          )
        )
        || vi_array[0]->width != vi_array[i]->width
        || vi_array[0]->height != vi_array[i]->height)
        env->ThrowError("Expr: All inputs must have the same number of planes and the same dimensions, subsampling included");

      if (vi_array[i]->IsRGB() && !vi_array[i]->IsPlanar())
        env->ThrowError("Expr: No packed RGB format allowed for clip #%d, use planar RGB instead",i+1);
      if (vi_array[i]->IsYUY2())
        env->ThrowError("Expr: YUY2 format not allowed for clip #%d", i+1);
    }

    // format override
    if (_newformat != nullptr) {
      int pixel_type = GetPixelTypeFromName(_newformat);
      if (pixel_type == VideoInfo::CS_UNKNOWN)
        env->ThrowError("Expr: Invalid video format string parameter");
      d.vi.pixel_type = pixel_type;
      if(d.vi.IsRGB() && !d.vi.IsPlanar())
        env->ThrowError("Expr: No packed RGB format allowed");
      if (d.vi.IsYUY2())
        env->ThrowError("Expr: YUY2 format not allowed");

      const bool isSinglePlaneInput = vi_array[0]->IsY();

      // input number of planes >= output planes
      if (!isSinglePlaneInput && vi_array[0]->NumComponents() < d.vi.NumComponents())
        env->ThrowError("Expr: number of planes in input should be greater than or equal than of output");

      // subsampling should match
      int *plane_enums_s = (vi_array[0]->IsYUV() || vi_array[0]->IsYUVA()) ? planes_y : planes_r;
      int *plane_enums_d = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;
      for (int p = 0; p < d.vi.NumComponents(); p++) {
        const int plane_enum_s = isSinglePlaneInput ? plane_enums_s[0] : plane_enums_s[p]; // for Y inputs, reference is Y for each output plane
        const int plane_enum_d = plane_enums_d[p];
        if (vi_array[0]->GetPlaneWidthSubsampling(plane_enum_s) != d.vi.GetPlaneWidthSubsampling(plane_enum_d)
          || vi_array[0]->GetPlaneHeightSubsampling(plane_enum_s) != d.vi.GetPlaneHeightSubsampling(plane_enum_d)) {
          if(isSinglePlaneInput)
            env->ThrowError("Expr: output must not be a subsampled format for Y-only input(s)");
          else
            env->ThrowError("Expr: inputs and output must have the same subsampling");
        }
      }

      vi = d.vi;
    }

    // check expression count, duplicate omitted expressions from previous one
    int nexpr = (int)expressions.size();
    if (nexpr > d.vi.NumComponents()) // ->numPlanes)
      env->ThrowError("Expr: More expressions given than there are planes");

    std::string expr[4]; // 4th: alpha
    for (int i = 0; i < nexpr; i++)
      expr[i] = expressions[i];
    if (nexpr == 1) {
      expr[1] = expr[0];
      expr[2] = expr[0]; // e.g. exprU = exprV = exprY
    }
    else if (nexpr == 2) {
      expr[2] = expr[1]; // e.g. exprV = exprU
    }
    if(nexpr <= 3)
      expr[3] = ""; // do not use previous expression to alpha expr. Default: "" (copy)

    // default: all clips unused
    for (int i = 0; i < MAX_EXPR_INPUTS; i++) {
      d.clipsUsed[i] = false;
    }

    for (int i = 0; i < 4; i++) {
      if (!expr[i].empty()) {
        d.plane[i] = poProcess;
      }
      else {
        if (d.vi.BitsPerComponent() == vi_array[0]->BitsPerComponent()) {
          d.plane[i] = poCopy; // copy only when target clip format bit depth == 1st clip's bit depth
          d.planeCopySourceClip[i] = 0; // default source clip from empty expression: first one
          d.clipsUsed[0] = true; // mark clip to have its GetFrame
        }
        else
          d.plane[i] = poUndefined;
      }
    }

    d.maxStackSize = 0;
    for (int i = 0; i < d.vi.NumComponents(); i++) {
      const int plane_enum = plane_enums[i];
      const int planewidth = d.vi.width >> d.vi.GetPlaneWidthSubsampling(plane_enum);
      const int planeheight = d.vi.height >> d.vi.GetPlaneHeightSubsampling(plane_enum);
      const bool chroma = (plane_enum == PLANAR_U || plane_enum == PLANAR_V);
      d.maxStackSize = std::max(parseExpression(expr[i], d.ops[i], vi_array, &d.vi, getStoreOp(&d.vi), d.numInputs, planewidth, planeheight, chroma, 
        autoconv_full_scale, autoconv_conv_int, autoconv_conv_float, clamp_float,
        env), d.maxStackSize);
      foldConstants(d.ops[i]);

      // optimize constant store, change operation to "fill"
      if (d.plane[i] == poProcess && d.ops[i].size() == 2 && d.ops[i][0].op == opLoadConst) {
        uint32_t op = d.ops[i][1].op;
        if (op == opStore8 || op == opStore10 || op == opStore12 || op == opStore14 || op == opStore16 || op == opStoreF32)
        {
          d.plane[i] = poFill;
          d.planeFillValue[i] = d.ops[i][0].e.fval;
        }
      }

      // optimize single clip letter in expression: Load-Store. Change operation to "copy"
      // no relative loads here
      if (d.plane[i] == poProcess && d.ops[i].size() == 2 && 
        (d.ops[i][0].op == opLoadSrc8 || d.ops[i][0].op == opLoadSrc16 || d.ops[i][0].op == opLoadSrcF16 || d.ops[i][0].op == opLoadSrcF32) &&
        (d.ops[i][1].op == opStore8 || d.ops[i][1].op == opStore10 || d.ops[i][1].op == opStore12 || d.ops[i][1].op == opStore14 || d.ops[i][1].op == opStore16 || d.ops[i][1].op == opStoreF16 || d.ops[i][1].op == opStoreF32))
      {
        const int sourceClip = d.ops[i][0].e.ival;
        // check target vs source bit depth
        if(d.vi.BitsPerComponent() == vi_array[sourceClip]->BitsPerComponent()) // no 16bit float in avs+
        {
          d.plane[i] = poCopy;
          d.planeCopySourceClip[i] = sourceClip;
          d.clipsUsed[sourceClip] = true; // mark clip to have its GetFrame
        }
      }

      // optimize: mark referenced input clips in order to not call GetFrame for unused inputs
      for (size_t j = 0; j < d.ops[i].size(); j++) {
        const uint32_t op = d.ops[i][j].op;
        if (op == opLoadSrc8 || op == opLoadSrc16 || op == opLoadSrcF16 || op == opLoadSrcF32 ||
           op == opLoadRelSrc8 || op == opLoadRelSrc16 || op == opLoadRelSrcF32)
        {
          const int sourceClip = d.ops[i][j].e.ival;
          d.clipsUsed[sourceClip] = true;
        }
      }

      // Check CPU instuction level constraints:
      // opLoadRel8/16/32: minimum SSSE3 (pshufb, alignr) for SIMD, and no AVX2 support
      // Trig.func: C only
      d.planeOptAvx2[i] = optAvx2;
      d.planeOptSSE2[i] = optSSE2;
      for (size_t j = 0; j < d.ops[i].size(); j++) {
        const uint32_t op = d.ops[i][j].op;
        if (op == opLoadRelSrc8 || op == opLoadRelSrc16 || op == opLoadRelSrcF32)
        {
          d.planeOptAvx2[i] = false; // avx2 path not implemented
          if(!(env->GetCPUFlags() & CPUF_SSSE3)) // required minimum (pshufb, alignr)
            d.planeOptSSE2[i] = false;
        }
        // trig.functions C only
        if (op == opSin || op == opCos || op == opTan || op == opAsin || op == opAcos || op == opAtan) {
          d.planeOptAvx2[i] = false;
          d.planeOptSSE2[i] = false;
          break;
        }
      }

    }

#ifdef VS_TARGET_CPU_X86
    // optAvx2 can only disable avx2 when available

    for (int i = 0; i < d.vi.NumComponents(); i++) {
      if (d.plane[i] == poProcess) {

        const int plane_enum = plane_enums[i];
        int planewidth = d.vi.width >> d.vi.GetPlaneWidthSubsampling(plane_enum);
        int planeheight = d.vi.height >> d.vi.GetPlaneHeightSubsampling(plane_enum);

        if (optAvx2 && d.planeOptAvx2[i]) {

          // avx2
          ExprEvalAvx2 ExprObj(d.ops[i], d.numInputs, env->GetCPUFlags(), planewidth, planeheight, optSingleMode);
          if (ExprObj.GetCode(true) && ExprObj.GetCodeSize()) { // PF modded jitasm. true: epilog with vmovaps, and vzeroupper
#ifdef VS_TARGET_OS_WINDOWS
            d.proc[i] = (ExprData::ProcessLineProc)VirtualAlloc(nullptr, ExprObj.GetCodeSize(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
            d.proc[i] = (ExprData::ProcessLineProc)mmap(nullptr, ExprObj.GetCodeSize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
#endif
            memcpy((void *)d.proc[i], ExprObj.GetCode(), ExprObj.GetCodeSize());
          }
        }
        else if (optSSE2 && d.planeOptSSE2[i]) {
          // sse2, sse4
          ExprEval ExprObj(d.ops[i], d.numInputs, env->GetCPUFlags(), planewidth, planeheight, optSingleMode);
          if (ExprObj.GetCode() && ExprObj.GetCodeSize()) {
#ifdef VS_TARGET_OS_WINDOWS
            d.proc[i] = (ExprData::ProcessLineProc)VirtualAlloc(nullptr, ExprObj.GetCodeSize(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
            d.proc[i] = (ExprData::ProcessLineProc)mmap(nullptr, ExprObj.GetCodeSize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
#endif
            memcpy((void *)d.proc[i], ExprObj.GetCode(), ExprObj.GetCodeSize());
          }
        }


#if 0
        // under construction

        // check LUT possibility: 
        // * 8-16bit 1D lut, or 8-10 bit 2D lut
        // * bit depth of input clip(s) and output must match
        bool useLut =
          (d.numInputs == 1
            && d.vi.BitsPerComponent() <= 16
            && d.vi.BitsPerComponent() == vi_array[0]->BitsPerComponent())
          ||
          (
            d.numInputs == 2 && d.vi.BitsPerComponent() <= 8/*10*/
            && d.vi.BitsPerComponent() == vi_array[0]->BitsPerComponent()
            && d.vi.BitsPerComponent() == vi_array[1]->BitsPerComponent()
            );
        
        // todo: 
        // if there is a 'runtime' variable in the expression (framecount, relative_time), then lut is not possible

        if (useLut) {
          d.plane[i] = poLut; // change processing mode
          d.planeLutIndex[i] = i;

          int bits_per_pixel = d.vi.BitsPerComponent();
          if (d.numInputs == 1 && bits_per_pixel >= 10) // 1D lut 10-16 bits: use 16bit safety area
            bits_per_pixel = 16;

          int lut_size = (1 << bits_per_pixel);
          if (d.numInputs == 1)
            lut_size *= (1 << bits_per_pixel);
          lut_size *= d.vi.ComponentSize(); // bytes per entry

          d.luts[i].resize(lut_size); // allocate

          // fill LUT tables
          if (d.numInputs == 1)
          {
            std::vector<uint8_t> pixels(lut_size);
            uint8_t *ptr = pixels.data();
            uint8_t *ptr_out = d.luts[i].data();
            if (bits_per_pixel == 8) {
              for (int i = 0; i < (1 << bits_per_pixel); i++)
                ptr[i] = i;
            }
            else {
              for (int i = 0; i < (1 << bits_per_pixel); i++)
                reinterpret_cast<uint16_t *>(ptr)[i] = i;
            }
            // treat it as a clip. width=(1 << bits_per_pixel), height=1
            // GetReadPtr is 'ptr'
            // GetWritePtr is 'ptr_out'
            // pitch does not count, we have height==1
          }

        }
        // end of LUT
#endif
      } // if plane is to be processed
    }
#ifdef VS_TARGET_OS_WINDOWS
    if (optSSE2)
      FlushInstructionCache(GetCurrentProcess(), nullptr, 0);
#endif
#endif

  }
  catch (std::runtime_error &e) {
    for (int i = 0; i < MAX_EXPR_INPUTS; i++)
      d.node[i] = nullptr; //  vsapi->freeNode(d->node[i]);
    std::string s = "Expr: ";
    s += e.what();
    env->ThrowError(s.c_str());
    return;
  }

}
