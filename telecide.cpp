/*
	Telecide plugin for Avisynth -- recovers original progressive
	frames from a telecined stream. The filter operates by matching
	fields and automatically adapts to phase/pattern changes. It
	operates on any arbitrary telecine pattern, e.g., 3:2 NTSC or
	1:1 PAL. An optional postprocessing phase deinterlaces frames
	that emerge from the field matching still combed.

	Copyright (C) 2002 Donald A. Graft

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	The author can be contacted at:
	Donald Graft
	neuron2@attbi.com.
*/


#include "telecide.h"


Telecide::Telecide( PClip _child, bool _reverse, bool _swap, bool _firstlast, int _guide,
					          int _gthresh, bool _postprocess, int _threshold, int _dthreshold, bool _blend,
					          bool _chroma, int _y0, int _y1, bool _debug, IScriptEnvironment* env ) 
  : GenericVideoFilter(_child), reverse(_reverse), swap(_swap), firstlast(_firstlast),
	  guide(_guide), gthresh(_gthresh), postprocess(_postprocess), threshold(_threshold), 
    dthreshold(_dthreshold), blend(_blend), chroma(_chroma), y0(_y0), y1(_y1), debug(_debug)
{
	if (!vi.IsYUY2())
		env->ThrowError("Telecide: YUY2 data only (no RGB); use ConvertToYUY2");
	if (debug)
	{
		char b[80];
		sprintf(b, "Telecide %s by Donald Graft, Copyright 2002\n", VERSION);
		OutputDebugString(b);
	}
}



/***************************
 *******   GetFrame   ******
 **************************/

