
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

#include "conditional_functions.h"

AVSFunction Conditional_funtions_filters[] = {
  {  "AverageLuma","c", AveragePlane::Create_y },
  {  "AverageChromaU","c", AveragePlane::Create_u },
  {  "AverageChromaV","c", AveragePlane::Create_v },
  { 0 }
};


AVSValue __cdecl AveragePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return AvgPlane(args[0],user_data, PLANAR_Y, env);
}


AVSValue __cdecl AveragePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return AvgPlane(args[0],user_data, PLANAR_U, env);
}


AVSValue __cdecl AveragePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return AvgPlane(args[0],user_data, PLANAR_V, env);
}

AVSValue AveragePlane::AvgPlane(AVSValue clip, void* user_data, int plane, IScriptEnvironment* env) {
		if (!clip.IsClip())
			env->ThrowError("Average Plane: No clip supplied!");

		PClip child = clip.AsClip();
		VideoInfo vi = child->GetVideoInfo();

		if (!vi.IsPlanar())
			env->ThrowError("Average Plane: Only planar images (as YV12) supported!");

		AVSValue cn = env->GetVar("current_frame");
		if (!cn.IsInt())
			env->ThrowError("Average Plane: This filter can only be used within ConditionalFilter");

		int n = cn.AsInt();

		PVideoFrame src = child->GetFrame(n,env);

		const BYTE* srcp = src->GetReadPtr(plane);
		int h = src->GetHeight(plane);
		int w = src->GetRowSize(plane|PLANAR_ALIGNED);
		int pitch = src->GetPitch(plane);
		w=(w/16)*16;

		int b = isse_average_plane(srcp, h, w, pitch);

		if (!b) b=1;
		float f = (float)b / (float)(h * src->GetRowSize(plane));

		return (AVSValue)f;
}

// Average plane

int AveragePlane::isse_average_plane(const BYTE* c_plane, int height, int width, int c_pitch) {
  int hp=height;
  int returnvalue=0xbadbad00;
  __asm {
    xor ebx,ebx     // Height
    mov edx, c_pitch    //copy pitch
    pxor mm5,mm5  // Cleared
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    jmp yloopover
    align 16
yloop:
    inc ebx
    add esi,edx  // add pitch
yloopover:
    cmp ebx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[width]    
    jge yloop

    movq mm0,[esi+eax]
    movq mm2,[esi+eax+8]
    psadbw mm0,mm5    // Sum of absolute difference (= sum of all pixels)
     psadbw mm2,mm5
    paddd mm6,mm0     // Add...
     paddd mm7,mm2

    add eax,16
    jmp xloop
endframe:
    paddd mm7,mm6
    movd returnvalue,mm7
    emms
  }
  return returnvalue;
}

