#ifndef AVS_LRUCACHE_H
#define AVS_LRUCACHE_H

#include <boost/unordered_map.hpp>
#include <boost/pool/object_pool.hpp>
#include <boost/pool/pool_alloc.hpp>
#include <boost/atomic.hpp>
#include <boost/thread.hpp>
#include <list>
#include <functional>

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

  LruGhostEntry() :
    key(0), ghosted(0)
  {
  }

  LruGhostEntry(K key, size_t ghosted) :
    key(key), ghosted(ghosted)
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

  LruEntry(K key = K()) :
    key(key), locks(1), ghosted(0), state(LRU_ENTRY_EMPTY)
  {
  }
};



template<typename K, typename V>
class SimpleLruCache
{
public:
  struct Entry
  {
    Entry* next;
    Entry* prev;
    K key;
    V value;
    unsigned int ref;

    Entry() :
      next(NULL),
      prev(NULL)
    {}

    void Relink(Entry* node)
    {
      assert(node != NULL);

      // Make pointer backups
      Entry* p = this->prev;
      Entry* n = this->next;

      // Place this node to its new position, after "node" parameter
      this->next = node->next;
      if (node->next) { node->next->prev = this; }
      this->prev = node;
      node->next = this;

      // Tie together earlier neighbors
      if (p) { p->next = n; }
      if (n) { n->prev = p; }
    }

    void Remove()
    {
      if (prev) prev->next = next;
      if (next) next->prev = prev;
      prev = NULL;
      next = NULL;
    }
  };

  typedef V value_type;
  typedef K key_type;

  typedef std::function<bool(SimpleLruCache*, const Entry&, void*)> EvictEventType;

private:
  size_t capacity;
  size_t size;
  Entry* head;
  Entry* tail;
  boost::object_pool<Entry> EntryPool;

  void* EventUserData;
  const EvictEventType EvictEvent;

  void remove(Entry* evictItem)
  {
    if (evictItem == tail)
      tail = tail->prev;

    if (evictItem == head)
      head = head->next;

    evictItem->Remove();
  }

public:
  SimpleLruCache(size_t capacity, const EvictEventType& evict, void* evData) :
    capacity(capacity),
    size(0),
    head(NULL),
    tail(NULL),
    EventUserData(evData),
    EvictEvent(evict)
  {
  }

  size_t get_size() const
  {
    return size;
  }
  size_t get_capacity() const
  {
    return capacity;
  }

  void remove(const K& key)
  {
    // Try to find existing item
    Entry* search_hand = head;
    for (size_t i = 0; i < size; ++i)
    {
      if (search_hand->key == key)
      {
        remove(search_hand);
        break;
      }

      search_hand = search_hand->next;
    }
  }

  V* lookup(const K& key, bool *found)
  {
    // If the cache is empty, treat it specially
    if (size == 0)
    {
      head = EntryPool.construct();
      tail = head;
      *found = false;
      ++size;
      return &(head->value);
    }

    // Try to find existing item
    Entry* search_hand = head;
    for (size_t i = 0; i < size; ++i)
    {
      if (search_hand->key == key)
      {
        // Item found, put it to the front of the list.
        search_hand->Relink(head);

        *found = true;
        return &(search_hand->value);
      }

      search_hand = search_hand->next;
    }
    
    // Nothing found

    Entry* newItem;
    if (size < capacity)
    {
      // Create new item from scratch
      newItem = EntryPool.construct();
      ++size;
    }
    else
    {
      // Recycle item from end of list

      Entry* evictItem = NULL;
      if (EvictEvent == NULL)
      {
        evictItem = tail;
        newItem = evictItem;
      }
      else
      {
        Entry* search_hand = tail;
        for (size_t i = 0; i < size; ++i)
        {
          if (EvictEvent(this, *search_hand, EventUserData))
          {
            evictItem = search_hand;
            newItem = search_hand;
            break;
          }
          search_hand = search_hand->prev;
        }
        if (evictItem == NULL)
        {
          // Create new item from scratch
          newItem = EntryPool.construct();
          ++size;
        }
        else
        {
          remove(evictItem);
        }
      }
    }

    // Fill in data members
    newItem->key = key;
    newItem->ref = 1;

    // Add new item to front of list
    assert(newItem != NULL);
    Entry* oldHead = head;
    head = newItem;
    head->next = oldHead;
    if (oldHead) { oldHead->prev = head; }

    if (tail == NULL)
    {
      assert(size == 1);
      tail = head;
    }

    *found = false;
    return &(head->value);
  }

