/*
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

#include "fielddeinterlace.h"






/**************************************************
 *******   FieldDeinterlace public methods   ******
 *************************************************/

FieldDeinterlace::FieldDeinterlace( PClip _child, bool _full, int _threshold, int _dthreshold, 
                                    bool _blend, bool _chroma, bool _debug, 
                                    IScriptEnvironment* env ) 
  : GenericVideoFilter(_child), full(_full), threshold(_threshold), dthreshold(_dthreshold), 
    blend(_blend), chroma(_chroma),	debug(_debug)
{
	if (!vi.IsYUY2())
		env->ThrowError("FieldDeinterlace: YUY2 data only (no RGB); use ConvertToYUY2");
	if (debug)
	{
		char b[80];
		sprintf(b, "FieldDeinterlace %s by Donald Graft, Copyright 2002\n", VERSION);
		OutputDebugString(b);
	}
}




PVideoFrame __stdcall FieldDeinterlace::GetFrame(int n, IScriptEnvironment* env)
{
	int x, y;
#ifdef BLEND_MMX_BUILD
int asm_blend(const BYTE *dstp, const BYTE *cprev,
								const BYTE *cnext, BYTE *finalp,
								BYTE *dmaskp, int count);
#else
	unsigned int p0;
#endif
#ifdef INTERPOLATE_MMX_BUILD
int asm_interpolate(const BYTE *dstp, const BYTE *cprev,
								const BYTE *cnext, BYTE *finalp,
								BYTE *dmaskp, int count);
#endif
	unsigned int p1, p2;
	const BYTE *cprev, *cnext;

	src = child->GetFrame(n, env);
	srcp_saved = srcp = src->GetReadPtr();
	pitch = src->GetPitch();
	w = src->GetRowSize();
	h = src->GetHeight();

	fmask = env->NewVideoFrame(vi);
	fmaskp_saved = fmaskp = fmask->GetWritePtr();
	dpitch = fmask->GetPitch();

	dmask = env->NewVideoFrame(vi);
	dmaskp_saved = dmaskp = dmask->GetWritePtr();

	if (MotionMask_YUY2(n, env) == false && full == false)
		return src;  // clean frame; return original

	/* Frame is combed or user wants all frames, so deinterlace. */
	PVideoFrame final = env->NewVideoFrame(vi);

	srcp = srcp_saved;
	dmaskp = dmaskp_saved;
	finalp = final->GetWritePtr();
	cprev = srcp - pitch;
	cnext = srcp + pitch;
	if (blend)
	{
		/* First line. */
		for (x=0; x<w; x+=4)
		{
			p1 = *(int *)(srcp+x) & 0xfefefefe;
			p2 = *(int *)(cnext+x) & 0xfefefefe;
			*(int *)(finalp+x) = (p1>>1) + (p2>>1);
		}
		srcp += pitch;
		dmaskp += dpitch;
		finalp += dpitch;
		cprev += pitch;
		cnext += pitch;
		for (y = 1; y < h - 1; y++)
		{
#ifdef BLEND_MMX_BUILD
			asm_blend(srcp, cprev, cnext, finalp, dmaskp, w/8);
#else
			for (x=0; x<w; x+=4)
			{
				if (dmaskp[x])
				{
					p0 = *(int *)(srcp+x) & 0xfefefefe;
					p1 = *(int *)(cprev+x) & 0xfcfcfcfc;
					p2 = *(int *)(cnext+x) & 0xfcfcfcfc;
					*(int *)(finalp+x) = (p0>>1) + (p1>>2) + (p2>>2);
				}
				else
				{
					*(int *)(finalp+x)=*(int *)(srcp+x);
				}
			}
#endif
			srcp += pitch;
			dmaskp += dpitch;
			finalp += dpitch;
			cprev += pitch;
			cnext += pitch;
		}
        /* Last line. */
		for (x=0; x<w; x+=4)
		{
			p1 = *(int *)(srcp+x) & 0xfefefefe;
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
				memcpy(finalp,srcp,w);
			}
			else
			{
#ifdef INTERPOLATE_MMX_BUILD
			asm_interpolate(srcp, cprev, cnext, finalp, dmaskp, w/8);
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
						*(int *)(finalp+x) = *(int *)(srcp+x);
					}
				}
#endif
			}
			srcp += dpitch;
			dmaskp += dpitch;
			finalp += dpitch;
			cprev += pitch;
			cnext += pitch;
		}
		/* Last line. */
		memcpy(finalp,srcp,w);
