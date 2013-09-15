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

#include <vector>
#include <avisynth.h>
#include "SoundTouch/SoundTouch.h"

#define BUFFERSIZE 8192
using namespace soundtouch;

class AVSsoundtouch : public GenericVideoFilter 
{
private:
  std::vector<SoundTouch*> samplers;

  unsigned last_nch;
  int dst_samples_filled;

  SFLOAT* dstbuffer;
  SFLOAT* passbuffer;
  __int64 next_sample;
  __int64 inputReadOffset;
  double sample_multiplier;
  float tempo;
  float rate;
  float pitch;

public:
static AVSValue __cdecl Create(AVSValue args, void*, IScriptEnvironment* env);


AVSsoundtouch(PClip _child, float _tempo, float _rate, float _pitch, const AVSValue* args, IScriptEnvironment* env)
: GenericVideoFilter(_child), 
  tempo(_tempo/100.0f), rate(_rate/100.0f), pitch(_pitch/100.0f)
{
	try {	// HIDE DAMN SEH COMPILER BUG!!!
  last_nch = vi.AudioChannels();
  
  dstbuffer = new SFLOAT[BUFFERSIZE * vi.AudioChannels()];
  passbuffer = new SFLOAT[BUFFERSIZE];  // One channel at the time.

  sample_multiplier  = tempo / pitch;  // Do it the same way the library does it!
  sample_multiplier *= pitch * rate;

  {for(unsigned n=0; n<last_nch; n++) 
    samplers.push_back(new SoundTouch());
  }

  {for(unsigned n=0; n<last_nch; n++) {
    samplers[n]->setRate(rate);
    samplers[n]->setTempo(tempo);
    samplers[n]->setPitch(pitch);
    samplers[n]->setChannels(1);
    samplers[n]->setSampleRate(vi.audio_samples_per_second);
    setSettings(samplers[n], args, env);
  }}

  vi.num_audio_samples = (__int64)((long double)(vi.num_audio_samples) / sample_multiplier);

  next_sample = 0;  // Next output sample
  inputReadOffset = 0;  // Next input sample
  dst_samples_filled = 0;

	}
	catch (...) { throw; }
}

static void setSettings(SoundTouch* sampler, const AVSValue* args, IScriptEnvironment* env)
{

  if (args[0].Defined()) sampler->setSetting(SETTING_SEQUENCE_MS,   args[0].AsInt());
  if (args[1].Defined()) sampler->setSetting(SETTING_SEEKWINDOW_MS, args[1].AsInt());
  if (args[2].Defined()) sampler->setSetting(SETTING_OVERLAP_MS,    args[2].AsInt());

  if (args[3].Defined()) sampler->setSetting(SETTING_USE_QUICKSEEK, args[3].AsBool() ? 1 : 0);

  if (args[4].Defined()) {
	int i = args[4].AsInt();
	if (i<0 || i%4 != 0)
	  env->ThrowError("TimeStretch: AntiAliaser filter length must divisible by 4.");

	if (i)
	  sampler->setSetting(SETTING_AA_FILTER_LENGTH, i);
	else
	  sampler->setSetting(SETTING_USE_AA_FILTER,    0);
  }
  
}

void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{

  if (start != next_sample) {  // Reset on seek
    for(unsigned n=0; n<last_nch; n++)  // Clear all resamplers
      samplers[n]->clear();

    next_sample = start;
    inputReadOffset = (__int64)(sample_multiplier * (long double)start);  // Reset at new read position (NOT sample exact :( ).
    dst_samples_filled=0;
  }

  bool buffer_full = false;
  int samples_filled = 0;

  do {
    // Empty buffer if something is still left.
    if (dst_samples_filled) {
      int copysamples = std::min((int)count-samples_filled, dst_samples_filled);
      // Copy finished samples
      memcpy((BYTE*)buf+vi.BytesFromAudioSamples(samples_filled), (BYTE*)dstbuffer, (size_t)vi.BytesFromAudioSamples(copysamples));

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
        for(unsigned n=0; n<last_nch; n++) {  // Copies back samples from individual filters
          int old_g = gotsamples;
          gotsamples = samplers[n]->receiveSamples(passbuffer, BUFFERSIZE - samples_out);

          if (n>0) {
            if (old_g!=gotsamples) {
              _RPT1(0,"SoundTouch: Got %d too few samples!!!\n", gotsamples-old_g);
            }
          }
          for(int s=0, r=samples_out*last_nch + n; s<gotsamples; s++, r+=last_nch)
            dstbuffer[r] = passbuffer[s];
        }
        samples_out += gotsamples;

      } while (gotsamples > 0);

      dst_samples_filled = samples_out;

      if (!dst_samples_filled) {  // We didn't get any samples
          // Feed new samples to filter
        child->GetAudio(dstbuffer, inputReadOffset, BUFFERSIZE, env);
        inputReadOffset += BUFFERSIZE;

        for(unsigned n=0; n<last_nch; n++) {  // Copies n channels to separate buffers to individual filters
          for(unsigned s=0, r=n; s<BUFFERSIZE; s++, r+=last_nch)
            passbuffer[s] = dstbuffer[r];
          
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

    for (size_t i = 0; i < samplers.size(); ++i)
      delete samplers[i];
  }
};


/*********** Separate Class for Stereo only material *******************/

class AVSStereoSoundTouch : public GenericVideoFilter 
{
private:
  SoundTouch* sampler;

  int dst_samples_filled;

  SFLOAT* dstbuffer;
  __int64 next_sample;
  __int64 inputReadOffset;
  double sample_multiplier;
  float tempo;
  float rate;
  float pitch;

public:
AVSStereoSoundTouch(PClip _child, float _tempo, float _rate, float _pitch, const AVSValue* args, IScriptEnvironment* env)
: GenericVideoFilter(_child), 
  tempo(_tempo/100.0f), rate(_rate/100.0f), pitch(_pitch/100.0f)
{
//  last_nch = vi.AudioChannels();
  
  dstbuffer = new SFLOAT[BUFFERSIZE * vi.AudioChannels()];

  sample_multiplier  = tempo / pitch;  // Do it the same way the library does it!
  sample_multiplier *= pitch * rate;

  sampler = new SoundTouch();

  sampler->setRate(rate);
  sampler->setTempo(tempo);
  sampler->setPitch(pitch);
  sampler->setChannels(2);
  sampler->setSampleRate(vi.audio_samples_per_second);
  AVSsoundtouch::setSettings(sampler, args, env);

  vi.num_audio_samples = (__int64)((long double)vi.num_audio_samples / sample_multiplier);

  next_sample = 0;  // Next output sample
  inputReadOffset = 0;  // Next input sample
  dst_samples_filled = 0;

}

void __stdcall GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{

  if (start != next_sample) {  // Reset on seek
    sampler->clear();
    next_sample = start;
    inputReadOffset = (__int64)(sample_multiplier * (long double)start);  // Reset at new read position (NOT sample exact :( ).
    dst_samples_filled=0;
  }

  bool buffer_full = false;
  int samples_filled = 0;

  do {
    // Empty buffer if something is still left.
    if (dst_samples_filled) {
      int copysamples = std::min((int)count-samples_filled, dst_samples_filled);
      // Copy finished samples
      if (copysamples) { 
        memcpy((BYTE*)buf+vi.BytesFromAudioSamples(samples_filled), (BYTE*)dstbuffer, (size_t)vi.BytesFromAudioSamples(copysamples));
        samples_filled += copysamples;

        dst_samples_filled -= copysamples;
        // Move non-used samples
        memcpy(dstbuffer, &dstbuffer[copysamples*2], (size_t)vi.BytesFromAudioSamples(dst_samples_filled));
      }
      if (samples_filled >= count)
        buffer_full = true;
    }

    // If buffer empty - refill
    if (dst_samples_filled==0) {
      // Read back samples from filter
      int samples_out = 0;
      int gotsamples = 0;
      do {
        gotsamples = sampler->receiveSamples(&dstbuffer[vi.BytesFromAudioSamples(samples_out)], BUFFERSIZE - samples_out);
        samples_out += gotsamples;
      } while (gotsamples > 0);

      dst_samples_filled = samples_out;

      if (!dst_samples_filled) {  // We didn't get any samples
          // Feed new samples to filter
        child->GetAudio(dstbuffer, inputReadOffset, BUFFERSIZE, env);
        inputReadOffset += BUFFERSIZE;
        sampler->putSamples(dstbuffer, BUFFERSIZE);
      } // End if no samples
    } // end if empty buffer
  } while (!buffer_full);
  next_sample += count;
}

~AVSStereoSoundTouch()
{
    delete[] dstbuffer;
    delete sampler;
}


};

AVSValue __cdecl Create_SoundTouch(AVSValue args, void*, IScriptEnvironment* env) {

  try {	// HIDE DAMN SEH COMPILER BUG!!!

  PClip clip = args[0].AsClip();

  if (!clip->GetVideoInfo().HasAudio())
    env->ThrowError("Input clip does not have audio.");

  if (!(clip->GetVideoInfo().SampleType()&SAMPLE_FLOAT))
    env->ThrowError("Input audio sample format to TimeStretch must be float.");

  if (args[0].AsClip()->GetVideoInfo().AudioChannels() == 2) {
    return new AVSStereoSoundTouch(args[0].AsClip(), 
      (float)args[1].AsFloat(100.0), 
      (float)args[2].AsFloat(100.0), 
      (float)args[3].AsFloat(100.0), 
	    &args[4],
      env);
  }
  return new AVSsoundtouch(args[0].AsClip(), 
    (float)args[1].AsFloat(100.0), 
    (float)args[2].AsFloat(100.0), 
    (float)args[3].AsFloat(100.0), 
	  &args[4],
    env);

	}
	catch (...) { throw; }
}

const AVS_Linkage * AVS_linkage = 0;
extern "C" __declspec(dllexport) const char* __stdcall AvisynthPluginInit3(IScriptEnvironment* env, const AVS_Linkage* const vectors) {
	AVS_linkage = vectors;

  // clip, base filename, start, end, image format/extension, info
  env->AddFunction("TimeStretch", "c[tempo]f[rate]f[pitch]f[sequence]i[seekwindow]i[overlap]i[quickseek]b[aa]i", Create_SoundTouch, 0);

  return "`TimeStretch' Changes tempo, pitch, and/or playback rate of audio.";
}
