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


#include "edit.h"



/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Edit_filters[] = {  
  { "Trim", "cii", Trim::Create },                          // first frame, last frame
  { "FreezeFrame", "ciii", FreezeFrame::Create },           // first frame, last frame, source frame
  { "DeleteFrame", "ci", DeleteFrame::Create },             // frame #
  { "DuplicateFrame", "ci", DuplicateFrame::Create },       // frame #
  { "UnalignedSplice", "cc+", Splice::CreateUnaligned },    // clips
  { "AlignedSplice", "cc+", Splice::CreateAligned },        // clips
  { "Dissolve", "cc+i", Dissolve::Create },                 // clips, overlap frames
  { "AudioDub", "cc", AudioDub::Create },                   // video src, audio src
  { "Reverse", "c", Reverse::Create },                      // plays backwards
  { "FadeOut", "ci", Create_FadeOut },                      // # frames
  { "FadeOut2", "ci", Create_FadeOut2 },                    // # frames
  { 0 }
};





/******************************
 *******   Trim Filter   ******
 ******************************/

Trim::Trim(int _firstframe, int _lastframe, PClip _child) : GenericVideoFilter(_child) 
{
  if (_lastframe == 0) _lastframe = vi.num_frames-1;
  firstframe = min(max(_firstframe, 0), vi.num_frames-1);
  int lastframe = min(max(_lastframe, firstframe), vi.num_frames-1);
  audio_offset = vi.AudioSamplesFromFrames(firstframe);
  vi.num_frames = lastframe+1 - firstframe;
  vi.num_audio_samples = vi.AudioSamplesFromFrames(lastframe+1) - audio_offset;
}


PVideoFrame Trim::GetFrame(int n, IScriptEnvironment* env) 
{ 
  return child->GetFrame(n+firstframe, env); 
}


void Trim::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  child->GetAudio(buf, start+audio_offset, count, env);
}


bool Trim::GetParity(int n) 
{ 
  return child->GetParity(n+firstframe); 
}


AVSValue __cdecl Trim::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Trim(args[1].AsInt(), args[2].AsInt(), args[0].AsClip());
}








/*******************************
 *******   Freeze Frame   ******
 *******************************/

FreezeFrame::FreezeFrame(int _first, int _last, int _source, PClip _child)
 : GenericVideoFilter(_child), first(_first), last(_last), source(_source) {}


PVideoFrame FreezeFrame::GetFrame(int n, IScriptEnvironment* env) 
{
  return child->GetFrame((n >= first && n <= last) ? source : n, env);
}


bool FreezeFrame::GetParity(int n) 
{
  return child->GetParity((n >= first && n <= last) ? source : n);
}


AVSValue __cdecl FreezeFrame::Create(AVSValue args, void*, IScriptEnvironment* env)
{
  return new FreezeFrame(args[1].AsInt(), args[2].AsInt(), args[3].AsInt(), args[0].AsClip());
}







/******************************
 *******   Delete Frame  ******
 ******************************/

AVSValue __cdecl DeleteFrame::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new DeleteFrame(args[1].AsInt(), args[0].AsClip());
}


DeleteFrame::DeleteFrame(int _frame, PClip _child)
 : GenericVideoFilter(_child), frame(_frame) { --vi.num_frames; }


PVideoFrame DeleteFrame::GetFrame(int n, IScriptEnvironment* env) 
{
  return child->GetFrame(n + (n>=frame), env);
}


bool DeleteFrame::GetParity(int n) 
{ 
  return child->GetParity(n + (n>=frame)); 
}







/*********************************
 *******   Duplicate Frame  ******
 *********************************/

DuplicateFrame::DuplicateFrame(int _frame, PClip _child)
 : GenericVideoFilter(_child), frame(_frame) { ++vi.num_frames; }


PVideoFrame DuplicateFrame::GetFrame(int n, IScriptEnvironment* env) 
{
  return child->GetFrame(n - (n>frame), env);
}


bool DuplicateFrame::GetParity(int n) 
{ 
  return child->GetParity(n - (n>frame)); 
}


