#include "pfc.h"
#ifdef __GNUC__
#include <x86intrin.h>
#else
#include <intrin.h>
#endif
#include <stdint.h>

int64_t profiler_local::get_timestamp()
{
  return __rdtsc();
}