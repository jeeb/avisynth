// Avisynth v1.0 beta.  Copyright 2000 Ben Rudiak-Gould.
// http://www.math.berkeley.edu/~benrg/avisynth.html

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


#include "audio.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Audio_filters[] = {
  { "DelayAudio", "cf", DelayAudio::Create },
  { "AmplifydB", "cf[]f", Amplify::Create_dB },
  { "Amplify", "cf[]f", Amplify::Create },
  { "Normalize", "c[left]f[right]f", Normalize::Create },
  { "MixAudio", "cc[clip1_factor]f[clip2_factor]f", MixAudio::Create },
  { "AssumeSampleRate", "ci", AssumeRate::Create },
  { "ResampleAudio", "ci", ResampleAudio::Create },
  { "LowPassAudio", "ci[]f", FilterAudio::Create_LowPass },
//  { "LowPassAudioALT", "ci", FilterAudio::Create_LowPassALT },
  { "HighPassAudio", "ci[]f", FilterAudio::Create_HighPass },
  { "ConvertAudioTo16bit", "c", ConvertAudioTo16bit::Create },
  { "ConvertToMono", "c", ConvertToMono::Create },
  { "EnsureVBRMP3Sync", "c", EnsureVBRMP3Sync::Create },
  { "MonoToStereo", "cc", MonoToStereo::Create },
  { "GetLeftChannel", "c", GetChannel::Create_left },
  { "GetRightChannel", "c", GetChannel::Create_right },
  { "KillAudio", "c", KillAudio::Create },
  { 0 }
};





 
/******************************************
 *******   Convert Audio -> 16 bit   ******
 *****************************************/

ConvertAudioTo16bit::ConvertAudioTo16bit(PClip _clip) 
  : GenericVideoFilter(_clip)
{
  vi.sixteen_bit = true;
}


void __stdcall ConvertAudioTo16bit::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  int n = vi.BytesFromAudioSamples(count)/2;
  signed short* p16 = (signed short*)buf;
  BYTE* p8 = (BYTE*)buf + n;
  child->GetAudio(p8, start, count, env);
  for (int i=0; i<n; ++i)
    p16[i] = short(p8[i] * 256 + 0x8080);
}


PClip ConvertAudioTo16bit::Create(PClip clip) 
{
  if (clip->GetVideoInfo().sixteen_bit)
    return clip;
  else
    return new ConvertAudioTo16bit(clip);
}


AVSValue __cdecl ConvertAudioTo16bit::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create(args[0].AsClip());
}



/******************************************
 *******   Convert Audio -> Mono     ******
 *****************************************/

ConvertToMono::ConvertToMono(PClip _clip) 
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_clip))
{
	
  vi.stereo = false;
  tempbuffer_size=0;
}


void __stdcall ConvertToMono::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      delete[] tempbuffer;
      tempbuffer=new signed short[count*2];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new signed short[count*2];
    tempbuffer_size=count;
  }
  child->GetAudio(tempbuffer, start, count, env);
  signed short* samples = (signed short*)buf;
  for (int i=0; i<count; i++)
    samples[i] = (tempbuffer[i*2] + tempbuffer[i*2+1])>>1;
}


PClip ConvertToMono::Create(PClip clip) 
{
  if (!clip->GetVideoInfo().stereo)
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


void __stdcall EnsureVBRMP3Sync::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  signed short* samples = (signed short*)buf;

  if (start!=last_end) { // Reread!
    int offset=0;
    if (start>last_end) offset=last_end; // Skip forward only if the skipped to position is in front of last position.
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
 *******   Mux 2 mono sources -> stereo ****
 *******************************************/

MonoToStereo::MonoToStereo(PClip _child, PClip _clip, IScriptEnvironment* env) 
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)),
	right(ConvertAudioTo16bit::Create(_clip))
{
	const VideoInfo& vi2 = right->GetVideoInfo();

	if (vi.audio_samples_per_second!=vi2.audio_samples_per_second) 
		env->ThrowError("MixAudio: Clips must have same sample rate! Use ResampleAudio()!");  // Could be removed for fun :)

	left_stereo=vi.stereo;
	right_stereo=vi2.stereo;

  vi.stereo = true;
  tempbuffer_size=0;
}


