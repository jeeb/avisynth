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


#include "limiter.h"
#include <emmintrin.h>
#include <avs/alignment.h>
#include <avs/win.h>


//min and max values are 16-bit integers either max_plane|max_plane for planar or max_luma|max_chroma for yuy2
inline void limit_plane_sse2(BYTE *ptr, int min_value, int max_value, int pitch, int width, int height) {
  __m128i min_vector = _mm_set1_epi16(min_value);
  __m128i max_vector = _mm_set1_epi16(max_value);
  BYTE* end_point = ptr + pitch * height;

  while(ptr < end_point) {
    __m128i src = _mm_load_si128(reinterpret_cast<const __m128i*>(ptr));
    src = _mm_max_epu8(src, min_vector);
    src = _mm_min_epu8(src, max_vector);
    _mm_store_si128(reinterpret_cast<__m128i*>(ptr), src);
    ptr += 16;
  }
}

#ifdef X86_32

//min and max values are 16-bit integers either max_plane|max_plane for planar or max_luma|max_chroma for yuy2
inline void limit_plane_isse(BYTE *ptr, int min_value, int max_value, int pitch, int width, int height) {
  __m64 min_vector = _mm_set1_pi16(min_value);
  __m64 max_vector = _mm_set1_pi16(max_value);
  int mod8_width = width / 8 * 8;

  for(int y = 0; y < height; y++) {
    for(int x = 0; x < mod8_width; x+=8) {
      __m64 src = *reinterpret_cast<__m64*>(ptr+x);
      src = _mm_max_pu8(src, min_vector);
      src = _mm_min_pu8(src, max_vector);
      *reinterpret_cast<__m64*>(ptr+x) = src;
    }

    if (mod8_width != width) {
      int x = width - 8;
      __m64 src = *reinterpret_cast<__m64*>(ptr+x);
      src = _mm_max_pu8(src, min_vector);
      src = _mm_min_pu8(src, max_vector);
      *reinterpret_cast<__m64*>(ptr+x) = src;
    }

    ptr += pitch;
  }
  _mm_empty();
}

#endif


Limiter::Limiter(PClip _child, int _min_luma, int _max_luma, int _min_chroma, int _max_chroma, int _show, IScriptEnvironment* env) :
  GenericVideoFilter(_child),
  min_luma(_min_luma),
  max_luma(_max_luma),
  min_chroma(_min_chroma),
  max_chroma(_max_chroma),
  show(show_e(_show))
{
  if (!vi.IsYUV() && !vi.IsYUVA())
      env->ThrowError("Limiter: Source must be YUV");

  if(show != show_none && !vi.IsYUY2() && !vi.IsYV24() && !vi.IsYV12())
      env->ThrowError("Limiter: Source must be YV24, YV12 or YUY2 with show option.");

  if ((min_luma<0)||(min_luma>255))
      env->ThrowError("Limiter: Invalid minimum luma");
  if ((max_luma<0)||(max_luma>255))
      env->ThrowError("Limiter: Invalid maximum luma");
  if ((min_chroma<0)||(min_chroma>255))
      env->ThrowError("Limiter: Invalid minimum chroma");
  if ((max_chroma<0)||(max_chroma>255))
      env->ThrowError("Limiter: Invalid maximum chroma");

}

