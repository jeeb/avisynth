// Avisynth filter: YUV merge
// by Klaus Post (kp@interact.dk)
// adapted by Richard Berg (avisynth-dev@richardberg.net)
//
// Released under the GNU Public License
// See http://www.gnu.org/copyleft/gpl.html for details


#include "merge.h"


/********************************************************************
***** Declare index of new filters for Avisynth's filter engine *****
********************************************************************/

AVSFunction Merge_filters[] = {
  { "MergeChroma", "cc[lumaweight]f", MergeChroma::Create },  // src, chroma src, weight
  { "MergeLuma", "cc[lumaweight]f", MergeLuma::Create },      // src, chroma src, weight
  { 0 }
};





/****************************
******   Merge Chroma   *****
****************************/

MergeChroma::MergeChroma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env) 
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  if (!vi.IsYUY2())
		env->ThrowError("MergeChroma: YUY2 data only (no RGB); use ConvertToYUY2");
  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;
  if (!clip)
		env->ThrowError("MergeChroma: Luma clip must be supplied");
}



PVideoFrame __stdcall MergeChroma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);
  
  unsigned int* srcp = (unsigned int*)src->GetWritePtr();
    
  const int src_pitch = (src->GetPitch())>>1;  // short pitch (one pitch=one pixel)
  const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
  const int row_size = src->GetRowSize();
    
  const int height = src->GetHeight();
  int h=height;
  int w=(row_size>>1); // width in pixels
  
  
  PVideoFrame luma = clip->GetFrame(n, env);
  
  if ((src->GetHeight!=luma->GetHeight)||(src->GetRowSize!=luma->GetRowSize))
    env->ThrowError("MergeChroma: Images must have same width and height!");
  
  if (srcp==0) 
    env->ThrowError("MergeChroma: srcp==0");
    
  unsigned int* lumap = (unsigned int*)luma->GetReadPtr();
  const int iluma_pitch = (luma->GetPitch())>>2;  // Ints
  
  if (lumap==0) 
    env->ThrowError("MergeChroma: lumap==0");
  if (weight<1.0f) {
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
      if (w&3)
        env->ThrowError("MergeChroma: width must be a multiple of 4; use Crop.");
      // ok to go!
      env->MakeWritable(&src);
      mmx_weigh_luma(srcp,lumap,isrc_pitch,iluma_pitch,w,h,(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
    } else {
      env->ThrowError("MergeChroma: Integer SSE required (K7, P3 or P4) for weighed mode.");
    }
  } else {
    if (env->GetCPUFlags() & CPUF_MMX) {
      if (w&7)
        env->ThrowError("MergeChroma: width must be a multiple of 8; use Crop or MergeChroma(clip,0.999) for mult of 4.");
      // ok to go!
      env->MakeWritable(&luma);
      mmx_merge_luma(lumap,srcp,iluma_pitch,isrc_pitch,w,h);  // Just swap luma/chroma
      __asm {emms};
      return luma;
    } else {
      env->ThrowError("MergeChroma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
    }
  }
  __asm {emms};
  return src;
}


AVSValue __cdecl MergeChroma::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeChroma(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(1.0f), env);
}







/**************************
******   Merge Luma   *****
**************************/


MergeLuma::MergeLuma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env) 
  :	GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
	if (!vi.IsYUY2())
		env->ThrowError("MergeLuma: YUY2 data only (no RGB); use ConvertToYUY2");
  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;
	if (!clip)
		env->ThrowError("MergeLuma: Luma clip must be supplied");
}
    

