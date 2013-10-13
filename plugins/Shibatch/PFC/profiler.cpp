#include "pfc.h"
#include <intrin.h>

__int64 profiler_local::get_timestamp()
{
  return __rdtsc();
}