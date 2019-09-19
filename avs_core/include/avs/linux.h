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

#ifdef AVS_LINUX
#ifndef AVSCORE_LINUX_H
#define AVSCORE_LINUX_H

#include <cstring>
#include <strings.h>
#include <unistd.h>

// Define these MSVC-extension used in Avisynth
#define __single_inheritance

// These things don't exist in Linux
#define __declspec(x)
#define lstrcmpi strcasecmp
#define _strnicmp strncasecmp
#define _strdup strdup

// Borrowing some compatibility macros from AvxSynth, slightly modified
#define UInt32x32To64(a, b) ((uint64_t)(((uint64_t)((uint32_t)(a))) * ((uint32_t)(b))))
#define Int32x32To64(a, b)  ((int64_t)(((int64_t)((long)(a))) * ((long)(b))))

#define InterlockedIncrement(x) __sync_fetch_and_add((x), 1)
#define InterlockedDecrement(x) __sync_fetch_and_sub((x), 1)
#define MulDiv(nNumber, nNumerator, nDenominator)   (int32_t) (((int64_t) (nNumber) * (int64_t) (nNumerator) + (int64_t) ((nDenominator)/2)) / (int64_t) (nDenominator))

#define S_FALSE       (0x00000001)
#define E_FAIL        (0x80004005)
#define FAILED(hr)    ((hr) & 0x80000000)
#define SUCCEEDED(hr) (!FAILED(hr))

// Calling convension
#define __stdcall
#define __cdecl

#endif // AVSCORE_LINUX_H
#endif // AVS_LINUX
