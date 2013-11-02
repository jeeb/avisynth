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


#ifndef AVSCORE_VARTABLE_H
#define AVSCORE_VARTABLE_H

#include "strings.h"
#include <avisynth.h>
#include <boost/unordered_map.hpp>

struct iequal_to_ascii
{
  bool operator()(const char* str1, const char* str2) const
  {
    return streqi(str1, str2);
  }
};

struct ihash_ascii
{
  std::size_t operator()(const char* s) const
  {
	  // NOTE the connection between the hash() and equals() functions!
	  // In order for the hash table to work correctly, if two strings compare
	  // equal, they MUST have the same hash.

    size_t hash = 0;
    while (*s)
      hash = hash * 101  +  tolower(*s++);

    return hash;
  }
};

class VarTable
{
private:
  VarTable* const dynamic_parent;
  VarTable* const lexical_parent;

  typedef boost::unordered_map<const char*, AVSValue, ihash_ascii, iequal_to_ascii> ValueMap;
  ValueMap variables;

public:
  VarTable(VarTable* _dynamic_parent, VarTable* _lexical_parent) :
    dynamic_parent(_dynamic_parent), lexical_parent(_lexical_parent),
    variables()
  {
    variables.max_load_factor(0.8f);
  }

  VarTable* Pop()
  {
    VarTable* _dynamic_parent = this->dynamic_parent;
    delete this;
    return _dynamic_parent;
  }

  // This method will not modify the *val argument if it returns false.
  bool Get(const char* name, AVSValue *val) const
  {
    ValueMap::const_iterator v = variables.find(name);
    if (v != variables.end())
    {
      *val = v->second;
      return true;
    }

    if (lexical_parent)
      return lexical_parent->Get(name, val);
    else
      return false;
  }

  bool Set(const char* name, const AVSValue& val)
  {
    std::pair<ValueMap::iterator, bool> ret = variables.insert(ValueMap::value_type(name, val));
    ret.first->second = val;
    return ret.second;
  }
};

#endif // AVSCORE_VARTABLE_H