AVSValue __cdecl DuplicateFrame::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new DuplicateFrame(args[1].AsInt(), args[0].AsClip());
}







/*******************************
 *******   Splice Filter  ******
 *******************************/

Splice::Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env)
 : GenericVideoFilter(_child1), child2(_child2)
{
  const VideoInfo& vi2 = child2->GetVideoInfo();

  if (vi.HasVideo() ^ vi2.HasVideo())
    env->ThrowError("Splice: one clip has video and the other doesn't (not allowed)");
  if (vi.HasAudio() ^ vi2.HasAudio())
    env->ThrowError("Splice: one clip has audio and the other doesn't (not allowed)");

  if (vi.HasVideo()) {
    if (vi.width != vi2.width || vi.height != vi2.height)
      env->ThrowError("Splice: frame sizes don't match");
    if (vi.pixel_type != vi2.pixel_type)
      env->ThrowError("Splice: video formats don't match");
  }
  if (vi.HasAudio()) {
    if (vi.stereo != vi2.stereo || vi.sixteen_bit != vi2.sixteen_bit)
      env->ThrowError("Splice: sound formats don't match");
  }

  video_switchover_point = vi.num_frames;

  if (realign_sound)
    audio_switchover_point = vi.AudioSamplesFromFrames(video_switchover_point);
  else
    audio_switchover_point = vi.num_audio_samples;

  vi.num_frames += vi2.num_frames;
  vi.num_audio_samples = audio_switchover_point + vi2.num_audio_samples;
}


PVideoFrame Splice::GetFrame(int n, IScriptEnvironment* env) 
{
  if (n < video_switchover_point)
    return child->GetFrame(n, env);
  else
    return child2->GetFrame(n - video_switchover_point, env);
}


void Splice::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (start+count <= audio_switchover_point)
    child->GetAudio(buf, start, count, env);
  else if (start >= audio_switchover_point)
    child2->GetAudio(buf, start - audio_switchover_point, count, env);
  else {
    const int count1 = audio_switchover_point - start;
    child->GetAudio(buf, start, count1, env);
    child2->GetAudio((char*)buf+vi.BytesFromAudioSamples(count1), 0, count-count1, env);
  }
}


bool Splice::GetParity(int n) 
{
  if (n < video_switchover_point)
    return child->GetParity(n);
  else
    return child2->GetParity(n - video_switchover_point);
}



AVSValue __cdecl Splice::CreateUnaligned(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip result = args[0].AsClip();
  for (int i=0; i<args[1].ArraySize(); ++i)
    result = new Splice(result, args[1][i].AsClip(), false, env);
  return result;
}



AVSValue __cdecl Splice::CreateAligned(AVSValue args, void*, IScriptEnvironment* env) 
{
  PClip result = args[0].AsClip();
  for (int i=0; i<args[1].ArraySize(); ++i)
    result = new Splice(result, args[1][i].AsClip(), true, env);
  return result;
}













/*********************************
 *******   Dissolve Filter  ******
 *********************************/

AVSValue __cdecl Dissolve::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  const int overlap = args[2].AsInt();
  PClip result = args[0].AsClip();
  for (int i=1; i < args[1].ArraySize(); ++i)
    result = new Dissolve(result, args[1][i].AsClip(), overlap, env);
  return result;
}


