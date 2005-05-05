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

#include "fps.h"





/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Fps_filters[] = {
  { "AssumeFPS", "ci[]i[sync_audio]b", AssumeFPS::Create },      // dst framerate, sync audio?
  { "AssumeFPS", "cf[sync_audio]b", AssumeFPS::CreateFloat },    // dst framerate, sync audio?
  { "AssumeFPS", "cc[sync_audio]b", AssumeFPS::CreateFromClip }, // clip with dst framerate, sync audio?
  { "ChangeFPS", "ci[]i[linear]b", ChangeFPS::Create },     // dst framerate, fetch all frames
  { "ChangeFPS", "cf[linear]b", ChangeFPS::CreateFloat },   // dst framerate, fetch all frames
  { "ChangeFPS", "cc[linear]b", ChangeFPS::CreateFromClip },// clip with dst framerate, fetch all frames
  { "ConvertFPS", "ci[]i[zone]i[vbi]i", ConvertFPS::Create },      // dst framerate, zone lines, vbi lines
  { "ConvertFPS", "cf[zone]i[vbi]i", ConvertFPS::CreateFloat },    // dst framerate, zone lines, vbi lines
  { "ConvertFPS", "cc[zone]i[vbi]i", ConvertFPS::CreateFromClip }, // clip with dst framerate, zone lines, vbi lines
  { 0 }
};






/************************************
 *******   AssumeFPS Filters   ******
 ************************************/

AssumeFPS::AssumeFPS(PClip _child, int numerator, int denominator, bool sync_audio, IScriptEnvironment* env)
 : GenericVideoFilter(_child)
{
  if (denominator == 0)
    env->ThrowError("AssumeFPS: Denominator cannot be 0 (zero).");

  if (sync_audio) 
  {
    __int64 a = __int64(vi.fps_numerator) * denominator;
    __int64 b = __int64(vi.fps_denominator) * numerator;
    vi.audio_samples_per_second = int((vi.audio_samples_per_second * b + (a>>1)) / a);
  }
  vi.SetFPS(numerator, denominator);
}


AVSValue __cdecl AssumeFPS::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new AssumeFPS( args[0].AsClip(), args[1].AsInt(), 
                        args[2].AsInt(1), args[3].AsBool(false), env );
}


AVSValue __cdecl AssumeFPS::CreateFloat(AVSValue args, void*, IScriptEnvironment* env)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  double n = args[1].AsFloat();
  int d = 1;
  while (n < 16777216 && d < 16777216) { n*=2; d*=2; } // 2^24, floats precision
  return new AssumeFPS(args[0].AsClip(), int(n+0.5), d, args[2].AsBool(false), env);
	}
	catch (...) { throw; }
}

AVSValue __cdecl AssumeFPS::CreateFromClip(AVSValue args, void*, IScriptEnvironment* env)
{
  const VideoInfo& vi = args[1].AsClip()->GetVideoInfo();

  if (!vi.HasVideo()) {
    env->ThrowError("AssumeFPS: The clip supplied to get the FPS from must contain video.");
  }

  return new AssumeFPS( args[0].AsClip(), vi.fps_numerator,
                        vi.fps_denominator, args[2].AsBool(false), env );
}





/************************************
 *******   ChangeFPS Filters   ******
 ************************************/


ChangeFPS::ChangeFPS(PClip _child, int new_numerator, int new_denominator, bool _linear, IScriptEnvironment* env)
  : GenericVideoFilter(_child), linear(_linear)
{
  if (new_denominator == 0)
    env->ThrowError("ChangeFPS: Denominator cannot be 0 (zero).");

  a = __int64(vi.fps_numerator) * new_denominator;
  b = __int64(vi.fps_denominator) * new_numerator;
  vi.SetFPS(new_numerator, new_denominator);
  vi.num_frames = int((vi.num_frames * b + (a >> 1)) / a);
  lastframe = -1;
}


PVideoFrame __stdcall ChangeFPS::GetFrame(int n, IScriptEnvironment* env)
{
  int getframe = int(((__int64)n * a + (b>>1)) / b);  // Which frame to get next?

  if (linear) {
    if ((lastframe < (getframe-1)) && (getframe - lastframe < 10)) {  // Do not decode more than 10 frames
      while (lastframe < (getframe-1)) {
        lastframe++;
        PVideoFrame p = child->GetFrame(lastframe, env);  // If MSVC optimizes this I'll kill it ;)
      }
    }
  }

  lastframe = getframe;
  return child->GetFrame(getframe , env );
}