PVideoFrame __stdcall Telecide::GetFrame(int frame, IScriptEnvironment* env)
{
	int p, c, n, lowest;
#ifndef MATCH_MMX_BUILD
	int C;
	int comb;
#endif
    BYTE *curr, *pprev, *pnext, *cprev, *cnext, *nprev, *nnext;
	BYTE *PP, *P, *N, *NN;
#ifndef BLEND_MMX_BUILD
	unsigned int p0;
#endif
	unsigned int p1, p2;
    int hminus2, hover2, hover4minus2, hplus1over2;
	int x, y, val;
	int chosen;

	/* It's possible to get a bad match on the first and last frames. Interpolate them
	   if that's what the user wants. */
	if (firstlast && (frame == 0 || frame == vi.num_frames - 1))
	{
		if (debug)
		{
			char buf[80];
			sprintf(buf, "Interpolating frame %d\n", frame);
			OutputDebugString(buf);
		}

		fc = child->GetFrame(frame, env);
		dst = env->NewVideoFrame(vi);
		pitch = fc->GetPitch();
		dpitch = dst->GetPitch();
		w = fc->GetRowSize();
		if ((w/2) % 4)
			env->ThrowError("Telecide: width must be a multiple of 4; use Crop");
		h = fc->GetHeight();
		hover2 = h/2;

		if (reverse == true)
		{
			/* Copy through the even lines and the last odd line. */
			fcrp = fcrp_saved = (BYTE *) fc->GetReadPtr();
			dstp = dstp_saved = (BYTE *) dst->GetWritePtr();
			for (y = 0; y < hover2; y++)
			{
				memcpy(dstp, fcrp, w);
				dstp += dpitch << 1;
				fcrp += pitch << 1;
			}
			fcrp -= pitch;
			dstp -= dpitch;
			memcpy(dstp, fcrp, w);

			/* Use cubic for all but the two outside lines. */
			PP  = fcrp_saved - 2 * pitch;
			P  = fcrp_saved;
			dstp = dstp_saved + dpitch;
			N  = fcrp_saved + 2 * pitch;
			NN  = fcrp_saved + 4 * pitch;
			for (y = 0; y < hover2 - 1; y++)
			{
				if (y == 0 || y == hover2 - 2)
				{
					for (x = 0; x < w; x++)
					{
						dstp[x] = ((int)P[x] + (int)N[x]) >> 1;
					}
				}
				else
				{
					for (x = 0; x < w; x++)
					{
						val = (26 * ((int)P[x] + (int)N[x]) - 6 * ((int)PP[x] + (int)NN[x])) / 40;
						if (val > 255) val = 255;
						else if (val < 0) val = 0;
						dstp[x] = val;
					}
				}
				dstp += dpitch << 1;
				P    += pitch << 1;
				N    += pitch << 1;
				PP    += pitch << 1;
				NN    += pitch << 1;
			}
		}
		else
		{
			/* Copy through the odd lines and the first even line. */
			fcrp = fcrp_saved = (BYTE *) fc->GetReadPtr();
			dstp = dstp_saved = (BYTE *) dst->GetWritePtr();
			memcpy(dstp, fcrp, w);
			fcrp += pitch;
			dstp += dpitch;
			for (y = 0; y < hover2; y++)
			{
				memcpy(dstp, fcrp, w);
				dstp += dpitch << 1;
				fcrp += pitch << 1;
			}

			/* Use cubic for all but the two outside lines. */
			PP  = fcrp_saved - pitch;
			P  = fcrp_saved + pitch;
			dstp = dstp_saved + 2 * dpitch;
			N  = fcrp_saved + 3 * pitch;
			NN  = fcrp_saved + 5 * pitch;
			for (y = 1; y < hover2; y++)
			{
				if (y == 1 || y == hover2 - 1)
				{
					for (x = 0; x < w; x++)
					{
						dstp[x] = ((int)P[x] + (int)N[x]) >> 1;
					}
				}
				else
				{
					for (x = 0; x < w; x++)
					{
						val = (26 * ((int)P[x] + (int)N[x]) - 6 * ((int)PP[x] + (int)NN[x])) / 40;
						if (val > 255) val = 255;
						else if (val < 0) val = 0;
						dstp[x] = val;
					}
				}
				dstp += dpitch << 1;
				P    += pitch << 1;
				N    += pitch << 1;
				PP    += pitch << 1;
				NN    += pitch << 1;
			}
		}

		return dst;
	}

	/* Do field matching. */
	fp = child->GetFrame(frame == 0 ? 0 : frame - 1, env);
	fc = child->GetFrame(frame, env);
	fn = child->GetFrame(frame >= vi.num_frames - 1 ? vi.num_frames - 1: frame + 1, env);
	dst = env->NewVideoFrame(vi);

	pitch = fc->GetPitch();
	pitchtimes4 = pitch << 2;
	dpitch = dst->GetPitch();
	w = fc->GetRowSize();
	if ((w/2) % 4)
		env->ThrowError("Telecide: width must be a multiple of 4; use Crop");
	h = fc->GetHeight();
	if (y0 < 0 || y1 < 0 || y0 > y1)
		env->ThrowError("Telecide: bad y0/y1 values");
	hminus2 = h - 2;
	hover2 = h/2;
	hover4minus2 = h/4 - 2;
	hplus1over2 = (h+1)/2;

	fprp = (BYTE *) fp->GetReadPtr();
	fcrp = (BYTE *) fc->GetReadPtr();
	fnrp = (BYTE *) fn->GetReadPtr();

	if (reverse == true)
	{
		curr =  (BYTE *) (fcrp + 2 * pitch);
		pprev = (BYTE *) (fprp + pitch);
		pnext = (BYTE *) (fprp + 3 * pitch);
		cprev = (BYTE *) (fcrp + pitch);
		cnext = (BYTE *) (fcrp + 3 * pitch);
		nprev = (BYTE *) (fnrp + pitch);
		nnext = (BYTE *) (fnrp + 3 * pitch);
	}
	else
	{
		curr =  (BYTE *) (fcrp + pitch);
		pprev = (BYTE *) (fprp);
		pnext = (BYTE *) (fprp + 2 * pitch);
		cprev = (BYTE *) (fcrp);
		cnext = (BYTE *) (fcrp + 2 * pitch);
		nprev = (BYTE *) (fnrp);
		nnext = (BYTE *) (fnrp + 2 * pitch);
	}
	p = c = n = 0;
	/* Try to match the top field of the current frame to the
	   bottom fields of the previous, current, and next frames.
	   Output the assembled frame that matches up best. For
	   matching, subsample the frames in the x dimension
	   for speed. */
	for (y = 0; y < hminus2; y+=4)
	{
		/* Exclusion band. Good for ignoring subtitles. */
		if (y0 != y1 && y >= y0 && y <= y1) continue;
#ifdef MATCH_MMX_BUILD
		p += asm_match(curr, pprev, pnext, w / 16);
		c += asm_match(curr, cprev, cnext, w / 16);
		n += asm_match(curr, nprev, nnext, w / 16);
#else
		for (x = 0; x < w;)
		{
			C = curr[x];
#define T 100
			/* This combing metric is based on an original idea of Gunnar Thalin. */
			comb = ((long)pprev[x] - C) * ((long)pnext[x] - C);
			if (comb > T) p++;

			comb = ((long)cprev[x] - C) * ((long)cnext[x] - C);
			if (comb > T) c++;

			comb = ((long)nprev[x] - C) * ((long)nnext[x] - C);
			if (comb > T) n++;

			if (!(++x&3)) x += 12;
		}
#endif
		curr  += pitchtimes4;
		pprev += pitchtimes4;
		pnext += pitchtimes4;
		cprev += pitchtimes4;
		cnext += pitchtimes4;
		nprev += pitchtimes4;
		nnext += pitchtimes4;
	}
#ifdef MATCH_MMX_BUILD
	__asm emms;
#endif

	/* The idea of guiding blind field matching with the prevailing
	   pattern was pioneered by Alan Liu ("daxab"). */
	if (guide == 1)
	{
		/* NTSC guidance. */
		predicted = 0xff;
		if (store[4].frame == frame - 1 &&
			store[3].frame == frame - 2 &&
			store[2].frame == frame - 3 &&
			store[1].frame == frame - 4 &&
			store[0].frame == frame - 5)
		{
			switch ((store[0].choice << 16) +
					(store[1].choice << 12) +
					(store[2].choice <<  8) +
					(store[3].choice <<  4) +
					(store[4].choice))
			{
			case 0x11100:
			case 0x11001:
			case 0x10011:
				predicted = 1;
				break;
			case 0x00111:
			case 0x01110:
				predicted = 0;
				break;
			case 0x11122:
			case 0x11221:
			case 0x12211:
				predicted = 1;
				break;
			case 0x22111:
			case 0x21112:
				predicted = 2;
				break;
			}
		}

		store[0] = store[1];
		store[1] = store[2];
		store[2] = store[3];
		store[3] = store[4];
		store[4].frame = frame;
	}
	else if (guide == 2)
	{
		/* PAL guidance. */
		predicted = 0xff;
		if (store[2].frame == frame - 1 &&
			store[1].frame == frame - 2 &&
			store[0].frame == frame - 3)
		{
			switch ((store[0].choice << 8) +
					(store[1].choice << 4) +
					(store[2].choice))
			{
			case 0x000:
				predicted = 0;
				break;
			case 0x111:
				predicted = 1;
				break;
			case 0x222:
				predicted = 2;
				break;
			}
		}

		store[0] = store[1];
		store[1] = store[2];
		store[2].frame = frame;
	}
	lowest = c;
	chosen = 1;
	if (guide == 1) store[4].choice = 1;
	else if (guide == 2) store[2].choice = 1;
	if (p < lowest)
	{
		lowest = p;
		chosen = 0;
		if (guide == 1) store[4].choice = 0;
		else if (guide == 2) store[2].choice = 0;
	}
	if (n < lowest)
	{
		lowest = n;
		chosen = 2;
		if (guide == 1) store[4].choice = 2;
		else if (guide == 2) store[2].choice = 2;
	}
	if (guide == 1 || guide == 2)
	{
		if (predicted != 0xff && predicted != chosen)
		{
			int val;
			switch (predicted)
			{
			case 0: val = p; break;
			case 1: val = c; break;
			case 2: val = n; break;
			}
			/* If the predicted match metric is within defined percentage of the
			   best calculated one, then override the calculated match with the
			   predicted match. */
			if (abs(val - lowest) < (val * gthresh) / 100)
			{
				if (debug)
				{
					char buf[80];
					sprintf(buf,"override %d: val=%d lowest=%d\n", frame, val, lowest);
					OutputDebugString(buf);
				}
				chosen = predicted;
				if (guide == 1) store[4].choice = predicted;
				else if (guide == 2) store[2].choice = predicted;
			}
		}
	}

	if (debug)
	{
		char buf[80];
		sprintf(buf,"frame %d: p=%u  c=%u  n=%u [using %d]\n", frame, p, c, n, chosen);
		OutputDebugString(buf);
		if (guide == 1)
		{
			sprintf(buf,"frame %d: %d %d %d %d %d\n", frame,
					store[0].choice, store[1].choice, store[2].choice,
					store[3].choice, store[4].choice);
			OutputDebugString(buf);
		}
		else if (guide == 2)
		{
			sprintf(buf,"frame %d: %d %d %d\n", frame,
					store[0].choice, store[1].choice, store[2].choice);
			OutputDebugString(buf);
		}
	}

	/* Set up the pointers in preparation to output final frame. */
	dstp_saved = dstp = dst->GetWritePtr();
	if (reverse == true) dstp += dpitch;
	if (swap == true) dstp = (dstp == dstp_saved ? dstp_saved + dpitch : dstp_saved);
	if (chosen == 0) curr = fprp;
	else if (chosen == 1) curr = fcrp;
	else curr = fnrp;
	if (reverse == true) curr += pitch;

	/* First output the bottom field selected from the set of three stored
	   frames. */
	for (y = 0; y < hplus1over2; y++)
	{
		memcpy(dstp, curr, w);
		curr += pitch << 1;
		dstp += dpitch << 1;
	}

	/* Now output the top field of the current frame. */
	if (reverse == true)
	{
		curr = (BYTE *) (fcrp);
		dstp = dstp_saved;
	}
	else
	{
		curr = (BYTE *) (fcrp + pitch);
		dstp = dstp_saved + dpitch;
	}
	if (swap == true) dstp = (dstp == dstp_saved ? dstp_saved + dpitch : dstp_saved);
	for (y = 0; y < hover2; y++)
	{
		memcpy(dstp, curr, w);
		curr += pitch << 1;
		dstp += dpitch << 1;
	}

	if (postprocess == false) return dst;

	/* Do postprocessing. */
	dstp = dstp_saved;
	fmask = env->NewVideoFrame(vi);
	fmaskp_saved = fmaskp = fmask->GetWritePtr();

	dmask = env->NewVideoFrame(vi);
	dmaskp_saved = dmaskp = dmask->GetWritePtr();

	if (Analyse(frame, env) == false)
		return dst;  // clean frame; return original

	/* Frame is combed. */
	PVideoFrame final = env->NewVideoFrame(vi);

	dstp = dstp_saved;
	dmaskp = dmaskp_saved;
	finalp = final->GetWritePtr();
	cprev = dstp - dpitch;
	cnext = dstp + dpitch;
	if (blend)
	{
		/* First line. */
		for (x=0; x<w; x+=4)
		{
			p1 = *(int *)(dstp+x) & 0xfefefefe;
			p2 = *(int *)(cnext+x) & 0xfefefefe;
			*(int *)(finalp+x) = (p1>>1) + (p2>>1);
		}
		dstp += dpitch;
		dmaskp += dpitch;
		finalp += dpitch;
		cprev += dpitch;
		cnext += dpitch;
		for (y = 1; y < h - 1; y++)
		{
#ifdef BLEND_MMX_BUILD
			asm_blend(dstp, cprev, cnext, finalp, dmaskp, w/8);
#else
			for (x=0; x<w; x+=4)
			{
				if (dmaskp[x])
				{
					p0 = *(int *)(dstp+x) & 0xfefefefe;
					p1 = *(int *)(cprev+x) & 0xfcfcfcfc;
					p2 = *(int *)(cnext+x) & 0xfcfcfcfc;
					*(int *)(finalp+x) = (p0>>1) + (p1>>2) + (p2>>2);
				}
				else
				{
					*(int *)(finalp+x)=*(int *)(dstp+x);
				}
			}
#endif
			dstp += dpitch;
			dmaskp += dpitch;
			finalp += dpitch;
			cprev += dpitch;
			cnext += dpitch;
		}
        /* Last line. */
		for (x=0; x<w; x+=4)
		{
			p1 = *(int *)(dstp+x) & 0xfefefefe;
			p2 = *(int *)(cprev+x) & 0xfefefefe;
			*(int *)(finalp+x) = (p1>>1) + (p2>>1);
		}
#ifdef BLEND_MMX_BUILD
		__asm emms;
#endif
	}
	else
	{
		for (y = 0; y < h - 1; y++)
		{
			if (!(y&1))
			{
				memcpy(finalp,dstp,w);
			}
			else
			{
#ifdef INTERPOLATE_MMX_BUILD
			asm_interpolate(dstp, cprev, cnext, finalp, dmaskp, w/8);
#else
				for (x = 0; x < w; x += 4)
				{
					if (dmaskp[x])
					{
						p1 = *(int *)(cprev+x) & 0xfefefefe;
						p2 = *(int *)(cnext+x) & 0xfefefefe;
						*(int *)(finalp+x) = (p1>>1) + (p2>>1);
					}
					else
					{
						*(int *)(finalp+x) = *(int *)(dstp+x);
					}
				}
#endif
			}
			dstp += dpitch;
			dmaskp += dpitch;
			finalp += dpitch;
			cprev += dpitch;
			cnext += dpitch;
		}
		/* Last line. */
		memcpy(finalp,dstp,w);
#ifdef INTERPOLATE_MMX_BUILD
		__asm emms;
#endif
	}

	return final;
}







