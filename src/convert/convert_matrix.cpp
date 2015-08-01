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

// ConvertMatrix (c) 2005 by Klaus Post and Ian Brabham


#include "stdafx.h"

#include "convert_matrix.h"


#define USE_DYNAMIC_COMPILER true



MatrixGenerator3x3::MatrixGenerator3x3() { }

MatrixGenerator3x3::~MatrixGenerator3x3() {
  assembly.Free();
  unpacker.Free();
  packer.Free();
}

/***************************
 * Planar to interleaved converter
 *
 * Unpacks 8 pixels per loop.
 *
 * MMX, SSE2 code by IanB
 ***************************/

void MatrixGenerator3x3::GeneratePacker(int width, IScriptEnvironment* env) {

  __declspec(align(8)) static const __int64 rounder_ones = 0x0101010101010101;

  Assembler x86;   // This is the class that assembles the code.

  bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool fast128 = !!(env->GetCPUFlags() & (CPUF_SSE3|CPUF_SSSE3|CPUF_SSE4_1|CPUF_SSE4_2));

  const int loops  = (width+7) / 8;
  const int loopsA = (width+15) / 16;

  // Store registers
  x86.push(eax);
  x86.mov(eax, dword_ptr[esp+4+4]);              // Pointer to args list
  x86.push(ebx);
  x86.push(ecx);
  x86.push(edx);
  x86.push(esi);
  x86.push(edi);

  x86.mov(edi, dword_ptr [eax+12]);              // Arg4 -- Dest
  x86.mov(ecx, dword_ptr [eax+8]);               // Arg3 -- Plane 3
  x86.mov(ebx, dword_ptr [eax+4]);               // Arg2 -- Plane 2
  x86.mov(eax, dword_ptr [eax+0]);               // Arg1 -- Plane 1

  if (!sse2) {
    x86.movq(mm7, qword_ptr[(int)&rounder_ones]);// +1+1+1+1+1+1+1+1

    x86.xor(esi, esi);

    x86.mov(edx, loops);
    x86.align(16);
    x86.label("loopback");

    x86.movq(mm0,qword_ptr[eax+esi]);            // S1R1Q1P1s1r1q1p1
    x86.movq(mm2,qword_ptr[ecx+esi]);            // S3R3Q3P3s3r3q3p3

    x86.movq(mm4,mm0);                           // S1R1Q1P1s1r1q1p1
    x86.punpcklbw(mm0,qword_ptr[ebx+esi]);       // s2s1r2r1q2q1p2p1

    x86.movq(mm6,mm2);                           // S3R3Q3P3s3r3q3p3
    x86.punpcklbw(mm2,mm7);                      // +1s3+1r3+1q3+1p3

    x86.punpckhbw(mm4,qword_ptr[ebx+esi]);       // S2S1R2R1Q2Q1P2P1
    x86.punpckhbw(mm6,mm7);                      // +1S3+1R3+1Q3+1P3

    x86.add(esi, 8);

    x86.movq(mm1,mm0);                           // s2s1r2r1q2q1p2p1
    x86.movq(mm5,mm4);                           // S2S1R2R1Q2Q1P2P1

    x86.punpcklwd(mm0,mm2);                      // +1q3q2q1+1p3p2p1
    x86.punpckhwd(mm1,mm2);                      // +1s3s2s1+1r3r2r1
    x86.punpcklwd(mm4,mm6);                      // +1Q3Q2Q1+1P3P2P1
    x86.punpckhwd(mm5,mm6);                      // +1S3S2S1+1R3R2R1
    x86.dec(edx);
    x86.movq(qword_ptr[edi+esi*4-32],mm0);       // +1q3q2q1+1p3p2p1
    x86.movq(qword_ptr[edi+esi*4-24],mm1);       // +1s3s2s1+1r3r2r1
    x86.movq(qword_ptr[edi+esi*4-16],mm4);       // +1Q3Q2Q1+1P3P2P1
    x86.movq(qword_ptr[edi+esi*4- 8],mm5);       // +1S3S2S1+1R3R2R1
    x86.jnz("loopback");

    x86.emms();

  } else {
    x86.movq(xmm7, qword_ptr[(int)&rounder_ones]);// +1+1+1+1+1+1+1+1

    x86.xor(esi, esi);

    x86.mov(edx, loops);

    // Surprisingly the Unaligned code path is faster on my P4,
    // however my core2 loves the big code.
    if (fast128) {
	  // Check source planes are mod 16 aligned
      x86.test(eax, 0xF);
      x86.jnz("loopback");

      x86.test(ebx, 0xF);
      x86.jnz("loopback");

      x86.test(ecx, 0xF);
      x86.jnz("loopback");

      x86.punpcklqdq(xmm7,xmm7);                   // lo -> hi,lo

      x86.mov(edx, loopsA);

      x86.align(16);
      x86.label("loopbackA");  // Aligned access 16 pixels per loop

      x86.movdqa(xmm0,xmmword_ptr[eax+esi]);       // W1V1U1T1S1R1Q1P1w1v1u1t1s1r1q1p1
      x86.movdqa(xmm2,xmmword_ptr[ecx+esi]);       // W3V3U3T3S3R3Q3P3w3v3u3t3s3r3q3p3

      x86.movdqa(xmm4,xmm0);                       // W1V1U1T1S1R1Q1P1w1v1u1t1s1r1q1p1
      x86.punpcklbw(xmm0,xmmword_ptr[ebx+esi]);    // w2w1v2v1u2u1t2t1s2s1r2r1q2q1p2p1

      x86.movdqa(xmm6,xmm2);                       // W3V3U3T3S3R3Q3P3w3v3u3t3s3r3q3p3
      x86.punpcklbw(xmm2,xmm7);                    // +1w3+1v3+1u3+1t3+1s3+1r3+1q3+1p3
      x86.punpckhbw(xmm6,xmm7);                    // +1W3+1V3+1U3+1T3+1S3+1R3+1Q3+1P3

      x86.punpckhbw(xmm4,xmmword_ptr[ebx+esi]);    // W2W1V2V1U2U1T2T1S2S1R2R1Q2Q1P2P1

      x86.add(esi, 16);

      x86.movdqa(xmm1,xmm0);                       // w2w1v2v1u2u1t2t1s2s1r2r1q2q1p2p1
      x86.movdqa(xmm5,xmm4);                       // W2W1V2V1U2U1T2T1S2S1R2R1Q2Q1P2P1

      x86.punpcklwd(xmm0,xmm2);                    // +1s3s2s1+1r3r2r1+1q3q2q1+1p3p2p1
      x86.punpckhwd(xmm1,xmm2);                    // +1w3w2w1+1v3v2v1+1u3u2u1+1t3t2t1
      x86.punpcklwd(xmm4,xmm6);                    // +1S3S2S1+1R3R2R1+1Q3Q2Q1+1P3P2P1
      x86.punpckhwd(xmm5,xmm6);                    // +1W3W2W1+1V3V2V1+1U3U2U1+1T3T2T1
      x86.dec(edx);
      x86.movdqa(xmmword_ptr[edi+esi*4-64],xmm0);  // +1s3s2s1+1r3r2r1+1q3q2q1+1p3p2p1
      x86.movdqa(xmmword_ptr[edi+esi*4-48],xmm1);  // +1w3w2w1+1v3v2v1+1u3u2u1+1t3t2t1
      x86.movdqa(xmmword_ptr[edi+esi*4-32],xmm4);  // +1S3S2S1+1R3R2R1+1Q3Q2Q1+1P3P2P1
      x86.movdqa(xmmword_ptr[edi+esi*4-16],xmm5);  // +1W3W2W1+1V3V2V1+1U3U2U1+1T3T2T1
      x86.jnz("loopbackA");

      x86.jmp("exitA");
    }

    x86.align(16);
    x86.label("loopback");  // Unaligned access 8 pixels per loop

    x86.movq(xmm0,qword_ptr[eax+esi]);        // S1R1Q1P1s1r1q1p1
    x86.movq(xmm1,qword_ptr[ebx+esi]);        // S2R2Q2P2s2r2q2p2
    x86.movq(xmm2,qword_ptr[ecx+esi]);        // S3R3Q3P3s3r3q3p3

    x86.add(esi, 8);
    x86.dec(edx);

    x86.punpcklbw(xmm0,xmm1);                 // S2S1R2R1Q2Q1P2P1s2s1r2r1q2q1p2p1
    x86.punpcklbw(xmm2,xmm7);                 // +1S3+1R3+1Q3+1P3+1s3+1r3+1q3+1p3

    x86.movdqa(xmm1,xmm0);                    // S2S1R2R1Q2Q1P2P1s2s1r2r1q2q1p2p1

    x86.punpcklwd(xmm0,xmm2);                 // +1s3s2s1+1r3r2r1+1q3q2q1+1p3p2p1
    x86.punpckhwd(xmm1,xmm2);                 // +1S3S2S1+1R3R2R1+1Q3Q2Q1+1P3P2P1

    x86.movdqa(xmmword_ptr[edi+esi*4-32],xmm0);// +1s3s2s1+1r3r2r1+1q3q2q1+1p3p2p1
    x86.movdqa(xmmword_ptr[edi+esi*4-16],xmm1);// +1S3S2S1+1R3R2R1+1Q3Q2Q1+1P3P2P1
    x86.jnz("loopback");

    x86.align(16);
    x86.label("exitA");
  }
    // Restore registers
  x86.pop(edi);
  x86.pop(esi);
  x86.pop(edx);
  x86.pop(ecx);
  x86.pop(ebx);
  x86.pop(eax);
  x86.ret();
  packer = DynamicAssembledCode(x86, env, "ConvertMatrix: Dynamic MMX code could not be compiled.");
}