void __stdcall MonoToStereo::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<(count*4)) {
      delete[] tempbuffer;
      tempbuffer=new signed short[count*4];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new signed short[count*4];
    tempbuffer_size=count;
  }
	int t_offset=(tempbuffer_size*2);

  child->GetAudio(tempbuffer, start, count, env);
  right->GetAudio((tempbuffer+t_offset), start, count, env);
	
  signed short* samples = (signed short*)buf;
  signed short* left_samples = (signed short*)tempbuffer;
  signed short* right_samples = (signed short*)(tempbuffer+t_offset);

	if (left_stereo) {
		if (right_stereo) { //Ls Rs
			for (int i=0; i<count; i++) {
				samples[i*2] = (left_samples[i*2]);
				samples[i*2+1] = (right_samples[i*2+1]);
			}
		} else { // Ls Rm
			for (int i=0; i<count; i++) {
				samples[i*2] = (left_samples[i*2]);
				samples[i*2+1] = (right_samples[i]);
			}
		}
	} else { // Lm
		if (right_stereo) { //Lm Rs
			for (int i=0; i<count; i++) {
				samples[i*2] = (left_samples[i]);
				samples[i*2+1] = (right_samples[i*2+1]);
			}
		} else { // Lm Rm
			for (int i=0; i<count; i++) {
				samples[i*2] = (left_samples[i]);
				samples[i*2+1] = (right_samples[i]);
			}
		}
	}
}


AVSValue __cdecl MonoToStereo::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new MonoToStereo(args[0].AsClip(),args[1].AsClip(),env);
}


/***************************************************
 *******   Get left or right                 *******
 *******    channel from a stereo source     *******
 ***************************************************/

GetChannel::GetChannel(PClip _clip, bool _left) 
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_clip)),
		left(_left)
{	
  vi.stereo = false;
  tempbuffer_size=0;
}


void __stdcall GetChannel::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<(count*2)) {
      delete[] tempbuffer;
      tempbuffer=new signed short[count*2];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new signed short[count*2];
    tempbuffer_size=count;
  }
  child->GetAudio(tempbuffer, start, count, env);
  signed short* samples = (signed short*)buf;
	if (left) {
		for (int i=0; i<count; i++)
			samples[i] = tempbuffer[i*2];
	} else {
		for (int i=0; i<count; i++)
			samples[i] = tempbuffer[i*2+1];
	}
}


PClip GetChannel::Create_left(PClip clip) 
{
  if (!clip->GetVideoInfo().stereo)
    return clip;
  else
    return new GetChannel(clip,true);
}

PClip GetChannel::Create_right(PClip clip) 
{
  if (!clip->GetVideoInfo().stereo)
    return clip;
  else
    return new GetChannel(clip,false);
}

AVSValue __cdecl GetChannel::Create_left(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create_left(args[0].AsClip());
}

AVSValue __cdecl GetChannel::Create_right(AVSValue args, void*, IScriptEnvironment*) 
{
  return Create_right(args[0].AsClip());
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


/*************************************
 *******   Assume SampleRate  ********
 *************************************/

AssumeRate::AssumeRate(PClip _clip, int _rate) 
  : GenericVideoFilter(_clip)
{
  if (_rate<0) 
    _rate=0;

    vi.audio_samples_per_second=_rate;
} 

AVSValue __cdecl AssumeRate::Create(AVSValue args, void*, IScriptEnvironment*) 
{
  return new AssumeRate(args[0].AsClip(),args[1].AsInt());
}



/******************************
 *******   Delay Audio   ******
 *****************************/

DelayAudio::DelayAudio(double delay, PClip _child)
 : GenericVideoFilter(_child), delay_samples(int(delay * vi.audio_samples_per_second + 0.5))
{
  vi.num_audio_samples += delay_samples;
}


void DelayAudio::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
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

Amplify::Amplify(PClip _child, double _left_factor, double _right_factor)
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)),
    left_factor(int(_left_factor*65536+.5)),
    right_factor(int(_right_factor*65536+.5)) {}


void __stdcall Amplify::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  child->GetAudio(buf, start, count, env);
  short* samples = (short*)buf;
  if (vi.stereo) {
    for (int i=0; i<count; ++i) {
      samples[i*2] = Saturate(int(Int32x32To64(samples[i*2],left_factor) >> 16));
      samples[i*2+1] = Saturate(int(Int32x32To64(samples[i*2+1],right_factor) >> 16));
    }
  } 
  else {
    for (int i=0; i<count; ++i)
      samples[i] = Saturate(int(Int32x32To64(samples[i],left_factor) >> 16));
  }
}