/*************************
 *******   Create   ******
 ************************/

AVSValue __cdecl Telecide::Create(AVSValue args, void* user_data, IScriptEnvironment* env) 
{
  return new Telecide( args[0].AsClip(),
	  			             args[1].AsBool(false),	  // reverse
					             args[2].AsBool(false),	  // swap
					             args[3].AsBool(false),	  // firstlast
					             args[4].AsInt(0),		    // guide
					             args[5].AsInt(15),		    // gthresh
					             args[6].AsBool(true),	  // postprocess
					             args[7].AsInt(15),		    // threshold
					             args[8].AsInt(9),		    // dthreshold
					             args[9].AsBool(true),	  // blend
					             args[10].AsBool(false),	// chroma
					             args[11].AsInt(0),		    // y0
					             args[12].AsInt(0),		    // y1
					             args[13].AsBool(false),	// debug
					             env );
}






/**************************
 *******   Analyze   ******
 *************************/

bool Telecide::Analyse(int frame, IScriptEnvironment* env)
{
	int x, y, thresh, dthresh;
	int *counts, box, boxy;
#ifndef DEINTERLACE_MMX_BUILD
	int val;
#endif
	const CT = 15;
	int boxarraysize;

	thresh = threshold*threshold;
	dthresh = dthreshold*dthreshold;

	p = dstp - dpitch;
	n = dstp + dpitch;
	memset(fmaskp, 0, w);
	for (x = 0; x < w; x += 4) *(int *)(dmaskp+x) = 0x00ff00ff;

	for (y = 1; y < h - 1; y++)
	{
		fmaskp += dpitch;
		dmaskp += dpitch;
		dstp += dpitch;
		p += dpitch;
		n += dpitch;
#ifdef DEINTERLACE_MMX_BUILD
		if (chroma)
			asm_deinterlace_chroma(dstp, p, n, fmaskp, dmaskp, thresh+1, dthresh+1, w/4);
		else
			asm_deinterlace(dstp, p, n, fmaskp, dmaskp, thresh+1, dthresh+1, w/4);
#else
		for (x = 0; x < w; x += 4)
		{
			val = ((long)p[x] - (long)dstp[x]) * ((long)n[x] - (long)dstp[x]);
			if (val > thresh)
				*(int *)(fmaskp+x) = 0x000000ff;
			else
				*(int *)(fmaskp+x) = 0;
			if (val > dthresh)
				*(int *)(dmaskp+x) = 0x000000ff;
			else
				*(int *)(dmaskp+x) = 0;

			val = ((long)p[x+2] - (long)dstp[x+2]) * ((long)n[x+2] - (long)dstp[x+2]);
			if (val > thresh)
				*(int *)(fmaskp+x) |= 0x00ff0000;
			if (val > dthresh)
				*(int *)(dmaskp+x) |= 0x00ff0000;

			if (chroma)
			{
				if ((((long)p[x+1] - (long)dstp[x+1]) * ((long)n[x+1] - (long)dstp[x+1]) > dthresh) ||
				    (((long)p[x+3] - (long)dstp[x+3]) * ((long)n[x+3] - (long)dstp[x+3]) > dthresh))
					*(int *)(dmaskp+x) |= 0x00ff00ff;
			}
		}
#endif
	}
#ifdef DEINTERLACE_MMX_BUILD
	__asm emms;
#endif
	fmaskp += dpitch;
	memset(fmaskp, 0, w);
	dmaskp += dpitch;
	for (x = 0; x < w; x += 4) *(int *)(dmaskp+x) = 0x00ff00ff;

	// interlaced frame detection
	boxarraysize = ((h+8)>>3) * ((w+2)>>3);
	if ((counts = (int *) malloc(boxarraysize << 2)) == 0)
	{
		env->ThrowError("Telecide: malloc failure");
	}
	memset(counts, 0, boxarraysize << 2);

	fmaskp = fmaskp_saved;
	p = fmaskp - dpitch;
	n = fmaskp + dpitch;
	for (y = 1; y < h - 1; y++)
	{
		boxy = (y >> 3) * (w >> 3);
		fmaskp += dpitch;
		p += dpitch;
		n += dpitch;
		for (x = 0; x < w; x += 4)
		{
			box =  boxy + (x >> 3);
			if (fmaskp[x] == 0xff && p[x] == 0xff && n[x] == 0xff)
			{
				counts[box]++;
			}

			if (fmaskp[x+2] == 0xff && p[x+2] == 0xff && n[x+2] == 0xff)
			{
				counts[box]++;
			}
		}
	}

	for (x = 0; x < boxarraysize; x++)
	{
		if (counts[x] > CT)
		{
			if (debug)
			{
				char b[80];
				sprintf(b,"%d: Combed frame!\n", frame);
				OutputDebugString(b);
			}
			free(counts);
			return(true);
		}
	}
	free(counts);
	return(false);
}











