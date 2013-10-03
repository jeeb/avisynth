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

#include "strings.h"
#include <cassert>
#include <string>
#include <algorithm>

static inline char tolower(char c)
{
  // Works for letters of the english alphabet in ASCII
  return ((c >=65) && (c <=90)) ? c + 32 : c;
}

bool streqi(const char* s1, const char* s2)
{
  // Why we dont use Windows's lstrcmpi? It is by multiple orders of magnitude slower and non-portable.
  // lstrcmpi handles locales and UTF and whatnot, but because variable and function names in Avisynth
  // are limited to ASCII, we don't need that funcationality.

  while(1)
  {
    if ((*s1 == 0) && (*s2 == 0))
      return true;

    if (tolower(*s1) != tolower(*s2))
      return false;

    ++s1;
    ++s2;
  }

  assert(0);
  return false;
}

std::string concat(const std::string &s1, const std::string &s2)
{
  std::string ret(s1);
  return ret.append(s2);
}

bool replace(std::string &haystack, const std::string &needle, const std::string &newStr)
{
  bool replaced = false;
  for(size_t pos = 0; ; pos += newStr.length())
  {
    // Locate the substring to replace
    pos = haystack.find(needle, pos);
    if(pos == std::string::npos) break;
    // Replace by erasing and inserting
    haystack.erase(pos, needle.length());
    haystack.insert(pos, newStr);
    replaced = true;
  }

  return replaced;
}

bool replace_beginning(std::string &haystack, const std::string &needle, const std::string &newStr)
{
  bool replaced = false;

  // Locate the substring to replace
  size_t pos = haystack.find(needle);
  if(pos == std::string::npos) return false;
  if(pos != 0) return false;

  // Replace by erasing and inserting
  haystack.erase( pos, needle.length() );
  haystack.insert( pos, newStr );

  return true;
}

bool replace(std::string &haystack, char needle, char newChar)
{
  std::string haystack_bck = haystack;
  std::replace(haystack.begin(),haystack.end(), needle, newChar);
  return haystack.compare(haystack_bck) != 0;
}