PVideoFrame __stdcall Limiter::GetFrame(int n, IScriptEnvironment* env) {
  PVideoFrame frame = child->GetFrame(n, env);
  env->MakeWritable(&frame);
  unsigned char* srcp = frame->GetWritePtr();
  int pitch = frame->GetPitch();
  int row_size = frame->GetRowSize();
  int height = frame->GetHeight();

  if (vi.IsYUY2()) {

		if (show == show_luma) {  // Mark clamped pixels red/yellow/green over a colour image
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < row_size; x+=4) {
					int uv = 0;
					if      (srcp[x  ] < min_luma) { srcp[x  ] =  81; uv |= 1;}
					else if (srcp[x  ] > max_luma) { srcp[x  ] = 145; uv |= 2;}
					if      (srcp[x+2] < min_luma) { srcp[x+2] =  81; uv |= 1;}
					else if (srcp[x+2] > max_luma) { srcp[x+2] = 145; uv |= 2;}
					switch (uv) {
						case 1: srcp[x+1] = 91; srcp[x+3] = 240; break;     // red:   Y= 81, U=91 and V=240
						case 2: srcp[x+1] = 54; srcp[x+3] =  34; break;     // green: Y=145, U=54 and V=34
						case 3: srcp[x  ] =     srcp[x+2] = 210;
						        srcp[x+1] = 16; srcp[x+3] = 146; break;     // yellow:Y=210, U=16 and V=146
						default: break;
					}
				}
				srcp += pitch;
			}
			return frame;
		}
		else if (show == show_luma_grey) {    // Mark clamped pixels coloured over a greyscaled image
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < row_size; x+=4) {
					int uv = 0;
					if      (srcp[x  ] < min_luma) { srcp[x  ] =  81; uv |= 1;}
					else if (srcp[x  ] > max_luma) { srcp[x  ] = 145; uv |= 2;}
					if      (srcp[x+2] < min_luma) { srcp[x+2] =  81; uv |= 1;}
					else if (srcp[x+2] > max_luma) { srcp[x+2] = 145; uv |= 2;}
					switch (uv) {
						case 1: srcp[x+1] = 91; srcp[x+3] = 240; break;     // red:   Y=81, U=91 and V=240
						case 2: srcp[x+1] = 54; srcp[x+3] =  34; break;     // green: Y=145, U=54 and V=34
						case 3: srcp[x+1] = 90; srcp[x+3] = 134; break;     // puke:  Y=81, U=90 and V=134
						default: srcp[x+1] = srcp[x+3] = 128; break;        // olive: Y=145, U=90 and V=134
					}
				}
				srcp += pitch;
			}
			return frame;
		}
		else if (show == show_chroma) {    // Mark clamped pixels yellow over a colour image
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < row_size; x+=4) {
					if ( (srcp[x+1] < min_chroma)  // U-
					  || (srcp[x+1] > max_chroma)  // U+
					  || (srcp[x+3] < min_chroma)  // V-
					  || (srcp[x+3] > max_chroma) )// V+
					 { srcp[x]=srcp[x+2]=210; srcp[x+1]=16; srcp[x+3]=146; }    // yellow:Y=210, U=16 and V=146
				}
				srcp += pitch;
			}
			return frame;
		}
		else if (show == show_chroma_grey) {    // Mark clamped pixels coloured over a greyscaled image
			for (int y = 0; y < height; y++) {
				for (int x = 0; x < row_size; x+=4) {
					int uv = 0;
					if      (srcp[x+1] < min_chroma) uv |= 1; // U-
					else if (srcp[x+1] > max_chroma) uv |= 2; // U+
					if      (srcp[x+3] < min_chroma) uv |= 4; // V-
					else if (srcp[x+3] > max_chroma) uv |= 8; // V+
					switch (uv) {
						case  8: srcp[x] = srcp[x+2] =  81; srcp[x+1] =  91; srcp[x+3] = 240; break;    //   +V Red
						case  9: srcp[x] = srcp[x+2] = 146; srcp[x+1] =  53; srcp[x+3] = 193; break;    // -U+V Orange
						case  1: srcp[x] = srcp[x+2] = 210; srcp[x+1] =  16; srcp[x+3] = 146; break;    // -U   Yellow
						case  5: srcp[x] = srcp[x+2] = 153; srcp[x+1] =  49; srcp[x+3] =  49; break;    // -U-V Green
						case  4: srcp[x] = srcp[x+2] = 170; srcp[x+1] = 165; srcp[x+3] =  16; break;    //   -V Cyan
						case  6: srcp[x] = srcp[x+2] = 105; srcp[x+1] = 203; srcp[x+3] =  63; break;    // +U-V Teal
						case  2: srcp[x] = srcp[x+2] =  41; srcp[x+1] = 240; srcp[x+3] = 110; break;    // +U   Blue
						case 10: srcp[x] = srcp[x+2] = 106; srcp[x+1] = 202; srcp[x+3] = 222; break;    // +U+V Magenta
						default: srcp[x+1] = srcp[x+3] = 128; break;
					}
				}
				srcp += pitch;
			}
			return frame;
		}

    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16)) {
      limit_plane_sse2(srcp, min_luma | (min_chroma << 8), max_luma | (max_chroma << 8), pitch, row_size, height);
      return frame;
    }

    /** Run emulator if CPU supports it**/
#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE)
    {
      //limit_plane_mmx(srcp, min_luma, max_luma, pitch, row_size, height);
      limit_plane_isse(srcp, min_luma | (min_chroma << 8), max_luma | (max_chroma << 8), pitch, row_size, height);
      return frame;
    }
