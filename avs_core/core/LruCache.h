#ifndef AVS_LRUCACHE_H
#define AVS_LRUCACHE_H

#include <boost/unordered_map.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <list>

enum LruEntryState
{
  LRU_ENTRY_EMPTY,
  LRU_ENTRY_AVAILABLE,
  LRU_ENTRY_ROLLED_BACK
};

template<typename K>
struct LruGhostEntry
{
  K key;
  size_t ghosted;

  LruGhostEntry(K key) :
    key(key), ghosted(0)
  {
  }
};

template<typename K, typename V>
struct LruEntry
{
  K key;
  V value;
  size_t locks;      // the number of threads waiting on this entry. used to prevent eviction when readers are waiting on it
  size_t ghosted;    // the number of times this entry has entered the ghost list
  boost::condition_variable ready_cond;
  enum LruEntryState state;

  LruEntry(K key) :
    key(key), locks(1), ghosted(0), state(LRU_ENTRY_EMPTY)
  {
  }
};







template<typename K, typename V>
class SimpleLruCache
{
private:
  typedef size_t  size_type;
  typedef std::pair<K, V> entry_type;
  typedef std::list<entry_type, boost::fast_pool_allocator<entry_type> >  list_type;
  typedef typename list_type::iterator  list_iterator;
  typedef boost::unordered_map<K, list_iterator, boost::hash<K>, std::equal_to<K>, boost::fast_pool_allocator<std::pair<K const, list_iterator> > > map_type;

  list_type list;
  map_type map;

public:

  size_type max_size;

  SimpleLruCache(size_type max_size) :
    max_size(max_size)
  {
    const float MAX_LOAD_FACOR = 0.8f;
    map.max_load_factor(MAX_LOAD_FACOR);
    map.reserve((map_type::size_type)(max_size/MAX_LOAD_FACOR)+1);
  }

  size_type size() const
  {
    return list.size();
  }

  V* lookup(K key)
  {
    map_type::iterator it = map.find(key);
    if (it != map.end())
    {
      // move element to front of lru list
      list.splice(list.begin(), list, it->second);

      return &(it->second->second);
    }
    else
    {
      // add new element
      list_iterator li = list.insert(list.begin(), std::make_pair(key, V(key)));
      map.insert(map_type::value_type(key, li));
      
      // trim cache
      // we only delete elements from the back
      while(list.size() > max_size)
      {
        const entry_type& e = list.back();
        map.erase(e.second.key);
        list.pop_back();
      }
      
      return &(li->second);
    } // if
  }
};






template<typename K, typename V>
class LruCache : public boost::enable_shared_from_this<LruCache<K, V> >
{
private:
  typedef size_t  size_type;
  typedef LruEntry<K, V> entry_type;
  typedef entry_type* entry_ptr;
  typedef std::list<entry_ptr, boost::fast_pool_allocator<entry_ptr> >  list_type;
  typedef typename list_type::iterator  list_iterator;
  typedef boost::unordered_map<K, list_iterator, boost::hash<K>, std::equal_to<K>, boost::fast_pool_allocator<std::pair<K const, list_iterator> > > map_type;

  boost::object_pool<entry_type> entry_pool;
  list_type list;
  map_type map;
  boost::mutex mutex;

  size_type max_size;
  SimpleLruCache<K, LruGhostEntry<K> > ghosts;

public:

  typedef std::pair<list_iterator, boost::shared_ptr<LruCache<K, V> > > handle;

  LruCache(size_type max_size) :
    max_size(max_size),
    ghosts(max_size*2)
  {
    const float MAX_LOAD_FACOR = 0.8f;
    map.max_load_factor(MAX_LOAD_FACOR);
    map.reserve((map_type::size_type)(max_size/MAX_LOAD_FACOR)+1);
  }

/*
  ~LruCache()
  {
    for (list_iterator it = list.begin();
      it != list.end();
      ++it)
    {
      delete *it;
    }
  }
*/

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
      ++(e->locks);
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
      --(e->locks);
      return true;
    }
    else
    {
      // add new element
      *hndl = handle(list.insert(list.begin(), entry_pool.construct(key)), this->shared_from_this());
      map.insert(map_type::value_type(key, hndl->first));

      LruGhostEntry<K>* g = ghosts.lookup(key);
      if (g->ghosted > 0)
      {
        max_size += 1;
        ghosts.max_size += 2;
      }
      list.front()->ghosted = g->ghosted;
      
      // trim cache
      // we only delete elements from the back, and only those which have no referrers
      while(list.size() > max_size)
      {
        entry_ptr e = list.back();
        if (e->locks == 0)
        {
          LruGhostEntry<K>* g = ghosts.lookup(key);
          g->ghosted = e->ghosted + 1;

          map.erase(e->key);
          list.pop_back();
          entry_pool.destroy(e);
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
    --(e->locks);

    // notify waiters
    global_lock.unlock();
    e->ready_cond.notify_all();

    hndl->second.reset();
  }

  void rollback(handle *hndl)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);
    
    entry_ptr e = *(hndl->first);
    if (e->locks == 1)
    {
      // we are the only one needing this data, so delete it
      map.erase(e->key);
      list.erase(hndl->first);
      entry_pool.destroy(e);
    }
    else
    {
      // others have started waiting for this data, so another one will have to take over
      --(e->locks);
      e->state = LRU_ENTRY_ROLLED_BACK;
      global_lock.unlock();
      e->ready_cond.notify_one();
    }

    hndl->second.reset();
  }
};

#endif  // AVS_LRUCACHE_H
