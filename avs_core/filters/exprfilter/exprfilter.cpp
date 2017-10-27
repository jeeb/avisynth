/*
*
* Avisynth+ Expression filter, ported from the VapourSynth project
* Copyright (c) 2012-2015 Fredrik Mellbin
* 
* Additions and differences to VS r39 version:
* ------------------------------
* (similar features to the masktools mt_lut family syntax)
* Built-in constants
*   ymin, ymax (ymin_a .. ymin_z for individual clips) - the usual luma limits (16..235 or scaled equivalents)
*   cmin, cmax (cmin_a .. cmin_z) - chroma limits (16..240 or scaled equivalents)
*   range_half (range_half_a .. range_half_z) - half of the range, (128 or scaled equivalents)
*   range_size, range_half, range_max (range_size_a .. range_size_z , etc..)
* Autoscale helper functions (operand is treated as being a 8 bit constant unless i8..i16 or f32 is specified)
*   scaleb (scale by bit shift - mul or div by 2, 4, 6, 8...)
*   scalef (scale by stretch full scale - mul or div by source_max/target_max
* Keywords for modifying base bit depth for scaleb and scalef
*   i8, i10, i12, i14, i16, f32
* Built-in math constant
*   pi
* Alpha plane handling
* Proper clamping when storing 10,12 or 14 bit outputs
* 
* Differences from masktools 2.2.9
* --------------------------------
*   Up to 26 clips are allowed (x,y,z,a,b,...w). Masktools handles only up to 4 clips with its mt_lut, my_lutxy, mt_lutxyz, mt_lutxyza
*   Clips with different bit depths are allowed
*   works with 32 bit floats instead of 64 bit double internally 
*   less functions (e.g. no bit shifts)
*   no float clamping and float-to-8bit-and-back load/store autoscale magic
*   logical 'false' is 0 instead of -1
*   avs+: ymin, ymax, etc built-in constants can have a _X suffix, where X is the corresponding clip designator letter. E.g. cmax_z, range_half_x
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

#ifdef VS_TARGET_CPU_X86

#define OneArgOp(instr) \
auto &t1 = stack.back(); \
instr(t1.first, t1.first); \
instr(t1.second, t1.second);

#define TwoArgOp(instr) \
auto t1 = stack.back(); \
stack.pop_back(); \
auto &t2 = stack.back(); \
instr(t2.first, t1.first); \
instr(t2.second, t1.second);

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

enum {
    elabsmask, elc7F, elmin_norm_pos, elinv_mant_mask,
    elfloat_one, elfloat_half, elstore8, elstore10, elstore12, elstore14, elstore16,
    elexp_hi, elexp_lo, elcephes_LOG2EF, elcephes_exp_C1, elcephes_exp_C2, elcephes_exp_p0, elcephes_exp_p1, elcephes_exp_p2, elcephes_exp_p3, elcephes_exp_p4, elcephes_exp_p5, elcephes_SQRTHF,
    elcephes_log_p0, elcephes_log_p1, elcephes_log_p2, elcephes_log_p3, elcephes_log_p4, elcephes_log_p5, elcephes_log_p6, elcephes_log_p7, elcephes_log_p8, elcephes_log_q1 = elcephes_exp_C2, elcephes_log_q2 = elcephes_exp_C1
};

#define XCONST(x) { x, x, x, x }

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

struct ExprEval : public jitasm::function<void, ExprEval, uint8_t *, const intptr_t *, intptr_t> {

    std::vector<ExprOp> ops;
    int numInputs;
    int cpuFlags;

    ExprEval(std::vector<ExprOp> &ops, int numInputs, int cpuFlags) : ops(ops), numInputs(numInputs), cpuFlags(cpuFlags) {}

    void main(Reg regptrs, Reg regoffs, Reg niter)
    {
        XmmReg zero;
        pxor(zero, zero);
        Reg constptr;
        mov(constptr, (uintptr_t)logexpconst);

        L("wloop");

        std::list<std::pair<XmmReg, XmmReg>> stack;
        for (const auto &iter : ops) {
            if (iter.op == opLoadSrc8) {
                XmmReg r1, r2;
                Reg a;
                mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + 1)]);
                movq(r1, mmword_ptr[a]);
                punpcklbw(r1, zero);
                movdqa(r2, r1);
                punpckhwd(r1, zero);
                punpcklwd(r2, zero);
                cvtdq2ps(r1, r1);
                cvtdq2ps(r2, r2);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opLoadSrc16) {
                XmmReg r1, r2;
                Reg a;
                mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + 1)]);
                movdqa(r1, xmmword_ptr[a]);
                movdqa(r2, r1);
                punpckhwd(r1, zero);
                punpcklwd(r2, zero);
                cvtdq2ps(r1, r1);
                cvtdq2ps(r2, r2);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opLoadSrcF32) {
                XmmReg r1, r2;
                Reg a;
                mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + 1)]);
                movdqa(r1, xmmword_ptr[a]);
                movdqa(r2, xmmword_ptr[a + 16]);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opLoadSrcF16) { // not supported in avs+
                XmmReg r1, r2;
                Reg a;
                mov(a, ptr[regptrs + sizeof(void *) * (iter.e.ival + 1)]);
                vcvtph2ps(r1, qword_ptr[a]);
                vcvtph2ps(r2, qword_ptr[a + 8]);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opLoadConst) {
                XmmReg r1, r2;
                Reg a;
                mov(a, iter.e.ival);
                movd(r1, a);
                shufps(r1, r1, 0);
                movaps(r2, r1);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opDup) {
                auto p = std::next(stack.rbegin(), iter.e.ival);
                XmmReg r1, r2;
                movaps(r1, p->first);
                movaps(r2, p->second);
                stack.push_back(std::make_pair(r1, r2));
            } else if (iter.op == opSwap) {
                std::swap(stack.back(), *std::next(stack.rbegin(), iter.e.ival));
            } else if (iter.op == opAdd) {
                TwoArgOp(addps)
            } else if (iter.op == opSub) {
                TwoArgOp(subps)
            } else if (iter.op == opMul) {
                TwoArgOp(mulps)
            } else if (iter.op == opDiv) {
                TwoArgOp(divps)
            } else if (iter.op == opMax) {
                TwoArgOp(maxps)
            } else if (iter.op == opMin) {
                TwoArgOp(minps)
            } else if (iter.op == opSqrt) {
                auto &t1 = stack.back();
                maxps(t1.first, zero);
                maxps(t1.second, zero);
                sqrtps(t1.first, t1.first);
                sqrtps(t1.second, t1.second);
            } else if (iter.op == opStore8) {
                auto t1 = stack.back();
                stack.pop_back();
                XmmReg r1, r2;
                Reg a;
                maxps(t1.first, zero);
                maxps(t1.second, zero);  
                minps(t1.first, CPTR(elstore8));
                minps(t1.second, CPTR(elstore8));
                mov(a, ptr[regptrs]);
                cvtps2dq(t1.first, t1.first);
                cvtps2dq(t1.second, t1.second);
                movdqa(r1, t1.first);  // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
                movdqa(r2, t1.second); // 00 w7 00 w6 00 w5 00 w4
                // new
                packssdw(t1.second, t1.first);   // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
                /* old, kept for reference until figuring out why this method was chosen
                psrldq(t1.first, 6);   // 00 00 00 00 w3 00 w2 00
                psrldq(t1.second, 6);  // 00 00 00 00 w7 00 w6 00
                por(t1.first, r1);     // 00 w3 00 w2 w3 w1 w2 w0
                por(t1.second, r2);    // 00 w7 00 w6 w7 w5 w6 w4 
                pshuflw(t1.first, t1.first, 0b11011000);   // 3-1-2-0, w3 w2 w1 w0 w3 w2 w1 w0
                pshuflw(t1.second, t1.second, 0b11011000); // 3-1-2-0, w7 w6 w5 w4 w7 w6 w5 w4
                punpcklqdq(t1.second, t1.first); // _mm_unpacklo_epi64: w7 w6 w5 w4 w3 w2 w1 w0
                */
                packuswb(t1.second, zero);       // _mm_packus_epi16: 0 0 0 0 0 0 0 0 b7 b6 b5 b4 b3 b2 b1 b0
                movq(mmword_ptr[a], t1.second);
            } else if (iter.op == opStore10 // avs+
              || iter.op == opStore12 // avs+ 
              || iter.op == opStore14 // avs+
              || iter.op == opStore16
              ) {
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
                movdqa(r1, t1.first);   // 00 w3 00 w2 00 w1 00 w0 -- min/max clamp ensures that high words are zero
                movdqa(r2, t1.second);  // 00 w7 00 w6 00 w5 00 w4
                // new
                switch (iter.op) {
                case opStore10:
                case opStore12:
                case opStore14:
                  packssdw(t1.second, t1.first);   // _mm_packs_epi32: w7 w6 w5 w4 w3 w2 w1 w0
                  break;
                case opStore16:
                  if (cpuFlags & CPUF_SSE4_1)
                    packusdw(t1.second, t1.first);   // _mm_packus_epi32: w7 w6 w5 w4 w3 w2 w1 w0
                  else {
                    // old, sse2
                    psrldq(t1.first, 6);
                    psrldq(t1.second, 6);
                    por(t1.first, r1);
                    por(t1.second, r2);
                    pshuflw(t1.first, t1.first, 0b11011000);
                    pshuflw(t1.second, t1.second, 0b11011000);
                    punpcklqdq(t1.second, t1.first);
                  }
                  break;
                }
                movdqa(xmmword_ptr[a], t1.second);
            } else if (iter.op == opStoreF32) {
                auto t1 = stack.back();
                stack.pop_back();
                Reg a;
                mov(a, ptr[regptrs]);
                movaps(xmmword_ptr[a], t1.first);
                movaps(xmmword_ptr[a + 16], t1.second);
            } else if (iter.op == opStoreF16) { // not supported in avs+
                auto t1 = stack.back();
                stack.pop_back();
                Reg a;
                mov(a, ptr[regptrs]);
                vcvtps2ph(qword_ptr[a], t1.first, 0);
                vcvtps2ph(qword_ptr[a + 8], t1.second, 0);
            } else if (iter.op == opAbs) {
                auto &t1 = stack.back();
                andps(t1.first, CPTR(elabsmask));
                andps(t1.second, CPTR(elabsmask));
            } else if (iter.op == opNeg) {
                auto &t1 = stack.back();
                cmpleps(t1.first, zero);
                cmpleps(t1.second, zero);
                andps(t1.first, CPTR(elfloat_one));
                andps(t1.second, CPTR(elfloat_one));
            } else if (iter.op == opAnd) {
                LogicOp(andps)
            } else if (iter.op == opOr) {
                LogicOp(orps)
            } else if (iter.op == opXor) {
                LogicOp(xorps)
            } else if (iter.op == opGt) {
                CmpOp(cmpltps)
            } else if (iter.op == opLt) {
                CmpOp(cmpnleps)
            } else if (iter.op == opEq) {
                CmpOp(cmpeqps)
            } else if (iter.op == opLE) {
                CmpOp(cmpnltps)
            } else if (iter.op == opGE) {
                CmpOp(cmpleps)
            } else if (iter.op == opTernary) {
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
            } else if (iter.op == opExp) {
                auto &t1 = stack.back();
                EXP_PS(t1.first)
                EXP_PS(t1.second)
            } else if (iter.op == opLog) {
                auto &t1 = stack.back();
                LOG_PS(t1.first)
                LOG_PS(t1.second)
            } else if (iter.op == opPow) {
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

        if (sizeof(void *) == 8) {
            int numIter = (numInputs + 1 + 1) / 2;

            for (int i = 0; i < numIter; i++) {
                XmmReg r1, r2;
                movdqu(r1, xmmword_ptr[regptrs + 16 * i]);
                movdqu(r2, xmmword_ptr[regoffs + 16 * i]);
                paddq(r1, r2);
                movdqu(xmmword_ptr[regptrs + 16 * i], r1);
            }
        } else {
            int numIter = (numInputs + 1 + 3) / 4;
            for (int i = 0; i < numIter; i++) {
                XmmReg r1, r2;
                movdqu(r1, xmmword_ptr[regptrs + 16 * i]);
                movdqu(r2, xmmword_ptr[regoffs + 16 * i]);
                paddd(r1, r2);
                movdqu(xmmword_ptr[regptrs + 16 * i], r1);
            }
        }

        sub(niter, 1);
        jnz("wloop");
    }
};
#endif

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

