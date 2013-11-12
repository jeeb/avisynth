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

/*
** Turn. version 0.1
** (c) 2003 - Ernst PechÚ
**
*/

#include "turn.h"
#include "resample.h"
#include "planeswap.h"
#include "../core/internal.h"


extern const AVSFunction Turn_filters[] = {
  { "TurnLeft","c",Turn::Create_TurnLeft },
  { "TurnRight","c",Turn::Create_TurnRight },
  { "Turn180","c",Turn::Create_Turn180 },
  { 0 }
};

enum TurnDirection
{
  DIRECTION_LEFT = 0,
  DIRECTION_RIGHT = 1,
  DIRECTION_180 = 2
};

static void turn_left_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  int dstp_offset;
  for (int y = 0; y<height; y++) {
    dstp_offset = (height-1-y)*3;
    for (int x=0; x<width; x+=3) {	
      dstp[dstp_offset+0] = srcp[x+0];
      dstp[dstp_offset+1] = srcp[x+1];
      dstp[dstp_offset+2] = srcp[x+2];
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch;
  }
}

static void turn_right_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  int dstp_offset;
  int dstp_base = (width/3-1) * dst_pitch;
  for (int y=0; y<height; y++) {
    dstp_offset = dstp_base + y*3;
    for (int x = 0; x<width; x+=3) {	
      dstp[dstp_offset+0] = srcp[x+0];
      dstp[dstp_offset+1] = srcp[x+1];
      dstp[dstp_offset+2] = srcp[x+2];
      dstp_offset -= dst_pitch;
    }
    srcp += src_pitch;
  }
}

static void turn_180_rgb24(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  dstp += (height-1)*dst_pitch + (width-3);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x+=3) {	
      dstp[-x+0] = srcp[x+0];
      dstp[-x+1] = srcp[x+1];
      dstp[-x+2] = srcp[x+2];
    }
    dstp -= dst_pitch;
    srcp += src_pitch;
  }
}

static void turn_left_rgb32(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  int dstp_offset;
  for (int y=0; y<height; y++) {
    dstp_offset = (height-1-y);
    for (int x=0; x<l_rowsize; x++) {	
      l_dstp[dstp_offset] = l_srcp[x];
      dstp_offset += l_dst_pitch;
    }
    l_srcp += l_src_pitch;
  }
}

static void turn_right_rgb32(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  int dstp_offset;
  int dstp_base = (l_rowsize-1) * l_dst_pitch;
  for (int y = 0; y<height; y++) {
    dstp_offset = dstp_base + y;
    for (int x = 0; x<l_rowsize; x++) {	
      l_dstp[dstp_offset] = l_srcp[x];
      dstp_offset -= l_dst_pitch;
    }
    l_srcp += l_src_pitch;
  }
}

static void turn_180_rgb32(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  const unsigned int *l_srcp = reinterpret_cast<const unsigned int *>(srcp);
  unsigned int *l_dstp = reinterpret_cast<unsigned int *>(dstp);
  int l_rowsize = width/4;
  int l_src_pitch = src_pitch/4;
  int l_dst_pitch = dst_pitch/4;

  l_dstp += (height-1)*l_dst_pitch + (l_rowsize-1);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<l_rowsize; x++) {	
      l_dstp[-x] = l_srcp[x];
    }
    l_dstp -= l_dst_pitch;
    l_srcp += l_src_pitch;
  }
}

static void turn_right_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  BYTE u,v;
  int dstp_offset;

  for (int y=0; y<height; y+=2)
  {
    dstp_offset = ((height-2-y)<<1);
    for (int x=0; x<width; x+=4)
    {
      u = (srcp[x+1] + srcp[x+1+src_pitch] + 1) >> 1;
      v = (srcp[x+3] + srcp[x+3+src_pitch] + 1) >> 1;
      dstp[dstp_offset+0] = srcp[x+src_pitch];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[x];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
      dstp[dstp_offset+0] = srcp[x+src_pitch+2];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[x+2];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch<<1;
  }
}

