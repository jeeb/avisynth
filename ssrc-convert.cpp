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

#include "ssrc-convert.h"

//#include "ssrc/ssrc.c"
//#include "ssrc/fftsg_fl.c"

/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction SSRC_filters[] = {
  { "SSRC", "ci", SSRC::Create },
  { 0 }
};

// Note - floats should not be clipped - they will be clipped, when they are converted back to ints.
// Vdub can handle 8/16 bit, and reads 32bit, but cannot play/convert it. Floats doesn't make sense in AVI. So for now convert back to 16 bit always
// FIXME: Most int64's are often cropped to ints - count is ok to be int, but not start
 
 
void __cdecl RecieveSamples(float* dst_samples,  unsigned int nsamples) {
  memcpy(convertedBuffer, dst_samples, nsamples*sizeof(SFLOAT));
  convertedLeft += nsamples;
}


/******************************************
 *******   Resample Audio using SSRC******
 *****************************************/


SSRC::SSRC(PClip _child, int _target_rate, IScriptEnvironment* env)
  : GenericVideoFilter(ConvertAudio::Create(_child,SAMPLE_FLOAT,SAMPLE_FLOAT)), target_rate(_target_rate)
{

  if ((target_rate==vi.audio_samples_per_second)||(vi.audio_samples_per_second==0)) {
		skip_conversion=true;
		return;
	} 
	skip_conversion=false;
  source_rate = vi.audio_samples_per_second;

	factor = double(target_rate) / vi.audio_samples_per_second;

  vi.num_audio_samples = MulDiv(vi.num_audio_samples, target_rate, vi.audio_samples_per_second);
 
  input_samples = *init_ssrc((unsigned long)source_rate, (unsigned long)target_rate, vi.AudioChannels(), RecieveSamples,  &ending);

  vi.audio_samples_per_second = target_rate;

  srcbuffer = new SFLOAT[input_samples];
  convertedBuffer = new SFLOAT[input_samples];  // Will alwats be less than input_samples.

  convertedLeft = 0;
  next_sample = 0;
  convertedReadOffset = 0;
  inputReadOffset=0;
  ending[0] = 0;

}



void __stdcall SSRC::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  if (skip_conversion) {
		child->GetAudio(buf, start, count, env);
		return;
	}

  count *= vi.AudioChannels();   // This is how SSRC keeps count. We'll do the same

  short* dst = (short*)buf;
  int ch = vi.AudioChannels();
  int samples_filled = 0;

  if (start != next_sample) {  // Reset on seek
    inputReadOffset = MulDiv(count, target_rate, source_rate);
    convertedLeft = 0;
  }

  SFLOAT* DestSamples = (SFLOAT*)buf;
  int nsamples = input_samples / vi.AudioChannels();  // How many samples per input fetch

  do {
    // Is there any samples left to copy?
    if (convertedLeft>0) {
      int ReadSamples = min(convertedLeft, count-samples_filled);
      memcpy(&DestSamples[samples_filled], &convertedBuffer[convertedReadOffset], ReadSamples*sizeof(SFLOAT));
      convertedLeft -= ReadSamples;
      convertedReadOffset += ReadSamples;
      samples_filled += ReadSamples;
    }

    // Convert Next block.
    if (convertedLeft==0) {
      child->GetAudio(srcbuffer, inputReadOffset, nsamples, env);
      inputReadOffset += nsamples;
      downsample(srcbuffer, input_samples);
      convertedReadOffset = 0;
    }
  } while (samples_filled < count);
  next_sample = start + (count/vi.AudioChannels());
  return;
}



AVSValue __cdecl SSRC::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new SSRC(args[0].AsClip(), args[1].AsInt(), env);
}

