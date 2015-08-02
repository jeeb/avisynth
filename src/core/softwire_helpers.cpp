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


static void ReportException(EXCEPTION_POINTERS* ei, BYTE* ret) {
  enum { buffsize=127 };
  static char buff[buffsize+1];

  const DWORD code  = ei->ExceptionRecord->ExceptionCode;
  const BYTE* addr  = (BYTE*)ei->ExceptionRecord->ExceptionAddress;
  const UINT  info0 = ei->ExceptionRecord->ExceptionInformation[0];
  const UINT  info1 = ei->ExceptionRecord->ExceptionInformation[1];
  const UINT  offst = (addr >= ret) ? addr - ret : 0xffffffff - (ret - addr) + 1;
  
  switch (code) {
  case EXCEPTION_ACCESS_VIOLATION:
    _snprintf(buff, buffsize, "Softwire: caught an access violation at 0x%08x(code+%u),\n"
                              "attempting to %s 0x%08x", 
                              addr, offst, info0 ? "write to" : "read from", info1);
    break;

  case EXCEPTION_ILLEGAL_INSTRUCTION:
    _snprintf(buff, buffsize, "Softwire: illegal instruction at 0x%08x(code+%u),\n"
                              "code bytes : %02x %02x %02x %02x %02x %02x %02x %02x ...",
                              addr, offst, addr[0], addr[1], addr[2], addr[3],
                                           addr[4], addr[5], addr[6], addr[7]); 
    break;

  default:
    _snprintf(buff, buffsize, "Softwire: exception 0x%08x at 0x%08x(code+%u)", code, addr, offst);
  }

  buff[buffsize] = '\0';
  throw AvisynthError(buff);

}


DynamicAssembledCode::DynamicAssembledCode(Assembler &x86, IScriptEnvironment* env, const char * err_msg) {
  entry = 0;
  const char* soft_err = "";

  try {
    entry = (void(__cdecl *)())x86.callable();
  } catch (Error _err) { soft_err = _err.getString(); }

  if(!entry)
  {
    _RPT1(0,"SoftWire Compilation error: %s\n", soft_err);
    env->ThrowError(err_msg);
  }

  ret = (BYTE*)x86.acquire();

#ifdef _DEBUG
//  int bytes=0;
//  while (ret[bytes]!=0xCC) { bytes++; };
//  _RPT1(0,"Dynamic code compiled into %i bytes.\n",bytes);
  
#endif
}

// No Args Call
void DynamicAssembledCode::Call() const {
  EXCEPTION_POINTERS* ei = 0;

  if (ret) {
    __try {
      entry();
    }
    __except ( ei = GetExceptionInformation(),
               (GetExceptionCode() >> 28) == 0xC )
               //  0=EXCEPTION_CONTINUE_SEARCH
               //  1=EXCEPTION_EXECUTE_HANDLER
               // -1=EXCEPTION_CONTINUE_EXECUTION
    {
      ReportException(ei, ret);
    }
  }
}
  

  /*
   * On entry here [esp+4] will contain arg1,
   * [esp+8] will contain arg2, [esp+12] ...
   *
   * When calling the assembler the code will be like
   *   lea    eax, ebp[arg1]
   *   push   eax
   *   call   ecx[entry]
   *   add    esp, 4
   *   ...    ..., eax   // eax has result
   *
   * Typically do something like this to get all the args
   *   x86.push(  ebp);
   *   x86.mov(   ebp, dword_ptr[esp+4 + 4]);
   *
   *   x86.mov(   eax, dword_ptr[ebp +  0]);    // arg1
   *   x86.mov(   ebx, dword_ptr[ebp +  4]);    // arg2
   *   x86.mov(   ecx, dword_ptr[ebp +  8]);    // arg3
   *   x86.mov(   edx, dword_ptr[ebp + 12]);    // arg4
   * ...
   *   x86.mov(   eax, ...                      // result
   *   x86.pop(   ebp);
   *   x86.ret();
   */

// Call with Args, optionally returning an int
int DynamicAssembledCode::Call(const void* arg1, ...) const {
  EXCEPTION_POINTERS* ei = 0;

  if (ret) {
    __try { 
      return ((int (__cdecl*)(const void* *))entry)(&arg1);
    }
    __except ( ei = GetExceptionInformation(),
               (GetExceptionCode() >> 28) == 0xC )
             //  0=EXCEPTION_CONTINUE_SEARCH
             //  1=EXCEPTION_EXECUTE_HANDLER
             // -1=EXCEPTION_CONTINUE_EXECUTION
    {
      ReportException(ei, ret);
    }
  }
  return 0;
}
  
void DynamicAssembledCode::Free() {
  if (ret) free(ret);
}


#if 0
class TestAsm : public  CodeGenerator {

public:
  static AVSValue __cdecl Create_TestAsm(AVSValue args, void*, IScriptEnvironment* env) {
    int i, j, k;
    DynamicAssembledCode code;
    Assembler x86;

    x86.push(  ebp);
    x86.mov(   ebp, dword_ptr[esp+4+4]);
    x86.mov(   eax, dword_ptr[ebp+0]);     // arg1
    x86.add(   eax, dword_ptr[ebp+4]);     // arg2
    x86.pop(   ebp);
    x86.ret();
    
    code = DynamicAssembledCode(x86, env, "TestAsm: Code could not be compiled.");

    j = args[0].AsInt();
    k = args[1].AsInt();

    i = code.Call(j, k);

    code.Free();

    return i;
  }
};
#endif