static void turn_left_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  BYTE u,v;
  int dstp_offset;
 
  srcp += width-4;
  for (int y=0; y<height; y+=2)
  {
    dstp_offset = (y<<1);
    for (int x=0; x<width; x+=4)
    {
      u = (srcp[-x+1] + srcp[-x+1+src_pitch] + 1) >> 1;
      v = (srcp[-x+3] + srcp[-x+3+src_pitch] + 1) >> 1;
      dstp[dstp_offset+0] = srcp[-x+2];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[-x+2+src_pitch];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
      dstp[dstp_offset+0] = srcp[-x];
      dstp[dstp_offset+1] = u;
      dstp[dstp_offset+2] = srcp[-x+src_pitch];
      dstp[dstp_offset+3] = v;
      dstp_offset += dst_pitch;
    }
    srcp += src_pitch<<1;
  }
}

static void turn_180_yuy2(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch)
{
  dstp += (height-1)*dst_pitch + (width-4);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x+=4) {	
      dstp[-x+2] = srcp[x+0];
      dstp[-x+1] = srcp[x+1];
      dstp[-x+0] = srcp[x+2];
      dstp[-x+3] = srcp[x+3];
    }
    dstp -= dst_pitch;
    srcp += src_pitch;
  }
}

static void turn_right_plane(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  for(int y=0; y<height; y++)
  {
    int offset = height-1-y;
    for (int x=0; x<width; x++)
    {
      dstp[offset] = srcp[x];
      offset += dst_pitch;
    }
    srcp += src_pitch;
  }
}

static void turn_left_plane(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  srcp += width-1;
  for(int y=0; y<height; y++)
  {
    int offset = y;
    for (int x=0; x<width; x++)
    {
      dstp[offset] = srcp[-x];
      offset += dst_pitch;
    }
    srcp += src_pitch;
  }
}

static void turn_180_plane(const BYTE *srcp, BYTE *dstp, int width, int height, int src_pitch, int dst_pitch) {
  dstp += (height-1)*dst_pitch + (width-1);
  for (int y = 0; y<height; y++) {
    for (int x = 0; x<width; x++) {	
      dstp[-x] = srcp[x];
    }
    dstp -= src_pitch;
    srcp += dst_pitch;
  }
}


Turn::Turn( PClip _child, int _direction, IScriptEnvironment* env ) : GenericVideoFilter(_child), Usource(0), Vsource(0)
{
  if (_direction == DIRECTION_LEFT || _direction == DIRECTION_RIGHT) {
    const int src_height = vi.height;
    vi.height = vi.width;
    vi.width = src_height;
  }
  direction = _direction;

  if (vi.IsRGB())
  {
    if (vi.BitsPerPixel() == 32) { 
      TurnFuncPtr functions[3] = {turn_left_rgb32, turn_right_rgb32, turn_180_rgb32};
      TurnFunc = functions[direction]; 
    }
    else if (vi.BitsPerPixel() == 24) {
      TurnFuncPtr functions[3] = {turn_left_rgb24, turn_right_rgb24, turn_180_rgb24};
      TurnFunc = functions[direction]; 
    }
    else env->ThrowError("Turn: Unsupported RGB bit depth");
  }
  else if (vi.IsYUY2())
  {
    if (vi.width%2) {
      env->ThrowError("Turn: YUY2 data must have MOD2 height");
    }
    TurnFuncPtr functions[3] = {turn_left_yuy2, turn_right_yuy2, turn_180_yuy2};
    TurnFunc = functions[direction]; 
  }
  else if (vi.IsPlanar())
  {
    TurnFuncPtr functions[3] = {turn_left_plane, turn_right_plane, turn_180_plane};
    TurnFunc = functions[direction]; 
    // rectangular formats?
    if ((_direction == DIRECTION_LEFT || _direction == DIRECTION_RIGHT) && !vi.IsY8() && 
      (vi.GetPlaneWidthSubsampling(PLANAR_U) != vi.GetPlaneHeightSubsampling(PLANAR_U)))
    {
      if (vi.width % (1<<vi.GetPlaneWidthSubsampling(PLANAR_U))) // YV16 & YV411
        env->ThrowError("Turn: Planar data must have MOD %d height",
        1<<vi.GetPlaneWidthSubsampling(PLANAR_U));

      if (vi.height % (1<<vi.GetPlaneHeightSubsampling(PLANAR_U))) // No current formats
        env->ThrowError("Turn: Planar data must have MOD %d width",
        1<<vi.GetPlaneHeightSubsampling(PLANAR_U));

      MitchellNetravaliFilter filter(1./3., 1./3.);
      AVSValue subs[4] = { 0.0, 0.0, 0.0, 0.0 }; 

      Usource = new SwapUVToY(child, SwapUVToY::UToY8, env);  
      Vsource = new SwapUVToY(child, SwapUVToY::VToY8, env);

      const VideoInfo vi_u = Usource->GetVideoInfo();

      const int uv_height = (vi_u.height << vi.GetPlaneHeightSubsampling(PLANAR_U)) >> vi.GetPlaneWidthSubsampling(PLANAR_U);
      const int uv_width  = (vi_u.width  << vi.GetPlaneWidthSubsampling(PLANAR_U))  >> vi.GetPlaneHeightSubsampling(PLANAR_U);

      Usource = FilteredResize::CreateResize(Usource, uv_width, uv_height, subs, &filter, env);
      Vsource = FilteredResize::CreateResize(Vsource, uv_width, uv_height, subs, &filter, env);
    }
  }
  else
  {
    env->ThrowError("Turn: Image format not supported!");
  }
}


