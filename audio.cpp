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

#include "audio.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Audio_filters[] = {
  { "DelayAudio", "cf", DelayAudio::Create },
  { "AmplifydB", "cf+", Amplify::Create_dB },
  { "Amplify", "cf+", Amplify::Create },
  { "AssumeSampleRate", "ci", AssumeRate::Create },
  { "Normalize", "c[volume]f[show]b", Normalize::Create },
  { "MixAudio", "cc[clip1_factor]f[clip2_factor]f", MixAudio::Create },
  { "ResampleAudio", "ci", ResampleAudio::Create },
  { "ConvertAudioTo16bit", "c", ConvertAudio::Create_16bit },
  { "ConvertAudioTo8bit", "c", ConvertAudio::Create_8bit },
  { "ConvertAudioTo32bit", "c", ConvertAudio::Create_32bit },
  { "ConvertAudioToFloat", "c", ConvertAudio::Create_float },
  { "ConvertToMono", "c", ConvertToMono::Create },
  { "EnsureVBRMP3Sync", "c", EnsureVBRMP3Sync::Create },
  { "MergeChannels", "c+", MergeChannels::Create },
  { "MonoToStereo", "cc", MergeChannels::Create },
  { "GetLeftChannel", "c", GetChannel::Create_left },
  { "GetRightChannel", "c", GetChannel::Create_right },
  { "GetChannel", "ci+", GetChannel::Create_n },
  { "GetChannels", "ci+", GetChannel::Create_n },    // Alias to ease use!
  { "KillAudio", "c", KillAudio::Create },
  { 0 }
};

// Note - floats should not be clipped - they will be clipped, when they are converted back to ints.
// Vdub can handle 8/16 bit, and reads 32bit, but cannot play/convert it. Floats doesn't make sense in AVI. So for now convert back to 16 bit always
// FIXME: Most int64's are often cropped to ints - count is ok to be int, but not start
 
 


AVSValue __cdecl ConvertAudio::Create_16bit(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip(),SAMPLE_INT16,SAMPLE_INT16);
}

AVSValue __cdecl ConvertAudio::Create_8bit(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip(),SAMPLE_INT8,SAMPLE_INT8);
}


AVSValue __cdecl ConvertAudio::Create_32bit(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip(),SAMPLE_INT32,SAMPLE_INT32);
}

AVSValue __cdecl ConvertAudio::Create_float(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip(),SAMPLE_FLOAT,SAMPLE_FLOAT);
}



/*******************************************
 *******   Convert Audio -> Arbitrary ******
 ******************************************/

// Fixme: Implement 24 bit samples
// Optme: Could be made onepass, but that would make it immensely complex
ConvertAudio::ConvertAudio(PClip _clip, int _sample_type) 
  : GenericVideoFilter(_clip)
{
  dst_format=_sample_type;
  src_format=vi.SampleType();
  // Set up convertion matrix
  src_bps=vi.BytesPerChannelSample();  // Store old size
  vi.sample_type=dst_format;
  tempbuffer_size=0;
}


void __stdcall ConvertAudio::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
  int channels=vi.AudioChannels();
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      delete[] floatbuffer;
      tempbuffer=new char[count*src_bps*channels];
      floatbuffer=new float[count*channels];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new char[count*src_bps*channels];
    floatbuffer=new float[count*channels];
    tempbuffer_size=count;
  }
  child->GetAudio(tempbuffer, start, count, env);
  convertToFloat(tempbuffer, floatbuffer, src_format, count*channels);
  convertFromFloat(floatbuffer, buf, dst_format, count*channels);

}