/***************************
 * Interleaved to planar converter
 *
 * Unpacks 8 pixels per loop.
 * Input must have enough space for this to happend.
 *
 * MMX, SSE2 code by IanB
 * SSSE3 code by Klaus Post and IanB
 ***************************/


void MatrixGenerator3x3::GenerateUnPacker(int width, IScriptEnvironment* env) {
  __declspec(align(16)) static const __int64 Shuf[] =  {0x0d0905010c080400,0xffffffff0e0a0602}; // x x x x 14 10 6 2 13 9 5 1 12 8 4 0

  Assembler x86;   // This is the class that assembles the code.

  const int loops = (width+7) / 8;

  bool ssse3 = !!(env->GetCPUFlags() & CPUF_SSSE3);  // We have a version for SSSE3 and
  bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);   // one for SSE2 and a plain MMX.

  // Store registers
  x86.push(eax);
  x86.mov(eax, dword_ptr[esp+4+4]);              // Pointer to args list
  x86.push(ebx);
  x86.push(ecx);
  x86.push(edx);
  x86.push(esi);
  x86.push(edi);

  x86.mov(esi, dword_ptr [eax+0]);               // Arg1 -- Pointer to src
  x86.mov(ecx, dword_ptr [eax+12]);              // Arg4 -- Plane 3 dest
  x86.mov(ebx, dword_ptr [eax+8]);               // Arg3 -- Plane 2 dest
  x86.mov(eax, dword_ptr [eax+4]);               // Arg2 -- Plane 1 dest

  if (sse2) {
    if (ssse3) {
      x86.movdqa(xmm7,xmmword_ptr[&Shuf]);
    }
    x86.xor(edi, edi);
    x86.mov(edx, loops);

    x86.align(16);
    x86.label("loopback");

    x86.movdqa(xmm0,xmmword_ptr[esi+edi*4]);    // XXQ3Q2Q1xxq3q2q1XXP3P2P1xxp3p2p1
    x86.movdqa(xmm1,xmmword_ptr[esi+edi*4+16]); // XXS3S2S1xxs3s2s1XXR3R2R1xxr3r2r1

    if (ssse3) {
      x86.pshufb(xmm0,xmm7);                    // 00000000Q3q3P3p3Q2q2P2p2Q1q1P1p1
      x86.pshufb(xmm1,xmm7);                    // 00000000S3s3R3r3S2s2R2r2S1s1R1r1

      x86.movdqa(xmm4,xmm0);

      x86.punpckldq(xmm0,xmm1);                 // S2s2R2r2Q2q2P2p2S1s1R1r1Q1q1P1p1
      x86.punpckhdq(xmm4,xmm1);                 // XXxxXXxxXXxxXXxxS3s3R3r3Q3q3P3p3
    } else {
#if 0
      x86.punpcklqdq(xmm2,xmm0);                // XXP3P2P1xxp3p2p1................
      x86.punpcklqdq(xmm3,xmm1);                // XXR3R2R1xxr3r2r1................

      x86.punpckhbw(xmm2,xmm0);                 // XXXXQ3P3Q2P2Q1P1xxxxq3p3q2p2q1p1
      x86.punpckhbw(xmm3,xmm1);                 // XXXXS3R3S2R2S1R1xxxxs3r3s2r2s1r1

      x86.punpcklqdq(xmm0,xmm2);                // xxxxq3p3q2p2q1p1................
      x86.punpcklqdq(xmm1,xmm3);                // xxxxs3r3s2r2s1r1................

      x86.punpckhbw(xmm0,xmm2);                 // XXxxXXxxQ3q3P3p3Q2q2P2p2Q1q1P1p1
      x86.punpckhbw(xmm1,xmm3);                 // XXxxXXxxS3s3R3r3S2s2R2r2S1s1R1r1

      x86.movdqa(xmm4,xmm0);

      x86.punpckldq(xmm0,xmm1);                 // S2s2R2r2Q2q2P2p2S1s1R1r1Q1q1P1p1
      x86.punpckhdq(xmm4,xmm1);                 // XXxxXXxxXXxxXXxxS3s3R3r3Q3q3P3p3
#else
      x86.movdqa(xmm4,xmm0);                    // XXQ3Q2Q1xxq3q2q1XXP3P2P1xxp3p2p1
      
      x86.punpcklbw(xmm0,xmm1);                 // XXXXR3P3R2P2R1P1xxxxr3p3r2p2r1p1
      x86.punpckhbw(xmm4,xmm1);                 // XXXXS3Q3S2Q2S1Q1xxxxs3q3s2q2s1q1
                                                
      x86.movdqa(xmm1,xmm0);                    // XXXXR3P3R2P2R1P1xxxxr3p3r2p2r1p1
      
      x86.punpcklbw(xmm0,xmm4);                 // xxxxxxxxs3r3q3p3s2r2q2p2s1r1q1p1
      x86.punpckhbw(xmm1,xmm4);                 // XXXXXXXXS3R3Q3P3S2R2Q2P2S1R1Q1P1
                                                
      x86.movdqa(xmm4,xmm0);                    // xxxxxxxxs3r3q3p3s2r2q2p2s1r1q1p1
      
      x86.punpcklbw(xmm0,xmm1);                 // S2s2R2r2Q2q2P2p2S1s1R1r1Q1q1P1p1
      x86.punpckhbw(xmm4,xmm1);                 // XXxxXXxxXXxxXXxxS3s3R3r3Q3q3P3p3
#endif
    }
    x86.movq(qword_ptr[eax+edi],xmm0);          // Store: S1s1R1r1Q1q1P1p1
    x86.punpckhqdq(xmm0,xmm0);                  // S2s2R2r2Q2q2P2p2S2s2R2r2Q2q2P2p2
    x86.movq(qword_ptr[ecx+edi],xmm4);          // Store: S3s3R3r3Q3q3P3p3
    x86.add(edi,8);
    x86.dec(edx);
    x86.movq(qword_ptr[ebx+edi-8],xmm0);        // Store: S2s2R2r2Q2q2P2p2

    x86.jnz("loopback");
  } else {
    x86.xor(edi, edi);

    x86.mov(edx, loops);

    x86.align(16);
    x86.label("loopback");

    x86.movq(mm0,qword_ptr[esi+edi*4]);    // XXP3P2P1xxp3p2p1
    x86.movq(mm1,qword_ptr[esi+edi*4+8]);  // XXQ3Q2Q1xxq3q2q1
    x86.movq(mm2,qword_ptr[esi+edi*4+16]); // XXR3R2R1xxr3r2r1
    x86.movq(mm3,qword_ptr[esi+edi*4+24]); // XXS3S2S1xxs3s2s1
#if 0
    x86.punpckldq(mm4,mm0);                // xxp3p2p1........
    x86.punpckldq(mm5,mm1);                // xxq3q2q1........
    x86.punpckldq(mm6,mm2);                // xxr3r2r1........
    x86.punpckldq(mm7,mm3);                // xxs3s2s1........

    x86.punpckhbw(mm4,mm0);                // XXxxP3p3P2p2P1p1
    x86.punpckhbw(mm5,mm1);                // XXxxQ3q3Q2q2Q1q1
    x86.punpckhbw(mm6,mm2);                // XXxxR3r3R2r2R1r1
    x86.punpckhbw(mm7,mm3);                // XXxxS3s3S2s2S1s1

    x86.movq(mm0,mm4);                     // XXxxP3p3P2p2P1p1
    x86.movq(mm2,mm6);                     // XXxxR3r3R2r2R1r1

    x86.punpcklwd(mm4,mm5);                // Q2q2P2p2Q1q1P1p1
    x86.punpckhwd(mm0,mm5);                // XXxxXXxxQ3q3P3p3
    x86.punpcklwd(mm6,mm7);                // S2s2R2r2S1s1R1r1
    x86.punpckhwd(mm2,mm7);                // XXxxXXxxS3q3R3r3

    x86.movq(mm1,mm4);                     // Q2q2P2p2Q1q1P1p1
    x86.punpckldq(mm4,mm6);                // S1s1R1r1Q1q1P1p1
    x86.punpckhdq(mm1,mm6);                // S2s2R2r2Q2q2P2p2
    x86.punpckldq(mm0,mm2);                // S3s3R3r3Q3q3P3p3

    x86.movq(qword_ptr[eax+edi],mm4);      // S1s1R1r1Q1q1P1p1
    x86.movq(qword_ptr[ebx+edi],mm1);      // S2s2R2r2Q2q2P2p2
    x86.add(edi,8);
    x86.dec(edx);

    x86.movq(qword_ptr[ecx+edi-8],mm0);    // S3s3R3r3Q3q3P3p3
#else
    x86.movq(mm4,mm0);                     // XXP3P2P1xxp3p2p1
    x86.movq(mm5,mm2);                     // XXR3R2R1xxr3r2r1

    x86.punpcklbw(mm0,mm1);                // xxxxq3p3q2p2q1p1
    x86.punpckhbw(mm4,mm1);                // XXXXQ3P3Q2P2Q1P1
    x86.punpcklbw(mm2,mm3);                // xxxxs3r3s2r2s1r1
    x86.punpckhbw(mm5,mm3);                // XXXXS3R3S2R2S1R1

    x86.movq(mm1,mm0);                     // xxxxq3p3q2p2q1p1
    x86.movq(mm3,mm2);                     // xxxxs3r3s2r2s1r1

    x86.punpcklbw(mm0,mm4);                // Q2q2P2p2Q1q1P1p1
    x86.punpckhbw(mm1,mm4);                // XXxxXXxxQ3q3P3p3
    x86.punpcklbw(mm2,mm5);                // S2s2R2r2S1s1R1r1
    x86.punpckhbw(mm3,mm5);                // XXxxXXxxS3q3R3r3

    x86.movq(mm4,mm0);                     // Q2q2P2p2Q1q1P1p1

    x86.punpckldq(mm0,mm2);                // S1s1R1r1Q1q1P1p1
    x86.punpckhdq(mm4,mm2);                // S2s2R2r2Q2q2P2p2
    x86.punpckldq(mm1,mm3);                // S3s3R3r3Q3q3P3p3

    x86.movq(qword_ptr[eax+edi],mm0);      // S1s1R1r1Q1q1P1p1
    x86.movq(qword_ptr[ebx+edi],mm4);      // S2s2R2r2Q2q2P2p2
    x86.add(edi,8);
    x86.dec(edx);

    x86.movq(qword_ptr[ecx+edi-8],mm1);    // S3s3R3r3Q3q3P3p3
#endif
    x86.jnz("loopback");
    x86.emms();
  }
    // Restore registers
  x86.pop(edi);
  x86.pop(esi);
  x86.pop(edx);
  x86.pop(ecx);
  x86.pop(ebx);
  x86.pop(eax);
  x86.ret();
  unpacker = DynamicAssembledCode(x86, env, "ConvertMatrix: Dynamic MMX code could not be compiled.");
}