/**********************************
 *******   ASM Deinterlace   ******
 *********************************/

#ifdef DEINTERLACE_MMX_BUILD
#pragma warning(disable:4799)
void __declspec(naked) asm_deinterlace(const BYTE *dstp, const BYTE *p,
								  const BYTE *n, BYTE *fmask,
								  BYTE *dmask, int thresh, int dthresh, int row_size)
{
	__declspec(align(8)) static const __int64 Mask0 = 0xFFFF0000FFFF0000i64;
	__declspec(align(8)) static const __int64 Mask2 = 0x00000000000000FFi64;
	__declspec(align(8)) static const __int64 Mask3 = 0x0000000000FF0000i64;
	__asm
	{
		push		ebp
		push		ebx
		push		esi
		push		edi

		mov			edx,[esp+4+16]	// dstp
		mov			ecx,[esp+8+16]	// p
		mov			ebx,[esp+12+16]	// n
		mov			eax,[esp+16+16] // fmaskp
		mov			esi,[esp+20+16] // dmaskp
		movd		mm0,[esp+24+16] // threshold
		movq		mm3,mm0
		psllq		mm3,32
		por			mm3,mm0
		movd		mm4,[esp+28+16] // dthreshold
		movq		mm5,mm4
		psllq		mm5,32
		por			mm5,mm4
		mov			ebp,[esp+32+16]	// row_size
xloop:
		pxor		mm4,mm4
		movd		mm0,[ecx]		// load p
		punpcklbw	mm0,mm4			// unpack
		movd		mm2,[edx]		// load dstp
		punpcklbw	mm2,mm4			// unpack
		movd		mm1,[ebx]		// load n
		punpcklbw	mm1,mm4			// unpack
		psubw		mm0,mm2			// pprev - curr = P
		psubw		mm1,mm2			// pnext - curr = N
		movq		mm6,Mask0
		pandn		mm6,mm0			// mm6 = 0 | P2 | 0 | P0
		pmaddwd		mm6,mm1			// mm6 = P2*N2  | P0*N0
		movq		mm0,mm6

		pcmpgtd		mm6,mm3			// mm6 = P2*N2<T | P0*N0<T
		movq		mm4,mm6
		pand		mm6,Mask2
		movq		mm7,mm6
		psrlq		mm4,32
		pand		mm4,Mask3
		por			mm7,mm4
		movd		[eax],mm7

		pcmpgtd		mm0,mm5			// mm0 = P2*N2<DT | P0*N0<DT
		movq		mm4,mm0
		pand		mm0,Mask2
		movq		mm7,mm0
		psrlq		mm4,32
		pand		mm4,Mask3
		por			mm7,mm4
		movd		[esi],mm7

		add			edx,4
		add			ecx,4
		add			ebx,4
		add			eax,4
		add			esi,4
		dec			ebp
		jnz			xloop

		pop			edi
		pop			esi
		pop			ebx
		pop			ebp
		ret
	};	
}