AVSValue __cdecl Amplify::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  double left_factor = args[1].AsFloat();
  double right_factor = args[2].AsFloat(left_factor);
  return new Amplify(args[0].AsClip(), left_factor, right_factor);
}

  

AVSValue __cdecl Amplify::Create_dB(AVSValue args, void*, IScriptEnvironment* env) 
{
    double left_factor = args[1].AsFloat();
    double right_factor = args[2].AsFloat(left_factor);
    return new Amplify(args[0].AsClip(), dBtoScaleFactor(left_factor), dBtoScaleFactor(right_factor));
}


/*****************************
 ***** Normalize audio  ******
******************************/


Normalize::Normalize(PClip _child, double _left_factor, double _right_factor)
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)),
    left_factor(int(_left_factor*65536+.5)),
    right_factor(int(_right_factor*65536+.5)) {
  max_volume=-1;
  }


void __stdcall Normalize::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  short* samples = (short*)buf;
  if (max_volume==-1) {
    int passes=vi.num_audio_samples/count;
    int num_samples=count;
    if (vi.stereo) num_samples*=2;
    // Read samples into buffer and test them
    for (int i=0;i<passes;i++) {
        child->GetAudio(buf, num_samples*i, count, env);
        for (int i=0;i<num_samples;i++) {
          max_volume=max(abs(samples[i]),max_volume);
        }
    }     
    // Remaining samples
    int rem_samples=vi.num_audio_samples%count;
    if (vi.stereo) rem_samples*=2;
    child->GetAudio(buf, num_samples*passes, rem_samples, env);
    for (i=0;i<rem_samples;i++) {
      max_volume=max(abs(samples[i]),max_volume);
    }

    double volume=32767.0/(double)max_volume;
    left_factor=(int)((double)left_factor*volume);
    right_factor=(int)((double)right_factor*volume);
  } 
  child->GetAudio(buf, start, count, env); 
  if (vi.stereo) {
    for (int i=0; i<count; ++i) {
      samples[i*2] = Saturate(int(Int32x32To64(samples[i*2],left_factor) >> 16));
      samples[i*2+1] = Saturate(int(Int32x32To64(samples[i*2+1],right_factor) >> 16));
    }
  } 
  else {
    for (int i=0; i<count; ++i)
      samples[i] = int(Int32x32To64(samples[i],left_factor) >> 16);
  }
}


AVSValue __cdecl Normalize::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  double left_factor = args[1].AsFloat(1.0);
  double right_factor = args[2].AsFloat(left_factor);
  return new Normalize(args[0].AsClip(), left_factor, right_factor);
}


/*****************************
 ***** Mix audio  tracks ******
 ******************************/

MixAudio::MixAudio(PClip _child, PClip _clip, double _track1_factor, double _track2_factor, IScriptEnvironment* env)
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)),
    clip(ConvertAudioTo16bit::Create(_clip)),
		track1_factor(int(_track1_factor*65536+.5)),
    track2_factor(int(_track2_factor*65536+.5)) {

		const VideoInfo& vi2 = clip->GetVideoInfo();

		if (vi.audio_samples_per_second!=vi2.audio_samples_per_second) 
			env->ThrowError("MixAudio: Clips must have same sample rate! Use ResampleAudio()!");  // Could be removed for fun :)

		if (vi.stereo!=vi2.stereo) 
			env->ThrowError("MixAudio: Clips must have same number of channels! Use ConvertToMono() or MonoToStereo()!");

		tempbuffer_size=0;
	}