PVideoFrame __stdcall MergeLuma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);
  
  env->MakeWritable(&src);
  unsigned int* srcp = (unsigned int*)src->GetWritePtr();
  
  const int src_pitch = (src->GetPitch())>>1;  // short pitch (one pitch=one pixel)
  const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
  const int row_size = src->GetRowSize();  
  
  const int height = src->GetHeight();
  int h=height;
  int w=(row_size>>1); // width in pixels
  
  
  PVideoFrame luma = clip->GetFrame(n, env);
  
  if ((src->GetHeight!=luma->GetHeight)||(src->GetRowSize!=luma->GetRowSize))
    env->ThrowError("MergeLuma: Images must have same width and height!");
  
  if (srcp==0) 
    env->ThrowError("MergeLuma: srcp==0");
    
  unsigned int* lumap = (unsigned int*)luma->GetReadPtr();
  const int iluma_pitch = (luma->GetPitch())>>2;  // Ints
  
  if (lumap==0) 
    env->ThrowError("MergeLuma: lumap==0");
  if (weight<1.0f) {
    if (env->GetCPUFlags() & CPUF_INTEGER_SSE) {
      if (w&3)
        env->ThrowError("MergeLuma: width must be a multiple of 4; use Crop.");
      // ok to go!
      mmx_weigh_luma(srcp,lumap,isrc_pitch,iluma_pitch,w,h,(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
    } else {
      env->ThrowError("MergeLuma: Integer SSE required (K7, P3 or P4) for weighed mode.");
    }
  } else {
    if (env->GetCPUFlags() & CPUF_MMX) {
      if (w&7)
        env->ThrowError("MergeLuma: width must be a multiple of 8; use Crop or MergeLuma(clip,0.999) for mult of 4.");
      // ok to go!
      mmx_merge_luma(srcp,lumap,isrc_pitch,iluma_pitch,w,h);
    } else {
      env->ThrowError("MergeLuma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
    }
  }
  __asm {emms};
  return src;
}


AVSValue __cdecl MergeLuma::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeLuma(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(1.0f),	env);
}









/****************************
******   MMX routines   *****
****************************/

void mmx_merge_luma( unsigned int *src, unsigned int *luma, int pitch, 
                     int luma_pitch,int width, int height ) 
{
	__int64 I1=0x00ff00ff00ff00ff;  // Luma mask
	__int64 I2=0xff00ff00ff00ff00;  // Chroma mask
  // [V][Y2][U][Y1]

	int fraction=width&7;       // fraction = w%8
	int lwidth=width-fraction;  // width handled by assembler loop
	int lwidth_bytes=lwidth*2;
	unsigned int *tlu;
	unsigned int *t_src=src;
  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
  }
	for (int y=0;y<height;y++) {
	  tlu=&luma[(y*luma_pitch)];  // offset

		// eax=src
		// ebx=luma
		// ecx:src offset
		// edx:luma offset
		
	__asm {
		mov eax,src
		mov ecx,0
		mov ebx,tlu
		mov edx,0
		jmp afterloop
		align 16
goloop:
		add ecx,16   // 16 bytes per pass = 8 pixels = 2 quadwords
		add edx,16    //  16 bytes per pass = 8 pixels = 2 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true
#ifdef ATHLON
	prefetchw [eax+ecx]
	prefetch [ebx+edx]
#endif

		; Processes 8 pixels at the time 
		movq mm0,[eax+ecx]		; chroma 4 pixels
    movq mm1,[eax+ecx+8]  ; chroma next 4 pixels
    pand mm0,mm6
    movq mm2,[ebx+edx]  ; load luma 4 pixels
    pand mm1,mm6
    movq mm3,[ebx+edx+8]  ; load luma next 4 pixels
    pand mm2,mm7
    pand mm3,mm7
    por mm0,mm2
    por mm1,mm3
    movq [eax+ecx],mm0
    movq [eax+ecx+8],mm1
		jmp goloop
outloop:				
		} // end __asm	
/*		if (fraction) {  // pixel missing  -- wrong code!
			int cpix=*(t_src+lwidth);
			short *tlu2=(int *)&dst[y*width+lwidth];
			*tlu2++=(cpix&0xff)<<8;
			*tlu2++=(cpix&0xff00);
			*tlu2++=(cpix&0xff0000)>>8;
			*tlu2++=256;
		}
*/
	
		src+=pitch;
	} // end for y
}