/***************************************
 *******   ASM Deinterlace Chroma ******
 **************************************/


void __declspec(naked) asm_deinterlace_chroma(const BYTE *dstp, const BYTE *p,
								  const BYTE *n, BYTE *fmask,
								  BYTE *dmask, int thresh, int dthresh, int row_size)
{
	__declspec(align(8)) static const __int64 Mask0 = 0xFFFF0000FFFF0000i64;
	__declspec(align(8)) static const __int64 Mask2 = 0x00000000000000FFi64;
	__declspec(align(8)) static const __int64 Mask3 = 0x0000000000FF0000i64;
	__declspec(align(8)) static const __int64 Mask4 = 0x0000000000FF00FFi64;
	__asm
	{
		push		ebp
		push		ebx
		push		esi
		push		edi

		mov			edx,[esp+4+16]	// dstp
		mov			ecx,[esp+8+16]	// p
		mov			ebx,[esp+12+16]	// n
		mov			eax,[esp+16+16] // fmaskp
		mov			esi,[esp+20+16] // dmaskp
		movd		mm0,[esp+24+16] // threshold
		movq		mm3,mm0
		psllq		mm3,32
		por			mm3,mm0
		movd		mm4,[esp+28+16] // dthreshold
		movq		mm5,mm4
		psllq		mm5,32
		por			mm5,mm4
		mov			ebp,[esp+32+16]	// row_size
xloop:
		pxor		mm4,mm4
		movd		mm0,[ecx]		// load p
		punpcklbw	mm0,mm4			// unpack
		movd		mm2,[edx]		// load dstp
		punpcklbw	mm2,mm4			// unpack
		movd		mm1,[ebx]		// load n
		punpcklbw	mm1,mm4			// unpack
		psubw		mm0,mm2			// pprev - curr = P
		psubw		mm1,mm2			// pnext - curr = N
		movq		mm6,Mask0
		pandn		mm6,mm0			// mm6 = 0 | P2 | 0 | P0
		pmaddwd		mm6,mm1			// mm6 = P2*N2  | P0*N0
		movq		mm0,mm6

		pcmpgtd		mm6,mm3			// mm6 = P2*N2<T | P0*N0<T
		movq		mm4,mm6
		pand		mm6,Mask2
		movq		mm7,mm6
		psrlq		mm4,32
		pand		mm4,Mask3
		por			mm7,mm4
		movd		[eax],mm7

		pcmpgtd		mm0,mm5			// mm0 = P2*N2<DT | P0*N0<DT
		movq		mm4,mm0			// save result
		pand		mm0,Mask2		// make 0x000000ff if P0*N0<DT
		movq		mm7,mm0			// move it into result
		psrlq		mm4,32			// get P2*N2<DT
		pand		mm4,Mask3		// make 0x00ff0000 if P2*N2<DT
		por			mm7,mm4			// move it into the result

		pxor		mm4,mm4			// do chroma bytes now
		movd		mm0,[ecx]		// load p
		punpcklbw	mm0,mm4			// unpack
		movd		mm1,[ebx]		// load n
		punpcklbw	mm1,mm4			// unpack
		psubw		mm0,mm2			// pprev - curr = P
		psubw		mm1,mm2			// pnext - curr = N
		movq		mm6,Mask0
		pand		mm6,mm0			// mm6 = P3 | 0 | P1| 0
		pmaddwd		mm6,mm1			// mm6 = P3*N3  | P1*N1
		movq		mm0,mm6			// 
		pcmpgtd		mm0,mm5			// mm0 = P3*N3<DT | P1*N1<DT
		movq		mm4,mm0			// save result
		pand		mm0,Mask4		// make 0x000000ff if P1*N1<DT
		por			mm7,mm0			// move it into result
		psrlq		mm4,32			// get P3*N3<DT
		pand		mm4,Mask4		// make 0x00ff0000 if P3*N3<DT
		por			mm7,mm4			// move it into the result
		movd		[esi],mm7		// store the result

		add			edx,4
		add			ecx,4
		add			ebx,4
		add			eax,4
		add			esi,4
		dec			ebp
		jnz			xloop

		pop			edi
		pop			esi
		pop			ebx
		pop			ebp
		ret
	};	
}
#pragma warning(default:4799)
#endif











