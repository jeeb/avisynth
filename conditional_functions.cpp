
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
  {  "RGBDifference","cc", ComparePlane::Create_rgb },
  {  "LumaDifference","cc", ComparePlane::Create_y },
  {  "ChromaUDifference","cc", ComparePlane::Create_u },
  {  "ChromaVDifference","cc", ComparePlane::Create_v },
  {  "YDifferenceFromPrevious","c", ComparePlane::Create_prev_y },
  {  "UDifferenceFromPrevious","c", ComparePlane::Create_prev_u },
  {  "VDifferenceFromPrevious","c", ComparePlane::Create_prev_v },
  {  "RGBDifferenceFromPrevious","c", ComparePlane::Create_prev_rgb },
  {  "YDifferenceToNext","c", ComparePlane::Create_prev_y },
  {  "UDifferenceToNext","c", ComparePlane::Create_prev_u },
  {  "VDifferenceToNext","c", ComparePlane::Create_prev_v },
  {  "RGBDifferenceToNext","c", ComparePlane::Create_prev_rgb },
//  {  "" },
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
    if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE))
      env->ThrowError("Average Plane: Requires Integer SSE capable CPU.");

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
		int w = src->GetRowSize(plane);
		int pitch = src->GetPitch(plane);
		w=(w/16)*16;

		int b = isse_average_plane(srcp, h, w, pitch);

		if (!b) b=1;
		float f = (float)b / (float)(h * w);

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




AVSValue __cdecl ComparePlane::Create_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlane(args[0],args[1],user_data, PLANAR_Y, env);
}


AVSValue __cdecl ComparePlane::Create_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlane(args[0],args[1],user_data, PLANAR_U, env);
}


AVSValue __cdecl ComparePlane::Create_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlane(args[0],args[1],user_data, PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlane(args[0],args[1],user_data, -1 , env);
}


AVSValue __cdecl ComparePlane::Create_prev_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, -1, PLANAR_Y, env);
}

AVSValue __cdecl ComparePlane::Create_prev_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, -1, PLANAR_U, env);
}

AVSValue __cdecl ComparePlane::Create_prev_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, -1, PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_prev_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, -1, -1, env);
}


AVSValue __cdecl ComparePlane::Create_next_y(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, 1, PLANAR_Y, env);
}

AVSValue __cdecl ComparePlane::Create_next_u(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, 1, PLANAR_U, env);
}

AVSValue __cdecl ComparePlane::Create_next_v(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, 1, PLANAR_V, env);
}

AVSValue __cdecl ComparePlane::Create_next_rgb(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return CmpPlaneSame(args[0],user_data, 1, -1, env);
}


AVSValue ComparePlane::CmpPlane(AVSValue clip, AVSValue clip2, void* user_data, int plane, IScriptEnvironment* env) {
		if (!clip.IsClip())
			env->ThrowError("Plane Difference: No clip supplied!");
		if (!clip2.IsClip())
			env->ThrowError("Plane Difference: Second parameter is not a clip!");
    if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE))
      env->ThrowError("Plane Difference: Requires Integer SSE capable CPU.");

		PClip child = clip.AsClip();
		VideoInfo vi = child->GetVideoInfo();
		PClip child2 = clip2.AsClip();
		VideoInfo vi2 = child2->GetVideoInfo();
    if (plane !=-1 ) {
		  if (!vi.IsPlanar())
			  env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
		  if (!vi2.IsPlanar())
			  env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
    } else {
		  if (!vi.IsRGB())
			  env->ThrowError("RGB Difference: RGB difference can only be tested on RGB images! (clip 1)");
		  if (!vi2.IsRGB())
			  env->ThrowError("RGB Difference: RGB difference can only be tested on RGB images! (clip 2)");
      plane = 0;
    }

		if (vi.height!=vi2.height || vi.width != vi2.width) 
			env->ThrowError("Plane Difference: Images are not the same size!");
			

		AVSValue cn = env->GetVar("current_frame");
		if (!cn.IsInt())
			env->ThrowError("Compare Plane: This filter can only be used within ConditionalFilter");

		int n = cn.AsInt();
		n = min(max(n,0),vi.num_frames);

		PVideoFrame src = child->GetFrame(n,env);
		PVideoFrame src2 = child2->GetFrame(n,env);

		const BYTE* srcp = src->GetReadPtr(plane);
		const BYTE* srcp2 = src2->GetReadPtr(plane);
		int h = src->GetHeight(plane);
		int w = src->GetRowSize(plane);
		int pitch = src->GetPitch(plane);
		int pitch2 = src2->GetPitch(plane);
		w=(w/16)*16;

		int b;
    if (vi.IsRGB32()) {
      b = isse_scenechange_rgb_16(srcp, srcp2, h, w, pitch, pitch2);
    } else {
      b = isse_scenechange_16(srcp, srcp2, h, w, pitch, pitch2);
    }

		if (!b) b=1;
		float f = (float)b / (float)(h * w);
    if (vi.IsRGB32()) 
      f = f * 4.0 / 3.0;

		return (AVSValue)f;
}