void ConvertAudio::convertToFloat(char* inbuf, float* outbuf, char sample_type, int count) {
  int i;
  switch (sample_type) {
    case SAMPLE_INT8: {
      unsigned char* samples = (unsigned char*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=((float)samples[i]-128.0f) / 128.0f;
      break;
      }
    case SAMPLE_INT16: {
      signed short* samples = (signed short*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=(float)samples[i] / 32768.0f;
      break;
      }

    case SAMPLE_INT32: {
      signed int* samples = (signed int*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=(float)samples[i] / (float)(MAX_INT);
      break;     
    }
    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=samples[i];
      break;     
    }
    default: { 
      for (i=0;i<count;i++) 
        outbuf[i]=0.0f;
      break;     
    }
  }
}

void ConvertAudio::convertFromFloat(float* inbuf,void* outbuf, char sample_type, int count) {
  int i;
  switch (sample_type) {
    case SAMPLE_INT8: {
      unsigned char* samples = (unsigned char*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]=(unsigned char)Saturate_int8(inbuf[i] * 128.0f)+128;
      break;
      }
    case SAMPLE_INT16: {
      signed short* samples = (signed short*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]=Saturate_int16(inbuf[i] * 32768.0f);
      break;
      }

    case SAMPLE_INT32: {
      signed int* samples = (signed int*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]= Saturate_int32(inbuf[i] * (float)(MAX_INT));
      break;     
    }
    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]=inbuf[i];
      break;     
    }
    default: { 
    }
  }
}
__inline int ConvertAudio::Saturate_int8(float n) {
    if (n <= -128.0f) return -128;
    if (n >= 127.0f) return 127;
    return (int)(n+0.5f);
}


__inline short ConvertAudio::Saturate_int16(float n) {
    if (n <= -32768.0f) return -32768;
    if (n >= 32767.0f) return 32767;
    return (short)(n+0.5f);
}

__inline int ConvertAudio::Saturate_int24(float n) {
    if (n <= (float)-(1<<23)) return -(1<<23);
    if (n >= (float)(1<<23)) return (1<<23);
    return (short)(n+0.5f);
}

__inline int ConvertAudio::Saturate_int32(float n) {
    if (n <= (float)MIN_INT) return MIN_INT;  
    if (n >= (float)MAX_INT) return MAX_INT;
    return (int)(n)+0.5f;
}

// There are two type parameters. Acceptable sample types and a prefered sample type.
// If the current clip is already one of the defined types in sampletype, this will be returned.
// If not, the current clip will be converted to the prefered type.
PClip ConvertAudio::Create(PClip clip, int sample_type, int prefered_type) 
{
  if ((!clip->GetVideoInfo().HasAudio()) || clip->GetVideoInfo().SampleType()&sample_type) {  // Sample type is already ok!
    return clip;
  }
  else 
    return new ConvertAudio(clip,prefered_type);
}



/*************************************
 *******   Assume SampleRate  ********
 *************************************/

AssumeRate::AssumeRate(PClip _clip, int _rate) 
  : GenericVideoFilter(_clip)
{
  if (_rate<0) 
    _rate=0;
  if (vi.SamplesPerSecond()==0)  // Don't add audio if none is present.
    _rate=0;

    vi.audio_samples_per_second=_rate;
} 

AVSValue __cdecl AssumeRate::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return new AssumeRate(args[0].AsClip(),args[1].AsInt());
}





/******************************************
 *******   Convert Audio -> Mono     ******
 *******   Supports int16 & float    ******
 *****************************************/

ConvertToMono::ConvertToMono(PClip _clip) 
  : GenericVideoFilter(ConvertAudio::Create(_clip,
  SAMPLE_INT16|SAMPLE_FLOAT, SAMPLE_FLOAT))
{
	
  vi.nchannels = 1;
  tempbuffer_size=0;
}


void __stdcall ConvertToMono::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
  int channels=vi.AudioChannels();
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      tempbuffer=new char[count*channels*vi.BytesPerChannelSample()];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new char[count*channels*vi.BytesPerChannelSample()];
    tempbuffer_size=count;
  }
  child->GetAudio(tempbuffer, start, count, env);
  if (vi.SampleType()&SAMPLE_INT16) {
    signed short* samples = (signed short*)buf;
    signed short* tempsamples = (signed short*)tempbuffer;
    for (int i=0; i<count; i++) {
      int tsample=0;    
      for (int j=0;j<channels;j++) 
        tsample+=tempbuffer[i*channels+j]; // Accumulate samples
      samples[i] =(short)((tsample+(channels>>1))/channels);
    }
  } else if (vi.SampleType()&SAMPLE_FLOAT) {
    SFLOAT* samples = (SFLOAT*)buf;
    SFLOAT* tempsamples = (SFLOAT*)tempbuffer;
    SFLOAT f_channels= (SFLOAT)channels;
    for (int i=0; i<count; i++) {
      SFLOAT tsample=0.0f;    
      for (int j=0;j<channels;j++) 
        tsample+=tempsamples[i*channels+j]; // Accumulate samples
      samples[i] =(tsample/f_channels);
    }
  }
}


PClip ConvertToMono::Create(PClip clip) 
{
  if (clip->GetVideoInfo().AudioChannels()==1)
    return clip;
  else
    return new ConvertToMono(clip);
}