PVideoFrame __stdcall Turn::GetFrame(int n, IScriptEnvironment* env)
{

	PVideoFrame src = child->GetFrame(n, env);

  PVideoFrame dst = env->NewVideoFrame(vi);

	if (Usource && Vsource) 
  {
		PVideoFrame usrc = Usource->GetFrame(n, env);
		PVideoFrame vsrc = Vsource->GetFrame(n, env);

    TurnFunc(src->GetReadPtr(PLANAR_Y),  dst->GetWritePtr(PLANAR_Y), src->GetRowSize(PLANAR_Y),  src->GetHeight(PLANAR_Y),  src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y));
    TurnFunc(usrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_U), usrc->GetRowSize(PLANAR_Y), usrc->GetHeight(PLANAR_Y), usrc->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_U));
    TurnFunc(vsrc->GetReadPtr(PLANAR_Y), dst->GetWritePtr(PLANAR_V), usrc->GetRowSize(PLANAR_Y), usrc->GetHeight(PLANAR_Y), vsrc->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_V));
	}
  else if (vi.IsPlanar()) 
  {
    TurnFunc(src->GetReadPtr(PLANAR_Y),  dst->GetWritePtr(PLANAR_Y), src->GetRowSize(PLANAR_Y),  src->GetHeight(PLANAR_Y),  src->GetPitch(PLANAR_Y), dst->GetPitch(PLANAR_Y));
    TurnFunc(src->GetReadPtr(PLANAR_U),  dst->GetWritePtr(PLANAR_U), src->GetRowSize(PLANAR_U),  src->GetHeight(PLANAR_U),  src->GetPitch(PLANAR_U), dst->GetPitch(PLANAR_U));
    TurnFunc(src->GetReadPtr(PLANAR_V),  dst->GetWritePtr(PLANAR_V), src->GetRowSize(PLANAR_V),  src->GetHeight(PLANAR_V),  src->GetPitch(PLANAR_V), dst->GetPitch(PLANAR_V));
  } 
  else 
  {
		TurnFunc(src->GetReadPtr(),dst->GetWritePtr(),src->GetRowSize(),
				 src->GetHeight(),src->GetPitch(),dst->GetPitch());
  }
  return dst;
}


AVSValue __cdecl Turn::Create_TurnLeft(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_LEFT, env);
}

AVSValue __cdecl Turn::Create_TurnRight(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_RIGHT, env);
}

AVSValue __cdecl Turn::Create_Turn180(AVSValue args, void* user_data, IScriptEnvironment* env) {
	return new Turn(args[0].AsClip(), DIRECTION_180, env);
}


