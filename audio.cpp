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
  { "ResampleAudio", "ci", ResampleAudio::Create },
  { "ConvertAudioTo16bit", "c", ConvertAudioTo16bit::Create },
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
