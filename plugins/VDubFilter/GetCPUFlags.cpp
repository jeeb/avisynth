// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

//	VirtualDub - Video processing and capture application
//	Copyright (C) 1998-2000 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <intrin.h>

#define IS_BIT_SET(bitfield, bit) ((bitfield) & (1<<(bit)) ? true : false)

// For GetCPUFlags.  These are backwards-compatible with those in VirtualDub.
enum {
                    /* oldest CPU to support extension */
  CPUF_FORCE        =  0x01,   //  N/A
  CPUF_FPU          =  0x02,   //  386/486DX
  CPUF_MMX          =  0x04,   //  P55C, K6, PII
  CPUF_INTEGER_SSE  =  0x08,   //  PIII, Athlon
  CPUF_SSE          =  0x10,   //  PIII, Athlon XP/MP
  CPUF_SSE2         =  0x20,   //  PIV, K8
  CPUF_3DNOW        =  0x40,   //  K6-2
  CPUF_3DNOW_EXT    =  0x80,   //  Athlon
  CPUF_X86_64       =  0xA0,   //  Hammer (note: equiv. to 3DNow + SSE2, which
                               //          only Hammer will have anyway)
  CPUF_SSE3         = 0x100,   //  PIV+, K8 Venice
  CPUF_SSSE3        = 0x200,   //  Core 2
  CPUF_SSE4_1       = 0x400,   //  Penryn, Wolfdale, Yorkfield
  CPUF_AVX          = 0x800,   //  Sandy Bridge, Bulldozer
};

static int CPUCheckForExtensions()
{
  int result = 0;
  int cpuinfo[4];

  __cpuid(cpuinfo, 1);
  if (IS_BIT_SET(cpuinfo[3], 0))
    result |= CPUF_FPU;
  if (IS_BIT_SET(cpuinfo[3], 23))
    result |= CPUF_MMX;
  if (IS_BIT_SET(cpuinfo[3], 25))
    result |= CPUF_SSE | CPUF_INTEGER_SSE;
  if (IS_BIT_SET(cpuinfo[3], 26))
    result |= CPUF_SSE2;
  if (IS_BIT_SET(cpuinfo[2], 0))
    result |= CPUF_SSE3;
  if (IS_BIT_SET(cpuinfo[2], 9))
    result |= CPUF_SSSE3;
  if (IS_BIT_SET(cpuinfo[2], 19))
    result |= CPUF_SSE4_1;

  // 3DNow!, 3DNow!, and ISSE
  __cpuid(cpuinfo, 0x80000000);   
  if (cpuinfo[0] >= 0x80000001) 
  {
    __cpuid(cpuinfo, 0x80000001);   
   
    if (IS_BIT_SET(cpuinfo[3], 31))
      result |= CPUF_3DNOW;   
   
    if (IS_BIT_SET(cpuinfo[3], 30))
      result |= CPUF_3DNOW_EXT;   
   
    if (IS_BIT_SET(cpuinfo[3], 22))
      result |= CPUF_INTEGER_SSE;   
  }

  // AVX
#if (_MSC_FULL_VER >= 160040219)    // We require VC++2010 SP1 at least
  bool xgetbv_supported = IS_BIT_SET(cpuinfo[2], 27);
  bool avx_supported = IS_BIT_SET(cpuinfo[2], 28);
  if (xgetbv_supported && avx_supported)
  {
    if ((_xgetbv(_XCR_XFEATURE_ENABLED_MASK) & 0x6ull) == 0x6ull)
      result |= CPUF_AVX;   
  }
#endif

  return result;
}

int GetCPUFlags() {
  static int lCPUExtensionsAvailable = CPUCheckForExtensions();
  return lCPUExtensionsAvailable;
}