AVSValue __cdecl ConvertToMono::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip());
}

/******************************************
 *******   Ensure VBR mp3 sync,      ******
 *******    by always reading audio  ******
 *******    sequencial.              ******             
 *****************************************/

EnsureVBRMP3Sync::EnsureVBRMP3Sync(PClip _clip) 
  : GenericVideoFilter(_clip)
{	
  last_end=0;
}


void __stdcall EnsureVBRMP3Sync::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  signed short* samples = (signed short*)buf;

  if (start != last_end) { // Reread!
    __int64 offset = 0;
    if (start>last_end) offset = last_end; // Skip forward only if the skipped to position is in front of last position.
    while (offset+count<start) { // Read whole blocks of 'count' samples
      child->GetAudio(samples, offset, count, env);
      offset+=count;
    } // Read until 'start'
      child->GetAudio(samples, offset, start-offset, env);  // Now we're at 'start'
      offset+=start-offset;
      if (offset!=start)
        env->ThrowError("EnsureVBRMP3Sync [Internal error]: Offset should be %i, but is %i",start,offset);
  }
  child->GetAudio(samples, start, count, env);
  last_end=start+count;
}


PClip EnsureVBRMP3Sync::Create(PClip clip) 
{
    return new EnsureVBRMP3Sync(clip);
}

AVSValue __cdecl EnsureVBRMP3Sync::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip());
}


/*******************************************
 *******   Mux two sources, so the      ****
 *******   total channels  is the same  ****
 *******   as the two clip              ****
 *******************************************/

MergeChannels::MergeChannels(PClip _clip, int _num_children, PClip* _child_array, IScriptEnvironment* env) 
  : GenericVideoFilter(_clip), num_children(_num_children), child_array(_child_array)
{
  clip1_channels=vi.AudioChannels();
  clip_channels = new int[num_children]; // fixme: deleteme!
  clip_offset = new signed char*[num_children]; // fixme: deleteme!
  clip_channels[0]= clip1_channels;

  for (int i=1;i<num_children;i++) {
    tclip = child_array[i];
    child_array[i] = ConvertAudio::Create(tclip,vi.SampleType(),vi.SampleType());  // Clip 2 should now be same type as clip 1.
    vi2 = child_array[i]->GetVideoInfo();

    if (vi.audio_samples_per_second!=vi2.audio_samples_per_second)  {
		  env->ThrowError("MergeChannels: Clips must have same sample rate! Use ResampleAudio()!");  // Could be removed for fun :)
    }
	  if (vi.SampleType()!=vi2.SampleType()) 
		  env->ThrowError("MergeChannels: Clips must have same sample type! Use ConvertAudio()!");    // Should never happend!
    clip_channels[i] = vi2.AudioChannels();
    vi.nchannels += vi2.AudioChannels();
  }

  tempbuffer_size=0;
}


void __stdcall MergeChannels::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      tempbuffer=new signed char[count*vi.BytesPerAudioSample()];
      tempbuffer_size=count;
    }
  } else {
      tempbuffer=new signed char[count*vi.BytesPerAudioSample()];
      tempbuffer_size=count;
  }
// Get audio:
  int channel_offset = count*vi.BytesPerChannelSample();  // Offset per channel
  int c_channel=0;

  for (int i = 0;i<num_children;i++) {
    child_array[i]->GetAudio(tempbuffer+(c_channel*channel_offset), start, count, env);
    clip_offset[i]=tempbuffer+(c_channel*channel_offset);
    c_channel += clip_channels[i];
  }
  
  // Interleave channels
  char *samples=(char*) buf;
  int bpcs = vi.BytesPerChannelSample();
  int bps = vi.BytesPerAudioSample();
  int dst_offset=0;
  for (i=0;i<num_children;i++) {
    signed char* src_buf=clip_offset[i];
    for (int l=0;l<count;l++) {
      for (int k=0;k<(bpcs*clip_channels[i]);k++) {
        samples[dst_offset+(l*bps)+k] = src_buf[(l*bpcs*clip_channels[i])+k];
      }
    }
    dst_offset+=clip_channels[i]*bpcs;
  }
}


