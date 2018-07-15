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
#include "avs/alignment.h"
#include "avs/minmax.h"
#include <avisynth.h>
#include <unordered_map>
#include <mutex>

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

#if 0
class VarTable
{
private:
  VarTable* const dynamic_parent;
  VarTable* const lexical_parent;

  typedef std::unordered_map<const char*, AVSValue, ihash_ascii, iequal_to_ascii> ValueMap;
  ValueMap variables;

  // avoid write/read concurrency of global variables in runtime scripts in MT
  mutable std::mutex var_mutex;

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
    std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
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
    std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
    std::pair<ValueMap::iterator, bool> ret = variables.insert(ValueMap::value_type(name, val));
    ret.first->second = val;
    return ret.second;
  }
};
#endif

// This doles out storage space for strings.  No space is ever freed
// until the class instance is destroyed (which happens when a script
// file is closed).
class StringDump {
   enum { BLOCK_SIZE = 32768 };
   char* current_block;
   size_t block_pos, block_size;

   void ensure_length(int len)
   {
     if (block_pos + len + 1 > block_size) {
       char* new_block = new char[block_size = max(block_size, len + 1 + sizeof(char*))];
       _RPT0(0, "StringDump: Allocating new stringblock.\r\n");
       *(char**)new_block = current_block;   // beginning of block holds pointer to previous block
       current_block = new_block;
       block_pos = sizeof(char*);
     }
   }

public:
   StringDump() : current_block(0), block_pos(BLOCK_SIZE), block_size(BLOCK_SIZE) {}

   ~StringDump() {
      _RPT0(0, "StringDump: DeAllocating all stringblocks.\r\n");
      char* p = current_block;
      while (p) {
         char* next = *(char**)p;
         delete[] p;
         p = next;
      }
   }

   char* SaveString(const char* s, int len = -1, bool escape = false) {
     int srclen = (len == -1) ? (int)strlen(s) : len;

#if 0
     std::unique_ptr<char[]> ss;
#else
     std::string ws = s;
#endif

      if (escape) {
#if 0
        auto ws = MultiByteToWide(AreFileApisANSI() ? CP_ACP : CP_OEMCP, s, srclen);
#else
        // utf8: ascii characters are freely searchable w/o conversion (?) fixme PF: check it again
#endif
        len = 0;
        for (int i = 0; ws[i]; ++i, ++len) {
          if (ws[i] == '\\') {
            switch (ws[i + 1]) {
            case 'n': ws[len] = '\n'; ++i; continue;
            case 'r': ws[len] = '\r'; ++i; continue;
            case 't': ws[len] = '\t'; ++i; continue;
            case '0': ws[len] = '\0'; ++i; continue;
            case 'a': ws[len] = '\a'; ++i; continue;
            case 'f': ws[len] = '\f'; ++i; continue;
            case '\\': ws[len] = '\\'; ++i; continue;
            case '\"': ws[len] = '\"'; ++i; continue;
            }
          }
          ws[len] = ws[i];
        }
        ws[len] = 0;
#if 0
        ss = WideToMultiByte(AreFileApisANSI() ? CP_ACP : CP_OEMCP, ws.get(), len);
        s = ss.get();
        len = (int)strlen(s);
#else
        s = ws.c_str();
        len = ws.size();
#endif
      }
      else {
        len = srclen;
      }

      ensure_length(len);
      char* result = current_block + block_pos;
      memcpy(result, s, len);
      result[len] = 0;
      block_pos += AlignNumber(len + 1, (int)sizeof(char*)); // Keep word-aligned
      return result;
   }

   void Clear() {
      if (current_block) {
         // deallocate string blocks except the first one
         while (char* p = *(char**)current_block) {
            delete[] current_block;
            current_block = p;
         }
         block_pos = sizeof(char*);
         block_size = BLOCK_SIZE;
      }
   }
};

class VarFrame
{
   typedef std::unordered_map<const char*, AVSValue, ihash_ascii, iequal_to_ascii> ValueMap;
   ValueMap variables;

public:
   VarFrame() {
      variables.max_load_factor(0.8f);
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
      return false;
   }

