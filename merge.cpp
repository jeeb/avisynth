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
  { "MergeChroma", "cc[chromaweight]f", MergeChroma::Create },  // src, chroma src, weight
  { "MergeLuma", "cc[lumaweight]f", MergeLuma::Create },      // src, luma src, weight
  { 0 }
};





/****************************
******   Merge Chroma   *****
****************************/

MergeChroma::MergeChroma(PClip _child, PClip _clip, float _weight, IScriptEnvironment* env) 
  : GenericVideoFilter(_child), clip(_clip), weight(_weight)
{
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsYUY2() || !vi2.IsYUY2())
		env->ThrowError("MergeChroma: YUY2 data only (no RGB); use ConvertToYUY2");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeChroma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;

  if (weight>=1.0f && !(env->GetCPUFlags() & CPUF_MMX))
      env->ThrowError("MergeChroma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
}



PVideoFrame __stdcall MergeChroma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame chroma = clip->GetFrame(n, env);
  
  const int h = src->GetHeight();
  const int w = src->GetRowSize()>>1; // width in pixels
  
  if (weight<1.0f) {
    env->MakeWritable(&src);
    unsigned int* srcp = (unsigned int*)src->GetWritePtr();
    unsigned int* chromap = (unsigned int*)chroma->GetReadPtr();

    const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
    const int ichroma_pitch = (chroma->GetPitch())>>2;  // Ints
  
    (env->GetCPUFlags() & CPUF_INTEGER_SSE ? isse_weigh_chroma : weigh_chroma)
							(srcp,chromap,isrc_pitch,ichroma_pitch,w,h,(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
  }
  else
  {
    unsigned int* srcp = (unsigned int*)src->GetReadPtr();
    env->MakeWritable(&chroma);
    unsigned int* chromap = (unsigned int*)chroma->GetWritePtr();

    const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
    const int ichroma_pitch = (chroma->GetPitch())>>2;  // Ints

    mmx_merge_luma(chromap,srcp,ichroma_pitch,isrc_pitch,w,h);  // Just swap luma/chroma
    return chroma;
  }

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
  const VideoInfo& vi2 = clip->GetVideoInfo();

  if (!vi.IsYUY2() || !vi.IsYUY2())
    env->ThrowError("MergeLuma: YUY2 data only (no RGB); use ConvertToYUY2");

  if (vi.width!=vi2.width || vi.height!=vi2.height)
    env->ThrowError("MergeLuma: Images must have same width and height!");

  if (weight<0.0f) weight=0.0f;
  if (weight>1.0f) weight=1.0f;

  if (weight>=1.0f && !(env->GetCPUFlags() & CPUF_MMX))
      env->ThrowError("MergeLuma: MMX required (K6, K7, P5 MMX, P-II, P-III or P4)");
}
    

PVideoFrame __stdcall MergeLuma::GetFrame(int n, IScriptEnvironment* env)
{
  
  PVideoFrame src = child->GetFrame(n, env);
  PVideoFrame luma = clip->GetFrame(n, env);
  
  env->MakeWritable(&src);
  unsigned int* srcp = (unsigned int*)src->GetWritePtr();
  unsigned int* lumap = (unsigned int*)luma->GetReadPtr();

  const int isrc_pitch = (src->GetPitch())>>2;  // int pitch (one pitch=two pixels)
  const int iluma_pitch = (luma->GetPitch())>>2;  // Ints
  
  const int h = src->GetHeight();
  const int w = src->GetRowSize()>>1; // width in pixels
    
  if (weight<1.0f)
    (env->GetCPUFlags() & CPUF_INTEGER_SSE ? isse_weigh_luma : weigh_luma)
							(srcp,lumap,isrc_pitch,iluma_pitch,w,h,(int)(weight*32767.0f),32767-(int)(weight*32767.0f));
  else
		mmx_merge_luma(srcp,lumap,isrc_pitch,iluma_pitch,w,h);
  return src;
}


AVSValue __cdecl MergeLuma::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
  return new MergeLuma(args[0].AsClip(), args[1].AsClip(), args[2].AsFloat(1.0f), env);
}





/****************************
******    C routines    *****
****************************/


void weigh_luma(unsigned int *src,unsigned int *luma, int pitch, int luma_pitch,int width, int height, int weight, int invweight) {

	int lwidth=width>>1;

	for (int y=0;y<height;y++) {
	  unsigned int *tlu=&luma[y*luma_pitch];  // offset
		unsigned int *src2=src;
		for (int x=0;x<lwidth;x++) {
			unsigned int lumapix=*tlu++;
			unsigned int destpix=*src2;
			unsigned int luma1=lumapix&0xff;
			unsigned int luma2=(lumapix&0xff0000)>>16;

			unsigned int orgluma1=destpix&0xff;
			unsigned int orgluma2=(destpix&0xff0000)>>16;
			
			*src2++ = ( destpix & 0xff00ff00 ) |
				( ( luma1 * weight + orgluma1 * invweight + 16384 ) >> 15 ) |
				( ( ( luma2 * weight + orgluma2 * invweight + 16384 ) << 1 ) &0xff0000 );  // Insert new luma values
		}
		src+=pitch;
	} // end for y
}




