// Avisynth v2.5. 
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

// AviSynth -> SoundTouch interface (c) 2004, Klaus Post.



#include "stdafx.h"

#include "avs-soundtouch.h"


#define BUFFERSIZE 4096

using namespace soundtouch;


class AVSsoundtouch : public GenericVideoFilter 
{
private:
	ptr_list_simple<SoundTouch> samplers;

 	UINT last_nch;
  int dstbuffer_size;
  int dst_samples_filled;

  SFLOAT* dstbuffer;
  SFLOAT* passbuffer;
  __int64 next_sample;
  __int64 inputReadOffset;
  double sample_multiplier;
  double tempo;
  double rate;
  double pitch;

public:
static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);


AVSsoundtouch(PClip _child, double _tempo, double _rate, double _pitch, IScriptEnvironment* env)
: GenericVideoFilter(ConvertAudio::Create(_child, SAMPLE_FLOAT, SAMPLE_FLOAT)), 
  tempo(_tempo), rate(_rate), pitch(_pitch)
{
  last_nch = vi.AudioChannels();
  
  dstbuffer = new SFLOAT[BUFFERSIZE * vi.AudioChannels()];  // Our buffer can minimum contain one second.
  passbuffer = new SFLOAT[BUFFERSIZE];  // Our buffer can minimum contain one second. One channel at the time.
  dstbuffer_size = vi.audio_samples_per_second;

  sample_multiplier = 100.0 / tempo;
  sample_multiplier *= 100.0 / rate;

  int n;
  
  for(n=0;n<last_nch;n++) 
    samplers.add_item(new SoundTouch());

  for(n=0;n<last_nch;n++) {
    samplers[n]->setRate(rate/100.0);
    samplers[n]->setTempo(tempo/100.0);
    samplers[n]->setPitch(pitch/100.0);
    samplers[n]->setChannels(1);
    samplers[n]->setSampleRate(vi.audio_samples_per_second);
  }

  vi.num_audio_samples = (double)vi.num_audio_samples * sample_multiplier;

  sample_multiplier = 1.0 / sample_multiplier;  // We need the inserse to use it for sample offsets in the GetAudio loop.

  next_sample = 0;  // Next output sample
  inputReadOffset = 0;  // Next input sample
  dst_samples_filled = 0;

}

void __stdcall AVSsoundtouch::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{

  if (start != next_sample) {  // Reset on seek
    for(int n=0;n<last_nch;n++) 
      samplers[n]->clear();
    inputReadOffset = (__int64)(sample_multiplier * (double)start);  // Reset at new read position (NOT sample exact :( ).
    dst_samples_filled=0;
  }

  bool buffer_full = false;
  int samples_filled = 0;

  do {
    // Empty buffer if something is still left.
    if (dst_samples_filled) {
      int copysamples = min((int)count-samples_filled, dst_samples_filled);
      // Copy finished samples
      env->BitBlt((BYTE*)buf+samples_filled*last_nch*sizeof(SFLOAT),0,
        (BYTE*)dstbuffer,0,copysamples*last_nch*sizeof(SFLOAT),1);

      dst_samples_filled -= copysamples;

      if (dst_samples_filled) { // Move non-used samples
        memcpy(dstbuffer, &dstbuffer[copysamples*last_nch], dst_samples_filled*sizeof(SFLOAT)*last_nch);
      }
      samples_filled += copysamples;
      if (samples_filled >= count)
        buffer_full = true;
    }

    // If buffer empty - refill
    if (dst_samples_filled==0) {
      // Read back samples from filter
		  int samples_out = 0;
      int gotsamples = 0;
      do {
		    for(int n=0;n<last_nch;n++)  // Copies back samples from individual filters
		    {
          int old_g = gotsamples;
          gotsamples = samplers[n]->receiveSamples(passbuffer, BUFFERSIZE - samples_out);

          if (n>0) {
            if (old_g!=gotsamples) {
              _RPT1(0,"SoundTouch: Got %d too few samples!!!\n", gotsamples-old_g);
            }
          }
			    UINT s;
			    for(s=0;s<(UINT)gotsamples;s++)
				    dstbuffer[(samples_out+s)*last_nch + n] = passbuffer[s];
        }
        samples_out += gotsamples;

      } while (gotsamples > 0);

			dst_samples_filled = samples_out;

      if (!dst_samples_filled) {  // We didn't get any samples
          // Feed new samples to filter
        child->GetAudio(dstbuffer, inputReadOffset, BUFFERSIZE, env);
        inputReadOffset += BUFFERSIZE;

			  for(int n=0;n<last_nch;n++)  // Copies n channels to separate buffers to individual filters
			  {
				  UINT s;
				  for(s=0;s<BUFFERSIZE;s++)
					  passbuffer[s] = dstbuffer[s*last_nch + n];
          
          samplers[n]->putSamples(passbuffer, BUFFERSIZE);
			  }
      } // End if no samples
		} // end if empty buffer
  } while (!buffer_full);
  next_sample += count;
}

~AVSsoundtouch()
	{
    delete[] dstbuffer;
    delete[] passbuffer;
		samplers.delete_all();
}


};

AVSValue __cdecl AVSsoundtouch::Create(AVSValue args, void*, IScriptEnvironment* env) {
  return new AVSsoundtouch(args[0].AsClip(), 
    args[1].AsFloat(100.0), 
    args[2].AsFloat(100.0), 
    args[3].AsFloat(100.0), 
    env);
}


AVSFunction Soundtouch_filters[] = {
  { "SoundTouch", "c[tempo]f[rate]f[pitch]f", AVSsoundtouch::Create },
  { 0 }
};