extern const AVSFunction Exprfilter_filters[] = {
  { "Expr", BUILTIN_FUNC_PREFIX, "c+s+[format]s", Exprfilter::Create },
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
      env->ThrowError("Invalid parameter type for expression string");
    }
  }

  // optional named argument: format
  const char *newformat = nullptr;
  if (args[next_paramindex].Defined()) {
    // always string
    newformat = args[next_paramindex].AsString();
  }

  return new Exprfilter(children, expressions, newformat, env);

}


PVideoFrame __stdcall Exprfilter::GetFrame(int n, IScriptEnvironment *env) {
  // ExprData d class variable already filled 
  int numInputs = d.numInputs;

  std::vector<PVideoFrame> src;
  src.reserve(children.size());

  for (const auto& child : children)
    src.emplace_back(child->GetFrame(n, env));

  PVideoFrame *srcf[4] = { d.plane[0] != poCopy ? nullptr : &src[0], d.plane[1] != poCopy ? nullptr : &src[0], d.plane[2] != poCopy ? nullptr : &src[0], d.plane[3] != poCopy ? nullptr : &src[0] };
  PVideoFrame dst = env->NewVideoFrame(d.vi);

  const uint8_t *srcp[MAX_EXPR_INPUTS] = {};
  int src_stride[MAX_EXPR_INPUTS] = {};

  const bool use_simd = !!(env->GetCPUFlags() && CPUF_SSE2); // C path only for reference

  int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
  int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
  int *plane_enums = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;

  if (use_simd) {
    intptr_t ptroffsets[MAX_EXPR_INPUTS + 1] = { d.vi.ComponentSize() * 8 };

    for (int plane = 0; plane < d.vi.NumComponents(); plane++) {

      const int plane_enum = plane_enums[plane];

      if (d.plane[plane] == poProcess) {
        for (int i = 0; i < numInputs; i++) {
          if (d.node[i]) {
            srcp[i] = src[i]->GetReadPtr(plane_enum);
            src_stride[i] = src[i]->GetPitch(plane_enum);
            ptroffsets[i + 1] = d.node[i]->GetVideoInfo().ComponentSize() * 8;
          }
        }

        uint8_t *dstp = dst->GetWritePtr(plane_enum);
        int dst_stride = dst->GetPitch(plane_enum);
        int h = d.vi.height >> d.vi.GetPlaneHeightSubsampling(plane_enum);
        int w = d.vi.width >> d.vi.GetPlaneWidthSubsampling(plane_enum);
        int niterations = (w + 7) / 8;

        ExprData::ProcessLineProc proc = d.proc[plane];

        for (int y = 0; y < h; y++) {
          const uint8_t *rwptrs[MAX_EXPR_INPUTS + 1] = { dstp + dst_stride * y };
          for (int i = 0; i < numInputs; i++)
            rwptrs[i + 1] = srcp[i] + src_stride[i] * y;
          proc(rwptrs, ptroffsets, niterations);
        }
      }
      // avs+: copy plane here
      else if (d.plane[plane] == poCopy) {
        env->BitBlt(dst->GetWritePtr(plane_enum), dst->GetPitch(plane_enum),
          src[0]->GetReadPtr(plane_enum),
          src[0]->GetPitch(plane_enum),
          src[0]->GetRowSize(plane_enum),
          src[0]->GetHeight(plane_enum)
        );
      }

    }
  }
  else {
    // C version
    std::vector<float> stackVector(d.maxStackSize);

    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *plane_enums = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;

    for (int plane = 0; plane < d.vi.NumComponents(); plane++) {

      const int plane_enum = plane_enums[plane];

      if (d.plane[plane] == poProcess) {

        for (int i = 0; i < numInputs; i++) {
          if (d.node[i]) {
            srcp[i] = src[i]->GetReadPtr(plane_enum);
            src_stride[i] = src[i]->GetPitch(plane_enum);
          }
        }

        uint8_t *dstp = dst->GetWritePtr(plane_enum);
        int dst_stride = dst->GetPitch(plane_enum);
        int h = d.vi.height >> d.vi.GetPlaneHeightSubsampling(plane_enum);
        int w = d.vi.width >> d.vi.GetPlaneWidthSubsampling(plane_enum);

        const ExprOp *vops = d.ops[plane].data();
        float *stack = stackVector.data();
        float stacktop = 0;

        for (int y = 0; y < h; y++) {
          for (int x = 0; x < w; x++) {
            int si = 0;
            int i = -1;
            while (true) {
              i++;
              switch (vops[i].op) {
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
              case opLoadConst:
                stack[si] = stacktop;
                stacktop = vops[i].e.fval;
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
              case opSqrt:
                stacktop = std::sqrt(stacktop);
                break;
              case opAbs:
                stacktop = std::abs(stacktop);
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
              }
            }
          loopend:;
          }
          dstp += dst_stride;
          for (int i = 0; i < numInputs; i++)
            srcp[i] += src_stride[i];
        }
      }
      // avs+: copy plane here
      else if (d.plane[plane] == poCopy) {
        env->BitBlt(dst->GetWritePtr(plane_enum), dst->GetPitch(plane_enum),
          src[0]->GetReadPtr(plane_enum),
          src[0]->GetPitch(plane_enum),
          src[0]->GetRowSize(plane_enum),
          src[0]->GetHeight(plane_enum)
        );
      }
    }
  }
  return dst;
}