void weigh_chroma(unsigned int *src,unsigned int *chroma, int pitch, int chroma_pitch,int width, int height, int weight, int invweight) {

	int lwidth=width>>1;

	for (int y=0;y<height;y++) {
	  unsigned int *tch=&chroma[y*chroma_pitch];  // offset
		unsigned int *src2=src;
		for (int x=0;x<lwidth;x++) {
			unsigned int chromapix=*tch++;
			unsigned int destpix=*src2;
			unsigned int chroma1=(chromapix&0xff00)>>8;
			unsigned int chroma2=(chromapix&0xff000000)>>24;

			unsigned int orgchroma1=(destpix&0xff00)>>8;
			unsigned int orgchroma2=(destpix&0xff000000)>>24;
			
			*src2++ = ( destpix & 0x00ff00ff ) |
				( ( ( chroma1 * weight + orgchroma1 * invweight + 16384 ) >> 7 ) & 0xff00 ) |
				( ( ( chroma2 * weight + orgchroma2 * invweight + 16384 ) << 9 ) & 0xff000000 );  // Insert new chroma values
		}
		src+=pitch;
	} // end for y
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

  int row_size = width * 2;
	int lwidth_bytes = row_size & -16;	// bytes handled by the MMX loop
  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		mov ecx,0
		mov ebx,luma
		jmp afterloop
		align 16
goloop:
		add ecx,16   // 16 bytes per pass = 8 pixels = 2 quadwords
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true

#ifdef ATHLON
	prefetchw [eax+ecx]
	prefetch [ebx+ecx]
#endif

		; Processes 8 pixels at the time 
		movq mm0,[eax+ecx]		; chroma 4 pixels
    movq mm1,[eax+ecx+8]  ; chroma next 4 pixels
    pand mm0,mm6
    movq mm2,[ebx+ecx]  ; load luma 4 pixels
    pand mm1,mm6
    movq mm3,[ebx+ecx+8]  ; load luma next 4 pixels
    pand mm2,mm7
    pand mm3,mm7
    por mm0,mm2
    por mm1,mm3
    movq [eax+ecx],mm0
    movq [eax+ecx+8],mm1
		jmp goloop
outloop:
		; processes remaining pixels
		cmp ecx,[row_size]
		jge exitloop
		mov esi,[eax+ecx]
		mov edi,[ebx+ecx]
		and esi,0xff00ff00
		and	edi,0x00ff00ff
		or esi,edi
		mov [eax+ecx],esi
		add ecx,4
		jmp outloop
exitloop:
		} 
	
		src += pitch;
		luma += luma_pitch;
	} // end for y
  __asm {emms};
}




void isse_weigh_luma(unsigned int *src,unsigned int *luma, int pitch,
										int luma_pitch,int width, int height, int weight, int invweight)
{
	__int64 I1=0x00ff00ff00ff00ff;  // Luma mask
	__int64 I2=0xff00ff00ff00ff00;  // Chroma mask
  // [V][Y2][U][Y1]

  int row_size = width * 2;
	int lwidth_bytes = row_size & -8;	// bytes handled by the main loop
  __int64 weight64  = (__int64)invweight | (((__int64)weight)<<16)| (((__int64)invweight)<<32) |(((__int64)weight)<<48);
  // weight LLLL llll LLLL llll
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
		movq mm7,[I1]     ; Luma
		movq mm6,[I2]     ; Chroma
    movq mm5,[weight64] ; Weight
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		mov ecx,0
		mov ebx,luma
		jmp afterloop
		align 16
goloop:
		add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true

#ifdef ATHLON
	prefetchw [eax+ecx]
	prefetch [ebx+ecx]
#endif

		; Processes 4 pixels at the time
		movq mm0,[eax+ecx]		; original 4 pixels   (cc)
    movq mm2,[ebx+ecx]    ; load 4 pixels
    movq mm3,mm0          ; move original pixel into mm3
    punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		movq mm1,mm0
		pand mm3,mm7					; mm3= 00LL 00ll 00LL 00ll
    pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		pand mm0,mm6					; mm0= cc00 cc00 cc00 cc00
		pand mm1,mm7
		paddd mm3,mm4					; round to nearest
		psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00LL 0000 00LL
		pmaddwd mm1,mm5
		paddd mm1,mm4					; round to nearest
		psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00LL 0000 00LL
		packssdw mm1, mm3			; mm1 = 00LL 00LL 00LL 00LL
		por mm0,mm1
    movq [eax+ecx],mm0
  
		jmp goloop
outloop:				
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm0,[eax+ecx]			; original 2 pixels
		movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm1,mm0
		punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		pand mm1,mm7						; mm1= 00LL 00ll 00LL 00ll
		pmaddwd mm1,mm5
		pand mm0,mm6						; mm0= 0000 0000 cc00 cc00
		paddd mm1,mm4						; round to nearest
		psrld mm1,15						; mm1= 0000 00LL 0000 00LL
		packssdw mm1,mm1				; mm1= 00LL 00LL 00LL 00LL
		por mm0,mm1							; mm0 finished
		movd [eax+ecx],mm0
		// no loop since there is at most 2 remaining pixels
exitloop:
		} 
		src += pitch;
		luma += luma_pitch;
	} // end for y
  __asm {emms};
}





