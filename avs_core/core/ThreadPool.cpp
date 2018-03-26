#include "ThreadPool.h"
#include "ScriptEnvironmentTLS.h"
#include <cassert>
#include <thread>

struct ThreadPoolGenericItemData
{
  ThreadWorkerFuncPtr Func;
  void* Params;
  InternalEnvironment* Environment;
  AVSPromise* Promise;
};

enum ThreadMessagesType{
  INVALID_MSG,
  QUEUE_GENERIC_ITEM,
  THREAD_STOP
};

struct ThreadMessage
{
  ThreadMessagesType Type;
  ThreadPoolGenericItemData GenericWorkItemData;

  ThreadMessage() :
    Type(INVALID_MSG)
  {}
  ThreadMessage(ThreadMessagesType type) :
    Type(type)
  {}
  ThreadMessage(ThreadMessagesType type, const ThreadPoolGenericItemData &data) :
    Type(type), GenericWorkItemData(data)
  {}
};

#include "mpmc_bounded_queue.h"
typedef mpmc_bounded_queue<ThreadMessage> MessageQueue;

static void ThreadFunc(size_t thread_id, MessageQueue *msgQueue)
{
  ScriptEnvironmentTLS EnvTLS(thread_id);

  bool runThread = true;
  while(runThread)
  {
    ThreadMessage msg;
    msgQueue->pop_back(&msg);

    switch(msg.Type)
    {
    case THREAD_STOP:
      {
        runThread = false;
        break;
      }
    case QUEUE_GENERIC_ITEM:
      {
        ThreadPoolGenericItemData &data = msg.GenericWorkItemData;
        EnvTLS.Specialize(data.Environment);
        if (data.Promise != NULL)
        {
          try
          {
            data.Promise->set_value(data.Func(&EnvTLS, data.Params));
          }
          catch(const AvisynthError&)
          {
            data.Promise->set_exception(std::current_exception());
          }
          catch(const std::exception&)
          {
            data.Promise->set_exception(std::current_exception());
          }
          catch(...)
          {
            data.Promise->set_exception(std::current_exception());
            //data.Promise->set_value(AVSValue("An unknown exception was thrown in the thread pool."));
          }
        }
        else
        {
          try
          {
            data.Func(&EnvTLS, data.Params);
          } catch(...){}
        }
        break;
      }
    default:
      {
        assert(0);
        break;
      }
    } // switch
  } //while
}

__declspec(thread) size_t g_thread_id;
__declspec(thread) int g_suppress_thread_count;

class ThreadPoolPimpl
{
public:
  std::vector<std::thread> Threads;
  MessageQueue MsgQueue;

  ThreadPoolPimpl(size_t nThreads) :
    Threads(),
    MsgQueue(nThreads * 6)
  {}
};

ThreadPool::ThreadPool(size_t nThreads) :
  _pimpl(new ThreadPoolPimpl(nThreads))
{
  _pimpl->Threads.reserve(nThreads);

  // i is used as the thread id. Skip id zero because that is reserved for the main thread.
  for (size_t i = 1; i <= nThreads; ++i)
    _pimpl->Threads.emplace_back(ThreadFunc, i, &(_pimpl->MsgQueue));
}

void ThreadPool::QueueJob(ThreadWorkerFuncPtr clb, void* params, InternalEnvironment *env, JobCompletion *tc)
{
  ThreadPoolGenericItemData itemData;
  itemData.Func = clb;
  itemData.Params = params;
  itemData.Environment = env;

  if (tc != NULL)
    itemData.Promise = tc->Add();
  else
    itemData.Promise = NULL;

  _pimpl->MsgQueue.push_front(ThreadMessage(QUEUE_GENERIC_ITEM, itemData));
}

size_t ThreadPool::NumThreads() const
{
  return _pimpl->Threads.size();
}

ThreadPool::~ThreadPool()
{
  for (size_t i = 0; i < _pimpl->Threads.size(); ++i)
  {
    _pimpl->MsgQueue.push_front(THREAD_STOP);
  }
  for (size_t i = 0; i < _pimpl->Threads.size(); ++i)
  {
    if (_pimpl->Threads[i].joinable())
      _pimpl->Threads[i].join();
  }

  delete _pimpl;
}