void mmx_weigh_luma( unsigned int *src,unsigned int *luma, int pitch, 
                     int luma_pitch,int width, int height, int weight, int invweight ) 
{
	__int64 I1=0x00ff00ff00ff00ff;  // Luma mask
	__int64 I2=0xff00ff00ff00ff00;  // Chroma mask
  // [V][Y2][U][Y1]

	int fraction=width&3;       // fraction = w%4
	int lwidth=width-fraction;  // width handled by assembler loop
	int lwidth_bytes=lwidth*2;
	unsigned int *tlu;
	unsigned int *t_src=src;
  __int64 weight64  = (__int64)invweight | (((__int64)weight)<<16)| (((__int64)invweight)<<32) |(((__int64)weight)<<48);

  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
    movq mm5,[weight64] ; Weight
  }
	for (int y=0;y<height;y++) {
	  tlu=&luma[(y*luma_pitch)];  // offset

		// eax=src
		// ebx=luma
		// ecx:src offset
		// edx:luma offset
		
	__asm {
		mov eax,src
		mov ecx,0
		mov ebx,tlu
		mov edx,0
		jmp afterloop
		align 16
goloop:
		add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
		add edx,8    //  8 bytes per pass = 4 pixels = 1 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true
#ifdef ATHLON
	prefetchw [eax+ecx]
	prefetch [ebx+edx]
#endif

		; Processes 4 pixels at the time mm0=org luma mm1=temp unpack
    movq mm2,[ebx+edx]    ; load 4 pixels
    pxor mm3,mm3
		movq mm0,[eax+ecx]		; original 4 pixels   (cc)
    pand mm2,mm6          ; mm2 = chroma | mm2= CC00 CC00 CC00 CC00
    pxor mm1,mm1
    movq mm4,mm0          ; move original pixel into mm4
    punpckhbw mm3,mm2     ; Unpack upper luma to mm3 | mm3= LL00 0000 0000 LL00
    pand mm4,mm6          ; mm4 = original chroma | mm4 = cc00 cc00 cc00 cc00
    psrlq mm3,8           ; Move incoming chroma down | mm3 = 00CC 0000 00CC 0000 
    punpckhbw mm1,mm4     ; Unpack original upper luma to mm1 | mm1= cc00 0000 cc00 0000 np    
    psrlq mm1,24           ; Move incoming luma down | mm1 = 0000 00cc 0000 00cc  np
    por mm1,mm3           ; mm1=00LL 00ll 00LL 00ll np
    pxor mm3,mm3          ; Clear mm3 np
    pmaddwd mm1, mm5      ; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
    punpcklbw mm3,mm4     ; Unpack lower original chroma to mm3 | mm3= cc00 0000 cc00 0000  | mm4 free
    pxor mm4,mm4
    psrlq mm3,24          ; align mm3 
    punpcklbw mm4,mm2     ; Unpack lower luma to mm4 | mm4= CC00 0000 CC00 0000  | mm0 luma free
    psrlq mm4,8           ; align mm4
    pand mm0,mm7          ; mm0 = original luma
    por mm3,mm4           ; mm3=00LL 00ll 00LL 00ll np    
    psrld mm1,15          ; Divide with total weight (=15bits) mm1 = 0000 00CC 0000 00CC
    pmaddwd mm3, mm5      ; Mult with weights and add. Latency 2 cycles - mult unit cannot be used.  mm0, mm3 & mm1 used
    pshufw mm1,mm1,133    ; Move luma to proper place. 10000101 = 133 mm1= 00CC 00CC 0000 0000
    pslld mm1,8           ; Move chroma up mm1 = 0000 CC00 0000 CC00
    psrld mm3,15          ; Divide with total weight (=15bits) mm3 = 0000 00LL 0000 00LL
    por mm0,mm1
    pshufw mm3,mm3,88     ; Move luma to proper place. 01011000 = 88 mm3= 0000 0000 00LL 00LL
    pslld mm3,8           ; Move chroma up mm1 = 0000 CC00 0000 CC00
    por mm0,mm3           ; Or into mm0 - mm0 finished
    movq [eax+ecx],mm0
    
		jmp goloop
outloop:				
		} // end __asm	
/*		if (fraction) {  // pixel missing  -- wrong code!
			int cpix=*(t_src+lwidth);
			short *tlu2=(int *)&dst[y*width+lwidth];
			*tlu2++=(cpix&0xff)<<8;
			*tlu2++=(cpix&0xff00);
			*tlu2++=(cpix&0xff0000)>>8;
			*tlu2++=256;
		}
*/
		src+=pitch;
	} // end for y
}
