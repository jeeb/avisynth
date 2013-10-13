// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA, or visit
// http://www.gnu.org/copyleft/gpl.html .
//
// Linking Avisynth statically or dynamically with other modules is making a
// combined work based on Avisynth.  Thus, the terms and conditions of the GNU
// General Public License cover the whole combination.
//
// As a special exception, the copyright holders of Avisynth give you
// permission to link Avisynth with independent modules that communicate with
// Avisynth solely through the interfaces defined in avisynth.h, regardless of the license
// terms of these independent modules, and to copy and distribute the
// resulting combined work under terms of your choice, provided that
// every copy of the combined work is accompanied by a complete copy of
// the source code of Avisynth (the version of Avisynth used to produce the
// combined work), being distributed under the terms of the GNU General
// Public License plus this exception.  An independent module is a module
// which is not derived from or based on Avisynth, such as 3rd-party filters,
// import and export plugins, or graphical user interfaces.


#ifndef AVSCORE_HASHTABLE_H
#define AVSCORE_HASHTABLE_H

// This is a simple implementation of a generic hash container.
// The goal is to be able to add, remove and retrieve elements very fast.
// We could use std::unordered_hash, but that is only available in C++11,
// so older VC++ wouldn't be able to use it. We could also link to
// Boost/Qt/..., but setting up such libraries would make it a hassle
// for newbies to compile the core. Therefore, here is my own implementation.

#include <cstring>
#include "strings.h"
#include <cassert>

template<typename V>
class hashtable
{

private:

  // Prevent copying
  hashtable(const hashtable&); // no implementation
  hashtable& operator=(const hashtable&); // no implementation 
    
  struct bucket;
  typedef bucket* bucket_ptr;

  struct bucket
  {
    bucket_ptr next;
    const char* key;
    V value;
  };
 
  bucket_ptr *buckets;
  size_t n_buckets;
  size_t n_items;

  #ifndef UNICODE

  static size_t hash(const char* s)
  {
	  // NOTE the connection between the hash() and equals() functions!
	  // In order for the hash table to work correctly, if two strings compare
	  // equal, they MUST have the same hash.

    size_t hash = 0;
    while (*s)
      hash = hash * 101  +  tolower(*s++);

    return hash;
  }

  #else
  The above functions are supposed to be case-insensitive, but the given 
  implementations only work for ASCII characters < 128.
  The functions need to be adapted for a unicode build.
  #endif
  
public:

  // For good results, choose a prime number for the size.
  hashtable(size_t init_size) :
    buckets(NULL), n_buckets(init_size), n_items(0)
  {
    buckets = new bucket_ptr[init_size];
    memset(buckets, 0, sizeof(buckets[0])*init_size);
  }

  // This method returns a /pointer/ to the value,
  // and NULL if the value has not been found.
  V* get(const char* key) const
  {
    size_t i = hash(key) % n_buckets;
    
    bucket_ptr b = buckets[i];
    while(b != NULL)
    {
      if (streqi(b->key, key))
        return &(b->value);

      b = b->next;
    }
     
    return NULL;
  }

  bool add(const char* key, const V& val)
  {
    // Get index of the bucket
    size_t i = hash(key) % n_buckets;
    
    if (buckets[i] == NULL)
    { // This is the first bucket at this index
      bucket_ptr b = new bucket();
      b->next = NULL;
      b->key = key;
      b->value = val;
      buckets[i] = b;
      
      ++n_items;
      return true;
    }
    else
    { 
      bucket_ptr b = buckets[i];
      bucket_ptr bprev = NULL;
      while(b != NULL)
      {
        if (streqi(b->key, key))
        { // An item with the same key has been found, update it
          b->value = val;
          return false;
        }
        bprev = b;
        b = b->next;
      }
        
      bucket_ptr new_b = new bucket();
      new_b->next = NULL;
      new_b->key = key;
      new_b->value = val;
      bprev->next = new_b;
      
      ++n_items;
      return true;
    }
  }
  
  bool remove(const char* key)
  {
    // Get index of the bucket
    size_t i = hash(key) % n_buckets;
    
    if (buckets[i] == NULL)
      return false;
      
    bucket_ptr b = buckets[i];
    bucket_ptr bprev = NULL;
    while(b != NULL)
    {
      if (equals(b->key, key))
      { // An item with the same key has been found, remove it
      
        if ((bprev == NULL) && (b->next == NULL)) // Are we the only one in the list?
          bucket[i] = NULL;
        else if (bprev == NULL) // Are we the first in the list?
          bucket[i] = b->next;
        else if (b->next == NULL) // Are we the last in the list?
          bprev->next = NULL;
        else // We must be somewhere in the middle of the list
          bprev->next = b->next;
          
        delete b;
        --n_items;
        return true;
      } // if

      bprev = b;
      b = b->next;

    } // while
  }

  bool contains(const char* key) const
  {
    return (get(key) != NULL);
  }

  void clear()
  {
    for(size_t i = 0; i < n_buckets; ++i)
    {
      if (buckets[i] == NULL)
        continue;
        
      bucket_ptr b = buckets[i];
      bucket_ptr bnext = NULL;
      while(b != NULL)
      {
        bnext = b->next;
        delete b;
        b = bnext;
      } // while
      
      buckets[i] = NULL;
    } // if
    
    n_items = 0;
  }
  
  ~hashtable()
  {
    clear();
    delete [] buckets;
    buckets = NULL;
  }
};

#endif // AVSCORE_HASHTABLE_H