AVSValue __cdecl MergeChannels::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  int num_args;
  PClip* child_array;

  if (args.IsArray()) {
    num_args = args.ArraySize();
    if (num_args == 1) return args[0];

    child_array = new PClip[num_args];
      for (int i=0; i<num_args; ++i)
    child_array[i] = args[i].AsClip();

  } else {
    num_args = 2;
    child_array = new PClip[num_args];
    child_array[0] = args[0].AsClip();
    child_array[1] = args[1].AsClip();
  }
  return new MergeChannels(args[0].AsClip(),num_args, child_array, env);
}


/***************************************************
 *******   Get left or right                 *******
 *******    channel from a stereo source     *******
 ***************************************************/



GetChannel::GetChannel(PClip _clip, int* _channel, int _numchannels) 
  : GenericVideoFilter(_clip),
		channel(_channel),
		numchannels(_numchannels)
{	
  src_bps=vi.BytesPerAudioSample();
  src_cbps=vi.BytesPerChannelSample();
  vi.nchannels = numchannels;
  tempbuffer_size=0;
  dst_bps=vi.BytesPerAudioSample();
  dst_cbps=vi.BytesPerChannelSample();
}


void __stdcall GetChannel::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      tempbuffer=new char[count*src_bps];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new char[count*src_bps];
    tempbuffer_size=count;
  }
  child->GetAudio(tempbuffer, start, count, env);
  char* samples = (char*)buf;
  int dst_o;
  int src_o;
  for (int i=0; i<count; i++) {
    dst_o=i*dst_bps;
    src_o=i*src_bps;
    for (int k=0; k<numchannels; k++) {
      int ch = channel[k];
      for (int j=0;j<dst_cbps;j++)
      samples[dst_o+(k*dst_cbps)+j] = tempbuffer[src_o+(ch*src_cbps)+j];
    }
  }

}


PClip GetChannel::Create_left(PClip clip) 
{
  int* ch=new int[1]; 
  ch[0]=0;
  if (clip->GetVideoInfo().AudioChannels()==1)
    return clip;
  else
    return new GetChannel(clip,ch,1);
}

PClip GetChannel::Create_right(PClip clip) 
{
  int* ch=new int[1]; 
  ch[0]=1;
  if (clip->GetVideoInfo().AudioChannels()==1)
    return clip;
  else
    return new GetChannel(clip,ch,1);
}

PClip GetChannel::Create_n(PClip clip, int* n, int numchannels) 
{
   return new GetChannel(clip,n,numchannels);
}

AVSValue __cdecl GetChannel::Create_left(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create_left(args[0].AsClip());
}

AVSValue __cdecl GetChannel::Create_right(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create_right(args[0].AsClip());
}

AVSValue __cdecl GetChannel::Create_n(AVSValue args, void*, IScriptEnvironment* env) 
{
  AVSValue args_c = args[1];
  const int num_args = args_c.ArraySize();
  int* child_array = new int[num_args];
  for (int i=0; i<num_args; ++i) {
    child_array[i] = args_c[i].AsInt()-1;  // Beware: Channel is 0-based in code and 1 based in scripts
    if (child_array[i]>=args[0].AsClip()->GetVideoInfo().AudioChannels()) 
      env->ThrowError("GetChannel: Attempted to request a channel that didn't exist!");
    if (child_array[i]<0) 
      env->ThrowError("GetChannel: There are no channels below 1! (first channel is 1)");
  }
  return Create_n(args[0].AsClip(),child_array,num_args);
}

/******************************
 *******   Kill Audio  ********
 ******************************/

KillAudio::KillAudio(PClip _clip) 
  : GenericVideoFilter(_clip)
{
  vi.audio_samples_per_second=0;
  vi.num_audio_samples=0;
}

AVSValue __cdecl KillAudio::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return new KillAudio(args[0].AsClip());
}





/******************************
 *******   Delay Audio   ******
 *****************************/

DelayAudio::DelayAudio(double delay, PClip _child)
 : GenericVideoFilter(_child), delay_samples(__int64(delay * vi.audio_samples_per_second + 0.5))
{
  vi.num_audio_samples += delay_samples;
}


void DelayAudio::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  child->GetAudio(buf, start-delay_samples, count, env);
}


AVSValue __cdecl DelayAudio::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new DelayAudio(args[1].AsFloat(), args[0].AsClip());
}





/********************************
 *******   Amplify Audio   ******
 *******************************/


Amplify::Amplify(PClip _child, float* _volumes)
  : GenericVideoFilter(ConvertAudio::Create(_child,SAMPLE_INT16|SAMPLE_FLOAT|SAMPLE_INT32,SAMPLE_FLOAT)),
    volumes(_volumes) {
}


