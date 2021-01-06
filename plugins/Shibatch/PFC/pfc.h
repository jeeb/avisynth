#ifndef ___PFC_H___
#define ___PFC_H___

#ifdef _WIN32
#ifndef STRICT
#define STRICT
#endif
#include <windows.h>
#endif

#define PFC_ALLOCA_LIMIT (4096)

#define INDEX_INVALID ((unsigned)(-1))

#include <avs/config.h>
#include <stdlib.h>

#ifdef _WIN32
#include <tchar.h>
#endif
#include <stdio.h>

#include <assert.h>

#include <math.h>
#include <float.h>

#ifdef _MSC_VER

#define NOVTABLE _declspec(novtable)

#ifdef _DEBUG
#define ASSUME(X) assert(X)
#else
#define ASSUME(X) __assume(X)
#endif

#else

#define NOVTABLE

#define ASSUME(X) assert(X)

#endif

#ifndef WCHAR
#define WCHAR wchar_t
#endif

#include <stdint.h>
typedef unsigned long DWORD;   // DWORD = unsigned 32 bit value
typedef uint16_t WORD;   // WORD = unsigned 16 bit value
typedef uint8_t BYTE;     // BYTE = unsigned 8 bit value

#define tabsize(x) (sizeof(x)/sizeof(*x))

#ifdef _WIN32
#include <guiddef.h>
#else
typedef struct _GUID {
  uint32_t Data1;
  uint16_t Data2;
  uint16_t Data3;
  uint8_t Data4[8];
} GUID;
#endif

#ifdef GCC
#define __cdecl __attribute__((__cdecl__))
#include <wchar.h>
#define _strdup strdup
#endif

#include <stdlib.h>
#include <malloc.h>

#include <cstring>
#include "bit_array.h"
//#include "critsec.h"
#include "mem_block.h"
#include "list.h"
#include "ptr_list.h"
#include "string.h"
#include "profiler.h"
#include "cfg_var.h"
#include "cfg_memblock.h"
#include "guid.h"
#include "byte_order_helper.h"
#include "other.h"
#include "chainlist.h"
#endif //___PFC_H___