#ifdef INTERPOLATE_MMX_BUILD
		__asm emms;
#endif
	}

	return final;
}




AVSValue __cdecl FieldDeinterlace::Create(AVSValue args, void* user_data, IScriptEnvironment* env)
{
	return new FieldDeinterlace( args[0].AsClip(),		    // clip
		                           args[1].AsBool(true),    // full -- deinterlace all frames
		                           args[2].AsInt(15),		    // threshold
		                           args[3].AsInt(9),		    // dthreshold
		                           args[4].AsBool(true),	  // blend rather than interpolate
		                           args[5].AsBool(false),	  // use chroma for deinterlacing
		                           args[6].AsBool(false),   // debug mode
		                           env );
}








/***************************************************
 *******   FieldDeinterlace private methods   ******
 **************************************************/

bool FieldDeinterlace::MotionMask_YUY2(int frame, IScriptEnvironment* env)
{
	int x, y, thresh, dthresh;
	int *counts, box, boxy;
#ifdef DEINTERLACE_MMX_BUILD
	void asm_deinterlace(const BYTE *srcp, const BYTE *p,
								  const BYTE *n, BYTE *fmask,
								  BYTE *dmask, int thresh, int dthresh, int row_size);
	void asm_deinterlace_chroma(const BYTE *srcp, const BYTE *p,
								  const BYTE *n, BYTE *fmask,
								  BYTE *dmask, int thresh, int dthresh, int row_size);
#else
	int val, val2;
#endif
	const int CT = 15;
	int boxarraysize;

	thresh = threshold*threshold;
	dthresh = dthreshold*dthreshold;

	p = srcp - pitch;
	n = srcp + pitch;
	memset(fmaskp, 0, w);
	for (x = 0; x < w; x += 4) *(int *)(dmaskp+x) = 0x00ff00ff;

	for (y = 1; y < h - 1; y++)
	{
		fmaskp += dpitch;
		dmaskp += dpitch;
		srcp += pitch;
		p += pitch;
		n += pitch;
#ifdef DEINTERLACE_MMX_BUILD
		if (chroma)
			asm_deinterlace_chroma(srcp, p, n, fmaskp, dmaskp, thresh+1, dthresh+1, w/4);
		else
			asm_deinterlace(srcp, p, n, fmaskp, dmaskp, thresh+1, dthresh+1, w/4);
#else
		for (x = 0; x < w; x += 4)
		{
			val = ((long)p[x] - (long)srcp[x]) * ((long)n[x] - (long)srcp[x]);
			if (val > thresh)
				*(int *)(fmaskp+x) = 0x000000ff;
			else
				*(int *)(fmaskp+x) = 0;
			if (val > dthresh)
				*(int *)(dmaskp+x) = 0x000000ff;
			else
				*(int *)(dmaskp+x) = 0;

			val = ((long)p[x+2] - (long)srcp[x+2]) * ((long)n[x+2] - (long)srcp[x+2]);
			if (val > thresh)
				*(int *)(fmaskp+x) |= 0x00ff0000;
			if (val > dthresh)
				*(int *)(dmaskp+x) |= 0x00ff0000;

			if (chroma)
			{
				val  = ((long)p[x+1] - (long)srcp[x+1]) * ((long)n[x+1] - (long)srcp[x+1]);
				val2 = ((long)p[x+3] - (long)srcp[x+3]) * ((long)n[x+3] - (long)srcp[x+3]);
				if (val > dthresh || val2 > dthresh)
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

	if (full == true) return(true);

	// interlaced frame detection
	boxarraysize = ((h+8)>>3) * ((w+2)>>3);
	if ((counts = (int *) malloc(boxarraysize << 2)) == 0)
	{
		env->ThrowError("FieldDeinterlace: malloc failure");
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