#ifndef _AVS_BUFFERPOOL_H
#define _AVS_BUFFERPOOL_H

#include <map>

class IScriptEnvironment2;

class BufferPool
{
private:

  struct BufferDesc;
  typedef std::multimap<size_t, BufferDesc*> MapType;

  IScriptEnvironment2* Env;
  MapType Map;

  void* PrivateAlloc(size_t nBytes, size_t alignment, void* user);
  void PrivateFree(void* buffer);

public:

  BufferPool(IScriptEnvironment2* env);
  ~BufferPool();

  void* Allocate(size_t nBytes, size_t alignment, bool pool);
  void Free(void* ptr);

};

#endif  // _AVS_BUFFERPOOL_H
