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

// ConvertAudio classes
// Copyright (c) Klaus Post 2001 - 2004

#include "stdafx.h"

#include "../core/avisynth.h"

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



/*******************************************
 *******   Convert Audio -> Arbitrary ******
 ******************************************/

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

  ConvertAudio::~ConvertAudio() {
    if (tempbuffer_size) {
      _aligned_free(tempbuffer); 
      _aligned_free(floatbuffer); 
      tempbuffer_size=0;
    }
  }

void __stdcall ConvertAudio::GetAudio(void* buf, __int64 start, __int64 count, IScriptEnvironment* env) 
{
  int channels=vi.AudioChannels();
  if (tempbuffer_size) {
    if (tempbuffer_size<count) {
      _aligned_free(tempbuffer);
      _aligned_free(floatbuffer);
      tempbuffer = (char *) _aligned_malloc(count*src_bps*channels, 16);
      floatbuffer = (SFLOAT*) _aligned_malloc(count*channels*sizeof(SFLOAT), 16);
      tempbuffer_size=count;
    }
  } else {
    tempbuffer = (char *) _aligned_malloc(count*src_bps*channels, 16);
    floatbuffer = (SFLOAT*)_aligned_malloc(count*channels*sizeof(SFLOAT),16);
    tempbuffer_size=count;
  }

  child->GetAudio(tempbuffer, start, count, env);

  float* tmp_fb = floatbuffer;

  if (src_format != SAMPLE_FLOAT) {  // Skip initial copy, if samples are already float
    if ((env->GetCPUFlags() & CPUF_3DNOW_EXT)) {
      convertToFloat_3DN(tempbuffer, tmp_fb, src_format, count*channels);
    } else {
      convertToFloat(tempbuffer, tmp_fb, src_format, count*channels);
    }
  } else {
    tmp_fb = (float*)tempbuffer;
  }

  if ((env->GetCPUFlags() & CPUF_3DNOW_EXT)) {
    convertFromFloat_3DN(tmp_fb, buf, dst_format, count*channels);
  } else if ((env->GetCPUFlags() & CPUF_SSE)) {
    convertFromFloat_SSE(tmp_fb, buf, dst_format, count*channels);
  } else {
    convertFromFloat(tmp_fb, buf, dst_format, count*channels);
  }
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
    case SAMPLE_INT24: {
      unsigned char* samples = (unsigned char*)inbuf;
      for (i=0;i<count;i++) {
        signed int tval = (samples[i*3]<<8) | (samples[i*3+1] << 16) | (samples[i*3+2] << 24); 
        outbuf[i] = (float)(tval / 256) / (float)(1<<23);
      }
      break;
    }
    default: { 
      for (i=0;i<count;i++) 
        outbuf[i]=0.0f;
      break;     
    }
  }
}

void ConvertAudio::convertToFloat_3DN(char* inbuf, float* outbuf, char sample_type, int count) {
  int i;
  switch (sample_type) {
    case SAMPLE_INT8: {
      unsigned char* samples = (unsigned char*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=((float)samples[i]-128.0f) / 128.0f;
      break;
      }
    case SAMPLE_INT16: {
      float divisor = 1.0 / 32768.0;
      signed short* samples = (signed short*)inbuf;
      int c_miss = count & 3;
      int c_loop = (count - c_miss);  // Number of samples.
      if (c_loop) {        
        __asm {
          xor eax,eax                   // count
          mov ebx, [c_loop]
          shl ebx, 1  // Number of input bytes.
          mov esi, [samples];
          mov edi, [outbuf];
          movd mm7,[divisor]
          pshufw mm7,mm7, 01000100b
          pxor mm6,mm6
          align 16
c16_loop:
          movq mm0, [esi+eax]          //  d c | b a
          movq mm1, mm0
          punpcklwd mm0, mm6             //  b b | a a
          punpckhwd mm1, mm6             //  d d | c c
          pi2fw mm0, mm0                 //  xb=float(b) | xa=float(a)
          pi2fw mm1, mm1                 //  xb=float(d) | xa=float(c)
          pfmul mm0,mm7                  // x / 32768.0
          pfmul mm1,mm7                  // x / 32768.0
          movq [edi+eax*2], mm0          //  store xb | xa
          movq [edi+eax*2+8], mm1        //  store xd | xc
          add eax,8
          cmp eax, ebx
          jne c16_loop
          emms
        }
      }
      for (i=0; i<c_miss; i++) {
        outbuf[i+c_loop]=(float)samples[i+c_loop] * divisor;
      }
      break;
    }
    case SAMPLE_INT32: {
      float divisor = (float)1.0 / (float)MAX_INT;
      signed int* samples = (signed int*)inbuf;
      int c_miss = count & 3;
      int c_loop = count-c_miss; // in samples
      if (c_loop) {        
        __asm {
          xor eax,eax                   // count
          mov ebx, [c_loop]
          shl ebx, 2                     // in input bytes (*4)
          mov esi, [samples];
          mov edi, [outbuf];
          movd mm7,[divisor]
            pshufw mm7,mm7, 01000100b
            align 16
c32_loop:
          movq mm1, [esi+eax]            //  b b | a a
          movq mm2, [esi+eax+8]          //  d d | c c
          pi2fd mm1, mm1                 //  xb=float(b) | xa=float(a)
          pi2fd mm2, mm2                 //  xb=float(d) | xa=float(c)
          pfmul mm1,mm7                  // x / 32768.0
          pfmul mm2,mm7                  // x / 32768.0
          movq [edi+eax], mm1            //  store xb | xa
          movq [edi+eax+8], mm2          //  store xd | xc
          add eax,16
          cmp eax, ebx
          jne c32_loop
          emms
        }
      }
      for (i=0; i<c_miss; i++) {
        outbuf[i+c_loop]=(float)samples[i+c_loop] * divisor;
      }
      break;
    }

    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)inbuf;
      for (i=0;i<count;i++) 
        outbuf[i]=samples[i];
      break;     
    }

    case SAMPLE_INT24: {
      unsigned char* samples = (unsigned char*)inbuf;
      for (i=0;i<count;i++) {
        signed int tval = (samples[i*3]<<8) | (samples[i*3+1] << 16) | (samples[i*3+2] << 24); 
        outbuf[i] = (float)(tval / 256) / (float)(1<<23);
      }
      break;
    }

    default: { 
      for (i=0;i<count;i++) 
        outbuf[i]=0.0f;
      break;     
    }
  }
}


