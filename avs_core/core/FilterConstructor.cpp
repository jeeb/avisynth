#include "FilterConstructor.h"

FilterConstructor::FilterConstructor(IScriptEnvironment2 * env, const AVSFunction *func, std::vector<AVSValue>* argStorage, std::vector<AVSValue>* ctorArgs) :
  Env(env),
  Func(func),
  ArgStorage(std::move(*argStorage)),
  CtorArgs(std::move(*ctorArgs))
{
}

AVSValue FilterConstructor::InstantiateFilter() const
{
  AVSValue funcArgs(CtorArgs.data(), CtorArgs.size());
  return Func->apply(funcArgs, Func->user_data, Env);
}
