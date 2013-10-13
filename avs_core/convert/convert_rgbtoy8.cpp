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

// ConvertRGBtoY8 (c) 2011 by Ian Brabham


#include "convert_rgbtoy8.h"

#ifdef X86_32


RGBtoY8Generator::RGBtoY8Generator() { }

RGBtoY8Generator::~RGBtoY8Generator() {
  assembly.Free();
}

/***************************
 * RGB32 to Y8 converter
 *
 * Unpacks 4 pixels per loop.
 * handles non mod4 widths
 ***************************/


void RGBtoY8Generator::genRGB32toY8(int width, int height, int offset_y, signed short* matrix,
                                      IScriptEnvironment* env) {
  enum {             // Argument offsets
    src     = 0,
    dst     = 4,
    pitchS  = 8,
    pitchD = 12,
  };

  Assembler x86;   // This is the class that assembles the code.

  const bool sse   = !!(env->GetCPUFlags() & CPUF_SSE);
  const bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);

  const int awidth = width & ~3;
  const int rwidth = width &  3;

  // Store registers and get arg pointers
  x86.push(         eax);
  x86.mov(          eax, dword_ptr[esp+4+4]);       // Pointer to args list

  x86.push(         ebx);
  x86.push(         ecx);
  x86.push(         edx);
  x86.push(         esi);
  x86.push(         edi);

  x86.mov(          esi, dword_ptr[eax+src]);
  x86.mov(          edi, dword_ptr[eax+dst]);
  x86.mov(          ebx, dword_ptr[eax+pitchS]);
  x86.mov(          edx, dword_ptr[eax+pitchD]);

  x86.mov(          eax, offset_y*2+1);
  x86.pxor(         mm0, mm0);
  x86.movd(         mm1, eax);
  x86.movq(         mm2, qword_ptr[matrix]);        // [0|cyr|cyg|cyb]
  x86.sub(          edi, 4);                        // adjust for early +=4
  x86.psllq(        mm1, 46);                       // 0x0008400000000000

  x86.mov(          eax, height);

  x86.align(        16);
  x86.label("yloop");

  if (sse2) {
    x86.prefetchnta(byte_ptr[esi]);
    x86.prefetchnta(byte_ptr[esi+64]);
  } else if (sse) {
    x86.prefetchnta(byte_ptr[esi]);
    x86.prefetchnta(byte_ptr[esi+32]);
    x86.prefetchnta(byte_ptr[esi+64]);
    x86.prefetchnta(byte_ptr[esi+96]);
  }
  x86.xor(          ecx, ecx);

  for (int i=0; i<3; i++) {
    if (i==0 && awidth > 0) {
      x86.movq(     mm6, qword_ptr[esi]);           // Get pixels 1 & 0
      x86.movq(     mm4, qword_ptr[esi+8]);         // Get pixels 3 & 2
      if (awidth > 8) {
        x86.align(  16);
        x86.label("loop4");
        if (sse)
          x86.prefetchnta(byte_ptr[esi+ecx*4+128]);
      }
    } else if (i==2 && rwidth == 3) {
      x86.movq(         mm6, qword_ptr[esi+ecx*4]);     // Get pixels 1 & 0
      x86.movd(         mm4, dword_ptr[esi+ecx*4+8]);   // Get pixel 2, zero pixel 3
    } else if (i==2 && rwidth == 2) {
      x86.movq(         mm6, qword_ptr[esi+ecx*4]);     // Get pixels 1 & 0
    } else if (i==2 && rwidth == 1) {
      x86.movd(         mm6, dword_ptr[esi+ecx*4]);     // Get pixels 0, zero pixel 1
    }
    if ((i==0 && awidth > 4) ||
        (i==1 && awidth > 0) ||
        (i==2 && rwidth > 0) ) {
      x86.movq(         mm5, mm6);                      // duplicate pixels
      x86.punpckhbw(    mm6, mm0);                      // [00ha|00rr|00gg|00bb]        -- 1
      x86.punpcklbw(    mm5, mm0);                      // [00la|00rr|00gg|00bb]        -- 0
      x86.pmaddwd(      mm6, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 1
      x86.pmaddwd(      mm5, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 0
      x86.punpckldq(    mm7, mm6);                      // [loDWmm6|junk]               -- 1
      x86.paddd(        mm6, mm1);                      // +=0.5
      x86.punpckldq(    mm3, mm5);                      // [loDWmm5|junk]               -- 0
      x86.paddd(        mm5, mm1);                      // +=0.5
      x86.paddd(        mm6, mm7);                      // [hiDWmm6+32768+loDWmm6|junk] -- 1
      x86.paddd(        mm5, mm3);                      // [hiDWmm5+32768+loDWmm5|junk] -- 0
      x86.psrad(        mm6, 15);                       // -> 8 bit result              -- 1
      if (i!=2 || rwidth == 3) {
        x86.movq(       mm3, mm4);                      // duplicate pixels
        x86.psrad(      mm5, 15);                       // -> 8 bit result              -- 0
        x86.punpcklbw(  mm4, mm0);                      // [00la|00rr|00gg|00bb]        -- 2
        x86.punpckhwd(  mm5, mm6);                      // [....|....|..11|..00]
        x86.punpckhbw(  mm3, mm0);                      // [00ha|00rr|00gg|00bb]        -- 3
        x86.pmaddwd(    mm4, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 2
        x86.pmaddwd(    mm3, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 3
        x86.punpckldq(  mm7, mm4);                      // [loDWmm4|junk]               -- 2
        x86.paddd(      mm4, mm1);                      // +=0.5
        x86.punpckldq(  mm6, mm3);                      // [loDWmm3|junk]               -- 3
        x86.paddd(      mm3, mm1);                      // +=0.5
        x86.paddd(      mm4, mm7);                      // [hiDWmm4+32768+loDWmm4|junk] -- 2
        x86.paddd(      mm3, mm6);                      // [hiDWmm3+32768+loDWmm3|junk] -- 3
        if (i==0 && awidth > 4) {
          x86.movq(     mm6, qword_ptr[esi+ecx*4+16]);     // Get pixels 1 & 0
          x86.psrad(    mm4, 15);                       // -> 8 bit result              -- 2
          x86.psrad(    mm3, 15);                       // -> 8 bit result              -- 3
          x86.add(      ecx, 4);                        // loop counter
          x86.punpckhwd(mm4, mm3);                      // [....|....|..33|..22]
          x86.punpckldq(mm5, mm4);                      // [..33|..22|..11|..00]
          x86.movq(     mm4, qword_ptr[esi+ecx*4+8]);   // Get pixels 3 & 2
        } else {
          x86.psrad(    mm4, 15);                       // -> 8 bit result              -- 2
          x86.psrad(    mm3, 15);                       // -> 8 bit result              -- 3
          x86.add(      ecx, 4);                        // loop counter
          x86.punpckhwd(mm4, mm3);                      // [....|....|..33|..22]
          x86.punpckldq(mm5, mm4);                      // [..33|..22|..11|..00]
        }
      } else { // (i==2 && rwidth != 3)
        x86.psrad(      mm5, 15);                       // -> 8 bit result              -- 0
        x86.punpckhwd(  mm5, mm6);                      // [....|....|..11|..00]
        x86.add(        ecx, 4);                        // loop counter
      }
      x86.packuswb(     mm5, mm0);                      // [..|..|..|..|33|22|11|00]
      if (i==0 && awidth > 8) {
        x86.cmp(        ecx, awidth-4);                 // break >= myx & ~3
        x86.movd(       dword_ptr[edi+ecx], mm5);       // write 4 pixels
        x86.jb(         "loop4");
      } else {
        x86.movd(       dword_ptr[edi+ecx], mm5);       // write 4 pixels
      }
    }
  }

  x86.add(          esi, ebx);                      // += pitchS
  x86.add(          edi, edx);                      // += pitchD

  x86.dec(          eax);
  x86.jnz(          "yloop");

  x86.emms(         );

   // Restore registers
  x86.pop(          edi);
  x86.pop(          esi);
  x86.pop(          edx);
  x86.pop(          ecx);
  x86.pop(          ebx);
  x86.pop(          eax);
  x86.ret();

  assembly = DynamicAssembledCode(x86, env, "ConvertToY8::RGB32 Dynamic MMX code could not be compiled.");
}


/***************************
 * RGB24 to Y8 converter
 *
 * Unpacks 4 pixels per loop.
 * handles non mod4 widths
 ***************************/


void RGBtoY8Generator::genRGB24toY8(int width, int height, int offset_y, signed short* matrix,
                                      IScriptEnvironment* env) {
  enum {             // Argument offsets
    src     = 0,
    dst     = 4,
    pitchS  = 8,
    pitchD = 12,
  };

  Assembler x86;   // This is the class that assembles the code.

  const bool sse   = !!(env->GetCPUFlags() & CPUF_SSE);
  const bool sse2  = !!(env->GetCPUFlags() & CPUF_SSE2);

  const int awidth = width & ~3;
  const int rwidth = width &  3;
  const int lwidth = (width+3) & ~3;

  // Store registers and get arg pointers
  x86.push(         eax);
  x86.mov(          eax, dword_ptr[esp+4+4]);       // Pointer to args list

  x86.push(         ebx);
  x86.push(         ecx);
  x86.push(         edx);
  x86.push(         esi);
  x86.push(         edi);

  x86.mov(          esi, dword_ptr[eax+src]);
  x86.mov(          edi, dword_ptr[eax+dst]);
  x86.mov(          ebx, dword_ptr[eax+pitchS]);
  x86.mov(          edx, dword_ptr[eax+pitchD]);

  x86.mov(          eax, offset_y*2+1);
  x86.pxor(         mm0, mm0);
  x86.movd(         mm1, eax);
  x86.movq(         mm2, qword_ptr[matrix]);        // [0|cyr|cyg|cyb]
  x86.psllq(        mm1, 46);                       // 0x0008400000000000

  x86.sub(          ebx, lwidth*3);                 // pitch - arowsize
  x86.sub(          edx, lwidth);                   // pitch - arowsize
  x86.sub(          edi, 4);                        // adjust for early +=4

  x86.mov(          eax, height);

  x86.align(        16);
  x86.label("yloop");

  x86.mov(          ecx, awidth-4);
  if (sse2) {
    x86.prefetchnta(byte_ptr[esi]);
    x86.prefetchnta(byte_ptr[esi+64]);
  } else if (sse) {
    x86.prefetchnta(byte_ptr[esi]);
    x86.prefetchnta(byte_ptr[esi+32]);
    x86.prefetchnta(byte_ptr[esi+64]);
    x86.prefetchnta(byte_ptr[esi+96]);
  }
  x86.add(          ecx, edi);

  for (int i=0; i<3; i++) {
    if (i==0 && awidth > 0) {
      x86.movd(     mm3, dword_ptr[esi+0]);             // Get pixel 0
      x86.movd(     mm6, dword_ptr[esi+3]);             // Get pixel 1
      x86.movd(     mm4, dword_ptr[esi+6]);             // Get pixel 2
      if (awidth > 8) {
        x86.align(  16);
        x86.label("loop4");
        if (sse)
          x86.prefetchnta(byte_ptr[esi+128]);
      }
    } else if (i==2 && rwidth == 3) {
      x86.movd(         mm3, dword_ptr[esi+0]);         // Get pixel 0
      x86.movd(         mm6, dword_ptr[esi+3]);         // Get pixel 1
      x86.movd(         mm4, dword_ptr[esi+6]);         // Get pixel 2
    } else if (i==2 && rwidth == 2) {
      x86.movd(         mm3, dword_ptr[esi+0]);         // Get pixel 0
      x86.movd(         mm6, dword_ptr[esi+3]);         // Get pixel 1
    } else if (i==2 && rwidth == 1) {
      x86.movd(         mm3, dword_ptr[esi+0]);         // Get pixel 0
      x86.pxor(         mm6, mm6);                      // zero pixel 1
    }
    if ((i==0 && awidth > 4) ||
        (i==1 && awidth > 0) ||
        (i==2 && rwidth > 0) ) {
      x86.punpcklbw(    mm3, mm0);                      // [00l?|00rr|00gg|00bb]        -- 0
      x86.punpcklbw(    mm6, mm0);                      // [00h?|00rr|00gg|00bb]        -- 1
      x86.pmaddwd(      mm3, mm2);                      // [0*?+cyr*r|cyg*g+cyb*b]      -- 0
      x86.pmaddwd(      mm6, mm2);                      // [0*?+cyr*r|cyg*g+cyb*b]      -- 1
      x86.punpckldq(    mm5, mm3);                      // [loDWmm5|junk]               -- 0
      x86.paddd(        mm3, mm1);                      // +=0.5
      x86.punpckldq(    mm7, mm6);                      // [loDWmm6|junk]               -- 1
      x86.paddd(        mm6, mm1);                      // +=0.5
      x86.paddd(        mm5, mm3);                      // [hiDWmm5+32768+loDWmm5|junk] -- 0
      x86.paddd(        mm6, mm7);                      // [hiDWmm6+32768+loDWmm6|junk] -- 1
      x86.psrad(        mm5, 15);                       // -> 8 bit result              -- 0
      if (i!=2 || rwidth == 3) {
        if (i!=2) {
          x86.movd(     mm3, dword_ptr[esi+9]);         // Get pixel 3
        } else {
          x86.pxor(     mm3, mm3);                      // zero pixel 3
        }
        x86.psrad(      mm6, 15);                       // -> 8 bit result              -- 1
        x86.punpcklbw(  mm4, mm0);                      // [00la|00rr|00gg|00bb]        -- 2
        x86.punpckhwd(  mm5, mm6);                      // [....|....|..11|..00]
        x86.punpcklbw(  mm3, mm0);                      // [00ha|00rr|00gg|00bb]        -- 3
        x86.pmaddwd(    mm4, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 2
        x86.pmaddwd(    mm3, mm2);                      // [0*a+cyr*r|cyg*g+cyb*b]      -- 3
        x86.punpckldq(  mm7, mm4);                      // [loDWmm4|junk]               -- 2
        x86.paddd(      mm4, mm1);                      // +=0.5
        x86.punpckldq(  mm6, mm3);                      // [loDWmm3|junk]               -- 3
        x86.paddd(      mm3, mm1);                      // +=0.5
        x86.paddd(      mm4, mm7);                      // [hiDWmm4+32768+loDWmm4|junk] -- 2
        x86.paddd(      mm3, mm6);                      // [hiDWmm3+32768+loDWmm3|junk] -- 3
        x86.psrad(      mm4, 15);                       // -> 8 bit result              -- 2
        x86.psrad(      mm3, 15);                       // -> 8 bit result              -- 3
        x86.add(        esi, 12);                       // src ptr
        x86.punpckhwd(  mm4, mm3);                      // [....|....|..33|..22]
        if (i==0 && awidth > 4) {
          x86.movd(     mm3, dword_ptr[esi+0]);         // Get pixel 0
          x86.add(      edi, 4);                        // dst ptr+=4
          x86.movd(     mm6, dword_ptr[esi+3]);         // Get pixel 1
          x86.punpckldq(mm5, mm4);                      // [..33|..22|..11|..00]
          x86.movd(     mm4, dword_ptr[esi+6]);         // Get pixel 2
        } else {
          x86.add(      edi, 4);                        // dst ptr+=4
          x86.punpckldq(mm5, mm4);                      // [..33|..22|..11|..00]
        }
      } else { // (i==2 && rwidth != 3)
        x86.psrad(      mm6, 15);                       // -> 8 bit result              -- 1
        x86.add(        edi, 4);                        // dst ptr+=4
        x86.punpckhwd(  mm5, mm6);                      // [....|....|..11|..00]
        x86.add(        esi, 12);                       // src ptr
      }
      x86.packuswb(     mm5, mm0);                      // [..|..|..|..|33|22|11|00]
      if (i==0 && awidth > 8) {
        x86.cmp(        edi, ecx);
        x86.movd(       dword_ptr[edi], mm5);           // write 4 pixels
        x86.jb(         "loop4");
      } else {
        x86.movd(       dword_ptr[edi], mm5);           // write 4 pixels
      }
    }
  }

  x86.add(          esi, ebx);                      // += pitchS - arowsize
  x86.add(          edi, edx);                      // += pitchD - arowsize

  x86.dec(          eax);
  x86.jnz(          "yloop");

  x86.emms(         );

   // Restore registers
  x86.pop(          edi);
  x86.pop(          esi);
  x86.pop(          edx);
  x86.pop(          ecx);
  x86.pop(          ebx);
  x86.pop(          eax);
  x86.ret();

  assembly = DynamicAssembledCode(x86, env, "ConvertToY8::RGB24 Dynamic MMX code could not be compiled.");
}

#endif