void __stdcall MixAudio::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (tempbuffer_size) {
    if (tempbuffer_size<(count*2)) {
      delete[] tempbuffer;
      tempbuffer=new signed short[count*2];
      tempbuffer_size=count;
    }
  } else {
    tempbuffer=new signed short[count*2];
    tempbuffer_size=count;
  }

  child->GetAudio(buf, start, count, env);
  clip->GetAudio(tempbuffer, start, count, env);
  short* samples = (short*)buf;
  short* clip_samples = (short*)tempbuffer;

  if (vi.stereo) {
    for (int i=0; i<count; ++i) {
      samples[i*2] = Saturate( int(Int32x32To64(samples[i*2],track1_factor) >> 16) +
				int(Int32x32To64(clip_samples[i*2],track2_factor) >> 16) );
      samples[i*2+1] = Saturate( int(Int32x32To64(samples[i*2+1],track1_factor) >> 16) +
				int(Int32x32To64(clip_samples[i*2+1],track2_factor) >> 16) );
    }
  } 
  else {
    for (int i=0; i<count; ++i) {
      samples[i] = Saturate( int(Int32x32To64(samples[i],track1_factor) >> 16) +
				int(Int32x32To64(clip_samples[i],track2_factor) >> 16) );
		}
  }
}



AVSValue __cdecl MixAudio::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  double track1_factor = args[2].AsFloat(0.5);
  double track2_factor = args[3].AsFloat(1.0-track1_factor);
  return new MixAudio(args[0].AsClip(),args[1].AsClip(), track1_factor, track2_factor,env);
}

  
/**********************************************
 *******   Lowpass and highpass filter  *******
 *******                                *******
 *******   Type : biquad,               *******
 *******          tweaked butterworth   *******
 *******  Source: musicdsp.org          *******
 *******          Posted by Patrice Tarrabia **
 *******  Adapted by Klaus Post         *******
 ***********************************************/

FilterAudio::FilterAudio(PClip _child, int _cutoff, float _rez, int _lowpass)
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)),
    cutoff(_cutoff),
    rez(_rez), 
    lowpass(_lowpass) {
      l_vibrapos = 0;
      l_vibraspeed = 0;
      r_vibrapos = 0;
      r_vibraspeed = 0;
      lastsample=-1;
      tempbuffer_size=0;

} 