/**************************
 *******   ASM Match ******
 *************************/

#ifdef MATCH_MMX_BUILD
#pragma warning(disable:4799)
int __declspec(naked) asm_match(const BYTE *curr, const BYTE *pprev,
								  const BYTE *pnext, int row_size)
{
	__declspec(align(8)) static const __int64 Threshold = 0x0000006500000065i64;
	__declspec(align(8)) static const __int64 Mask = 0xFFFF0000FFFF0000i64;
	__declspec(align(8)) static const __int64 Ones = 0x0000000100000001i64;
	__asm
	{
		push ebp
		push ebx

		mov			edx,[esp+4+8]	// curr
		mov			ecx,[esp+8+8]	// pprev
		mov			ebx,[esp+12+8]	// pnext
		mov			ebp,[esp+16+8]	// row_size
		movq		mm7,Threshold	
		movq		mm6,Mask
		pxor		mm3,mm3			// running sum
xloop:
		movd		mm0,[ecx]		// load pprev
		pxor		mm4,mm4

		movd		mm2,[edx]		// load curr
		punpcklbw	mm0,mm4			// unpack

		movd		mm1,[ebx]		// load pnext
		punpcklbw	mm2,mm4			// unpack

		punpcklbw	mm1,mm4			// unpack
		psubw		mm0,mm2			// pprev - curr

		psubw		mm1,mm2			// pnext - curr
		movq		mm4,mm6

		pandn		mm6,mm0
		pand		mm0,mm4
		pmaddwd		mm0,mm1			// mm1 = P3*N3  | P1*N1
		pmaddwd		mm6,mm1			// mm6 = P2*N2  | P0*N0
		movq		mm5,mm7
		movq		mm2,mm7
		pcmpgtd		mm2,mm0
		pcmpgtd		mm5,mm6

		movq		mm6,Ones
		pandn		mm5,mm6
		pandn		mm2,mm6
		paddd		mm3,mm5	
		paddd		mm3,mm2
		movq		mm6,mm4

		add			edx,16
		add			ecx,16
		add			ebx,16

		dec			ebp
		jnz			xloop

		movq		mm0,mm3
		psrlq		mm3,32
		paddd		mm0,mm3
		movd		eax,mm0
		pop			ebx
		pop			ebp
		ret
	};
}
#pragma warning(default:4799)
#endif