void ConvertAudio::convertFromFloat_3DN(float* inbuf,void* outbuf, char sample_type, int count) {
  int i;
  switch (sample_type) {
    case SAMPLE_INT8: {
      unsigned char* samples = (unsigned char*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]=(unsigned char)Saturate_int8(inbuf[i] * 128.0f)+128;
      break;
      }
    case SAMPLE_INT16: {
      float multiplier = 32768.0;
      signed short* samples = (signed short*)outbuf;
      int c_miss = count & 3;
      int c_loop = count-c_miss; // in samples
      if (c_loop) {        
        __asm {
          xor eax,eax                   // count
            mov ebx, [c_loop]
            shl ebx, 1                     // in output bytes (*2)
            mov esi, [inbuf];
          mov edi, [outbuf];
          movd mm7,[multiplier]
            pshufw mm7,mm7, 01000100b
            align 16
c16f_loop:
          movq mm1, [esi+eax*2]            //  b b | a a
            movq mm2, [esi+eax*2+8]          //  d d | c c
            pfmul mm1,mm7                  // x * 32 bit
            pfmul mm2,mm7                  // x * 32 bit
            pf2iw mm1, mm1                 //  xb=int(b) | xa=int(a)
            pf2iw mm2, mm2                 //  xb=int(d) | xa=int(c)
            packssdw mm1,mm2
            movq [edi+eax], mm1            //  store xb | xa
            add eax,8
            cmp eax, ebx
            jne c16f_loop
            emms
        }
      }
      for (i=0; i<c_miss; i++) {
        samples[i+c_loop]=Saturate_int16(inbuf[i+c_loop] * 32768.0f);
      }
      break;
    }

    case SAMPLE_INT32: {
      float multiplier = (float)MAX_INT;
      signed int* samples = (signed int*)outbuf;
      int c_miss = count & 3;
      int c_loop = count-c_miss; // in samples
      if (c_loop) {        
        __asm {
          xor eax,eax                   // count
            mov ebx, [c_loop]
            shl ebx, 2                     // in output bytes (*4)
            mov esi, [inbuf];
          mov edi, [outbuf];
          movd mm7,[multiplier]
            pshufw mm7,mm7, 01000100b
            align 16
c32f_loop:
          movq mm1, [esi+eax]            //  b b | a a
            movq mm2, [esi+eax+8]          //  d d | c c
            pfmul mm1,mm7                  // x * 32 bit
            pfmul mm2,mm7                  // x * 32 bit
            pf2id mm1, mm1                 //  xb=int(b) | xa=int(a)
            pf2id mm2, mm2                 //  xb=int(d) | xa=int(c)
            movq [edi+eax], mm1            //  store xb | xa
            movq [edi+eax+8], mm2          //  store xd | xc
            add eax,16
            cmp eax, ebx
            jne c32f_loop
            emms
        }
      }
      for (i=0; i<c_miss; i++) {
        samples[i+c_loop]=Saturate_int32(inbuf[i+c_loop] * (float)(MAX_INT));
      }
      break;
    }
    case SAMPLE_INT24: {
      unsigned char* samples = (unsigned char*)outbuf;
      for (i=0;i<count;i++) {
        signed int tval = Saturate_int24(inbuf[i] * (float)(1<<23));
        samples[i*3] = tval & 0xff;
        samples[i*3+1] = (tval & 0xff00)>>8;
        samples[i*3+2] = (tval & 0xff0000)>>16;
      }
      break;
    }
    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)outbuf;      
      for (i=0;i<count;i++) {
        samples[i]=inbuf[i];
      }
      break;     
    }
    default: { 
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
      for (i=0;i<count;i++) {
        samples[i]=Saturate_int16(inbuf[i] * 32768.0f);
      }
      break;
      }

    case SAMPLE_INT32: {
      signed int* samples = (signed int*)outbuf;
      for (i=0;i<count;i++) 
        samples[i]= Saturate_int32(inbuf[i] * (float)(MAX_INT));
      break;     
    }
    case SAMPLE_INT24: {
      unsigned char* samples = (unsigned char*)outbuf;
      for (i=0;i<count;i++) {
        signed int tval = Saturate_int24(inbuf[i] * (float)(1<<23));
        samples[i*3] = tval & 0xff;
        samples[i*3+1] = (tval & 0xff00)>>8;
        samples[i*3+2] = (tval & 0xff0000)>>16;
      }
      break;
    }
    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)outbuf;      
      for (i=0;i<count;i++) {
        samples[i]=inbuf[i];
      }
      break;     
    }
    default: { 
    }
  }
}