Dissolve::Dissolve(PClip _child1, PClip _child2, int _overlap, IScriptEnvironment* env)
 : GenericVideoFilter(_child1), child2(_child2), overlap(_overlap), audbuffer(0), audbufsize(0)
{
  const VideoInfo& vi2 = child2->GetVideoInfo();

  if (vi.HasVideo() ^ vi2.HasVideo())
    env->ThrowError("Dissolve: one clip has video and the other doesn't (not allowed)");
  if (vi.HasAudio() ^ vi2.HasAudio())
    env->ThrowError("Dissolve: one clip has audio and the other doesn't (not allowed)");

  if (vi.HasVideo()) {
    if (vi.width != vi2.width || vi.height != vi2.height)
      env->ThrowError("Dissolve: frame sizes don't match");
    if (vi.pixel_type != vi2.pixel_type)
      env->ThrowError("Dissolve: video formats don't match");
  }
  if (vi.HasAudio()) {
    if (vi.stereo != vi2.stereo || vi.sixteen_bit != vi2.sixteen_bit)
      env->ThrowError("Dissolve: sound formats don't match");
  }

  video_fade_start = vi.num_frames - overlap;
  video_fade_end = vi.num_frames - 1;

  audio_fade_end = vi.AudioSamplesFromFrames(video_fade_end+1)-1;
  audio_fade_start = vi.AudioSamplesFromFrames(video_fade_start);

  vi.num_frames = video_fade_start + vi2.num_frames;
  vi.num_audio_samples = audio_fade_start + vi2.num_audio_samples;
}

PVideoFrame Dissolve::GetFrame(int n, IScriptEnvironment* env) 
{
  if (n < video_fade_start)
    return child->GetFrame(n, env);
  if (n > video_fade_end)
    return child2->GetFrame(n - video_fade_start, env);

  PVideoFrame a = child->GetFrame(n, env);
  PVideoFrame b = child2->GetFrame(n - video_fade_start, env);

  const int multiplier = n - video_fade_start + 1;

  PVideoFrame c;
  if (!a->IsWritable())
    c = env->NewVideoFrame(vi);

  const BYTE *src1 = a->GetReadPtr(), *src2 = b->GetReadPtr();
  BYTE* dst = (c?c:a)->GetWritePtr();
  int src1_pitch = a->GetPitch(), src2_pitch = b->GetPitch(), dst_pitch = (c?c:a)->GetPitch();
  const int row_size = a->GetRowSize(), height = a->GetHeight();

  for (int y=height; y>0; --y) {
    for (int x=0; x<row_size; ++x)
      dst[x] = src1[x] + ((src2[x]-src1[x]) * multiplier + (overlap>>1)) / (overlap+1);
    dst += dst_pitch;
    src1 += src1_pitch;
    src2 += src2_pitch;
  }

  return (c?c:a);
}


void Dissolve::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  if (start+count <= audio_fade_start)
    child->GetAudio(buf, start, count, env);

  else if (start > audio_fade_end)
    child2->GetAudio(buf, start - audio_fade_start, count, env);

  else {
    const int bytes = vi.BytesFromAudioSamples(count);
    if (audbufsize < bytes) {
      delete[] audbuffer;
      audbuffer = new BYTE[bytes];
      audbufsize = bytes;
    }

    child->GetAudio(buf, start, count, env);
    child2->GetAudio(audbuffer, start - audio_fade_start, count, env);

    int denominator = (audio_fade_end - audio_fade_start);
    int numerator = (audio_fade_end - start);
    if (vi.sixteen_bit) {
      short *a = (short*)buf, *b = (short*)audbuffer;
      if (vi.stereo) {
        for (int i=0; i<count*2; i+=2) {
          if (numerator <= 0) {
            a[i] = b[i];
            a[i+1] = b[i+1];
          } else if (numerator < denominator) {
            a[i] = b[i] + MulDiv(a[i]-b[i], numerator, denominator);
            a[i+1] = b[i+1] + MulDiv(a[i+1]-b[i+1], numerator, denominator);
          }
          numerator--;
        }
      } else {
        for (int i=0; i<count; ++i) {
          if (numerator <= 0)
            a[i] = b[i];
          else if (numerator < denominator)
            a[i] = b[i] + MulDiv(a[i]-b[i], numerator, denominator);
          numerator--;
        }
      }
    } else {
      BYTE *a = (BYTE*)buf, *b = (BYTE*)audbuffer;
      for (int i=0; i<count; ++i) {
        if (numerator <= 0)
          a[i] = b[i];
        else if (numerator < denominator)
          a[i] = b[i] + MulDiv(a[i]-b[i], numerator, denominator);
        numerator -= ((i&1) | !vi.stereo);
      }
    }
  }
}