bool __stdcall ChangeFPS::GetParity(int n) 
{
  return child->GetParity( int((n * a + (b>>1)) / b) );
}


AVSValue __cdecl ChangeFPS::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ChangeFPS( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(1), args[3].AsBool(true), env);
}


AVSValue __cdecl ChangeFPS::CreateFloat(AVSValue args, void*, IScriptEnvironment* env) 
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  double n = args[1].AsFloat();
  int d = 1;
  while (n < 16777216 && d < 16777216) { n*=2; d*=2; } // 2^24, floats precision
  return new ChangeFPS(args[0].AsClip(), int(n+0.5), d, args[2].AsBool(true), env);
	}
	catch (...) { throw; }
}

AVSValue __cdecl ChangeFPS::CreateFromClip(AVSValue args, void*, IScriptEnvironment* env)
{
  const VideoInfo& vi = args[1].AsClip()->GetVideoInfo();

  if (!vi.HasVideo()) {
    env->ThrowError("ChangeFPS: The clip supplied to get the FPS from must contain video.");
  }

  return new ChangeFPS( args[0].AsClip(), vi.fps_numerator, vi.fps_denominator,
                        args[2].AsBool(true), env);
}







/*************************************
 *******   ConvertFPS Filters   ******
 *************************************/

ConvertFPS::ConvertFPS( PClip _child, int new_numerator, int new_denominator, int _zone, 
                        int _vbi, IScriptEnvironment* env )
	: GenericVideoFilter(_child), zone(_zone), vbi(_vbi), lps(0)
{
  if (!vi.IsYUY2())
   env->ThrowError("ConvertFPS: requires YUY2 input");
  fa = __int64(vi.fps_numerator) * new_denominator;
  fb = __int64(vi.fps_denominator) * new_numerator;
  if( zone >= 0 ) 
  {
    if( vbi < 0 ) vbi = 0;
		if( vbi > vi.height ) vbi = vi.height;
		lps = int( (vi.height + vbi) * fb / fa );
		if( zone > lps )
			env->ThrowError("ConvertFPS: 'zone' too large. Maximum allowed %d", lps);
	} 
  else if( 3*fb < (fa<<1) ) {
	  int dec = MulDiv(vi.fps_numerator, 20000, vi.fps_denominator);
      env->ThrowError("ConvertFPS: New frame rate too small. Must be greater than %d.%04d "
				"Increase or use 'zone='", dec/30000, (dec/3)%10000);
	}
	vi.SetFPS(new_numerator, new_denominator);
	vi.num_frames = int((vi.num_frames * fb + (fa>>1)) / fa);
}


