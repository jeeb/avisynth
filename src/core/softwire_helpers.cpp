// Avisynth v2.5.  Copyright 2002 Ben Rudiak-Gould et al.
// http://www.avisynth.org

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

#include "stdafx.h"

#include "../internal.h"
#include "softwire_helpers.h"


DynamicAssembledCode::DynamicAssembledCode(Assembler &x86, IScriptEnvironment* env, const char * err_msg) {
  entry = 0;
  const char* soft_err = "";
  try {
    entry = (void(*)())x86.callable();
  } catch (Error _err) { soft_err = _err.getString(); }
  if(!entry)
  {
    _RPT0(0,"SoftWire Compilation error:");
    _RPT0(0,soft_err);
    _RPT0(0,"\n");
    env->ThrowError(err_msg);
  }
  ret = (BYTE*)x86.acquire();
#ifdef _DEBUG
  int bytes=0;
  while (ret[bytes]!=0xCC) { bytes++; };
  _RPT1(0,"Dynamic code compiled into %i bytes.\n",bytes);
#endif
}

void DynamicAssembledCode::Call() const {
  if (ret) entry();
}
  
void DynamicAssembledCode::Free() {
  if (ret) free(ret);
}