bool Dissolve::GetParity(int n) 
{
  return (n < video_fade_start) ? child->GetParity(n) : child2->GetParity(n - video_fade_start);
}









/*********************************
 *******   AudioDub Filter  ******
 *********************************/

AudioDub::AudioDub(PClip child1, PClip child2, IScriptEnvironment* env) 
{
  const VideoInfo& vi1 = child1->GetVideoInfo();
  const VideoInfo& vi2 = child2->GetVideoInfo();
  const VideoInfo *vi_video, *vi_audio;
  if (vi1.HasVideo() && vi2.HasAudio()) {
    vchild = child1; achild = child2;
    vi_video = &vi1, vi_audio = &vi2;
  } else if (vi2.HasVideo() && vi1.HasAudio()) {
    vchild = child2; achild = child1;
    vi_video = &vi2, vi_audio = &vi1;
  } else {
    env->ThrowError("AudioDub: need an audio and a video track");
  }

  vi = *vi_video;
  vi.audio_samples_per_second = vi_audio->audio_samples_per_second;
  vi.num_audio_samples = vi_audio->num_audio_samples;
  vi.sixteen_bit = vi_audio->sixteen_bit;
  vi.stereo = vi_audio->stereo;
}


const VideoInfo& AudioDub::GetVideoInfo() 
{ 
  return vi; 
}


PVideoFrame AudioDub::GetFrame(int n, IScriptEnvironment* env) 
{ 
  return vchild->GetFrame(n, env); 
}


bool AudioDub::GetParity(int n) 
{ 
  return vchild->GetParity(n); 
}


void AudioDub::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  achild->GetAudio(buf, start, count, env);
}


AVSValue __cdecl AudioDub::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new AudioDub(args[0].AsClip(), args[1].AsClip(), env);
}









/*******************************
 *******   Revese Filter  ******
 *******************************/

Reverse::Reverse(PClip _child) : GenericVideoFilter(_child) {}


PVideoFrame Reverse::GetFrame(int n, IScriptEnvironment* env) 
{
  return child->GetFrame(vi.num_frames-n-1, env);
}


bool Reverse::GetParity(int n) 
{ 
  return child->GetParity(vi.num_frames-n-1); 
}


void Reverse::GetAudio(void* buf, int start, int count, IScriptEnvironment* env) 
{
  child->GetAudio(buf, vi.num_audio_samples - start - count, count, env);
  int xor = vi.BytesPerAudioSample() - 1;
  char* buf2 = (char*)buf;
  const int count_bytes = vi.BytesFromAudioSamples(count);
  for (int i=0; i<(count_bytes>>1); ++i) {
    char temp = buf2[i]; buf2[i] = buf2[count_bytes-1-(i^xor)]; buf2[count_bytes-1-(i^xor)] = temp;
  }
}


AVSValue __cdecl Reverse::Create(AVSValue args, void*, IScriptEnvironment* env) 
{
  return new Reverse(args[0].AsClip());
}




/******************************
 ** Assorted factory methods **
 *****************************/

AVSValue __cdecl Create_FadeOut(AVSValue args, void*, IScriptEnvironment* env) 
{
  int duration = args[1].AsInt();
  PClip a = args[0].AsClip();
  AVSValue blackness_args[] = { a, duration+1 };
  PClip b = env->Invoke("Blackness", AVSValue(blackness_args, 2)).AsClip();
  return new Dissolve(a, b, duration, env);
}

AVSValue __cdecl Create_FadeOut2(AVSValue args, void*, IScriptEnvironment* env) 
{
  int duration = args[1].AsInt();
  PClip a = args[0].AsClip();
  AVSValue blackness_args[] = { a, duration+2 };
  PClip b = env->Invoke("Blackness", AVSValue(blackness_args, 2)).AsClip();
  return new Dissolve(a, b, duration, env);
}




PClip new_Splice(PClip _child1, PClip _child2, bool realign_sound, IScriptEnvironment* env) 
{
  return new Splice(_child1, _child2, realign_sound, env);
}