PVideoFrame __stdcall ConvertFPS::GetFrame(int n, IScriptEnvironment* env)
{
	static const int resolution = 6; //bits. Must be >= 4, or modify next line
	static const int threshold  = 1<<(resolution-4);
	static const int one        = 1<<resolution;
	static const int half       = 1<<(resolution-1);

	int nsrc      = int( n * fa / fb );
	int frac      = int( (((n*fa) % fb) << resolution) / fb );
	int mix_ratio = one - min( int( (fb * (one - frac)) / fa ), one);

	// Don't bother if the blend ratio is small
	if( zone < 0 ) {
		if( mix_ratio <= threshold )
			return child->GetFrame(nsrc, env);
		if( (one - mix_ratio) <= threshold )
			return child->GetFrame(nsrc+1, env);
	}

	PVideoFrame a = child->GetFrame(nsrc, env);
	PVideoFrame b = child->GetFrame(nsrc+1, env);
	const BYTE*  b_data   = b->GetReadPtr();
	int          b_pitch  = b->GetPitch();
	const int    row_size = a->GetRowSize();
	const int    height   = a->GetHeight();

	if( zone < 0 ) {

   	// Mode 1: Blend full frames
		env->MakeWritable(&a);
		BYTE* a_data   = a->GetWritePtr();
		int   a_pitch  = a->GetPitch();
		for (int y = 0; y < height; y++) {
			for (int x = 0; x < row_size; x++)
				a_data[x] += ((b_data[x] - a_data[x]) * mix_ratio + half) >> resolution;
			a_data += a_pitch;
			b_data += b_pitch;
		}
		return a;

	} else {

	// Mode 2: Switch to next frame at the scan line corresponding to the source frame's timing.
	// If zone > 0, perform a gradual transition, i.e. blend one frame into the next
	// over the given number of lines.
	
		BYTE *pd;
		const BYTE *pa, *pb, *a_data = a->GetReadPtr();
		int   a_pitch = a->GetPitch();

		int switch_line = (lps * (one - frac)) >> resolution;
		int top = switch_line - (zone>>1);
		int bottom = switch_line + (zone>>1) - lps;
		if( bottom > 0 && nsrc > 0 ) {
		// Finish the transition from the previous frame
			switch_line -= lps;
			top -= lps;
			nsrc--;
			b = a;
			a = child->GetFrame( nsrc, env );
			b_pitch = a_pitch;
			b_data  = a_data;
			a_data  = a->GetReadPtr();
			a_pitch = a->GetPitch();
		} else if( top >= height )
			return a;

		// Result goes into a new buffer since it can be made up of a number of source frames
		PVideoFrame    d      = env->NewVideoFrame(vi);
		BYTE* data   = d->GetWritePtr();
		const int      pitch  = d->GetPitch();
		if( top > 0 )
			BitBlt( data, pitch, a_data, a_pitch, row_size, top );
loop:
		bottom = min( switch_line + (zone>>1), height );
		int safe_top = max(top,0);
		pd = data   + safe_top * pitch;
		pa = a_data + safe_top * a_pitch;
		pb = b_data + safe_top * b_pitch;
		for( int y = safe_top; y < bottom; y++ ) {
			int scale = y - top;
			for( int x = 0; x < row_size; x++ )
				pd[x] = pa[x] + ((pb[x] - pa[x]) * scale + (zone>>1)) / zone;
			pd += pitch;
			pa += a_pitch;
			pb += b_pitch;
		}
		switch_line += lps;
		top = switch_line - (zone>>1);
		int limit = min(height,top);
		if( bottom < limit ) {
			pd = data   + bottom * pitch;
			pb = b_data + bottom * b_pitch;
			BitBlt( pd, pitch, pb, b_pitch, row_size, limit-bottom );
		}
		if( top < height ) {
			nsrc++;
			a = b;
			b = child->GetFrame(nsrc+1, env);
			a_pitch = b_pitch;
			b_pitch = b->GetPitch();
			a_data  = b_data;
			b_data  = b->GetReadPtr();
			goto loop;
		}
		return d;
	}
}


bool __stdcall ConvertFPS::GetParity(int n) 
{
	if( vi.IsFieldBased())
		return child->GetParity(0) ^ (n&1);
	else
		return child->GetParity(0);
}

AVSValue __cdecl ConvertFPS::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ConvertFPS( args[0].AsClip(), args[1].AsInt(), args[2].AsInt(1), 
                         args[3].AsInt(-1), args[4].AsInt(0), env );
}
  

AVSValue __cdecl ConvertFPS::CreateFloat(AVSValue args, void*, IScriptEnvironment* env) 
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  double n = args[1].AsFloat();
  int d = 1;
  while (n < 16777216 && d < 16777216) { n*=2; d*=2; } // 2^24, floats precision
  return new ConvertFPS( args[0].AsClip(), int(n+0.5), d, args[2].AsInt(-1), 
                         args[3].AsInt(0), env );
	}
	catch (...) { throw; }
}

AVSValue __cdecl ConvertFPS::CreateFromClip(AVSValue args, void*, IScriptEnvironment* env)
{
  const VideoInfo& vi = args[1].AsClip()->GetVideoInfo();

  if (!vi.HasVideo()) {
    env->ThrowError("ConvertFPS: The clip supplied to get the FPS from must contain video.");
  }

  return new ConvertFPS( args[0].AsClip(), vi.fps_numerator, vi.fps_denominator,
                         args[2].AsInt(-1), args[3].AsInt(0), env );
}