void __stdcall Amplify::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  child->GetAudio(buf, start, count, env);
  int channels=vi.AudioChannels();
  if (vi.SampleType()==SAMPLE_INT16) {
    int* i_v=new int[channels];
    short* samples = (short*)buf;
    for (int v=0;v<channels;v++)
      i_v[v]=int(volumes[v]*65535.0f+0.5f);
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = Saturate(int(Int32x32To64(samples[i*channels+j],i_v[j]) >> 16));
      }
    } 
    return;
  }

  if (vi.SampleType()==SAMPLE_INT32) {
    int* samples = (int*)buf;
    int* i_v=new int[channels];
    for (int v=0;v<channels;v++)
      i_v[v]=int(volumes[v]*65535.0f+0.5f);
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = Saturate_int32(Int32x32To64(samples[i*channels+j],i_v[j]) >> 16);
      }
    } 
    return;
  }
  if (vi.SampleType()==SAMPLE_FLOAT) {
    SFLOAT* samples = (SFLOAT*)buf;
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = samples[i*channels+j]*volumes[j];   // Does not saturate, as other filters do. We should saturate only on conversion.
      }
    } 
    return;
  }
}


AVSValue __cdecl Amplify::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (!args[0].AsClip()->GetVideoInfo().AudioChannels())
    return args[0];
  AVSValue args_c = args[1];
  const int num_args = args_c.ArraySize();
  const int ch = args[0].AsClip()->GetVideoInfo().AudioChannels();
  float* child_array = new float[ch];
  for (int i=0; i<ch; ++i) {
    child_array[i] = args_c[min(i,num_args-1)].AsFloat();
  }
  return new Amplify(args[0].AsClip(), child_array);
}

  

AVSValue __cdecl Amplify::Create_dB(AVSValue args, void*, IScriptEnvironment* env) 
{
  if (!args[0].AsClip()->GetVideoInfo().AudioChannels())
    return args[0];
  AVSValue args_c = args[1];
  const int num_args = args_c.ArraySize();
  const int ch = args[0].AsClip()->GetVideoInfo().AudioChannels();
  float* child_array = new float[ch];
  for (int i=0; i<ch; ++i) {
    child_array[i] = dBtoScaleFactor(args_c[min(i,num_args-1)].AsFloat());
  }
  return new Amplify(args[0].AsClip(), child_array);
}


/*****************************
 ***** Normalize audio  ******
 ***** Supports int16,float******
 ******************************/
// Fixme: Maxfactor should be different on different types

Normalize::Normalize(PClip _child, double _max_factor, bool _showvalues)
  : GenericVideoFilter(ConvertAudio::Create(_child,SAMPLE_INT16|SAMPLE_FLOAT,SAMPLE_FLOAT)),
    max_factor(_max_factor),
    showvalues(_showvalues)
{
  max_volume=-1.0f;
}


void __stdcall Normalize::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  if (max_volume<0.0f) {
    __int64 passes = vi.num_audio_samples / count;
    __int64 num_samples = count;
    // Read samples into buffer and test them
    if (vi.SampleType() == SAMPLE_INT16) {
      short* samples = (short*)buf;
      short i_max_volume=0;
      for (int i=0;i<passes;i++) {
          child->GetAudio(buf, num_samples*(__int64)i, count, env);
          for (int i=0;i<num_samples;i++) {
            i_max_volume=max(abs(samples[i]),i_max_volume);
          }      
      }    
      // Remaining samples
      __int64 rem_samples = vi.num_audio_samples%count;
      child->GetAudio(buf, num_samples*passes, rem_samples, env);
      for (i=0;i<rem_samples;i++) {
        i_max_volume=max(abs(samples[i]),i_max_volume);
      }
      max_volume=(float)i_max_volume/32768.0f;
      max_factor=1.0f/max_volume;

    } else if (vi.SampleType()==SAMPLE_FLOAT) {  // Float 

      SFLOAT* samples = (SFLOAT*)buf;
      for (int i=0;i<passes;i++) {
          child->GetAudio(buf, num_samples*i, count, env);
          for (int i=0;i<num_samples;i++) {
            max_volume=max(fabs(samples[i]),max_volume);
          }      
      }    
      // Remaining samples
      __int64 rem_samples = vi.num_audio_samples%count;
      rem_samples*=vi.AudioChannels();
      child->GetAudio(buf, num_samples*passes, rem_samples, env);
      for (i=0;i<rem_samples;i++) {
        max_volume=max(fabs(samples[i]),max_volume);
      }

      max_factor=1.0f/max_volume;
    }
  } 

  if (vi.SampleType()==SAMPLE_INT16) {
    int factor=(int)(max_factor*65536.0f);
    short* samples = (short*)buf;
    child->GetAudio(buf, start, count, env); 
    int channels = vi.AudioChannels();
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = Saturate(int(Int32x32To64(samples[i*channels+j],factor) >> 16));
      }
    } 
  } else {
    SFLOAT* samples = (SFLOAT*)buf;
    child->GetAudio(buf, start, count, env); 
    int channels=vi.AudioChannels();
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = samples[i*channels+j]*max_factor;
      }
    } 
  }
}