/***
 *
 * (c) Klaus Post, 2005
 * (c) Ian Brabham, 2011
 *
 * MMX code by Klaus Post
 * SSE2 code by IanB
 * SSSE3 code by Klaus Post and IanB
 *
 * 8 bytes/loop
 * 2 pixels/loop
 *
 * Faction is the fraction part in bits.
 * rgb_out requires every fourth byte in the input to be one (packer does this), masks alpha channel on output
 ***/


void MatrixGenerator3x3::GenerateAssembly(int width, int frac_bits, bool rgb_out,
                                          const __int64 *pre_add, const __int64 *post_add,
                                          const int src_pixel_step, const int dest_pixel_step,
                                          const signed short* matrix, IScriptEnvironment* env) {

  Assembler x86;   // This is the class that assembles the code.

  bool unroll = width <= 2;   // Unrolled code ~30% slower on Athlon XP.
  bool ssse3 = !!(env->GetCPUFlags() & CPUF_SSSE3);
  bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);
  bool odd_rgb = !rgb_out && (width & 1);

  int loops = (width+1) / 2;

  if (pre_add && post_add)
    env->ThrowError("Internal error - only pre_add OR post_add can be used.");

  int last_pix_mask = 0;

  if (rgb_out && (width & 1))
    last_pix_mask = 0xffffffff;
  else if (dest_pixel_step == 3)
    last_pix_mask = 0xff000000;
  else if (dest_pixel_step == 2)
    last_pix_mask = 0xffff0000;
  else if (dest_pixel_step == 1)
    last_pix_mask = 0xffffff00;

  // Store registers
  x86.push(     eax);
  x86.mov(      eax, dword_ptr[esp+4+4]);                 // Pointer to args list
  x86.push(     ebx);
  x86.push(     ecx);
  x86.push(     esi);
  x86.push(     edi);

  x86.mov(      esi, dword_ptr [eax+0]);                  // Arg1
  x86.mov(      edi, dword_ptr [eax+4]);                  // Arg2

  x86.mov(      ebx, (int)matrix);                        // matrix[0] pointer

  if (last_pix_mask) {
    x86.push(   dword_ptr[edi+width*dest_pixel_step]);
  }

  if (pre_add) {
    x86.mov(    eax, (int)pre_add);
  }
  if (post_add) {
    x86.mov(    eax, (int)post_add);
  }

  if ( sse2 ) { // || ssse3
    x86.movq(        xmm2, qword_ptr [ebx]);               // matrix[0]
    x86.movq(        xmm3, qword_ptr [ebx+8]);             // matrix[1]
    x86.movq(        xmm4, qword_ptr [ebx+16]);            // matrix[2]
    x86.punpcklqdq(  xmm2, xmm2);
    x86.punpcklqdq(  xmm3, xmm3);
    x86.punpcklqdq(  xmm4, xmm4);
  }

  if ( ssse3 ) {
    x86.movq(      xmm5, qword_ptr[eax]);                // 0|-128|-128|(-16 or 0) or 0|128|128|(16 or 0)
    x86.movq(      xmm7, qword_ptr[ebx+24]);             // matrix[3] = 0xff000000ff000000
    x86.punpcklqdq(xmm5, xmm5);
  }
  else if ( sse2 ) {
    x86.prefetcht1(byte_ptr[eax]);                       // Get Pre/Post add ready
  }
  else if (pre_add) { // && MMX
    x86.movq(      mm2, qword_ptr[eax]);                 // Must be ready when entering loop
  }

  if (!unroll) {  // Should we create a loop instead of unrolling?
    x86.mov(ecx, odd_rgb ? loops-1 : loops);
    x86.align(16);
    x86.label("loopback");
    loops = odd_rgb ? 2 : 1;
  }