#endif
    // If not ISSE
    for(int y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_luma )
          srcp[x++] = (unsigned char)min_luma;
        else if(srcp[x] > max_luma)
          srcp[x++] = (unsigned char)max_luma;
        else
          x++;
        if(srcp[x] < min_chroma)
          srcp[x] = (unsigned char)min_chroma;
        else if(srcp[x] > max_chroma)
          srcp[x] = (unsigned char)max_chroma;
      }
      srcp += pitch;
    }
    return frame;
  } else if(vi.IsYV12()) {

		if (show == show_luma) {    // Mark clamped pixels red/yellow/green over a colour image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcn = srcp + pitch;
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height;h+=2) {
				for (int x = 0; x < row_size; x+=2) {
					int uv = 0;
					if      (srcp[x  ] < min_luma) { srcp[x  ] =  81; uv |= 1;}
					else if (srcp[x  ] > max_luma) { srcp[x  ] = 145; uv |= 2;}
					if      (srcp[x+1] < min_luma) { srcp[x+1] =  81; uv |= 1;}
					else if (srcp[x+1] > max_luma) { srcp[x+1] = 145; uv |= 2;}
					if      (srcn[x  ] < min_luma) { srcn[x  ] =  81; uv |= 1;}
					else if (srcn[x  ] > max_luma) { srcn[x  ] = 145; uv |= 2;}
					if      (srcn[x+1] < min_luma) { srcn[x+1] =  81; uv |= 1;}
					else if (srcn[x+1] > max_luma) { srcn[x+1] = 145; uv |= 2;}
					switch (uv) {
						case 1: srcpU[x/2] = 91; srcpV[x/2] = 240; break;       // red:   Y=81, U=91 and V=240
						case 2: srcpU[x/2] = 54; srcpV[x/2] =  34; break;       // green: Y=145, U=54 and V=34
						case 3: srcp[x]=srcp[x+2]=srcn[x]=srcn[x+2]=210;
						        srcpU[x/2] = 16; srcpV[x/2] = 146; break;       // yellow:Y=210, U=16 and V=146
						default: break;
					}
				}
				srcp += pitch*2;
				srcn += pitch*2;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_luma_grey) {       // Mark clamped pixels coloured over a greyscaled image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcn = srcp + pitch;
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height;h+=2) {
				for (int x = 0; x < row_size; x+=2) {
					int uv = 0;
					if      (srcp[x  ] < min_luma) { srcp[x  ] =  81; uv |= 1;}
					else if (srcp[x  ] > max_luma) { srcp[x  ] = 145; uv |= 2;}
					if      (srcp[x+1] < min_luma) { srcp[x+1] =  81; uv |= 1;}
					else if (srcp[x+1] > max_luma) { srcp[x+1] = 145; uv |= 2;}
					if      (srcn[x  ] < min_luma) { srcn[x  ] =  81; uv |= 1;}
					else if (srcn[x  ] > max_luma) { srcn[x  ] = 145; uv |= 2;}
					if      (srcn[x+1] < min_luma) { srcn[x+1] =  81; uv |= 1;}
					else if (srcn[x+1] > max_luma) { srcn[x+1] = 145; uv |= 2;}
					switch (uv) {
						case 1: srcpU[x/2] = 91; srcpV[x/2] = 240; break;       // red:   Y=81, U=91 and V=240
						case 2: srcpU[x/2] = 54; srcpV[x/2] =  34; break;       // green: Y=145, U=54 and V=34
						case 3: srcpU[x/2] = 90; srcpV[x/2] = 134; break;       // puke:  Y=81, U=90 and V=134
						default: srcpU[x/2] = srcpV[x/2] = 128; break;          // olive: Y=145, U=90 and V=134
					}
				}
				srcp += pitch*2;
				srcn += pitch*2;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_chroma) {   // Mark clamped pixels yellow over a colour image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcn = srcp + pitch;
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height;h+=2) {
				for (int x = 0; x < row_size; x+=2) {
					if ( (srcpU[x/2] < min_chroma)  // U-
					  || (srcpU[x/2] > max_chroma)  // U+
					  || (srcpV[x/2] < min_chroma)  // V-
					  || (srcpV[x/2] > max_chroma) )// V+
					{ srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=210; srcpU[x/2]= 16; srcpV[x/2]=146; }   // yellow:Y=210, U=16 and V=146
				}
				srcp += pitch*2;
				srcn += pitch*2;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_chroma_grey) {   // Mark clamped pixels coloured over a greyscaled image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcn = srcp + pitch;
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height;h+=2) {
				for (int x = 0; x < row_size; x+=2) {
					int uv = 0;
					if      (srcpU[x/2] < min_chroma) uv |= 1; // U-
					else if (srcpU[x/2] > max_chroma) uv |= 2; // U+
					if      (srcpV[x/2] < min_chroma) uv |= 4; // V-
					else if (srcpV[x/2] > max_chroma) uv |= 8; // V+
					switch (uv) {
						case  8: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]= 81; srcpU[x/2]= 91; srcpV[x/2]=240; break;   //   +V Red
						case  9: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=146; srcpU[x/2]= 53; srcpV[x/2]=193; break;   // -U+V Orange
						case  1: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=210; srcpU[x/2]= 16; srcpV[x/2]=146; break;   // -U   Yellow
						case  5: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=153; srcpU[x/2]= 49; srcpV[x/2]= 49; break;   // -U-V Green
						case  4: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=170; srcpU[x/2]=165; srcpV[x/2]= 16; break;   //   -V Cyan
						case  6: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=105; srcpU[x/2]=203; srcpV[x/2]= 63; break;   // +U-V Teal
						case  2: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]= 41; srcpU[x/2]=240; srcpV[x/2]=110; break;   // +U   Blue
						case 10: srcp[x]=srcp[x+1]=srcn[x]=srcn[x+1]=106; srcpU[x/2]=202; srcpV[x/2]=222; break;   // +U+V Magenta
						default: srcpU[x/2] = srcpV[x/2] = 128; break;
					}
				}
				srcp += pitch*2;
				srcn += pitch*2;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
  } else if(vi.IsYV24()) {

		if (show == show_luma) {    // Mark clamped pixels red/green over a colour image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height; h+=1) {
				for (int x = 0; x < row_size; x+=1) {
					if      (srcp[x] < min_luma) { srcp[x] =  81; srcpU[x] = 91; srcpV[x] = 240; }       // red:   Y=81, U=91 and V=240
					else if (srcp[x] > max_luma) { srcp[x] = 145; srcpU[x] = 54; srcpV[x] =  34; }       // green: Y=145, U=54 and V=34
				}
				srcp  += pitch;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_luma_grey) {       // Mark clamped pixels red/green over a greyscaled image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height; h+=1) {
				for (int x = 0; x < row_size; x+=1) {
					if      (srcp[x] < min_luma) { srcp[x] =  81; srcpU[x] = 91; srcpV[x] = 240; }       // red:   Y=81, U=91 and V=240
					else if (srcp[x] > max_luma) { srcp[x] = 145; srcpU[x] = 54; srcpV[x] =  34; }       // green: Y=145, U=54 and V=34
					else                         {                srcpU[x] =     srcpV[x] = 128; }       // grey
				}
				srcp  += pitch;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_chroma) {   // Mark clamped pixels yellow over a colour image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height; h+=1) {
				for (int x = 0; x < row_size; x+=1) {
					if ( (srcpU[x] < min_chroma)  // U-
					  || (srcpU[x] > max_chroma)  // U+
					  || (srcpV[x] < min_chroma)  // V-
					  || (srcpV[x] > max_chroma) )// V+
					{ srcp[x]=210; srcpU[x]= 16; srcpV[x]=146; }   // yellow:Y=210, U=16 and V=146
				}
				srcp  += pitch;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
		else if (show == show_chroma_grey) {   // Mark clamped pixels coloured over a greyscaled image
			const int pitchUV = frame->GetPitch(PLANAR_U);
			unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
			unsigned char* srcpU = frame->GetWritePtr(PLANAR_U);

			for (int h=0; h < height; h+=1) {
				for (int x = 0; x < row_size; x+=1) {
					int uv = 0;
					if      (srcpU[x] < min_chroma) uv |= 1; // U-
					else if (srcpU[x] > max_chroma) uv |= 2; // U+
					if      (srcpV[x] < min_chroma) uv |= 4; // V-
					else if (srcpV[x] > max_chroma) uv |= 8; // V+
					switch (uv) {
						case  8: srcp[x]= 81; srcpU[x]= 91; srcpV[x]=240; break;   //   +V Red
						case  9: srcp[x]=146; srcpU[x]= 53; srcpV[x]=193; break;   // -U+V Orange
						case  1: srcp[x]=210; srcpU[x]= 16; srcpV[x]=146; break;   // -U   Yellow
						case  5: srcp[x]=153; srcpU[x]= 49; srcpV[x]= 49; break;   // -U-V Green
						case  4: srcp[x]=170; srcpU[x]=165; srcpV[x]= 16; break;   //   -V Cyan
						case  6: srcp[x]=105; srcpU[x]=203; srcpV[x]= 63; break;   // +U-V Teal
						case  2: srcp[x]= 41; srcpU[x]=240; srcpV[x]=110; break;   // +U   Blue
						case 10: srcp[x]=106; srcpU[x]=202; srcpV[x]=222; break;   // +U+V Magenta
						default:              srcpU[x]=     srcpV[x]=128; break;
					}
				}
				srcp  += pitch;
				srcpV += pitchUV;
				srcpU += pitchUV;
			}
			return frame;
		}
  }
  if (vi.IsPlanar())
  {
    //todo: separate to functions and use sse2 for aligned planes even if some are unaligned
    if ((env->GetCPUFlags() & CPUF_SSE2) && IsPtrAligned(srcp, 16) &&
      IsPtrAligned(frame->GetWritePtr(PLANAR_U), 16) && IsPtrAligned(frame->GetWritePtr(PLANAR_V), 16)) {
        limit_plane_sse2(srcp, min_luma | (min_luma << 8), max_luma | (max_luma << 8), pitch, row_size, height);

        limit_plane_sse2(frame->GetWritePtr(PLANAR_U), min_chroma | (min_chroma << 8), max_chroma | (max_chroma << 8),
          frame->GetPitch(PLANAR_U), frame->GetRowSize(PLANAR_U), frame->GetHeight(PLANAR_U));

        limit_plane_sse2(frame->GetWritePtr(PLANAR_V), min_chroma | (min_chroma << 8), max_chroma | (max_chroma << 8),
          frame->GetPitch(PLANAR_V), frame->GetRowSize(PLANAR_V), frame->GetHeight(PLANAR_V));

        return frame;
    }

#ifdef X86_32
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE)
    {
      limit_plane_isse(srcp, min_luma | (min_luma << 8), max_luma | (max_luma << 8), pitch, row_size, height);
      limit_plane_isse(frame->GetWritePtr(PLANAR_U), min_chroma | (min_chroma << 8), max_chroma | (max_chroma << 8),
        frame->GetPitch(PLANAR_U), frame->GetRowSize(PLANAR_U), frame->GetHeight(PLANAR_U));
      limit_plane_isse(frame->GetWritePtr(PLANAR_V), min_chroma | (min_chroma << 8), max_chroma | (max_chroma << 8),
        frame->GetPitch(PLANAR_V), frame->GetRowSize(PLANAR_V), frame->GetHeight(PLANAR_V));

      return frame;
    }
#endif

    for(int y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_luma )
          srcp[x] = (unsigned char)min_luma;
        else if(srcp[x] > max_luma)
          srcp[x] = (unsigned char)max_luma;
      }
      srcp += pitch;
    }

    // Prepare for chroma
    srcp = frame->GetWritePtr(PLANAR_U);
    unsigned char* srcpV = frame->GetWritePtr(PLANAR_V);
    row_size = frame->GetRowSize(PLANAR_U);
    height = frame->GetHeight(PLANAR_U);
    pitch = frame->GetPitch(PLANAR_U);
    if (!pitch)
      return frame;

    for(int y = 0; y < height; y++) {
      for(int x = 0; x < row_size; x++) {
        if(srcp[x] < min_chroma)
          srcp[x] = (unsigned char)min_chroma;
        else if(srcp[x] > max_chroma)
          srcp[x] = (unsigned char)max_chroma;
        if(srcpV[x] < min_chroma)
          srcpV[x] = (unsigned char)min_chroma;
        else if(srcpV[x] > max_chroma)
          srcpV[x] = (unsigned char)max_chroma;
      }
      srcp += pitch;
      srcpV += pitch;
    }
  }
  return frame;
}

AVSValue __cdecl Limiter::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	const char* option = args[5].AsString(0);
	show_e show = show_none;

	if (option) {
	  if      (lstrcmpi(option, "luma") == 0)
			show = show_luma;
	  else if (lstrcmpi(option, "luma_grey") == 0)
			show = show_luma_grey;
	  else if (lstrcmpi(option, "chroma") == 0)
			show = show_chroma;
	  else if (lstrcmpi(option, "chroma_grey") == 0)
			show = show_chroma_grey;
	  else
			env->ThrowError("Limiter: show must be \"luma\", \"luma_grey\", \"chroma\" or \"chroma_grey\"");
	}

	return new Limiter(args[0].AsClip(), args[1].AsInt(16), args[2].AsInt(235), args[3].AsInt(16), args[4].AsInt(240), show, env);
}
