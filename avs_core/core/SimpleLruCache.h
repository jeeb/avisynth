#ifndef AVS_SIMPLELRUCACHE_H
#define AVS_SIMPLELRUCACHE_H

#include <list>
#include <functional>

template<typename K, typename V>
class SimpleLruCache
{
public:
  struct Entry
  {
    K key;
    V value;

    Entry(const K& k) :
      key(k)
    {}
  };

  typedef V value_type;
  typedef K key_type;
  typedef typename std::list<Entry>::iterator entry_type;

  typedef std::function<bool(SimpleLruCache*, const Entry&, void*)> EvictEventType;

private:
  size_t Capacity;
  std::list<Entry> Cache;
  std::list<Entry> Pool;

  void* EventUserData;
  const EvictEventType EvictEvent;

public:
  SimpleLruCache(size_t capacity, const EvictEventType& evict, void* evData) :
    Capacity(capacity),
    EventUserData(evData),
    EvictEvent(evict)
  {
  }

  size_t size() const
  {
    return Cache.size();
  }

  size_t capacity() const
  {
    return Capacity;
  }

  V* lookup(const K& key, bool *found)
  {
    // Look for an existing cache entry,
    // and return it when found
    for (
      entry_type it = Cache.begin();
      it != Cache.end();
      ++it
    )
    {
      if (it->key == key)
      {
        // Move found element to the front of the list
        if (it != Cache.begin())
          Cache.splice(Cache.begin(), Cache, it);

        *found = true;
        return &(Cache.front().value);
      }
    }

    // Nothing found
    *found = false;

    // Evict an old element if the cache is full
    trim();

    if (Capacity != 0)
    {
      // See if we can take one from our pool
      if (!Pool.empty())
      {
        Cache.splice(Cache.begin(), Pool, Pool.begin());
        Cache.front().key = key;
      }
      else
      {
        Cache.emplace_front(key);
      }

      return &(Cache.front().value);
    }

    // Return NULL-storage if we cannot store anything
    return NULL;
  }

  void remove(const K& key)
  {
    for (
      entry_type it = Cache.begin();
      it != Cache.end();
      ++it
    )
    {
      if (it->key == key)
      {
        Pool.splice(Pool.begin(), Cache, it);
        break;
      }
    }
  }

  void trim()
  {
    std::list<Entry>::reverse_iterator it = Cache.rbegin();
    while( (Cache.size() > Capacity) && (it != Cache.rend()))
    {
      std::list<Entry>::reverse_iterator next = it;
      ++next;

      std::list<Entry>::iterator fwd_it = next.base();
      assert(&(*it) == &(*fwd_it));

      if (EvictEvent != NULL)
      {
        if (EvictEvent(this, *it, EventUserData))
        {
          Pool.splice(Pool.begin(), Cache, fwd_it);
        }
      }
      else
      {
        // TODO: Do we want the consumer to always define EvictItem?
        Pool.splice(Pool.begin(), Cache, fwd_it);
      }

      it = next;
    }
  }

  void resize(size_t new_cap)
  {
    Capacity = new_cap;
    trim();
  }
};

#endif // AVS_SIMPLELRUCACHE_H