   bool Set(const char* name, const AVSValue& val)
   {
      std::pair<ValueMap::iterator, bool> ret = variables.insert(ValueMap::value_type(name, val));
      ret.first->second = val;
      return ret.second;
   }

   void Clear()
   {
      variables.clear();
   }
};

class VarStringFrame : public VarFrame
{
   StringDump string_dump;
public:
   char* SaveString(const char* s, int len = -1, bool escape = false) {
      return string_dump.SaveString(s, len, escape);
   }

   void Clear()
   {
      //string_dump.Clear(); // do not destroy string (it is destroyed at destructor)
      VarFrame::Clear();
   }
};

class ConcurrentVarStringFrame : protected VarStringFrame
{
   // avoid write/read concurrency of global variables in runtime scripts in MT
   mutable std::mutex var_mutex;

public:
   // This method will not modify the *val argument if it returns false.
   bool Get(const char* name, AVSValue *val) const
   {
      std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
      return VarFrame::Get(name, val);
   }

   bool Set(const char* name, const AVSValue& val)
   {
      std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
      return VarFrame::Set(name, val);
   }

   char* SaveString(const char* s, int len = -1, bool escape = false) {
      std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
      return VarStringFrame::SaveString(s, len, escape);
   }

   void Clear()
   {
      std::lock_guard<std::mutex> lock(var_mutex); // avoid concurrency for global variables
      VarStringFrame::Clear();
   }
};

class VarTable
{
private:
   ConcurrentVarStringFrame* topFrame;

   std::vector<std::unique_ptr<VarFrame>> stackFrames;
   std::vector<std::unique_ptr<VarStringFrame>> globalFrames;

   std::vector<std::unique_ptr<VarFrame>> stackPool;
   std::vector<std::unique_ptr<VarStringFrame>> globalPool;

public:
   VarTable(ConcurrentVarStringFrame* topFrame) : topFrame(topFrame)
   {
      Push();
   }

   void Clear()
   {
      stackFrames.clear();
      globalFrames.clear();
      stackPool.clear();
      globalPool.clear();
   }

   void Push()
   {
      if (stackPool.size() > 0) {
         stackFrames.emplace_back(std::move(stackPool.back()));
         stackPool.pop_back();
      }
      else {
         stackFrames.emplace_back(new VarFrame());
      }
   }

   void Pop()
   {
      assert(stackFrames.size() > 0);
      stackFrames.back()->Clear();
      stackPool.emplace_back(std::move(stackFrames.back()));
      stackFrames.pop_back();
   }

   void PushGlobal()
   {
      Push();
      if (globalPool.size() > 0) {
         globalFrames.emplace_back(std::move(globalPool.back()));
         globalPool.pop_back();
      }
      else {
         globalFrames.emplace_back(new VarStringFrame());
      }
   }

   void PopGlobal()
   {
      Pop();
      assert(globalFrames.size() > 0);
      globalFrames.back()->Clear();
      globalPool.emplace_back(std::move(globalFrames.back()));
      globalFrames.pop_back();
   }

   bool Set(const char* name, const AVSValue& val)
   {
      return stackFrames.back()->Set(name, val);
   }

   bool SetGlobal(const char* name, const AVSValue& val)
   {
      if (globalFrames.size() > 0) {
         return globalFrames.back()->Set(name, val);
      }
      return topFrame->Set(name, val);
   }

   bool Get(const char* name, AVSValue *val) const
   {
      if (stackFrames.size() > 0 && stackFrames.back()->Get(name, val)) {
         return true;
      }
      for (auto it = globalFrames.rbegin(); it != globalFrames.rend(); ++it) {
         if ((**it).Get(name, val)) {
            return true;
         }
      }
      return topFrame->Get(name, val);
   }

   char* SaveString(const char* s, int len = -1, bool escape = false)
   {
      if (globalFrames.size() > 0) {
         return globalFrames.back()->SaveString(s, len, escape);
      }
      return topFrame->SaveString(s, len, escape);
   }
};

#endif // AVSCORE_VARTABLE_H