void __stdcall FilterAudio::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (lowpass<2) {    
    //Algorithm 1: Lowpass is only used in ALT-mode
    
    if (tempbuffer_size) {
      if (tempbuffer_size<(count*2)) {
        delete[] tempbuffer;
        tempbuffer=new signed short[4+count*2];
        tempbuffer_size=count;
      }
    } else {
      tempbuffer=new signed short[4+count*2];
      tempbuffer_size=count;
    }
    child->GetAudio(tempbuffer, start-2, count+2, env);
    float a1,a2,a3,b1,b2,c; 
    if (lowpass) {
      c = 1.0 / tan(PI * (float)cutoff / (float)vi.audio_samples_per_second);
      a1 = 1.0 / ( 1.0 + rez * c + c * c);
      a2 = 2* a1;
      a3 = a1;
      b1 = 2.0 * ( 1.0 - c*c) * a1;
      b2 = ( 1.0 - rez * c + c * c) * a1; 
    } else {
      c = tan(PI * (float)cutoff / (float)vi.audio_samples_per_second);
      a1 = 1.0 / ( 1.0 + rez * c + c * c);
      a2 = -2*a1;
      a3 = a1;
      b1 = 2.0 * ( c*c - 1.0) * a1;
      b2 = ( 1.0 - rez * c + c * c) * a1;  
    }
    short* samples = (short*)buf;
    if (vi.stereo) {
      
      if (lastsample!=start) {  // Streaming has just started here.
        last_1=tempbuffer[3];
        last_2=tempbuffer[2];
        last_3=tempbuffer[1];
        last_4=tempbuffer[0]; 
      }
      unsigned int i=0;
      tempbuffer+=4;
      samples[0]=Saturate((int)(a1 * (float)tempbuffer[i*2] + a2 * (float)tempbuffer[i*2-2] + a3 * (float)tempbuffer[i*2-4] - b1*(float)last_2 - b2*(float)last_4));
      samples[1]=Saturate((int)(a1 * (float)tempbuffer[i*2+1] + a2 * (float)tempbuffer[i*2-2+1] + a3 * (float)tempbuffer[i*2-4+1] - b1*(float)last_1 - b2*(float)last_3+0.5f));
      i++;
      samples[2]=Saturate((int)(a1 * (float)tempbuffer[i*2] + a2 * (float)tempbuffer[i*2-2] + a3 * (float)tempbuffer[i*2-4] - b1*(float)samples[i*2-2] - b2*(float)last_2+0.5f));
      samples[3]=Saturate((int)(a1 * (float)tempbuffer[i*2+1] + a2 * (float)tempbuffer[i*2-2+1] + a3 * (float)tempbuffer[i*2-4+1] - b1*(float)samples[i*2-2+1] - b2*(float)last_1+0.5f));
      i++;
      for (; i<count; ++i) { 
        samples[i*2]=Saturate((int)(a1 * (float)tempbuffer[i*2] + a2 * (float)tempbuffer[i*2-2] + a3 * (float)tempbuffer[i*2-4] - b1*(float)samples[i*2-2] - b2*(float)samples[i*2-4]+0.5f));
        samples[i*2+1]=Saturate((int)(a1 * (float)tempbuffer[i*2+1] + a2 * (float)tempbuffer[i*2-2+1] + a3 * (float)tempbuffer[i*2-4+1] - b1*(float)samples[i*2-2+1] - b2*(float)samples[i*2-4+1]+0.5f));
      }
      last_1=samples[count*2-1];
      last_2=samples[count*2-2];
      last_3=samples[count*2-3];
      last_4=samples[count*2-4];
      lastsample=start+count;
    } 
    else { 
      if (lastsample!=start) {
        last_1=tempbuffer[1];
        last_2=tempbuffer[0];
      }
      tempbuffer+=2;
      unsigned int i=0;
      samples[i]=Saturate((int)(a1 * (float)tempbuffer[i] + a2 * (float)tempbuffer[i-1] + a3 * (float)tempbuffer[i-2] - b1*(float)last_1 - b2*(float)last_2+0.5f));
      i++;
      samples[i]=Saturate((int)(a1 * (float)tempbuffer[i] + a2 * (float)tempbuffer[i-1] + a3 * (float)tempbuffer[i-2] - b1*(float)samples[0] - b2*(float)last_1+0.5f));
      i++;
      for (; i<count; ++i) {
         samples[i]=Saturate((int)(a1 * (float)tempbuffer[i] + a2 * (float)tempbuffer[i-1] + a3 * (float)tempbuffer[i-2] - b1*(float)samples[i-1] - b2*(float)samples[i-2]+0.5f));
      }
      last_1=samples[count-1];
      last_2=samples[count-2];
      lastsample=start+count;
    }
    
    
  } else {
    //Algorithm 2: 
    // Only lowpass, but doesn't seem to amplify as much
    
    if (start==0) {
      l_vibrapos = 0;
      l_vibraspeed = 0;
      r_vibrapos = 0;
      r_vibraspeed = 0;
    }
    child->GetAudio(buf, start, count, env);
    float amp = 0.9f; 
    float w = 2.0*PI*(float)cutoff/(float)vi.audio_samples_per_second; // Pole angle
    float q = 1.0-w/(2.0*(amp+0.5/(1.0+w))+w-2.0); // Pole magnitude
    float r = q*q;
    float c = r+1.0-2.0*cos(w)*q;
    float temp;
    short* samples = (short*)buf;
    if (vi.stereo) {
      for (unsigned int i=0; i<count; ++i) { 
        
        // Accelerate vibra by signal-vibra, multiplied by lowpasscutoff 
        l_vibraspeed += ((float)samples[i*2] - l_vibrapos) * c;
        r_vibraspeed += ((float)samples[i*2+1] - r_vibrapos) * c;
        
        // Add velocity to vibra's position 
        l_vibrapos += l_vibraspeed;
        r_vibrapos += r_vibraspeed;
        
        // Attenuate/amplify vibra's velocity by resonance 
        l_vibraspeed *= r;
        r_vibraspeed *= r;
        
        // Check clipping 
        temp = l_vibrapos;
        if (temp > 32767.0) {
          temp = 32767.0;
        } else if (temp < -32768.0) temp = -32768.0;
        
        // Store new value  
        samples[i*2] = (short)temp; 

        temp = r_vibrapos;
        if (temp > 32767.0) {
          temp = 32767.0;
        } else if (temp < -32768.0) temp = -32768.0;
        
        // Store new value  
        samples[i*2+1] = (short)temp; 
      }
    } else {
      for (unsigned int i=0; i<count; ++i) { 
        
        // Accelerate vibra by signal-vibra, multiplied by lowpasscutoff 
        l_vibraspeed += ((float)samples[i] - l_vibrapos) * c;
        
        // Add velocity to vibra's position 
        l_vibrapos += l_vibraspeed;
        
        // Attenuate/amplify vibra's velocity by resonance 
        l_vibraspeed *= r;
        
        // Check clipping 
        temp = l_vibrapos;
        if (temp > 32767.0) {
          temp = 32767.0;
        } else if (temp < -32768.0) temp = -32768.0;
        
        // Store new value  
        samples[i] = (short)temp; 
      }
    }
  }
}


