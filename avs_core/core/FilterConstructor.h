#ifndef _AVS_FILTER_CONSTRUCTOR_H
#define _AVS_FILTER_CONSTRUCTOR_H

#include "internal.h"
#include <vector>

class FilterConstructor
{
private:
  IScriptEnvironment2* const Env;
  const AVSFunction* const Func;
  const std::vector<AVSValue> ArgStorage;
  const std::vector<AVSValue> CtorArgs;

public:
  FilterConstructor(IScriptEnvironment2 * env, const AVSFunction *func, std::vector<AVSValue>* argStorage, std::vector<AVSValue>* ctorArgs);
  AVSValue InstantiateFilter() const;

  const char* GetFilterName() const
  {
    return Func->name;
  }

  bool IsScriptFunction() const
  {
    return AVSFunction::IsScriptFunction(Func);
  }

#ifdef DEBUG_GSCRIPTCLIP_MT
  bool IsRuntimeScriptFunction() const
  {
    return Func->IsRuntimeScriptFunction();
  }
#endif

  const AVSFunction* GetAvsFunction() const
  {
      return Func;
  }
};

#endif  // _AVS_FILTER_CONSTRUCTOR_H
