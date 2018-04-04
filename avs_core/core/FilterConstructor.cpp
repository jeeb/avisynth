#include "FilterConstructor.h"

FilterConstructor::FilterConstructor(IScriptEnvironment2 * env, const Function *func, std::vector<AVSValue>* argStorage, std::vector<AVSValue>* ctorArgs) :
  Env(env),
  Func(func),
#ifndef NEW_AVSVALUE
  ArgStorage(std::move(*argStorage)),
  CtorArgs(std::move(*ctorArgs))
#else
  // no need to move, subarrays could not be returned
  ArgStorage(*argStorage),
  CtorArgs(*ctorArgs)
#endif
{
}

AVSValue FilterConstructor::InstantiateFilter() const
{
  AVSValue funcArgs(CtorArgs.data(), (int)CtorArgs.size());
  return Func->apply(funcArgs, Func->user_data, Env);
}