void isse_weigh_chroma( unsigned int *src,unsigned int *chroma, int pitch, 
                     int chroma_pitch,int width, int height, int weight, int invweight ) 
{
	__int64 I1=0x00ff00ff00ff00ff;  // Luma mask
//__int64 I2=0xff00ff00ff00ff00;  // Chroma mask // unused
  // [V][Y2][U][Y1]

  int row_size = width * 2;
	int lwidth_bytes = row_size & -8;	// bytes handled by the main loop
  __int64 weight64  = (__int64)invweight | (((__int64)weight)<<16) | (((__int64)invweight)<<32) |(((__int64)weight)<<48);
	__int64 rounder = 0x0000400000004000;		// (0.5)<<15 in each dword

  __asm {
		movq mm7,[I1]     ; Luma
//		movq mm6,[I2]     ; Chroma // unused
    movq mm5,[weight64] ; Weight
		movq mm4,[rounder]

  }
	for (int y=0;y<height;y++) {

		// eax=src
		// ebx=luma
		// ecx=src/luma offset
		
	__asm {
		mov eax,src
		mov ecx,0
		mov ebx,chroma
		jmp afterloop
		align 16
goloop:
		add ecx,8   // 8 bytes per pass = 4 pixels = 1 quadword
afterloop:
		cmp       ecx,[lwidth_bytes]	; Is eax(i) greater than endx
		jge       outloop		; Jump out of loop if true

#ifdef ATHLON
	prefetchw [eax+ecx]
	prefetch [ebx+ecx]
#endif

		; Processes 4 pixels at the time
		movq mm0,[eax+ecx]		; original 4 pixels   (cc)
    movq mm2,[ebx+ecx]    ; load 4 pixels
    movq mm3,mm0          ; move original pixel into mm3
    punpckhwd mm3,mm2     ; Interleave upper pixels in mm3 | mm3= CCLL ccll CCLL ccll
		movq mm1,mm0
		psrlw mm3,8						; mm3= 00CC 00cc 00CC 00cc
    pmaddwd mm3,mm5				; Mult with weights and add. Latency 2 cycles - mult unit cannot be used
		punpcklwd mm1,mm2     ; Interleave lower pixels in mm1 | mm1= CCLL ccll CCLL ccll
		pand mm0,mm7					; mm0= 00ll 00ll 00ll 00ll
		psrlw mm1,8
		paddd mm3,mm4					; round to nearest
		psrld mm3,15					; Divide with total weight (=15bits) mm3 = 0000 00CC 0000 00CC
		pmaddwd mm1,mm5
		paddd mm1,mm4					; round to nearest
		psrld mm1,15					; Divide with total weight (=15bits) mm1 = 0000 00CC 0000 00CC
		packssdw mm1, mm3			; mm1 = 00CC 00CC 00CC 00CC
		pslld mm1,8
		por mm0,mm1
    movq [eax+ecx],mm0

		jmp goloop
outloop:				
		// processes remaining pixels here
		cmp ecx,[row_size]
		jge	exitloop
		movd mm0,[eax+ecx]			; original 2 pixels
		movd mm2,[ebx+ecx]			; luma 2 pixels
		movq mm1,mm0
		punpcklwd mm1,mm2				; mm1= CCLL ccll CCLL ccll
		psrlw mm1,8							; mm1= 00CC 00cc 00CC 00cc
		pmaddwd mm1,mm5
		pand mm0,mm7						; mm0= 0000 0000 00ll 00ll
		paddd mm1,mm4						; round to nearest
		psrld mm1,15						; mm1= 0000 00CC 0000 00CC
		packssdw mm1,mm1
		pslld mm1,8							; mm1= CC00 CC00 CC00 CC00
		por mm0,mm1							; mm0 finished
		movd [eax+ecx],mm0
		// no loop since there is at most 2 remaining pixels
exitloop:
		}
		src += pitch;
		chroma += chroma_pitch;
	} // end for y
  __asm {emms};
}
