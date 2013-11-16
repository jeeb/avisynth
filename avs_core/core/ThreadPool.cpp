#include "ThreadPool.h"
#include "ScriptEnvironmentTLS.h"
#include <boost/shared_ptr.hpp>


struct ThreadPoolGenericItemData
{
  ThreadWorkerFuncPtr Func;
  void* Params;
  IScriptEnvironment2* Environment;
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
  ThreadMessage(ThreadMessagesType type, ThreadPoolGenericItemData &data) :
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
          catch(const AvisynthError& e)
          {
            data.Promise->set_exception(boost::copy_exception(e));
          }
          catch(const std::exception& e)
          {
            data.Promise->set_exception(boost::copy_exception(e));
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
    } // swicth
  } //while
}

class ThreadPoolPimpl
{
public:
  std::vector<boost::thread> Threads;  
  MessageQueue MsgQueue;

  ThreadPoolPimpl(size_t nThreads) :
    Threads(nThreads),
    MsgQueue(nThreads * 4)
  {}
};


ThreadPool::ThreadPool(size_t nThreads) :
  _pimpl(new ThreadPoolPimpl(nThreads))
{
  for (size_t i = 1; i < nThreads; ++i)
    _pimpl->Threads.push_back(boost::thread(ThreadFunc, i, &(_pimpl->MsgQueue)));
}

void ThreadPool::QueueJob(ThreadWorkerFuncPtr clb, void* params, IScriptEnvironment2 *env, JobCompletion *tc)
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
    _pimpl->Threads[i].join();
  }

  delete _pimpl;
}