PVideoFrame __stdcall Normalize::GetFrame(int n, IScriptEnvironment* env) {
  if (showvalues) {
    PVideoFrame src = child->GetFrame(n, env);
    env->MakeWritable(&src);
    char text[400];
    double maxdb = 8.685889638 * log(max_factor);
    // maxdb = (20 * log(factor)) / log(10);
    if (max_volume<0) {
      sprintf(text,"Normalize: Result not yet calculated!");
    } else {
    sprintf(text,"Amplify Factor: %8.4f\nAmplify DB: %8.4f", max_factor, maxdb);
    }
    ApplyMessage(&src, vi, text, vi.width/4, 0xf0f0f0,0,0 , env );
    return src;
  }
  return   child->GetFrame(n, env);

}



AVSValue __cdecl Normalize::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  double max_volume=args[1].AsFloat(1.0);
  return new Normalize(args[0].AsClip(), max_volume, args[2].AsBool(false));
}


/*****************************
 ***** Mix audio  tracks ******
 ******************************/

MixAudio::MixAudio(PClip _child, PClip _clip, double _track1_factor, double _track2_factor, IScriptEnvironment* env)
  : GenericVideoFilter(ConvertAudio::Create(_child,SAMPLE_INT16|SAMPLE_FLOAT,SAMPLE_FLOAT)),
    tclip(_clip),
		track1_factor(int(_track1_factor*65536+.5)),
    track2_factor(int(_track2_factor*65536+.5)) {

    clip = ConvertAudio::Create(tclip,vi.SampleType(),vi.SampleType());  // Clip 2 should now be same type as clip 1.
		const VideoInfo vi2 = clip->GetVideoInfo();

		if (vi.audio_samples_per_second!=vi2.audio_samples_per_second) 
			env->ThrowError("MixAudio: Clips must have same sample rate! Use ResampleAudio()!");  // Could be removed for fun :)

		if (vi.AudioChannels()!=vi2.AudioChannels()) 
			env->ThrowError("MixAudio: Clips must have same number of channels! Use ConvertToMono() or MergeChannels()!");

		tempbuffer_size=0;
	}


void __stdcall MixAudio::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      tempbuffer=new signed char[count*vi.BytesPerAudioSample()];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new signed char[count*vi.BytesPerAudioSample()];
    tempbuffer_size=count;
  }

  child->GetAudio(buf, start, count, env);
  clip->GetAudio(tempbuffer, start, count, env);
  int channels=vi.AudioChannels();

  if (vi.SampleType()&SAMPLE_INT16) {
    short* samples = (short*)buf;
    short* clip_samples = (short*)tempbuffer;
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j] = Saturate( int(Int32x32To64(samples[i*channels+j],track1_factor) >> 16) +
          int(Int32x32To64(clip_samples[i*channels+j],track2_factor) >> 16) );
      }
    }
  } else if (vi.SampleType()&SAMPLE_FLOAT) {
    SFLOAT* samples = (SFLOAT*)buf;
    SFLOAT* clip_samples = (SFLOAT*)tempbuffer;
    float t1factor=(float)track1_factor/65536.0f;
    float t2factor=(float)track2_factor/65536.0f;
    for (int i=0; i<count; ++i) {
      for (int j=0;j<channels;j++) {
        samples[i*channels+j]=(samples[i*channels+j]*t1factor) + (clip_samples[i*channels+j]*t2factor);
      }
    }
  }
}