/**************************
 *******   ASM Blend ******
 *************************/

#ifdef BLEND_MMX_BUILD
#pragma warning(disable:4799)
int __declspec(naked) asm_blend(const BYTE *dstp, const BYTE *cprev,
								const BYTE *cnext, BYTE *finalp,
								BYTE *dmaskp, int count)
{
	__declspec(align(8)) static const __int64 Mask1 = 0xFEFEFEFEFEFEFEFEi64;
	__declspec(align(8)) static const __int64 Mask2 = 0xFCFCFCFCFCFCFCFCi64;
	__asm
	{
		push ebp
		push ebx
		push edi
		push esi

		mov			edx,[esp+4+16]	// dstp
		mov			ecx,[esp+8+16]	// cprev
		mov			ebx,[esp+12+16]	// cnext
		mov			eax,[esp+16+16] // finalp
		mov			edi,[esp+20+16]	// dmaskp
		mov			ebp,[esp+24+16]	// count
		movq		mm7,Mask1
		movq		mm6,Mask2
xloop:
		mov			esi,[edi]
		or			esi,esi
		jnz			blend
		mov			esi,[edi+4]
		or			esi,esi
		jnz			blend
		movq		mm0,[edx]
		movq		[eax],mm0
		jmp			skip
blend:
		movq		mm0,[edx]		// load dstp
		pand		mm0,mm7

		movq		mm1,[ecx]		// load cprev
		pand		mm1,mm6

		movq		mm2,[ebx]		// load cnext
		pand		mm2,mm6

		psrlq		mm0,1
		psrlq		mm1,2
		psrlq       mm2,2
		paddusb		mm0,mm1
		paddusb		mm0,mm2
		movq		[eax],mm0
skip:
		add			edx,8
		add			ecx,8
		add			ebx,8
		add			eax,8
		add			edi,8
		dec			ebp
		jnz			xloop

		pop			esi
		pop			edi
		pop			ebx
		pop			ebp
		ret
	};
}
#pragma warning(default:4799)
#endif







