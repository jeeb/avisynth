// Richard Berg, yadda yadda

#ifndef __FastLib_H__
#define __FastLib_H__

#include <iostream>
#include <iomanip>
#include <windows.h>

                /* slowest CPU to support extension */
#define CPUF_FORCE	       0x01   // N/A
#define CPUF_FPU			     0x02   // 386/486DX
#define CPUF_MMX			     0x04   // P55C, K6, PII
#define CPUF_INTEGER_SSE	 0x08	  // PIII, Athlon
#define CPUF_SSE			     0x10	  // PIII, Athlon XP/MP
#define CPUF_SSE2			     0x20	  // PIV, Hammer
#define CPUF_3DNOW			   0x40   // K6-2
#define CPUF_3DNOW_EXT		 0x80	  // Athlon


#define CPU_P1    CPUF_FPU
#define CPU_P1MMX (CPU_P1 | CPUF_MMX)
#define CPU_P2    CPU_P1MMX
#define CPU_P3    (CPU_P2 | CPUF_INTEGER_SSE | CPUF_SSE)
#define CPU_P4    (CPU_P3 | CPUF_SSE2)

#define CPU_K6    (CPU_P2 | CPUF_3DNOW)
#define CPU_K7    (CPU_K6 | CPUF_3DNOW_EXT | CPUF_INTEGER_SSE)
#define CPU_K7XP  (CPU_K7 | CPUF_SSE)
#define CPU_K8    (CPU_K7XP | CPUF_SSE2)

typedef unsigned long UL;


class Fastlib
{
public:
  Fastlib (void) {};
  virtual ~Fastlib (void) {};

  virtual void fMemcpy (void * dst, const void * src, UL n) = 0;
  virtual void fMemset (void * dst, int val, UL n) = 0;
};



#define CPULIB P1Lib
#define CPUF CPU_P1
#include "Cpulib.h"
#undef CPUF
#undef CPULIB

#define CPULIB P2Lib
#define CPUF CPU_P2
#include "Cpulib.h"
#undef CPUF
#undef CPULIB

#define CPULIB P3Lib
#define CPUF CPU_P3
#include "Cpulib.h"
#undef CPUF
#undef CPULIB

#define CPULIB P4Lib
#define CPUF CPU_P4
#include "Cpulib.h"
#undef CPUF
#undef CPULIB

#define CPULIB K6Lib
#define CPUF CPU_K6
#include "Cpulib.h"
#undef CPUF
#undef CPULIB

#define CPULIB K7Lib
#define CPUF CPU_K7
#include "Cpulib.h"
#undef CPUF
#undef CPULIB



#endif  // __FastLib_H__