AVSValue __cdecl MixAudio::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  double track1_factor = args[2].AsFloat(0.5);
  double track2_factor = args[3].AsFloat(1.0-track1_factor);
  return new MixAudio(args[0].AsClip(),args[1].AsClip(), track1_factor, track2_factor,env);
}

  

/********************************
 *******   Resample Audio   ******
 *******************************/

// FIXME: Support float

ResampleAudio::ResampleAudio(PClip _child, int _target_rate, IScriptEnvironment* env)
  : GenericVideoFilter(ConvertAudio::Create(_child,SAMPLE_INT16,SAMPLE_INT16)), target_rate(_target_rate)
{
  srcbuffer=0;

  if ((target_rate==vi.audio_samples_per_second)||(vi.audio_samples_per_second==0)) {
		skip_conversion=true;
		return;
	} 
	skip_conversion=false;
	factor = double(target_rate) / vi.audio_samples_per_second;

  vi.num_audio_samples = MulDiv(vi.num_audio_samples, target_rate, vi.audio_samples_per_second);
  vi.audio_samples_per_second = target_rate;

  // generate filter coefficients
  makeFilter(Imp, &LpScl, Nwing, 0.90, 9);
  Imp[Nwing] = 0; // for "interpolation" beyond last coefficient

  /* Calc reach of LP filter wing & give some creeping room */
  Xoff = int(((Nmult+1)/2.0) * max(1.0,1.0/factor)) + 10;

  // Attenuate resampler scale factor by 0.95 to reduce probability of clipping
  LpScl = int(LpScl * 0.95);
  /* Account for increased filter gain when using factors less than 1 */
  if (factor < 1)
    LpScl = int(LpScl*factor + 0.5);

  double dt = 1.0/factor;            /* Output sampling period */
  dtb = int(dt*(1<<Np) + 0.5);
  double dh = min(double(Npc), factor*Npc);  /* Filter sampling period */
  dhb = int(dh*(1<<Na) + 0.5);
}


void __stdcall ResampleAudio::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env)
{
  if (skip_conversion) {
		child->GetAudio(buf, start, count, env);
		return;
	}
  __int64 src_start = __int64(start / factor * (1<<Np) + 0.5);
  __int64 src_end = __int64(((start+count) / factor) * (1<<Np) + 0.5);
  const __int64 source_samples = (src_end>>Np)-(src_start>>Np)+2*Xoff+1;
  const int source_bytes = vi.BytesFromAudioSamples(source_samples);
  if (!srcbuffer || source_bytes > srcbuffer_size) 
  {
    if (srcbuffer) delete[] srcbuffer;
    srcbuffer = new short[source_bytes>>1];
    srcbuffer_size = source_bytes;
  }
  child->GetAudio(srcbuffer, (src_start>>Np)-Xoff, source_samples, env);

  int pos = (int(src_start & Pmask)) + (Xoff << Np);
  int pos_end = ( int(src_end - (src_start& ~(__int64)Pmask))) + (Xoff << Np);

//  int pos_end = int(src_end) - (int(src_start) & ~Pmask) + (Xoff << Np);
  short* dst = (short*)buf;

  _ASSERT(pos_end - pos <= count*dtb);

  while (pos < pos_end)  {
    int ch = vi.AudioChannels();
    for (int q = 0; q < ch; q++) {
      short* Xp = &srcbuffer[(pos>>Np)*ch];
      int v = FilterUD(Xp + q, (short)(pos&Pmask), - ch);  /* Perform left-wing inner product */
      v += FilterUD(Xp+ ch + q, (short)((-pos)&Pmask), ch);  /* Perform right-wing inner product */
      v >>= Nhg;      /* Make guard bits */
      v *= LpScl;     /* Normalize for unity filter gain */
      *dst++ = IntToShort(v,NLpScl);   /* strip guard bits, deposit output */
    }
    pos += dtb;       /* Move to next sample by time increment */
  }

}

  
AVSValue __cdecl ResampleAudio::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new ResampleAudio(args[0].AsClip(), args[1].AsInt(), env);
}