  void set_capacity(size_t new_cap)
  {
    capacity = new_cap;
    Entry* search_hand = tail;
    for (size_t i = 0; i < size; ++i)
    {
      Entry* p = search_hand->prev;

      if ((EvictEvent == NULL) || EvictEvent(this, *search_hand, EventUserData))
      {
        remove(search_hand);
        EntryPool.destroy(search_hand);
        --size;
      }

      if (size <= capacity)
        break;

      search_hand = p;
    }
  }
};


template<typename K, typename V>
class LruCache : public boost::enable_shared_from_this<LruCache<K, V> >
{
private:
  typedef LruEntry<K, V> entry_type;
  typedef SimpleLruCache<K, LruEntry<K, V> > CacheType;
  typedef SimpleLruCache<K, LruGhostEntry<K> > GhostCacheType;

  typedef size_t  size_type;
  typedef entry_type* entry_ptr;
  typedef std::list<entry_ptr, boost::fast_pool_allocator<entry_ptr> >  list_type;

  CacheType MainCache;
  GhostCacheType Ghosts;

  boost::mutex mutex;
  size_type Capacity;
  size_type Size;

  static bool MainEvictEvent(CacheType* cache, typename const CacheType::Entry& entry, void* userData)
  {
    if (entry.value.locks > 0)
      return false;

    LruCache* me = reinterpret_cast<LruCache*>(userData);

    bool ghost_found;
    GhostCacheType::value_type* g = me->Ghosts.lookup(entry.key, &ghost_found);
    if (!ghost_found)
    {
      *g = LruGhostEntry<K>(entry.key, 0);
    }
    g->ghosted++;

    return true;
  }

public:

  typedef std::pair<entry_ptr, boost::shared_ptr<LruCache<K, V> > > handle;

  LruCache(size_type capacity) :
    Capacity(capacity),
    MainCache(capacity, &MainEvictEvent, reinterpret_cast<void*>(this)),
    Ghosts(capacity*2, GhostCacheType::EvictEventType(), reinterpret_cast<void*>(this))
  {
  }

  size_type size() const
  {
    return list.size();
  }

  bool get_insert(const K& key, V *ret_value, handle *hndl)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);

    bool found;
    entry_ptr entry = MainCache.lookup(key, &found);
    *hndl = handle(entry, this->shared_from_this());

    if (found)
    {
      // wait until data becomes available
      ++(entry->locks);
      while(entry->state == LRU_ENTRY_EMPTY)
      {
        entry->ready_cond.wait(global_lock);

        switch (entry->state)
        {
        case LRU_ENTRY_EMPTY:           // do nothing, spurious wakeup
          break;
        case LRU_ENTRY_AVAILABLE:       // finally, data available
          break;
        case LRU_ENTRY_ROLLED_BACK:     // whoever we were waiting for decided to step back. we take over his place.
          entry->state = LRU_ENTRY_EMPTY;
          return false;
        default:
          assert(0);
        }
      }
      *ret_value = entry->value;
      --(entry->locks);
      return true;
    }
    else
    {
      bool ghost_found;
      GhostCacheType::value_type* g = Ghosts.lookup(key, &ghost_found);
      if (!ghost_found)
      {
        *g = LruGhostEntry<K>(key, 0);
      }
      else if (g->ghosted > 0)
      {
        Capacity += 1;
        Ghosts.set_capacity(Ghosts.get_capacity() + 2);
      }
      else
      {
        // This cannot happen
        assert(0);
      }
      entry->ghosted = g->ghosted;
      
      return false;
    } // if
  }

  void commit_value(handle *hndl, const V& value)
  {
    boost::unique_lock<boost::mutex> global_lock(mutex);

    // insert data
    entry_ptr e = hndl->first;
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
    
    entry_ptr e = hndl->first;
    if (e->locks == 1)
    {
      MainCache.remove(e->key);
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