/**********************************
 *******   ASM Interpolate   ******
 *********************************/

#ifdef INTERPOLATE_MMX_BUILD
#pragma warning(disable:4799)
int __declspec(naked) asm_interpolate(const BYTE *dstp, const BYTE *cprev,
								const BYTE *cnext, BYTE *finalp,
								BYTE *dmaskp, int count)
{
	__declspec(align(8)) static const __int64 Mask = 0xFEFEFEFEFEFEFEFEi64;
	__asm
	{
		push ebp
		push ebx
		push edi
		push esi

		mov			edx,[esp+4+16]	// dstp
		mov			ecx,[esp+8+16]	// cprev
		mov			ebx,[esp+12+16]	// cnext
		mov			eax,[esp+16+16] // finalp
		mov			edi,[esp+20+16]	// dmaskp
		mov			ebp,[esp+24+16]	// count
		movq		mm7,Mask
xloop:
		mov			esi,[edi]
		or			esi,esi
		jnz			blend
		mov			esi,[edi+4]
		or			esi,esi
		jnz			blend
		movq		mm0,[edx]
		movq		[eax],mm0
		jmp			skip
blend:
		movq		mm0,[ecx]		// load cprev
		pand		mm0,mm7

		movq		mm1,[ebx]		// load cnext
		pand		mm1,mm7

		psrlq		mm0,1
		psrlq       mm1,1
		paddusb		mm0,mm1
		movq		[eax],mm0
skip:
		add			edx,8
		add			ecx,8
		add			ebx,8
		add			eax,8
		add			edi,8
		dec			ebp
		jnz			xloop

		pop			esi
		pop			edi
		pop			ebx
		pop			ebp
		ret
	};
}
#pragma warning(default:4799)
#endif