void ConvertAudio::convertFromFloat_SSE(float* inbuf,void* outbuf, char sample_type, int count) {
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
      int sleft = count & 3;
      count -= sleft;

      float mult[4];
      mult[0]=mult[1]=mult[2]=mult[3] = 32768.0;

      if (count) {        
      _asm {
        movups xmm7, [mult]
        mov eax, inbuf
        xor ebx, ebx
        mov ecx, count
        shl ecx, 1
        mov edx, outbuf
        align 16
cf16_loop:
        movups xmm0, [eax+ebx*2]
        mulps  xmm0, xmm7
        movhlps xmm1, xmm0
        cvtps2pi mm0, xmm0
        cvtps2pi mm1, xmm1
        packssdw mm0, mm1
        movq [edx+ebx], mm0

        add ebx,8
        cmp ecx, ebx
        jne cf16_loop
        emms
      }
      }
      for (i=0;i<sleft;i++) {
        samples[count+i]=Saturate_int16(inbuf[count+i] * 32768.0f);
      }

      break;
    }

    case SAMPLE_INT32: {
      signed int* samples = (signed int*)outbuf;
      int sleft = count & 3;
      count -= sleft;

      float mult[4];
      mult[0]=mult[1]=mult[2]=mult[3] = (float)(INT_MAX);
      if (count) {
        _asm {
          movups xmm7, [mult]
          mov eax, inbuf
          xor ebx, ebx
          mov ecx, count
          shl ecx, 2
          mov edx, outbuf
          align 16
cf32_loop:
          movups xmm0, [eax+ebx]
          mulps  xmm0, xmm7
          movhlps xmm1, xmm0
          cvtps2pi mm0, xmm0
          cvtps2pi mm1, xmm1
          movq [edx+ebx], mm0
          movq [edx+ebx+8], mm1
            
          add ebx,16
          cmp ecx, ebx
          jne cf32_loop
          emms
        }
      }
      for (i=0;i<sleft;i++) {
        samples[count+i]=Saturate_int32(inbuf[count+i] * (float)(INT_MAX));
      }

      break;
    }
    case SAMPLE_INT24: {
      unsigned char* samples = (unsigned char*)outbuf;
      for (i=0;i<count;i++) {
        signed int tval = Saturate_int24(inbuf[i] * (float)(1<<23));
        samples[i*3] = tval & 0xff;
        samples[i*3+1] = (tval & 0xff00)>>8;
        samples[i*3+2] = (tval & 0xff0000)>>16;
      }
      break;
    }
    case SAMPLE_FLOAT: {
      SFLOAT* samples = (SFLOAT*)outbuf;      
      for (i=0;i<count;i++) {
        samples[i]=inbuf[i];
      }
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
    if (n >= (float)((1<<23)-1)) return ((1<<23)-1);
    return (int)(n+0.5f);
}

__inline int ConvertAudio::Saturate_int32(float n) {
    if (n <= (float)MIN_INT) return MIN_INT;  
    if (n >= (float)MAX_INT) return MAX_INT;
    return (int)(n)+0.5f;
}