AVSValue ComparePlane::CmpPlaneSame(AVSValue clip, void* user_data, int offset, int plane, IScriptEnvironment* env) {
		if (!clip.IsClip())
			env->ThrowError("Plane Difference: No clip supplied!");
    if (!(env->GetCPUFlags() & CPUF_INTEGER_SSE))
      env->ThrowError("Plane Difference: Requires Integer SSE capable CPU.");

		PClip child = clip.AsClip();
		VideoInfo vi = child->GetVideoInfo();
    if (plane ==-1 ) {
		  if (!vi.IsRGB())
			  env->ThrowError("RGB Difference: RGB difference can only be calculated on RGB images");
      plane = 0;
    } else {
		  if (!vi.IsPlanar())
			  env->ThrowError("Plane Difference: Only planar images (as YV12) supported!");
    }

		AVSValue cn = env->GetVar("current_frame");
		if (!cn.IsInt())
			env->ThrowError("Compare Plane: This filter can only be used within ConditionalFilter");

		int n = cn.AsInt();
		n = min(max(n,0),vi.num_frames);
		int n2 = min(max(n+offset,1),vi.num_frames);

		PVideoFrame src = child->GetFrame(n,env);
		PVideoFrame src2 = child->GetFrame(n2,env);

		const BYTE* srcp = src->GetReadPtr(plane);
		const BYTE* srcp2 = src2->GetReadPtr(plane);
		int h = src->GetHeight(plane);
		int w = src->GetRowSize(plane);
		int pitch = src->GetPitch(plane);
		int pitch2 = src2->GetPitch(plane);
		w=(w/16)*16;

		int b;
    if (vi.IsRGB32()) {
      b = isse_scenechange_rgb_16(srcp, srcp2, h, w, pitch, pitch2);
    } else {
      b = isse_scenechange_16(srcp, srcp2, h, w, pitch, pitch2);
    }

		if (!b) b=1;
		float f = (float)b / (float)(h * w);

    if (vi.IsRGB32()) 
      f = f * 4.0 / 3.0;

		return (AVSValue)f;
}





/*********************
 * YV12 Scenechange detection.
 * 
 * (c) 2003, Klaus Post
 *
 * ISSE, MOD 16 version.
 *
 * Returns an int of the accumulated absolute difference
 * between two planes. 
 *
 * The absolute difference between two planes are returned as an int.
 * This version is optimized for mod16 widths. Others widths are allowed, 
 *  but the remaining pixels are simply skipped.
 *********************/


int ComparePlane::isse_scenechange_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
  int wp=width;
  int hp=height;
  int returnvalue=0xbadbad00;
  __asm {
    xor ebx,ebx     // Height
    pxor mm5,mm5  // Maximum difference
    mov edx, c_pitch    //copy pitch
    mov ecx, t_pitch    //copy pitch
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    mov edi, tplane
    jmp yloopover
    align 16
yloop:
    inc ebx
    add edi,ecx     // add pitch to both planes
    add esi,edx
yloopover:
    cmp ebx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[wp]    
    jge yloop

    movq mm0,[esi+eax]
     movq mm2,[esi+eax+8]
    movq mm1,[edi+eax]
     movq mm3,[edi+eax+8]
    psadbw mm0,mm1    // Sum of absolute difference
     psadbw mm2,mm3
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



/*********************
 * RGB32 Scenechange detection.
 * 
 * (c) 2003, Klaus Post
 *
 * ISSE, MOD 16 version.
 *
 * Returns an int of the accumulated absolute difference
 * between two planes. 
 *
 * The absolute difference between two planes are returned as an int.
 * This version is optimized for mod16 widths. Others widths are allowed, 
 *  but the remaining pixels are simply skipped.
 *********************/


int ComparePlane::isse_scenechange_rgb_16(const BYTE* c_plane, const BYTE* tplane, int height, int width, int c_pitch, int t_pitch) {
   __declspec(align(8)) static const __int64 Mask1 =  0x00ffffff00ffffff;
  int wp=width;
  int hp=height;
  int returnvalue=0xbadbad00;
  __asm {
    xor ebx,ebx     // Height
    movq mm5,[Mask1]  // Mask for RGB32
    mov edx, c_pitch    //copy pitch
    mov ecx, t_pitch    //copy pitch
    pxor mm6,mm6   // We maintain two sums, for better pairablility
    pxor mm7,mm7
    mov esi, c_plane
    mov edi, tplane
    jmp yloopover
    align 16
yloop:
    inc ebx
    add edi,ecx     // add pitch to both planes
    add esi,edx
yloopover:
    cmp ebx,[hp]
    jge endframe
    xor eax,eax     // Width
    align 16
xloop:
    cmp eax,[wp]    
    jge yloop

    movq mm0,[esi+eax]
     movq mm2,[esi+eax+8]
    pand mm0,mm5
     pand mm2,mm5
    movq mm1,[edi+eax]
     movq mm3,[edi+eax+8]
    pand mm1,mm5
     pand mm3,mm5
    psadbw mm0,mm1    // Sum of absolute difference
     psadbw mm2,mm3
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