AVSValue __cdecl FilterAudio::Create_LowPass(AVSValue args, void*, IScriptEnvironment* env) 
{
  int cutoff = args[1].AsInt();
  float rez = args[2].AsFloat(0.2);
  return new FilterAudio(args[0].AsClip(), cutoff, rez,2);
}


AVSValue __cdecl FilterAudio::Create_HighPass(AVSValue args, void*, IScriptEnvironment* env) 
{
  int cutoff = args[1].AsInt();
  float rez = args[2].AsFloat(0.2);
  return new FilterAudio(args[0].AsClip(), cutoff, rez,0);
}

AVSValue __cdecl FilterAudio::Create_LowPassALT(AVSValue args, void*, IScriptEnvironment* env) 
{
  int cutoff = args[1].AsInt();
  float rez = 0.0f;
  return new FilterAudio(args[0].AsClip(), cutoff, rez,1);
}
  


/********************************
 *******   Resample Audio   ******
 *******************************/

ResampleAudio::ResampleAudio(PClip _child, int _target_rate, IScriptEnvironment* env)
  : GenericVideoFilter(ConvertAudioTo16bit::Create(_child)), target_rate(_target_rate)
{
  if (target_rate==vi.audio_samples_per_second) {
		skip_conversion=true;
		return;
	} 
	skip_conversion=false;
	factor = double(target_rate) / vi.audio_samples_per_second;
  srcbuffer=0;

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


void __stdcall ResampleAudio::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (skip_conversion) {
		child->GetAudio(buf, start, count, env);
		return;
	}
  __int64 src_start = __int64(start / factor * (1<<Np) + 0.5);
  __int64 src_end = __int64((start+count) / factor * (1<<Np) + 0.5);
  const int source_samples = int(src_end>>Np)-int(src_start>>Np)+2*Xoff+1;
  const int source_bytes = vi.BytesFromAudioSamples(source_samples);
  if (!srcbuffer || source_bytes > srcbuffer_size) 
  {
    delete[] srcbuffer;
    srcbuffer = new short[source_bytes>>1];
    srcbuffer_size = source_bytes;
  }
  child->GetAudio(srcbuffer, int(src_start>>Np)-Xoff, source_samples, env);

  int pos = (int(src_start) & Pmask) + (Xoff << Np);
  int pos_end = int(src_end) - (int(src_start) & ~Pmask) + (Xoff << Np);
  short* dst = (short*)buf;

  if (!vi.stereo) {
    while (pos < pos_end) 
    {
      short* Xp = &srcbuffer[pos>>Np];
      int v = FilterUD(Xp, (short)(pos&Pmask), -1);  /* Perform left-wing inner product */
      v += FilterUD(Xp+1, (short)((-pos)&Pmask), 1);  /* Perform right-wing inner product */
      v >>= Nhg;      /* Make guard bits */
      v *= LpScl;     /* Normalize for unity filter gain */
      *dst++ = IntToShort(v,NLpScl);   /* strip guard bits, deposit output */
      pos += dtb;       /* Move to next sample by time increment */
    }
  }
  else {
    while (pos < pos_end) 
    {
      short* Xp = &srcbuffer[(pos>>Np)*2];
      int v = FilterUD(Xp, (short)(pos&Pmask), -2);
      v += FilterUD(Xp+2, (short)((-pos)&Pmask), 2);
      v >>= Nhg;
      v *= LpScl;
      *dst++ = IntToShort(v,NLpScl);
      int w = FilterUD(Xp+1, (short)(pos&Pmask), -2);
      w += FilterUD(Xp+3, (short)((-pos)&Pmask), 2);
      w >>= Nhg;
      w *= LpScl;
      *dst++ = IntToShort(w,NLpScl);
      pos += dtb;
    }
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