int ResampleAudio::FilterUD(short *Xp, short Ph, short Inc) 
{
  int v=0;
  unsigned Ho = (Ph*(unsigned)dhb)>>Np;
  unsigned End = Nwing;
  if (Inc > 0)        /* If doing right wing...              */
  {               /* ...drop extra coeff, so when Ph is  */
    End--;          /*    0.5, we don't do too many mult's */
    if (Ph == 0)        /* If the phase is zero...           */
      Ho += dhb;        /* ...then we've already skipped the */
  }               /*    first sample, so we must also  */
              /*    skip ahead in Imp[] and ImpD[] */
  while ((Ho>>Na) < End) {
    int t = Imp[Ho>>Na];      /* Get IR sample */
    int a = Ho & Amask;   /* a is logically between 0 and 1 */
    t += ((int(Imp[(Ho>>Na)+1])-t)*a)>>Na; /* t is now interp'd filter coeff */
    t *= *Xp;     /* Mult coeff by input sample */
    if (t & 1<<(Nhxn-1))  /* Round, if needed */
      t += 1<<(Nhxn-1);
    t >>= Nhxn;       /* Leave some guard bits, but come back some */
    v += t;           /* The filter output */
    Ho += dhb;        /* IR step */
    Xp += Inc;        /* Input signal step. NO CHECK ON BOUNDS */
  }
  return(v);
}









/********************************
 *******   Helper methods *******
 ********************************/

double Izero(double x)
{
   double sum, u, halfx, temp;
   int n;

   sum = u = n = 1;
   halfx = x/2.0;
   do {
      temp = halfx/(double)n;
      n += 1;
      temp *= temp;
      u *= temp;
      sum += u;
      } while (u >= IzeroEPSILON*sum);
   return(sum);
}


void LpFilter(double c[], int N, double frq, double Beta, int Num)
{
   double IBeta, temp, inm1;
   int i;

   /* Calculate ideal lowpass filter impulse response coefficients: */
   c[0] = 2.0*frq;
   for (i=1; i<N; i++) {
       temp = PI*(double)i/(double)Num;
       c[i] = sin(2.0*temp*frq)/temp; /* Analog sinc function, cutoff = frq */
   }

   /* 
    * Calculate and Apply Kaiser window to ideal lowpass filter.
    * Note: last window value is IBeta which is NOT zero.
    * You're supposed to really truncate the window here, not ramp
    * it to zero. This helps reduce the first sidelobe. 
    */
   IBeta = 1.0/Izero(Beta);
   inm1 = 1.0/((double)(N-1));
   for (i=1; i<N; i++) {
       temp = (double)i * inm1;
       c[i] *= Izero(Beta*sqrt(1.0-temp*temp)) * IBeta;
   }
}


/* ERROR return codes:
 *    0 - no error
 *    1 - Nwing too large (Nwing is > MAXNWING)
 *    2 - Froll is not in interval [0:1)
 *    3 - Beta is < 1.0
 *
 */

int makeFilter( short Imp[], int *LpScl, unsigned short Nwing, double Froll, double Beta)
{
   static const int MAXNWING = 8192;
   static double ImpR[MAXNWING];

   double DCgain, Scl, Maxh;
   short Dh;
   int i;

   if (Nwing > MAXNWING)                      /* Check for valid parameters */
      return(1);
   if ((Froll<=0) || (Froll>1))
      return(2);
   if (Beta < 1)
      return(3);

   /* 
    * Design Kaiser-windowed sinc-function low-pass filter 
    */
   LpFilter(ImpR, (int)Nwing, 0.5*Froll, Beta, Npc); 

   /* Compute the DC gain of the lowpass filter, and its maximum coefficient
    * magnitude. Scale the coefficients so that the maximum coeffiecient just
    * fits in Nh-bit fixed-point, and compute LpScl as the NLpScl-bit (signed)
    * scale factor which when multiplied by the output of the lowpass filter
    * gives unity gain. */
   DCgain = 0;
   Dh = Npc;                       /* Filter sampling period for factors>=1 */
   for (i=Dh; i<Nwing; i+=Dh)
      DCgain += ImpR[i];
   DCgain = 2*DCgain + ImpR[0];    /* DC gain of real coefficients */

   for (Maxh=i=0; i<Nwing; i++)
      Maxh = max(Maxh, fabs(ImpR[i]));

   Scl = ((1<<(Nh-1))-1)/Maxh;     /* Map largest coeff to 16-bit maximum */
   *LpScl = int(fabs((1<<(NLpScl+Nh))/(DCgain*Scl)));

   /* Scale filter coefficients for Nh bits and convert to integer */
   if (ImpR[0] < 0)                /* Need pos 1st value for LpScl storage */
      Scl = -Scl;
   for (i=0; i<Nwing; i++)         /* Scale & round them */
      Imp[i] = int(ImpR[i] * Scl + 0.5);

   return(0);
}
