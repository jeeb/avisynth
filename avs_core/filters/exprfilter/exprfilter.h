#ifndef __Exprfilter_h
#define __Exprfilter_h

#include <avisynth.h>

#define MAX_EXPR_INPUTS 26

struct split1 {
  enum empties_t { empties_ok, no_empties };
};

template <typename Container>
Container& split(
  Container& result,
  const typename Container::value_type& s,
  const typename Container::value_type& delimiters,
  split1::empties_t empties = split1::empties_ok)
{
  result.clear();
  size_t current;
  size_t next = -1;
  do {
    if (empties == split1::no_empties) {
      next = s.find_first_not_of(delimiters, next + 1);
      if (next == Container::value_type::npos) break;
      next -= 1;
    }
    current = next + 1;
    next = s.find_first_of(delimiters, current);
    result.push_back(s.substr(current, next - current));
  } while (next != Container::value_type::npos);
  return result;
}

typedef enum {
  opLoadSrc8, opLoadSrc16, opLoadSrcF32, opLoadSrcF16, opLoadConst,
  opLoadSpatialX, opLoadSpatialY,
  opStore8, opStore10, opStore12, opStore14, opStore16, opStoreF32, opStoreF16, // avs+: 10,12,14 bit store
  opDup, opSwap,
  opAdd, opSub, opMul, opDiv, opMax, opMin, opSqrt, opAbs,
  opGt, opLt, opEq, opNotEq, opLE, opGE, opTernary,
  opAnd, opOr, opXor, opNeg,
  opExp, opLog, opPow
} SOperation;

typedef union {
  float fval;
  int32_t ival;
} ExprUnion;

struct FloatIntUnion {
  ExprUnion u;
  FloatIntUnion(int32_t i) { u.ival = i; }
  FloatIntUnion(float f) { u.fval = f; }
};

struct ExprOp {
  ExprUnion e;
  uint32_t op;
  ExprOp(SOperation op, float val) : op(op) {
    e.fval = val;
  }
  ExprOp(SOperation op, int32_t val = 0) : op(op) {
    e.ival = val;
  }
};

enum PlaneOp {
  poProcess, poCopy, poUndefined, poFill
};

struct ExprData {
#ifdef __VAPOURSYNTH__
  VSNodeRef *node[MAX_EXPR_INPUTS];
  VSVideoInfo vi;
#else
  PClip node[MAX_EXPR_INPUTS];
  VideoInfo vi;
#endif
  bool clipsUsed[MAX_EXPR_INPUTS]; // not doing GetFrame unreferenced input clips
  std::vector<ExprOp> ops[4]; // 4th: alpha
  int plane[4];
  float planeFillValue[4]; // optimize: fill plane with const
  int planeCopySourceClip[4]; // optimize: copy plane from which clip
  size_t maxStackSize;
  int numInputs;
#ifdef VS_TARGET_CPU_X86
  typedef void(*ProcessLineProc)(void *rwptrs, intptr_t ptroff[MAX_EXPR_INPUTS + 1], intptr_t niter, uint32_t spatialY);
  ProcessLineProc proc[4]; // 4th: alpha
  ExprData() : node(), vi(), proc() {}
#else
  ExprData() : node(), vi() {}
#endif
  ~ExprData() {
#ifdef VS_TARGET_CPU_X86
    for (int i = 0; i < 4; i++) // 4th: alpha
#ifdef VS_TARGET_OS_WINDOWS
      VirtualFree((LPVOID)proc[i], 0, MEM_RELEASE);
#else
      munmap((void *)proc[i], 0);
#endif
#endif
  }
};

class Exprfilter : public IClip
/**
  * 
**/
{
private:
  std::vector<PClip> children;
  std::vector<std::string> expressions;
  VideoInfo vi;
  ExprData d;
  const bool optAvx2; // disable avx2 path
  const bool optSingleMode; // generate asm code using only one XMM/YMM register set instead of two
  const bool optSSE2; // disable simd path

public:
  Exprfilter(const std::vector<PClip>& _child_array, const std::vector<std::string>& _expr_array, const char *_newformat, const bool _optAvx2, 
    const bool _optSingleMode2, const bool _optSSE2, IScriptEnvironment *env);
  PVideoFrame __stdcall GetFrame(int n, IScriptEnvironment *env);
  ~Exprfilter();
  static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);

  inline void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) {
    children[0]->GetAudio(buf, start, count, env);
  }

  inline const VideoInfo& __stdcall GetVideoInfo() {
    return vi;
  }

  inline bool __stdcall GetParity(int n) {
    return children[0]->GetParity(n);
  }

  int __stdcall SetCacheHints(int cachehints, int frame_range) {
    return cachehints == CACHE_GET_MTMODE ? MT_NICE_FILTER : 0;
  }

};

#endif //__Exprfilter_h
