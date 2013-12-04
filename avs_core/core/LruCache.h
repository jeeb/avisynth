#ifndef AVS_LRUCACHE_H
#define AVS_LRUCACHE_H

#include <boost/unordered_map.hpp>
#include <boost/pool/pool.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <list>

enum LruEntryState
{
  LRU_ENTRY_EMPTY,
  LRU_ENTRY_AVAILABLE,
  LRU_ENTRY_ROLLED_BACK
};

template<typename K, typename V>
struct LruEntry
{
  K key;
  V value;
  size_t refs;    // the number of threads waiting on this entry. used to prevent eviction when readers are waiting on it
  boost::condition_variable ready_cond;
  enum LruEntryState state;

  LruEntry(K key) :
    key(key), refs(1), state(LRU_ENTRY_EMPTY)
  {
  }
};

template<typename K, typename V>
class LruCache : public boost::enable_shared_from_this<LruCache<K, V> >
{
private:
  typedef size_t  size_type;
  typedef LruEntry<K, V> entry_type;
  typedef entry_type* entry_ptr;
  typedef std::list<entry_ptr>  list_type;
  typedef typename list_type::iterator  list_iterator;
  typedef boost::unordered_map<K, list_iterator> map_type;

  list_type list;
  map_type map;
  boost::mutex mutex;

public:

  typedef std::pair<list_iterator, boost::shared_ptr<LruCache<K, V> > > handle;

  size_type max_size;

  LruCache(size_type max_size) :
    max_size(max_size)
  {
    const float MAX_LOAD_FACOR = 0.8f;
    map.max_load_factor(MAX_LOAD_FACOR);
    map.reserve((map_type::size_type)(max_size/MAX_LOAD_FACOR)+1);
  }

  ~LruCache()
  {
    for (list_iterator it = list.begin();
      it != list.end();
      ++it)
    {
      delete *it;
    }
  }

  size_type size() const
  {
    return list.size();
  }

  bool get_insert(K key, V *ret_value, handle *hndl)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);

    map_type::iterator it = map.find(key);
    if (it != map.end())
    {
      *hndl = handle(it->second, this->shared_from_this());
      entry_ptr e = *(it->second);

      // move element to front of lru list
      list.splice(list.begin(), list, it->second);

      // wait until data becomes available
      ++(e->refs);
      while(e->state == LRU_ENTRY_EMPTY)
      {
        e->ready_cond.wait(global_lock);

        switch (e->state)
        {
        case LRU_ENTRY_EMPTY:           // do nothing, spurious wakeup
          break;
        case LRU_ENTRY_AVAILABLE:       // finally, data available
          break;
        case LRU_ENTRY_ROLLED_BACK:     // whoever we were waiting for decided to step back. we take over his place.
          e->state = LRU_ENTRY_EMPTY;
          return false;
        default:
          assert(0);
        }
      }
      *ret_value = e->value;
      --(e->refs);
      return true;
    }
    else
    {
      // add new element
      *hndl = handle(list.insert(list.begin(), new entry_type(key)), this->shared_from_this());  // TODO prevent heap usage
      map.insert(map_type::value_type(key, hndl->first));
      
      // trim cache
      // we only delete elements from the back, and only those which have nor referrers
      while(list.size() >= max_size)
      {
        entry_ptr e = list.back();
        if (e->refs == 0)
        {
          map.erase(e->key);
          list.pop_back();
          delete e;
        }
        else
        {
          break;
        }
      }
      
      return false;
    } // if
  }

  void commit_value(handle *hndl, const V& value)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);
    // insert data
    entry_ptr e = *(hndl->first);
    e->value = value;
    e->state = LRU_ENTRY_AVAILABLE;
    --(e->refs);

    // notify waiters
    global_lock.unlock();
    e->ready_cond.notify_all();

    *hndl = handle();
  }

  void rollback(handle *hndl)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);
    
    entry_ptr e = *(hndl->first);
    if (e->refs == 1)
    {
      // we are the only one needing this data, so delete it
      map.erase(e->key);
      list.erase(hndl->first);
      delete e;
    }
    else
    {
      // others have started waiting for this data, so another one will have to take over
      --(e->refs);
      e->state = LRU_ENTRY_ROLLED_BACK;
      global_lock.unlock();
      e->ready_cond.notify_one();
    }

    *hndl = handle();
  }
};

#endif  // AVS_LRUCACHE_H