Exprfilter::~Exprfilter() {
  for (int i = 0; i < MAX_EXPR_INPUTS; i++)
    d.node[i] = nullptr;
}

static SOperation getLoadOp(const VideoInfo *vi) {
  if (!vi)
    return opLoadSrcF32;
  if (vi->BitsPerComponent() == 32) // float, avs has no f16c float
    return opLoadSrcF32;
  else if (vi->BitsPerComponent() == 8)
    return opLoadSrc8;
  else
    return opLoadSrc16; // 10..16 bits common
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

#define LOAD_OP(op,v,req) do { if (stackSize < req) env->ThrowError("Not enough elements on stack to perform operation %s", tokens[i].c_str()); ops.push_back(ExprOp(op, (v))); maxStackSize = std::max(++stackSize, maxStackSize); } while(0)
#define GENERAL_OP(op, v, req, dec) do { if (stackSize < req) env->ThrowError("Not enough elements on stack to perform operation %s", tokens[i].c_str()); ops.push_back(ExprOp(op, (v))); stackSize-=(dec); } while(0)
#define ONE_ARG_OP(op) GENERAL_OP(op, 0, 1, 0)
#define TWO_ARG_OP(op) GENERAL_OP(op, 0, 2, 1)
#define THREE_ARG_OP(op) GENERAL_OP(op, 0, 3, 2)

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

static size_t parseExpression(const std::string &expr, std::vector<ExprOp> &ops, const VideoInfo **vi, const VideoInfo *vi_output, const SOperation storeOp, int numInputs, IScriptEnvironment *env)
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
        else if (tokens[i] == "max")
            TWO_ARG_OP(opMax);
        else if (tokens[i] == "min")
            TWO_ARG_OP(opMin);
        else if (tokens[i] == "exp")
            ONE_ARG_OP(opExp);
        else if (tokens[i] == "log")
            ONE_ARG_OP(opLog);
        else if (tokens[i] == "pow")
            TWO_ARG_OP(opPow);
        else if (tokens[i] == "sqrt")
            ONE_ARG_OP(opSqrt);
        else if (tokens[i] == "abs")
            ONE_ARG_OP(opAbs);
        else if (tokens[i] == ">")
            TWO_ARG_OP(opGt);
        else if (tokens[i] == "<")
            TWO_ARG_OP(opLt);
        else if (tokens[i] == "=")
            TWO_ARG_OP(opEq);
        else if (tokens[i] == ">=")
            TWO_ARG_OP(opGE);
        else if (tokens[i] == "<=")
            TWO_ARG_OP(opLE);
        else if (tokens[i] == "?")
            THREE_ARG_OP(opTernary);
        else if (tokens[i] == "and")
            TWO_ARG_OP(opAnd);
        else if (tokens[i] == "or")
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
                  env->ThrowError("Dup suffix can't be less than 0 '%s'", tokens[i].c_str());
                LOAD_OP(opDup, tmp, tmp + 1);
              }
              catch (std::logic_error &) {
                env->ThrowError("Failed to convert dup suffix '%s' to valid index", tokens[i].c_str());
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
                  env->ThrowError("Swap suffix can't be less than 1 '%s'", tokens[i].c_str());
                GENERAL_OP(opSwap, tmp, tmp + 1, 0);
              }
              catch (std::logic_error &) {
                env->ThrowError("Failed to convert swap suffix '%s' to valid index", tokens[i].c_str());
              }
            }
        else if (tokens[i].length() == 1 && tokens[i][0] >= 'a' && tokens[i][0] <= 'z') {
          char srcChar = tokens[i][0];
          int loadIndex;
          if (srcChar >= 'x')
            loadIndex = srcChar - 'x';
          else
            loadIndex = srcChar - 'a' + 3;
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied to reference '%s'", tokens[i].c_str());
          LOAD_OP(getLoadOp(vi[loadIndex]), loadIndex, 0);
        } else if (tokens[i] == "pi") // avs+
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
        //   range_half : autoscaled 128 or 0.5 for float
        //   range_max  : 255 / 1023 / 4095 / 16383 / 65535 or 1.0 for float
        //   range_size : 256 / 1024...65536
        //   ymin, ymax, cmin, cmax : 16 / 235 and 16 / 240 autoscaled.

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
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

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
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

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
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

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
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 16.0f / 255 : (16 << (bitsPerComponent - 8)); // scale chroma min 16
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 4) == "cmax") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "cmax";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 240.0f / 255 : (240 << (bitsPerComponent - 8)); // scale chroma max 240
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 10) == "range_size") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_size";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 1.0f : (1 << bitsPerComponent); // 1.0, 256, 1024,... 65536
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 9) == "range_max") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_max";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 1.0f : ((1 << bitsPerComponent) - 1); // 1.0, 255, 1023,... 65535
          LOAD_OP(opLoadConst, q, 0);
        }
        else if (tokens[i].substr(0, 10) == "range_half") // avs+
        {
          int loadIndex = -1;
          std::string toFind = "range_half";
          if (tokens[i].substr(0, toFind.length()) == toFind)
            loadIndex = getSuffix(tokens[i], toFind);
          if (loadIndex < 0)
            env->ThrowError("Error in built-in constant expression '%s'", tokens[i].c_str());
          if (loadIndex >= numInputs)
            env->ThrowError("Too few input clips supplied for reference '%s'", tokens[i].c_str());

          int bitsPerComponent = vi[loadIndex]->BitsPerComponent();
          float q = bitsPerComponent == 32 ? 0.5f : (1 << (bitsPerComponent - 1)); // 0.5f, 128, 512, ... 32768
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
              float q = (float)((1 << autoScaleSourceBitDepth) - 1);
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opDiv);
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
              float q = (float)((1 << targetBitDepth) - 1);
              LOAD_OP(opLoadConst, q, 0);
            }
            else {
              // shift right by (targetBitDepth - currentBaseBitDepth), that is div by (1 << (currentBaseBitDepth - targetBitDepth))
              int shifts_to_right = autoScaleSourceBitDepth - targetBitDepth;
              float q = (float)(1 << shifts_to_right); // e.g. / 4.0  -> mul by 1/4.0 faster
              LOAD_OP(opLoadConst, 1.0f / q, 0);
            }
            TWO_ARG_OP(opMul);
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
              float q = (float)((1 << autoScaleSourceBitDepth) - 1); // divide by max, e.g. x -> x/255
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opDiv);
            }
            else {
              // keep max pixel value e.g. 8->12 bits: x * 4095.0 / 255.0
              float q = (float)((1 << autoScaleSourceBitDepth) - 1) / (float)((1 << targetBitDepth) - 1);
              LOAD_OP(opLoadConst, q, 0);
              TWO_ARG_OP(opMul);
            }
          }
          else if (targetBitDepth < autoScaleSourceBitDepth) // scale constant from 12 bits to 8 bit target: /16
          {
            // downscale
            float q;
            if (autoScaleSourceBitDepth == 32)
              q = (float)((1 << targetBitDepth) - 1);
            else {
              // keep max pixel value e.g. 12->8 bits: x * 255.0 / 4095.0
              q = (float)((1 << targetBitDepth) - 1) / (float)((1 << autoScaleSourceBitDepth) - 1);
            }
            LOAD_OP(opLoadConst, q, 0);
            TWO_ARG_OP(opMul);
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
              env->ThrowError("Failed to convert '%s' to float", tokens[i].c_str());
            if (numStream >> s)
              env->ThrowError("Failed to convert '%s' to float, not the whole token could be converted", tokens[i].c_str());
            LOAD_OP(opLoadConst, f, 0);
        }
    }

    if (tokens.size() > 0) {
        if (stackSize != 1)
            env->ThrowError("Stack unbalanced at end of expression. Need to have exactly one value on the stack to return.");
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
        case opDup:
            return 0;

        case opSqrt:
        case opAbs:
        case opNeg:
        case opExp:
        case opLog:
            return 1;

        case opSwap:
        case opAdd:
        case opSub:
        case opMul:
        case opDiv:
        case opMax:
        case opMin:
        case opGt:
        case opLt:
        case opEq:
        case opLE:
        case opGE:
        case opAnd:
        case opOr:
        case opXor:
        case opPow:
            return 2;

        case opTernary:
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
        PAIR(opLoadConst),
        PAIR(opStore8),
        PAIR(opStore16),
        PAIR(opStoreF32),
        PAIR(opStoreF16),
        PAIR(opDup),
        PAIR(opSwap),
        PAIR(opAdd),
        PAIR(opSub),
        PAIR(opMul),
        PAIR(opDiv),
        PAIR(opMax),
        PAIR(opMin),
        PAIR(opSqrt),
        PAIR(opAbs),
        PAIR(opGt),
        PAIR(opLt),
        PAIR(opEq),
        PAIR(opLE),
        PAIR(opGE),
        PAIR(opTernary),
        PAIR(opAnd),
        PAIR(opOr),
        PAIR(opXor),
        PAIR(opNeg),
        PAIR(opExp),
        PAIR(opLog),
        PAIR(opPow)
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
            case opMax:
            case opMin:
            case opGt:
            case opLt:
            case opEq:
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

Exprfilter::Exprfilter(const std::vector<PClip>& _child_array, const std::vector<std::string>& _expr_array, const char *_newformat, IScriptEnvironment *env) :
  children(_child_array), expressions(_expr_array) {

  vi = children[0]->GetVideoInfo();
  d.vi = vi;

  try {
    d.numInputs = (int)children.size(); // d->numInputs = vsapi->propNumElements(in, "clips");
    if (d.numInputs > 26)
      env->ThrowError("More than 26 input clips provided");
    
    for (int i = 0; i < d.numInputs; i++)
      d.node[i] = children[i];

    // checking formats
    const VideoInfo *vi_array[MAX_EXPR_INPUTS] = {};
    for (int i = 0; i < d.numInputs; i++)
      if (d.node[i])
        vi_array[i] = &d.node[i]->GetVideoInfo();


    int planes_y[4] = { PLANAR_Y, PLANAR_U, PLANAR_V, PLANAR_A };
    int planes_r[4] = { PLANAR_G, PLANAR_B, PLANAR_R, PLANAR_A };
    int *plane_enums = (d.vi.IsYUV() || d.vi.IsYUVA()) ? planes_y : planes_r;
    const int plane_enum = plane_enums[1]; // for subsampling

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
        env->ThrowError("All inputs must have the same number of planes and the same dimensions, subsampling included");

      if (vi_array[i]->IsRGB() && !vi_array[i]->IsPlanar())
        env->ThrowError("No packed RGB format allowed for clip #%d, use planar RGB instead",i+1);
      if (vi_array[i]->IsYUY2())
        env->ThrowError("YUY2 format not allowed for clip #%d", i+1);
    }

    // format override
    if (_newformat != nullptr) {
      int pixel_type = GetPixelTypeFromName(_newformat);
      if (pixel_type == VideoInfo::CS_UNKNOWN)
        env->ThrowError("Invalid video format string parameter");
      d.vi.pixel_type = pixel_type;
      if(d.vi.IsRGB() && !d.vi.IsPlanar())
        env->ThrowError("No packed RGB format allowed");
      if (d.vi.IsYUY2())
        env->ThrowError("YUY2 format not allowed");
      vi = d.vi;
    }

    // check expression count, duplicate omitted expressions from previous one
    size_t nexpr = expressions.size();
    if (nexpr > d.vi.NumComponents()) // ->numPlanes)
      env->ThrowError("More expressions given than there are planes");

    std::string expr[4]; // 4th: alpha
    for (int i = 0; i < nexpr; i++)
      expr[i] = expressions[i];
    if (nexpr == 1) {
      expr[1] = expr[0];
      expr[2] = expr[0];
      expr[3] = expr[0];
    }
    else if (nexpr == 2) {
      expr[2] = expr[1];
      expr[3] = expr[1];
    }
    else if (nexpr == 3) { // avs+
      expr[3] = expr[2];
    }

    for (int i = 0; i < 4; i++) {
      if (!expr[i].empty()) {
        d.plane[i] = poProcess;
      }
      else {
        if (d.vi.BitsPerComponent() == vi_array[0]->BitsPerComponent())
          d.plane[i] = poCopy; // copy only when target clip format bit depth == 1st clip's bit depth
        else
          d.plane[i] = poUndefined;
      }
    }

    d.maxStackSize = 0;
    for (int i = 0; i < d.vi.NumComponents(); i++) {
      d.maxStackSize = std::max(parseExpression(expr[i], d.ops[i], vi_array, &d.vi, getStoreOp(&d.vi), d.numInputs, env), d.maxStackSize);
      foldConstants(d.ops[i]);
    }

#ifdef VS_TARGET_CPU_X86
    for (int i = 0; i < d.vi.NumComponents(); i++) {
      if (d.plane[i] == poProcess) {
        ExprEval ExprObj(d.ops[i], d.numInputs, env->GetCPUFlags());
        if (ExprObj.GetCode() && ExprObj.GetCodeSize()) {
#ifdef VS_TARGET_OS_WINDOWS
          d.proc[i] = (ExprData::ProcessLineProc)VirtualAlloc(nullptr, ExprObj.GetCodeSize(), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
#else
          d.proc[i] = (ExprData::ProcessLineProc)mmap(nullptr, ExprObj.GetCodeSize(), PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANON | MAP_PRIVATE, 0, 0);
#endif
          memcpy((void *)d.proc[i], ExprObj.GetCode(), ExprObj.GetCodeSize());
        }
      }
    }
#ifdef VS_TARGET_OS_WINDOWS
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