/****
 * Inloop usage:
 * eax: pre/post-add pointer
 * ebx: matrix pointer
 * ecx: loop counter
 * esi: source
 * edi: destination
 *****/
  for (int i=0; i<loops; i++) {
    if (sse2) { // || ssse3
      if (odd_rgb && i == loops-1) {
        x86.movd(        xmm0, dword_ptr[esi]);                // 0000|argb -- RGB24 overruns 1 byte
      } else {
        if (src_pixel_step == 4) {  // Load both.
          x86.movq(      xmm0, qword_ptr[esi]);                // ARGB|argb or 1VUY|1vuy
        } else { // RGB24
          x86.movd(      xmm0, dword_ptr[esi]);                // 0000Brgb
          x86.movd(      xmm1, dword_ptr[esi+src_pixel_step]); // 0000?RGB
          x86.punpckldq( xmm0, xmm1);                          // ?RGB|Brgb
        }
      }
      if ( ssse3 ) {
        if (rgb_out) {
          x86.pxor(      xmm1, xmm1);
          x86.punpcklbw( xmm0, xmm1);                          // 0001|00VV|00UU|00YY|0001|00vv|00uu|00yy
        } else {
          x86.por(       xmm0, xmm7);                          // FRGB|frgb
          x86.punpcklbw( xmm0, xmm7);                          // FFFF|00RR|00GG|00BB|ffff|00rr|00gg|00bb
        }
        if (pre_add)
          x86.paddw(     xmm0, xmm5);                          // += 0|-128|-128|(-16 or 0)

        x86.movdqa(      xmm1, xmm0);
        x86.movdqa(      xmm6, xmm0);

        x86.pmaddwd(     xmm0, xmm2); // matrix[0]             // Part 1/3  [B|A|b|a]
        x86.pmaddwd(     xmm1, xmm3); // matrix[1]             // Part 2/3  [D|C|d|c]
        x86.pmaddwd(     xmm6, xmm4); // matrix[2]             // Part 3/3  [F|E|f|e]

        x86.phaddd(      xmm0, xmm0);                          // [B+A|b+a|B+A|b+a]
        x86.phaddd(      xmm1, xmm6);                          // [F+E|f+e|D+C|d+c]

        x86.psrad(       xmm0, (byte)frac_bits);               // Shift down - [---X|---x|---X|---x]
        x86.psrad(       xmm1, (byte)frac_bits);               // Shift down - [---Z|---z|---Y|---y]

        x86.punpckldq(   xmm0, xmm1);                          //  [---Y|---X|---y|---x]
        x86.punpckhdq(   xmm1, xmm1);                          //  [---Z|---Z|---z|---z]

        x86.movdqa(      xmm6, xmm0);                          //
        x86.punpcklqdq(  xmm0, xmm1);                          //  [---z|---z|---y|---x]
        x86.punpckhqdq(  xmm6, xmm1);                          //  [---Z|---Z|---Y|---X]

        x86.packssdw(    xmm0, xmm6);                          // Pack results into words (pixel 1 ready) - [??|-Z|-Y|-X|??|-z|-y|-x]

        if (post_add)
          x86.paddw(     xmm0, xmm5);                          // += 0|128|128|16 or 0
      }
      else { // SSE2
        if (rgb_out) {
          x86.pxor(      xmm1, xmm1);
        } else {
          x86.movq(      xmm1, qword_ptr[ebx+24]);             // matrix[3] = 0xff000000ff000000
          x86.por(       xmm0, xmm1);                          // FRGB|frgb
        }
        x86.punpcklbw(   xmm0, xmm1);                          // LO ffff|00rr|00gg|00bb or 0001|00vv|00uu|00yy
                                                               // HI FFFF|00RR|00GG|00BB or 0001|00VV|00UU|00YY
        if (pre_add)
          x86.paddw(     xmm0, xmmword_ptr[eax]);              // += 0|-128|-128|(-16 or 0)

        x86.movdqa(      xmm5, xmm0);
        x86.pmaddwd(     xmm0, xmm2); // matrix[0]             // Part 1/3  [B|A|b|a]

        x86.movdqa(      xmm1, xmm5);
        x86.pmaddwd(     xmm5, xmm3); // matrix[1]             // Part 2/3 [D|C|d|c]

        x86.movdqa(      xmm6, xmm0);
        x86.psrlq(       xmm0, 32);                            // [0|B|0|b]

        x86.pmaddwd(     xmm1, xmm4); // matrix[2]             // Part 3/3  [F|E|f|e]

        x86.movdqa(      xmm7, xmm5);
        x86.psrlq(       xmm5, 32);                            // [0|D|0|d]

        x86.paddd(       xmm6, xmm0);                          // First ready in upper - [X=B+0|A+B][x=b+0|a+b]
        x86.paddd(       xmm7, xmm5);                          // Second ready in upper - [Y=D+0|C+D][y=d+0|c+d]

        x86.movdqa(      xmm5, xmm1);
        x86.psrlq(       xmm1, 32);                            // [0|F|0|f]

        x86.psrad(       xmm6, (byte)frac_bits);               // Shift down - [????|---X|????|---x]
        x86.paddd(       xmm5, xmm1);                          // Third ready in upper - [Z=F+0|E+F][z=f+0|e+f]
        x86.psrad(       xmm7, (byte)frac_bits);               // Shift down - [????|---Y|????|---y]
        x86.psrad(       xmm5, (byte)frac_bits);               // Shift down - [????|---Z|????|---z]

        x86.movdqa(      xmm0, xmm6);                          // pixel 1
        x86.punpckhdq(   xmm6, xmm7);                          // Move xmm6 upper to lower, with xmm7 upper - [????|????|---Y|---X]
        x86.punpckldq(   xmm0, xmm7);                          // Move xmm7 lower to xmm0 upper - [????|????|---y|---x]

        x86.punpcklqdq(  xmm6, xmm6);                          // Move xmm6 lower to upper - [---Y|---X|---Y|---X]

        x86.punpcklqdq(  xmm0, xmm5);                          // Move xmm7 lower to xmm0 upper - [????|---z|---y|---x]
        x86.punpckhqdq(  xmm6, xmm5);                          // Move xmm6 upper to lower, with xmm7 upper - [????|---Z|---Y|---X]

        if (dest_pixel_step == 4 && rgb_out)
          x86.movq(      xmm7, qword_ptr [ebx+24]);            // matrix[3] = 0xff000000ff000000

        x86.packssdw(    xmm0, xmm6);                          // Pack results into words (pixel 1 ready) - [??|-Z|-Y|-X|??|-z|-y|-x]

        if (post_add)
          x86.paddw(     xmm0, xmmword_ptr[eax]);              // += 0|128|128|(16 or 0)
      }

      x86.add(         esi, src_pixel_step*2);

      x86.packuswb(    xmm0, xmm0);                          // Into bytes

      if (dest_pixel_step == 4) {
        if (rgb_out) {  // Set alpha channel
          x86.por(     xmm0, xmm7);
        }
        x86.movq(      qword_ptr[edi], xmm0);                // Store
      } else {
        x86.movd(      dword_ptr[edi], xmm0);                // Store
        x86.psrlq(     xmm0, 32);                            // [0|F|0|f]
        x86.movd(      dword_ptr[edi+dest_pixel_step], xmm0);
      }
    }
    else { // MMX
      x86.movq(        mm4, qword_ptr [ebx]);               // matrix[0]

      if (odd_rgb && i == loops-1) {
        x86.movd(      mm0, dword_ptr[esi]);                // 0000|argb -- RGB24 overruns 1 byte
        x86.movd(      mm7, dword_ptr [ebx+24]);            // matrix[3] = 0xff000000
        x86.por(       mm0, mm7);                           // 0000|frgb
        x86.punpcklbw( mm0, mm7);                           // ffff|00rr|00gg|00bb - Unpack bytes -> words Lo
        x86.movq(      mm1, mm0);                           // Copy to keep rest of code happy
      } else {
        if (src_pixel_step == 4) {  // Load both.
          x86.movq(    mm0, qword_ptr[esi]);                // ARGB|argb or 1VUY|1vuy
          if (rgb_out) {
            x86.pxor(  mm7, mm7);
          } else {
            x86.movq(  mm7, qword_ptr [ebx+24]);            // matrix[3] = 0xff000000ff000000
            x86.por(   mm0, mm7);                           // FRGB|frgb
          }
          x86.movq(    mm1, mm0);
          x86.punpcklbw(mm0, mm7);                          // ffff|00rr|00gg|00bb or 0001|00vv|00uu|00yy
          x86.punpckhbw(mm1, mm7);                          // FFFF|00RR|00GG|00BB or 0001|00VV|00UU|00YY
        } else { // RGB24
          x86.movd(    mm0, dword_ptr[esi]);                // 0000Brgb
          if (rgb_out) {
            x86.pxor(  mm7, mm7);
            x86.movd(  mm1, dword_ptr[esi+src_pixel_step]); // 0000?RGB
          } else {
            x86.movq(  mm7, qword_ptr [ebx+24]);            // matrix[3] = 0xff000000ff000000
            x86.movd(  mm1, dword_ptr[esi+src_pixel_step]); // 0000?RGB
            x86.por(   mm0, mm7);                           // 0000frgb
            x86.por(   mm1, mm7);                           // 0000FRGB
          }
          x86.punpcklbw(mm0, mm7);                          // ffff|00rr|00gg|00bb - Unpack bytes -> words Lo
          x86.punpcklbw(mm1, mm7);                          // FFFF|00RR|00GG|00BB - Unpack bytes -> words Hi
        }
      }
      if (pre_add) {
        x86.paddw(     mm0, mm2);                           // += 0|-128|-128|-16 or 0
        x86.paddw(     mm1, mm2);
      }
  // Element 1/3
      x86.movq(        mm2, mm0);
      x86.pmaddwd(     mm0, mm4);                           // Part 1/3 Lo - [b|a]

      x86.movq(        mm3, mm1);
      x86.pmaddwd(     mm1, mm4);                           // Part 1/3 Hi - [B|A]

      x86.movq(        mm4, qword_ptr [ebx+8]);             // matrix[1]

      x86.punpckldq(   mm6, mm0);                           // Move mm0 lower to mm6 upper - [a|?]
      x86.punpckldq(   mm5, mm1);                           // Move mm1 lower to mm5 upper - [A|?]

      x86.paddd(       mm6, mm0);                           // First Lo ready in upper - [x=a+b|a+?]
      x86.paddd(       mm5, mm1);                           // First Hi ready in upper - [X=A+B|A+?]

      x86.movq(        mm0, mm2);
      x86.punpckhdq(   mm6, mm5);                           // Move [mm5, mm6] uppers to mm6 [X, x]

  // Element 2/3
      x86.pmaddwd(     mm2, mm4);                           // Part 2/3 Lo - [d|c]

      x86.movq(        mm1, mm3);
      x86.pmaddwd(     mm3, mm4);                           // Part 2/3 Hi - [D|C]

      x86.movq(        mm4, qword_ptr [ebx+16]);            // matrix[2]

      x86.punpckldq(   mm7, mm2);                           // Move mm2 lower to mm7 upper - [c|?]
      x86.punpckldq(   mm5, mm3);                           // Move mm3 lower to mm5 upper - [C|?]

      x86.paddd(       mm7, mm2);                           // Second Lo ready in upper - [y=c+d|c+?]

  // Element 3/3
      x86.pmaddwd(     mm0, mm4);                           // Part 3/3 Lo - [f|e]
      x86.paddd(       mm5, mm3);                           // Second Hi ready in upper - [Y=C+D|C+?]

      x86.pmaddwd(     mm1, mm4);                           // Part 3/3 Hi - [F|E]
      x86.punpckhdq(   mm7, mm5);                           // Move uppers[mm5, mm7]  to mm7 [Y, y]

      x86.punpckldq(   mm4, mm0);                           // Move mm0 lower to mm4 upper - [e|?]
      x86.punpckldq(   mm5, mm1);                           // Move mm1 lower to mm5 upper - [E|?]

      x86.paddd(       mm4, mm0);                           // Third Lo ready in upper - [z=e+f|e+?]
      x86.paddd(       mm5, mm1);                           // Third Hi ready in upper - [Z=E+F|E+?]

      x86.psrad(       mm6, (byte)frac_bits);               // Shift down - [---X|---x]

      x86.punpckhdq(   mm4, mm5);                           // Move uppers[mm5, mm4]  to mm4[Z, z]
      x86.psrad(       mm7, (byte)frac_bits);               // Shift down - [---Y|---y]
      x86.movq(        mm0, mm6);                           // pixel 1
      x86.psrad(       mm4, (byte)frac_bits);               // Shift down - [---Z|---z]

      x86.punpckldq(   mm0, mm7);                           // Move mm7 lower to mm0 upper - [---y|---x]
      x86.punpckhdq(   mm6, mm7);                           // Move mm6 upper to lower, with mm7 upper - [---Y|---X]

      x86.packssdw(    mm0, mm4);                           // Pack results into words (pixel 1 ready) - [-Z|-z|-y|-x]
      x86.punpckhdq(   mm4, mm4);                           //                                           [---Z|---Z]

      if (post_add || pre_add)
        x86.movq(      mm2, qword_ptr[eax]);

      x86.packssdw(    mm6, mm4);                           // Pack results into words (pixel 2 ready) - [-Z|-Z|-Y|-X]

      if (post_add) {
        x86.paddw(     mm0, mm2);                           // += 0|128|128|16 or 0
        x86.paddw(     mm6, mm2);
      }

      x86.add(         esi, src_pixel_step*2);

      if (dest_pixel_step == 4) {
        x86.packuswb(  mm0, mm6);                           // Into bytes

        if (rgb_out)   // Set alpha channel
          x86.por(     mm0, qword_ptr [ebx+24]);            // matrix[3] = 0xff000000ff000000

        x86.movq(      qword_ptr[edi], mm0);                // Store
      } else {
        x86.packuswb(  mm0, mm0);                           // Into bytes
        x86.packuswb(  mm6, mm6);                           // Into bytes

        x86.movd(      dword_ptr[edi], mm0);                // Store
        x86.movd(      dword_ptr[edi+dest_pixel_step], mm6);
      }
    }
    x86.add(         edi, dest_pixel_step*2);

    if (!unroll && i == 0) {  // Loop on if not unrolling
      x86.dec(         ecx);
      x86.jnz(         "loopback");
    }
  }

  if (!sse2) x86.emms();

  if (last_pix_mask == 0xffffffff) {
    x86.pop(         dword_ptr[edi-dest_pixel_step]);  // Store saved pixel
  }
  else if (last_pix_mask) {
    x86.pop(         ebx);                                  // Load Stored pixel
    x86.mov(         ecx, dword_ptr[edi-dest_pixel_step]);  // Load new pixel
    x86.and(         ebx, last_pix_mask);
    x86.and(         ecx, ~last_pix_mask);
    x86.or(          ebx, ecx);
    x86.mov(         dword_ptr[edi-dest_pixel_step], ebx);  // Store new pixel
  }

   // Restore registers
  x86.pop(          edi);
  x86.pop(          esi);
  x86.pop(          ecx);
  x86.pop(          ebx);
  x86.pop(          eax);
  x86.ret();

  assembly = DynamicAssembledCode(x86, env, "ConvertMatrix: Dynamic MMX code could not be compiled.");
}
